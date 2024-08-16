/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/main.c	1.64"
#ident	"$Header: $"

/*
 * Miscellaneous generic system initialization.
 */

#include <mem/as.h>
#include <mem/hat.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/priocntl.h>
#include <proc/proc.h>
#include <proc/uidquota.h>
#include <proc/user.h>
#include <proc/usync.h>
#include <svc/clock.h>
#include <svc/memory.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/sysmacros.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern void metrics(void);
extern void engine_init(void);
extern void keyctl_init(void);
extern void uadmin_init(void);
extern void modinit(void);
extern void pm_init(void);

extern void spawn_sys_lwp_daemon(void);

#ifdef DKTEST
extern void runflavors(void *);
#endif

/*
 * Special processes which need to be identified.
 */
proc_t *proc_sys;		/* general system daemon process */
proc_t *proc_init;		/* init user process */

/*
 * sysdump to be done in software? - set by psm modules.
 */
boolean_t soft_sysdump;

/*
 * Startup message.  May be overriden by bootarg_parse() for localization.
 */
char *startupmsg = "The system is coming up.  Please wait.";

char *kernel_name = "/stand/unix";
extern void mod_set_loadpath(const char *);

/*
 * void 
 * main(void *argp)
 *
 *	Machine-independent system initialization code.
 *
 * Calling/Exit State:
 *
 *	Called from the low level startup code.
 *	Assumed to be only running a single processor.
 *	This function does not return.
 *
 * Description:
 *
 *	Main is executed by process zero immediately after system
 *	initialization and is responsible for creating
 *	the init process and other kernel processes. 
 */
/* ARGSUSED */
void
main(void *argp)
{
	void (**funcp)(void);
	extern void (*postroot_funcs[])(void);

	/*
	 * Print miscellaneous initial startup messages.
	 */
#ifdef DEBUG
	cmn_err(CE_CONT, "\ntotal real memory        = %d\n"
			   "total available memory   = %d\n",
#else
	cmn_err(CE_CONT, "!\ntotal real memory        = %d\n"
			   "total available memory   = %d\n",
#endif
			 totalmem, ptob(epages - pages));

	cmn_err(CE_CONT, "\n%s\n\n", startupmsg);

	/*
	 * Initialize system-wide metrics
	 */
	metrics();

	/*
	 * Initialize the engine online/offline mechanism.
	 */
	engine_init();

	/*
	 * Initialize sync queues, for user-level synch primitives.
	 */
	sq_init();

	/*
	 * Initialize the softwre key mechanism.
	 */
	keyctl_init();

	/*
	 * Initialize User Administration syscall support.
	 */
	uadmin_init();

	/*
	 * Initialize the loadable modules mechanism.
	 */
	modinit();

#ifndef NO_RDMA
	/*
	 * Optimization: see if we wish to convert this system into model
	 * RDMA_SMALL.
	 */
	rdma_convert();
#endif /* NO_RDMA */

	/*
	 * Cause root to be mounted
	 */
	vfs_mountroot();

	/*
	 * Set the DLM default load path based on the name of
	 * the running kernel.
	 */
	mod_set_loadpath(kernel_name);

	/*
	 * Privilege initialization.  Must be done after vfs_mountroot, and
	 * before postroot_funcs, since (for LPM) pm_init sets file privs
	 * in the root filesystem.
	 */
	pm_init();

	/*
	 * doing sysdump in software?
	 */
	if (soft_sysdump)
		sysdump_init();

	/*
	 * Invoke module "postroot" functions.
	 */
	for (funcp = postroot_funcs; *funcp != NULL;)
		(*(*funcp++))();

	/*
	 * Create the "init" process.
	 */
	(void)spawn_proc(NP_INIT|NP_FORK, NULL, exec_initproc, NULL);

#ifdef DKTEST
	/*
	 * Create a process for the "flavors" in-kernel tests.
	 */
	(void)spawn_proc(NP_SYSPROC|NP_FORK, NULL, runflavors, NULL);
#endif

	/*
	 * Create the spawn_sys_lwp daemon.
	 */
	spawn_sys_lwp_daemon();

	/* NOTREACHED */
}
