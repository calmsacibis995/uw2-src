/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/uadmin.c	1.30"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/faultcatch.h>
#include <mem/swap.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/sysconfig.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

/*
 * Administrative system call.
 */

int bootpanic = 0;
void shutdown(void);
STATIC void vnode_zap(proc_t *);
STATIC sleep_t uadmin_lck;
extern void mdboot(int, int);
extern int sysacct(void *uap, rval_t *rvp);

boolean_t in_shutdown;			/* shutdown in progress flag */

#define MAX_LWP	64			/* probably a large number */

LKINFO_DECL(uadmin_class, "uadmin syscall sleep lock", 0);

/*
 * Messages for reboot_prompt().
 */
char *rebootmsg = "Press any key to reboot...";
char *automsg = "Automatic Boot Procedure";

extern time_t c_correct;

struct uadmina {
	int	cmd;
	int	fcn;
	int	mdep;
};

/*
 * void uadmin_init(void)
 *
 *	Initialize priocntl state.
 *
 * Calling/Exit State:
 *
 *	Must be called on the boot processor at initialization time.
 *	Currently only needs to initialize the priocntl_update sleep lock.
 */

void
uadmin_init(void)
{
	SLEEP_INIT(&uadmin_lck, 0, &uadmin_class, KM_NOSLEEP);
}


/*
 * int
 * uadmin(struct uadmina *uap, rval_t *rvp)
 *	System call for administrative control functions
 *
 * Calling/Exit State:
 *	No special MP-specific state is required to make this call and
 *	no additional MP-specific state is held on return.
 *
 *	On success, zero is returned and fields in uap and rvp may be filled
 *	out as per the request specified in uap->cmd. Additional information
 *	may have been copied out to user-mode as well.
 *
 *	On failure, a non-zero errno is returned to indicate the failure
 *	mode.
 *
 * Description:
 *	The following is a list of valid requests (uap->cmd) and their
 *	effects:
 *
 *		A_SWAPCTL:	Perform the request indicated in uap->fcn
 *				by calling swapctl() and passing uap and rvp.
 *
 *				Note that this function is not documented on
 *				the uadmin(2) man page but is accessed through
 *				swapctl(3).
 *
 *		A_SHUTDOWN:	Shutdown the system. Kill all remaining
 *				processes....unmount all filesystems
 *				go to firmware monitor (if available)
 *
 *		A_REBOOT:	Reboot the system immediately without
 *				terminating processes
 *
 *		A_REMOUNT:	To be implemented.
 *
 *		A_CLOCK:	To be implemented.
 *
 *		A_SETCONFIG:	Currently supports the single function
 * 				AD_PANICBOOT which determines the system's
 *				behavior following a system panic.  If mdep
 *				is 1, the system will automatically reboot,
 *				otherwise if mdep is 0, the system will
 *				remain in firmware mode following a panic.
 *
 */
/* ARGSUSED */
int
uadmin(struct uadmina *uap, rval_t *rvp)
{
	int error = 0;

	/* check cmd arg (fcn is system dependent) */
	switch (uap->cmd) {

	case A_SWAPCTL:
	return swapctl((struct swapcmda *)uap, rvp);

	case A_SHUTDOWN:
		break;
	case A_REBOOT:
		break;
	case A_REMOUNT:
		break;
	case A_CLOCK:
		break;
	case A_SETCONFIG:
	        break;
	default:
		return EINVAL;
	}


	if (pm_denied(CRED(), P_SYSOPS))
		return EPERM;

	if (!SLEEP_LOCK_SIG(&uadmin_lck, PRISLEP))
		return EINTR;

	switch (uap->cmd) {


	case A_SHUTDOWN:
		shutdown();
		/*FALLTHRU*/

	case A_REBOOT:
		mdboot(uap->fcn, uap->mdep);
		/*NOTREACHED*/

	case A_REMOUNT:
		/* remount root file system */
		error = VFS_MOUNTROOT(rootvfs, ROOT_REMOUNT);
		break;

	case A_CLOCK:
		/*
		 * correction (in seconds) from local time to GMT is passed.
		 * This is used by wtodc() when called later from stime()
		 * so that rtc is in local time.
		 * Nothing else is done right now in order to avoid needing
		 * code in kernel to convert rtc format to internal format.
		 * That can be done at user level during startup.
		 */
		c_correct = uap->fcn;
		break;

	case A_SETCONFIG:
		switch (uap->fcn) {
		case AD_PANICBOOT:
			bootpanic = (int)uap->mdep;
			break;
		default:
			error = EINVAL;
			break;
		}
		break;
	}

	SLEEP_UNLOCK(&uadmin_lck);	   /* release the lock */
	return error;
}


/*
 * void
 * dis_vfs(int op)
 *	sync or unmount file systems.
 *
 * Calling/Exit State:
 *	No locking required.
 *
 * Description:
 *	Sync or unmount file systems.
 *
 */
void
dis_vfs(int op)
{
	vfs_t *pvfsp, *cvfsp, *ovfsp;
	vnode_t *fsrootvp;

	pvfsp = rootvfs;
	cvfsp = pvfsp->vfs_next;

	while (cvfsp != NULL) {
		ovfsp = cvfsp;

		switch (op) {
		case UADMIN_SYNC:
			(void)VFS_SYNC(cvfsp, SYNC_CLOSE, CRED());
			break;
		case UADMIN_UMOUNT:
			(void)VFS_ROOT(cvfsp, &fsrootvp);
			if (dounmount(cvfsp, CRED()) != 0)
				VN_RELE(fsrootvp);
			break;
		default:
			break;
		}

		cvfsp = pvfsp->vfs_next;
		if (cvfsp == ovfsp) {
			pvfsp = cvfsp;
			cvfsp = cvfsp->vfs_next;
		}
	}
}

/*
 * void
 * shutdown_proc(proc_t)
 *	shutdown process pp
 *
 *
 * Calling/Exit State:
 *	Called with no locks
 *	Returns with no locks held
 *
 * Description:
 *
 *	Terminate process pp with extreme prejudice.
 *
 *	If a SIGKILL dose not  bring it down forcefully,
 *	free it's current directory and root vnodes.
 *
 *	Unmount all filesystems (including / )
 *
 */

void
shutdown_proc(proc_t * pp )
{
	int i;
	void *proc;
	struct pid * pidp;

	(void) LOCK(&pp->p_mutex, PLHI);	/* re-lock process slot */
	sigtoproc_l(pp, SIGKILL, (sigqueue_t *)NULL);	/* kill the process */
	pidp = pp->p_pidp;

	UNLOCK(&pp->p_mutex, PLBASE);		/* Release p_mutex */
	proc = proc_ref_pp(pidp);

	/*
	 * Wait for the process to die
	 * or we timeout
	 */

	for (i = 0; i < 50; i++){
		delay(HZ/5);			/* give up the cpu */
		if (proc_valid_pp(proc) == B_FALSE )	/* this one gone ? */
			break;
	}

	if (!proc_valid_pp(proc)) {	/* process exited ? */
		proc_unref(proc);
		return;
	}

	proc_unref(proc);		/* free the reference to proc */

	/*
	 * If we get here process is unkillable;
	 * free current and root directory inodes.
	 */

	vnode_zap(pp);
}

/*
 * void
 * shutdown(void)
 *	shutdown all processes and unmount all filesystems
 *
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *	Returns with no locks held.
 *
 * Description:
 *	Shutdown the system by killing any remaining
 *	(non system deamon) processes in order to
 *	free vnode references they may hold
 *
 *	Unmount all filesystems (including / )
 *
 */

void
shutdown(void)
{
	struct proc *pp;
	struct proc *selfp = u.u_procp;		/* our process slot	*/
	void (**funcp)(void);
	extern void (*io_halt[])(void);
	extern int bindcpu_halt[];
	engine_t *bind_eng;
	int bind_engno;
	int i;
	char *acctname = NULL;

	ASSERT(getpl() == PLBASE);

	/*
	 * Ignore all signals, and clear the signal trace mask to protect
	 * us from all future signals.
	 */
	(void)LOCK(&selfp->p_mutex, PLHI);

	sigfillset(&selfp->p_sigignore);
	sigemptyset(&selfp->p_sigtrmask);

	UNLOCK(&selfp->p_mutex, PLBASE);

	in_shutdown = B_TRUE;			/* shutdown in progress */

	shutdown_proc(proc_init);		/* terminate init process*/

	/*
	 * traverse the active process list
	 * sending kill signals to all non system processes
	 */
again:
	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (pp = practive; pp != NULL; pp = pp->p_next) {

		(void)LOCK(&pp->p_mutex, PLHI);	/* lock process slot */

		if ((pp->p_nlwp > 0) && (pp->p_execinfo)
				     && ((pp->p_execinfo->ei_execvp) != NULL)
				     && (pp != selfp) ){

			UNLOCK(&pp->p_mutex, PL_PROCLIST); /* Release p_mutex */
			RW_UNLOCK(&proc_list_mutex, PLBASE);
			shutdown_proc(pp);		/* time to die */
			goto again;

		} else {
			UNLOCK(&pp->p_mutex, PL_PROCLIST);/* Release p_mutex */
		}
	}

	RW_UNLOCK(&proc_list_mutex, PLBASE);

	if (!(u.u_procp->p_flag & P_SYS)) {
		/*
		 * Close all files held (directly or indirectly) by this process,
		 * so they won't cause filesystems to be considered busy.
		 *
		 * First make sure we're single-threaded.
		 */

		(void)destroy_proc(B_TRUE);
		ASSERT(SINGLE_THREADED());

		relvm(u.u_procp);
		closeall(u.u_procp);
		vnode_zap(u.u_procp);
	}

	/* Turn off process accounting. */
	(void)sysacct(&acctname, (rval_t *)0);

	/*
	 * Sync and unmount all mounted filesystems.
	 */

	dis_vfs(UADMIN_SYNC);
	dis_vfs(UADMIN_UMOUNT);

	(void) VFS_MOUNTROOT(rootvfs, ROOT_UNMOUNT);

	/*
	 * Now that all the processes are killed and file systems
	 * unmounted, we can proceed to halt the drivers.
	 */
	for (funcp = io_halt, i = 0; *funcp != NULL;) {
		bind_engno = bindcpu_halt[i++];
		bind_eng = &engine[bind_engno];

		if (bind_engno != -1)
			(void) kbind(bind_eng);
		(*(*funcp++))();
	}
}


/*
 * void
 * vnode_zap(proc_t * pp)
 *	release vnodes from process p
 *
 *
 * Calling/Exit State:
 *	Called with no locks held
 *	On return no locks held
 *
 * Description:
 *	Forcefully free vnode references for process p
 *	a.out vnode, root/current directories, and any unique lwp directories
 *	the address space of the process is also freed
 *
 *	Process p is hung and we free the inodes to try to
 *	maintain a consistant filsystem.
 *
 */

STATIC void
vnode_zap(proc_t * pp)
{
	vnode_t * execvp;			/* vnodes that must be	*/
	vnode_t * vp_rdir;			/*  released for	*/
	vnode_t * vp_cdir;			/* each running process */
	vnode_t * lwp_vnodes[MAX_LWP];	/* lwp's with unique roots */
	vnode_t ** vp_lwp;
	lwp_t *lwpp;


	(void) LOCK(&pp->p_mutex, PLHI);	/* lock process slot */

	execvp = pp->p_execinfo->ei_execvp;
	pp->p_execinfo->ei_execvp = NULL;

	/*
	 * get the vnodes for current/root directories
	 */

	(void)CUR_ROOT_DIR_LOCK(pp);
	vp_rdir = pp->p_rdir;
	vp_cdir = pp->p_cdir;

	pp->p_rdir = NULL;
	pp->p_cdir = NULL;

	/*
	 *  scan lwp's for unique
	 *  root directories
	 *  save any we find in lwp_vnodes[] array
	 */

	vp_lwp = lwp_vnodes;			/* start of list */
	lwpp =	pp->p_lwpp;			/* first lwp */

	while (lwpp) {			/* collect the vnodes */

		(void)LOCK(&lwpp->l_mutex, PLHI);

		/* root dirs of lwp's may be unsynced */
		if (lwpp->l_rdir && lwpp->l_rdir != vp_rdir) {
			if (vp_lwp < &lwp_vnodes[MAX_LWP])
				*vp_lwp++ = lwpp->l_rdir;
		}

		UNLOCK(&lwpp->l_mutex, PLHI);
		lwpp = lwpp->l_next;		/* next lwp in list  */
	}

	CUR_ROOT_DIR_UNLOCK(pp, PLHI);
	UNLOCK(&pp->p_mutex, PLBASE);	 /* Release p_mutex */


	/* do the actual freeing of vnodes */

	if (execvp)		/* shed a.out vnode */
		VN_RELE(execvp);

	if (vp_rdir)		/* shed this root vnode */
		VN_RELE(vp_rdir);

	if (vp_cdir)		/* shed this current directory vnode */
		VN_RELE(vp_cdir);

	/* now free unique rdirs that the lwp's may hold */

	vp_lwp = lwp_vnodes;

	while (vp_lwp-- != lwp_vnodes)
		VN_RELE(*vp_lwp);

	return;
}


/*
 * void
 * drv_shutdown(shutdown_request_t sd_rqt, int fcn)
 *	Initiate a system shutdown
 *
 * Calling/Exit State:
 *	No locks required on entry or held on exit.
 *
 */
void
drv_shutdown(shutdown_request_t sd_rqt, int fcn)
{
	int sig, c;

	switch (sd_rqt) {
	case SD_HARD:
		/* Shutdown without trying to clean up at all. */
		mdboot(fcn, 0);
		/* NOTREACHED */
	case SD_SOFT:
		/*
		 * Do a full, clean shutdown.  This is done by sending a
		 * signal to "init", causing it to enter a new state and
		 * initiate a shutdown.
		 */
		switch (fcn) {
		case AD_HALT:
			sig = SIGINT;	/* init 0 */
			break;
		case AD_IBOOT:
			sig = SIGEMT;	/* init 5 */
			break;
		case AD_BOOT:
			sig = SIGFPE;	/* init 6 */
			break;
		}
		sigtoproc(proc_init, sig, NULL);
		break;
	case SD_PANIC:
		switch (fcn) {
		case AD_QUERY:
			cmn_err(CE_CONT, 
				"\nGenerate a panic on the system ? (y or n) : ");
			CONSOLE_SUSPEND();
			while ((c = CONSOLE_GETC()) == -1)
				;
			while (CONSOLE_PUTC(c) == 0)
				;
			CONSOLE_RESUME();

			if (c == 'y' || c == 'Y')
				cmn_err(CE_PANIC, "hot key panic\n");
			break;

		case AD_NOQUERY:
			cmn_err(CE_PANIC, "hot key panic\n");
			/* NOTREACHED */
		}
	default:
		break;
	}
}


/*
 * void
 * reboot_prompt(int fcn)
 *	Prompt with a reboot message and, if appropriate, waits for a key.
 *
 * Calling/Exit State:
 *	Called at shutdown time after other engines are halted.
 *
 */
void
reboot_prompt(int fcn)
{
	if (fcn == AD_BOOT)
		cmn_err(CE_CONT, "\n%s\n", automsg);
	else {
		/* Flush any pending console input. */
		CONSOLE_SUSPEND();
		while (CONSOLE_GETC() != -1)
			;

		cmn_err(CE_CONT, "\n%s\n", rebootmsg);

		/* Wait for a console key press. */
		while (CONSOLE_GETC() == -1)
			;
	}
}
