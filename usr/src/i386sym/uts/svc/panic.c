/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/panic.c	1.12"
#ident	"$Header: $"

#include <io/SGSproc.h>
#include <io/SGSmem.h>
#include <io/bdp.h>
#include <io/cfg.h>
#include <io/conf.h>
#include <io/intctl.h>
#include <io/scan.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <io/ssm/ssm.h>
#include <io/ssm/ssm_misc.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/bic.h>
#include <svc/fp.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/boot.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/kcontext.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

extern int upyet;

/*PRINTFLIKE1*/
extern void panic_printf(const char *fmt, ...);

struct panic_data panic_data;
int dblpanic;

STATIC void printprocregs(kcontext_t *);
STATIC void dump_bic_regs(int, int);
STATIC void dump_bdp_regs(int, int, int);
STATIC void dump_SGShw_regs(void);
STATIC void dump_SGSproc_regs(void);
#if defined(DEBUG) || defined(MFG)
STATIC void dump_SGSmem_regs(void);
#endif /* DEBUG || MFG */

/*
 * void
 * panic_start(kcontext_t *kcp)
 *	First half of platform-dependent panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller is responsible for mutexing other panic and cmn_err users.
 *
 * Description:
 *	Saves this engine's state in well-known places for dump analysis.
 *	Prints machine state (using panic_printf()).
 */
void
panic_start(kcontext_t *kcp)
{
	panic_data.pd_rp = kcp;
	if (upyet) {
		panic_data.pd_engine = l.eng;
		l.panicsp = kcp;
		l.panic_pt = READ_PTROOT();
		if (using_fpu)
			save_fpu();
	} else
		panic_data.pd_engine = engine;

	printprocregs(kcp);
	if (!(l.eng->e_flags & E_SGS2))
		dump_SGShw_regs();
}

/*
 * void
 * panic_shutdown(void)
 *	Second half of platform-dependent panic processing.
 *
 * Calling/Exit State:
 *	The caller is responsible for mutexing other panic and cmn_err users.
 *	The caller will already have printed the regular PANIC message.
 *
 * Description:
 *	Stops all running engines, then shuts down or reboots.
 *	If available, may invoke a kernel debugger.
 */
void
panic_shutdown(void)
{
	struct engine *eng;
	int i, stillalive;

#ifndef	NODEBUGGER
	(*cdebugger)(DR_PANIC, NO_FRAME);
#endif

	if (upyet) {
		/*
		 * Send NMIs to all other engines.
		 * Keep trying until all respond - for robustness.
		 * However, time out after about 0.5 seconds.
		 */
		for (i = 0; i < 100; i++) {
			stillalive = 0;
			for (eng=engine; eng < engine_Nengine; eng++) {
				if (eng==l.eng)
					continue;
				if ((eng->e_flags&E_OFFLINE)==E_OFFLINE)
					continue;
				if ((eng->e_flags&E_PAUSED)==E_PAUSED)
					continue;
				stillalive++;
				slic_nmIntr(eng->e_slicaddr, PAUSESELF);
			}
			if (stillalive == 0)
				break;
			DELAY(40000);
		}
	}

	/*
	 * Return to Firmware
	 */
	FW_BOOT(RB_PANIC, RB_AUTOBOOT);
	/*NOTREACHED*/
}

/*
 * void
 * double_panic(kcontext_t *kcp)
 *	Platform-dependent double-panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller is responsible for mutexing other panic users.
 *	The caller will already have printed the regular DOUBLE PANIC message.
 *
 * Description:
 *	Called when a panic occurs during another panic.
 *	Saves this engine's state in well-known places for dump analysis,
 *	and directly shuts down without printing anything.
 */
void
double_panic(kcontext_t *kcp)
{
	/*
	 * Set the dblpanic flag so that the firmware routine will not
	 * attempt flushing dirty memory to disk, but instead will reboot
	 * immediately.
	 */
	dblpanic = 1;
	panic_data.pd_dblrp = kcp;

	/*
	 * Return to Firmware
	 */
	FW_BOOT(RB_PANIC, RB_AUTOBOOT|RB_NOSYNC);
	/*NOTREACHED*/
}

/*
 * void
 * concurrent_panic(kcontext_t *kcp)
 *	Platform-dependent concurrent-panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller will already have printed the regular CONCURRENT PANIC
 *	message.
 *
 * Description:
 *	Called when this engine has panic'ed, but another engine has already
 *	panic'ed first.
 *	Saves this engine's state in well-known places for dump analysis,
 *	and directly shuts down without printing anything.
 */
/*ARGSUSED*/
void
concurrent_panic(kcontext_t *kcp)
{
	pause_self();
	/*NOTREACHED*/
}

/*
 * void
 * mdboot(int fcn, int mdep)
 *	Halt or Reboot the processor/s
 *
 * Calling/Exit State:
 *	This routine does not return.
 */

void
mdboot(int fcn, int mdep)
{
        int mode = RB_AUTOBOOT;
        int stillalive;
        struct engine *eng;
        int i;
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	/* Turn off local clock interrupts. */
	sl->sl_tctl = 0;

	/* Turn off TOD interrupts. */
	ssm_tod_freq(SM_NOLOCK, 0, SL_GROUP | TMPOS_GROUP,
                        TODCLKVEC, SL_MINTR | TODCLKBIN);

	/* Wait a while for pending interrupts to be serviced. */
	drv_usecwait(100000);

        for (i = 0; i < 100; i++) {
                stillalive = 0;
                for (eng = engine; eng < engine_Nengine; eng++) {
                        if ((eng != l.eng)
                        && ((eng->e_flags&E_OFFLINE) != E_OFFLINE)
                        && ((eng->e_flags&E_PAUSED) != E_PAUSED)) {
                                stillalive++;
                                slic_nmIntr(eng->e_slicaddr, PAUSESELF);
                        }
                }
                if (stillalive == 0)
                        break;

                DELAY(40000);
        }



	/* translate AD_* commands to RB_* */
        if (fcn == AD_HALT || fcn == AD_IBOOT)
        	mode |= RB_HALT;

       	FW_BOOT(RB_BOOT, mode|mdep);
	/*NOTREACHED*/
}


/*
 * STATIC void
 * printprocregs(int engnum, kcontext_t *kcp)
 *	Prints all the processor registers.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
printprocregs(kcontext_t *kcp)
{
	panic_printf("Processor registers:\n");
	panic_printf(" eax=%x ebx=%x ecx=%x edx=%x\n",
		     kcp->kctx_eax, kcp->kctx_ebx,
		     kcp->kctx_ecx, kcp->kctx_edx);
	panic_printf(" esi=%x edi=%x ebp=%x esp=%x\n",
		     kcp->kctx_esi, kcp->kctx_edi,
		     kcp->kctx_ebp, kcp->kctx_esp);
}

/*
 * void
 * dump_SGShw_regs(void)
 *	Print the hardware registers.
 *
 * Calling/Exit State:
 *	Returns: none.
 */

STATIC void
dump_SGShw_regs(void)
{
	dump_SGSproc_regs();
#if defined(DEBUG) || defined(MFG)
	dump_SGSmem_regs();
#endif	/* DEBUG||MFG  */
}

#if defined(DEBUG) || defined(MFG)

/*
 * STATIC void
 * dump_bic_regs(int slicid, int bic)
 *	dump bic registers selected
 *
 * Calling/Exit State:
 *	slic id - the SLIC ID of the processor
 *	bic - the bic (bus interface controller) id of the processor
 *
 * 	Returns: None
 */

STATIC void
dump_bic_regs(int slicid, int bic)
{

	panic_printf("BIC- errctl 0x%x buserr 0x%x slicerr 0x%x\n",
		     slic_rdSubslave(slicid,  bic, BIC_ERRCTL),
		     slic_rdSubslave(slicid,  bic, BIC_BUSERR),
		     slic_rdSubslave(slicid,  bic, BIC_SLICERR));
	panic_printf("     accerr0 0x%x accerr1 0x%x "
			"cdiagctl0 0x%x cdiagctl1 0x%x\n",
		     slic_rdSubslave(slicid,  bic, BIC_ACCERR0),
		     slic_rdSubslave(slicid,  bic, BIC_ACCERR1),
		     slic_rdSubslave(slicid,  bic, BIC_CDIAGCTL0),
		     slic_rdSubslave(slicid,  bic, BIC_CDIAGCTL1));
}

/*
 * STATIC void
 * dump_bdp_regs(int, int, int)
 *	dump selected bdp registers
 *
 * Calling/Exit State:
 *	Takes three parameters - the slic id of the processor,
 *		the bdp id and an integer (0 or 1) that indicates
 *		which bdp registers to print.
 *
 * 	Returns: None
 */

STATIC void
dump_bdp_regs(int slicid, int bdp, int lh)
{
	panic_printf("BDP-%s datapar 0x%x syspar 0x%x sysparad 0x%x\n",
		(lh==0)?"L":"H",
		slic_rdSubslave(slicid,  bdp, BDP_DATAPAR),
		slic_rdSubslave(slicid,  bdp, BDP_SYSPAR),
		slic_rdSubslave(slicid,  bdp, BDP_SYSPARAD));
}
#endif	/* DEBUG||MFG  */

/*
 * STATIC void
 * dump_SGSproc_regs(void)
 *	Print status, fault and BDP registers of all processors.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	For each SGS processor in the system, print it's
 *	status and fault registers, selected BDP LO and BDP HI registers,
 *	and selected BIC registers.
 */

STATIC void
dump_SGSproc_regs(void)
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register int i, unit;

	/*
	 * Load pointer to table of contents
	 * for SGS procesor boards in the system.
	 */
	toc = &((struct config_desc *)KVCD_LOC)->c_toc[SLB_SGSPROCBOARD];		/* SGS processors */

	/*
	 * Locate the array of descriptors for SGS memory boards.
	 *	search this array and print out information
	 *	on each configured memory board
	 */
	cd = &((struct config_desc *)KVCD_LOC)->c_ctlrs[toc->ct_start];
	unit = 0;
	for (i = 0; i < (int)(toc->ct_count); i++, cd++) {
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) {
			continue;
		}
		panic_printf("processor %d ", unit);
		unit++;
		panic_printf("stat 0x%x flt 0x%x\n",
			     slic_rdslave(cd->cd_slic, PROC_STAT),
			     slic_rdslave(cd->cd_slic, PROC_FLT));
	}
}

#if defined(DEBUG) || defined(MFG)

/*
 * STATIC void
 * dump_SGSmem_regs(void)
 *	Print status and BDP registers of all memory boards.
 *
 * Calling/Exit State:
 *	None.  Returns: none.
 *
 * Description:
 *	For each SGS memory board in the system, print its
 *	status register, selected BDP LO and BDP HI registers,
 *	and selected BIC registers.
 */

STATIC void
dump_SGSmem_regs(void)
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register int	i, unit;

	/*
	 * Load pointer to table of contents for SGS memory
	 * boards in the system.
	 */
	toc = &((struct config_desc *)KVCD_LOC)->c_toc[SLB_SGSMEMBOARD];

	/*
	 * Locate the array of descriptors for SGS memory boards.
	 *	search this array and print out information
	 *	on each configured memory board
	 */
	cd = &((struct config_desc *)KVCD_LOC)->c_ctlrs[toc->ct_start];
	unit = 0;
	for (i = 0; i < (int)(toc->ct_count); i++, cd++) {
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			continue;
		panic_printf("memory board %d ", unit);
		panic_printf("status 0x%x\n",
			     slic_rdslave(cd->cd_slic, MEM_EDC));
		dump_bdp_regs(cd->cd_slic, MEM_BDP_LO, 0);
		dump_bdp_regs(cd->cd_slic, MEM_BDP_HI, 1);
		dump_bic_regs(cd->cd_slic, MEM_BIC);
		unit++;
	}
}

#endif	/* DEBUG||MFG  */
