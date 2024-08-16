/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm.c	1.17"
#ident	"$Header: $"

/*
 * ssm.c
 *	Systems Services Module (SSM) driver code.
 */

#include <io/cfg.h>
#include <io/conf.h>
#include <io/intctl.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <io/ssm/ssm.h>
#include <io/ssm/ssm_misc.h>
#include <io/ssm/ssm_vme.h>
#include <mem/kmem.h>
#include <mem/vm_mdep.h>
#include <svc/clock_p.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <util/boot.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/* 
 * Array of SSM descriptors alloc'd 
 * and filled by conf_ssm() and a
 * description of its elements.
 */
struct ssm_desc	*SSM_desc;		/* Array of SSM descriptors. */
int ssm_cnt;				/* #elements in SSM_desc */
struct ssm_desc	*SSM_cons;		/* SSM descriptor of console board. */

static void ssm_return_fw(void), ssm_inittodr(void), ssm_resettodr(void);
static void ssm_wdtinit(void);
static void ssm_boot(int, int);

extern struct cons_cb *cb;

extern int fp_lights;
extern int light_show;

STATIC toid_t ssm_wd_tid;	/* watchdog timer timeout ID */

extern void dis_vfs(int);

/*
 * fw_boot, fw_return, fw_inittodr, fw_resettodr, 
 * fw_wdtinit, fw_wdtdisable, fw_wdtreenable
 */
struct firmwaresw ssm_interface = {
	ssm_boot,	ssm_return_fw,	ssm_inittodr,	 ssm_resettodr,
	ssm_wdtinit,    ssm_disable_wdt,	ssm_reenable_wdt
};

/*
 * ssminit()
 *	Initialize system services module.  
 *
 * Calling/Exit State:
 *	No locking assumed.  Assumes that only one processor is running,
 *	since this is called at system startup time.
 *
 * Description:
 *	Finds and initializes the SSM board(s) in the system.  The SSM with
 *	the front-panel connected to it is assigned to be the console
 *	controller.
 */
void 
ssminit(void)
{
        struct ctlr_toc *toc1, *toc2;
	register const struct ctlr_desc *cd;
	register struct	ssm_desc *ssm;
	int i;
        extern struct ssm_misc *ssm_misc_init(int, unchar);

        toc1 = KVCD_LOC->c_toc + SLB_SSMBOARD;
        toc2 = KVCD_LOC->c_toc + SLB_SSM2BOARD;

        ssm_cnt = toc1->ct_count + toc2->ct_count;

	if( ssm_cnt == 0 )
		return;            /* if no SSMs at all, short circuit */

	/*
	 * Allocate an array of descriptors, 
 	 * one per potential SSM. 
	 */
	ssm = SSM_desc = (struct ssm_desc *)
		kmem_zalloc(ssm_cnt * sizeof(struct ssm_desc), KM_NOSLEEP);
	ASSERT(ssm != NULL);

	/*
	 * Initialize each of the descriptors.
	 * Save the location of the master board
	 * for access by misc. functions.
	 */
	cd = &KVCD_LOC->c_ctlrs[toc1->ct_start];

        for (i = 0; i < ssm_cnt; i++, cd++, ssm++) {
		if (i == toc1->ct_count)
			cd = &KVCD_LOC->c_ctlrs[toc2->ct_start];

		ssm->ssm_cd = cd;

		if (cd->cd_ssm_cons) {
			cmn_err(CE_CONT, "# Console is on ssm at slic %d\n", 
				cd->cd_slic);
			SSM_cons = ssm;
			firmwaresw = &ssm_interface;
			if (light_show > 1) {
				fp_lights = -1;
			}
		}

		if (!FAILED(cd)) {

			/* 
			 * Allocate console/serial port, printer, and
			 * misc command blocks and notify the SSM of 
			 * their location.  Also, initialize VME 
			 * resources, if present and usable.
			 */

			ssm->ssm_cons_cbs = cb;/* initialized by init_console */
			ssm->ssm_prnt_cbs = init_ssm_prnt(ssm->ssm_slicaddr);

			ssm->ssm_mc = ssm_misc_init(ssm == SSM_cons,
					            (unchar)ssm->ssm_slicaddr);
			ssm_vme_init(ssm);

			cmn_err(CE_CONT, 
				"ssm %d found at slic %d\n", i, cd->cd_slic);

		} else {
			cmn_err(CE_CONT, "ssm %d at slic %d deconfigured\n", i, 
				cd->cd_slic);
		}
	}
}

/*
 * void
 * ssmstart()
 *	Complete initialization of the system services module.  
 *
 * Calling/Exit State:
 *	No locking assumed; only one processor is running
 *	during system startup time.  Called after most system
 *	services are available and interrupts are enabled.
 *
 * Description:
 *	Cycle through all the valid SSM boards and calling
 *	their VME start() interfaces.
 */
void 
ssmstart(void)
{
	register struct	ssm_desc *ssm;
	int i;

	for (ssm = SSM_desc, i = 0; i < ssm_cnt; i++, ssm++) {
		if (!FAILED(ssm->ssm_cd))
			ssm_vme_start(ssm);
	}
}

static ushort_t reboot_flags = RB_HALT;

/*
 * void ssm_boot(int, int)
 *	SSM dependent reboot the machine.
 * 	Sets up a return of control to the SSM firmware.  
 *
 * Calling/Exit State:
 *	paniced is RB_PANIC if we were called by cmn_err while panicing,
 *	or RB_BOOT if this is a normal shutdown.  Only ONE engine is
 *	alive at this point.
 *
 * Description:
 *	If we are panicing, then tell firmware to boot the alternate
 *	boot string (normally the dumper).  
 * 	If called by panic it returns specifying that the 
 *	alternate boot name is to be booted (normally the 
 *	Memory dumper). Prior to returning to firmware, attempt
 *	to sync the disks.
 *
 */
static void
ssm_boot(int paniced, int howto)
{
	pl_t s_ipl;
	extern int dblpanic;
	extern int upyet;

	if ((!upyet) || ((paniced == RB_PANIC) && dblpanic))
		return_fw();

	/*
	 * Now tell FW how to reboot
	 */
	if (paniced == RB_PANIC) {
		/*
		 * Setup reboot to perform a system dump
		 * and then turn on the error light.
		 */
		reboot_flags = RB_AUXBOOT;
		s_ipl = splhi();
		ssm_set_fpst(ssm_get_fpst(SM_NOLOCK) | FPST_ERR, SM_NOLOCK);
		splx(s_ipl);
	} else {
		/*
		 * Setup reboot to use 'howto' flags passed in.
		 * Then disable the watchdog timer to prevent
  		 * the error light from changing state.
		 */ 

		/*
		 *+ The ssm_boot routine was called to halt the system.
		 *+ Neither the RB_PANIC or the RB_BOOT bit was set in the
		 *+ parameter that tells ssm_boot what action to take.
		 */
		ASSERT(paniced == RB_BOOT);

		reboot_flags = (ushort_t)howto;

		if (ssm_wd_tid) {
			untimeout(ssm_wd_tid);		/* Stop wdt reset */
			ssm_wd_tid = 0;
		}
		s_ipl = splhi();
		FW_WDTDISABLE();			/* Disable wdt on SSM */
		splx(s_ipl);
	}
	dis_vfs(UADMIN_SYNC);
	return_fw();
}

/*
 * void return_fw(void)
 *	Return to Firmware.
 *
 * Calling/Exit State:
 *	We don't return from this routine.
 *	
 * Description:
 *	If we are panicing, then invoke custom
 *	panic handlers (if panicing) before calling it a day.
 *	Then take a deep breath and pull the trigger.
 */

void
return_fw(void)
{
	if (upyet) {
		(void) splhi();
	}
	FW_RETURN_FW();
	/*NOTREACHED*/
}

/*
 * void ssm_return_fw(void)
 *	Return control to the SSM firmware.
 *
 * Calling/Exit State:
 *	We don't come back from this routine.
 */
static void
ssm_return_fw(void)
{

	ssm_reboot(reboot_flags, 0, (char *)NULL);
	for (;;); 			/* SSM will take control.  */
	/*NOTREACHED*/
}


/*
 * Routines to manipulate the SSM based time-of-day register.
 * TOD clock interrupt handling done by todclock in kern_clock.c
 *
 */

extern timestruc_t hrestime;
extern time_t time;

/*
 * void ssm_inittodr(void)
 *	Initializes the time-of-day hardware which provides date functions.
 *	This starts the time-of-day clock.
 *
 * Calling/Exit State:
 *	None.
 */
static void
ssm_inittodr(void)
{
	/*
	 *+ ssm_inittodr was called to initialize the time of day hardware,
	 *+ but an SSM has not been selected as the controlling console.
	 */
	ASSERT(SSM_cons);

	/* 
	 * Verify that TOD clock passed diagnostics.
	 */
	if (SSM_cons->ssm_diag_flags & CFG_SSM_TOD) {
		/*
		 * Clear todr if TOD failed powerup 
		 * diagnostics.  
		 */

		/*
		 *+ The time of day clock on the SSM controlling the 
		 *+ system failed its power-up diagnostics.  
		 *+ The system date should be checked/reset after
		 *+ the system is booted and the failure is repaired.
		 */
		cmn_err(CE_WARN, 
			"WARNING: TOD on SSM failed powerup diagnostics");

		/* NOTE: this code is originally from check_time() */
		time = hrestime.tv_sec = 13*SECYR + 19*SECDAY + (2*SECDAY)/3;

		ssm_resettodr();
	} else {
		/*
		 * Fetch the current time-of-day from 
		 * the SSM tod clock.
		 */
		time = hrestime.tv_sec = ssm_get_tod(SM_NOLOCK);
	}

	/* 
	 * Start the tod clock.
	 */
	ssm_tod_freq(SM_NOLOCK, TODFREQ, SL_GROUP | PROC_GROUP,
		TODCLKVEC, SL_MINTR | TODCLKBIN);
}

/*
 * void ssm_resettodr(void)
 *	restores the time-of-day hardware after a time change.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Reset the TOD based on the time value; used when the TOD
 *	has a preposterous value and also when the time is reset
 *	by the stime system call.
 */
static void
ssm_resettodr(void)
{
	ssm_set_tod((ulong_t)hrestime.tv_sec);
}

/*
 * static void
 * ssm_wdtinit(void)
 *	Enables the SSM watchdog timer functionality.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Communicates to the SSM the frequency at which it should
 *	expect that the O.S. strokes a watchdog timer counter
 *	in main memory.  The SSM will read the value periodically.
 *	If it has not changed in that time period, the SSM assumes 
 *	that the O.S. is no longer functional and turns on the 
 *	"ERROR" light on the front panel.
 *
 *	To stroke the watchdog timer counter in main memory, the
 *	O.S. schedules a callout of ssm_poke_wdt() to occur which 
 *	increments the counter the SSM reads and then reschedules 
 *	itself for another callout.  The first callout is scheduled 
 *	from here.
 */
static void
ssm_wdtinit(void)
{
        pl_t s_ipl;

        ssm_wd_tid = itimeout(ssm_poke_wdt, NULL,
			      (HZ/2) | TO_PERIODIC, pltimeout);
	if (ssm_wd_tid == 0) {
		/*
		 *+ Insufficient resources to allocate ssm watchdog timer.
		 */
		cmn_err(CE_NOTE, "Can't start ssm watchdog timer.");
		return;
	}

	s_ipl = splhi();
        ssm_init_wdt((ulong)(SSM_WDT_TIMEOUT * 1000));  /* milliseconds */
        splx(s_ipl);
}
