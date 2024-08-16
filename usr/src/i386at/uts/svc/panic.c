/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/panic.c	1.24"
#ident	"$Header: $"

#include <io/conf.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/fp.h>
#include <svc/memory.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/kcontext.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern int upyet;

/*PRINTFLIKE1*/
extern void panic_printf(const char *fmt, ...);

struct panic_data panic_data;
int dblpanic;

/*
 * value to pass to psm_reboot().  Configurable panic (via uadmin())
 * allows the system to stay down or reboot.
 *
 *  zero        -> no auto reboot
 *  non-zero    -> automatic reboot
 */
extern int bootpanic;

extern void psm_reboot(int);

STATIC void printprocregs(kcontext_t *);

void mdboot(int, int);

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

	if (u.u_procp != NULL)
		panic_data.pd_lwp = u.u_procp->p_lwpp;

	if (upyet) {
		panic_data.pd_engine = l.eng;
		l.panic_pt = READ_PTROOT();
		if (using_fpu)
			save_fpu();
	} else
		panic_data.pd_engine = engine;

	printprocregs(kcp);
	psm_panic();
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
#ifndef NODEBUGGER
	(*cdebugger)(DR_PANIC, NO_FRAME);
#endif

	mdboot(bootpanic ? AD_BOOT : AD_HALT, 0);
	/* NOTREACHED */
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

	if (u.u_procp != NULL)
		panic_data.pd_lwp = u.u_procp->p_lwpp;

	panic_data.pd_engine = l.eng;
	mdboot(bootpanic ? AD_BOOT : AD_HALT, 0);
	/* NOTREACHED */
}

/*
 * void
 * mdboot(int fcn, int mdep)
 *
 *      Halt or Reboot the processor/s
 *
 * Calling/Exit State:
 *      This routine will not return.
 */

/* ARGSUSED */
void
mdboot(int fcn, int mdep)
{
	extern boolean_t soft_sysdump;

	if (l.panic_level && soft_sysdump)
		sysdump();

	psm_reboot(fcn);

	/* psm_reboot() shouldn't return, but just in case... */
	for (;;)
		;
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
 *
 * Remarks:
 *	Should never happen on a uniprocessor i386at.
 */
/* ARGSUSED */
void
concurrent_panic(kcontext_t *kcp)
{
}
