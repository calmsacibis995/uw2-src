/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/exit.c	1.67"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/cred.h>
#include <proc/resource.h>
#include <proc/exec.h>
#include <proc/wait.h>
#include <proc/proc_hier.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <proc/acct.h>
#include <mem/kmem.h>
#include <mem/ublock.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <svc/sco.h>
#include <svc/reg.h>
#include <util/dl.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <proc/usync.h>
#include <proc/bind.h>
#include <mem/lock.h>

extern void keyctl_exit(void);
extern void lwp_cleanup_f(lwp_t *);
extern void semexit(void);
extern void shmexit(proc_t *);
extern void xsdexit(void);
extern void timer_cancel(void);
STATIC void exit_final(lwp_t *);

/*
 * Key wait/exit printf() debugging off of DBG_WE_PRF.
 */
/*#define       DBG_WE_PRF*/

#if defined(DBG_WE_PRF) && !defined(lint)
#define DPRINTF1(arg1)			printf((arg1))
#define DPRINTF2(arg1, arg2)		printf((arg1), (arg2))
#define	DPRINTF3(arg1, arg2, arg3)	printf((arg1), (arg2), (arg3))
#else   /* !DBG_PRINTF */
#define	DPRINTF1(arg1)
#define DPRINTF2(arg1, arg2)
#define	DPRINTF3(arg1, arg2, arg3)
#endif  /* !DBG_PRINTF */


/*
 * Macro to remove a child process from the parent-child-sibling chain
 * of its parent process.
 *
 * Locking requirements:
 *	The p_mutex lock of the parent process must be held upon entry.
 *	The p_mutex lock remains held upon return.
 */
#define	RM_CHILD_FROM_PARENT(cp, pp)		\
{						\
	proc_t *prevsib, *nextsib;		\
						\
	ASSERT(LOCK_OWNED(&(pp)->p_mutex));	\
	nextsib = (cp)->p_nextsib;		\
	prevsib = (cp)->p_prevsib;		\
	if (prevsib == NULL)			\
		(pp)->p_child = nextsib;	\
	else					\
		prevsib->p_nextsib = nextsib;	\
	if (nextsib != NULL)			\
		nextsib->p_prevsib = prevsib;	\
}


/*
 * Macro for the designated next-of-kin process to inherit the {grand}child
 * resource usage statistics.
 *
 * Locking requirements:
 *	The p_mutex lock of the next-of-kin process must be held upon entry,
 *	unless the next-of-kin process is the parent, is exiting, and is
 *	reaping an ignored zombie child process.  If the p_mutex lock is
 *	held upon entry, it remains held upon return.
 */
#define	INHERIT_RESOURCE_USAGE(cp, nxtkp) \
	(void) ((nxtkp)->p_cutime = ladd((nxtkp)->p_cutime, (cp)->p_utime), \
		(nxtkp)->p_cstime = ladd((nxtkp)->p_cstime, (cp)->p_stime))


/*
 * Macro to make the given process invisible to prfind() and /proc.
 * Locking requirements:
 *	The p_mutex lock of the given process must be held upon entry.
 *	The p_mutex lock remains held upon return.
 */
#define	RM_PROC_FROM_SYSTEM(p)						\
{									\
	extern void prexit(proc_t *);					\
									\
	ASSERT(LOCK_OWNED(&(p)->p_mutex));				\
	if ((p)->p_trace != NULL) {					\
		prexit(p);		/* make invisible to /proc */	\
	}								\
	(p)->p_pidp->pid_procp = NULL;	/* make invisible to prfind */	\
}


/*
 *
 * int wstat(int code, int data)
 *	Convert code/data pair into old style wait status.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 *	The 'code' parameter identifies the reason for the process
 *	state change.  The data parameter identifies data associated
 *	with the state change (e.g., the signal that killed, stopped,
 *	or caused the process to core dump, the exit status given to
 *	the exit(2) system call, etc.).
 *
 */
int
wstat(int code, int data)
{
	int stat = data & 0377;

	switch (code) {
	case CLD_EXITED:
		stat <<= 8;
		break;
	case CLD_DUMPED:
		stat |= WCOREFLG;
		break;
	case CLD_KILLED:
		break;
	case CLD_TRAPPED:
	case CLD_STOPPED:
		stat <<= 8;
		stat |= WSTOPFLG;
		break;
	case CLD_CONTINUED:
		stat = WCONTFLG;
		break;
	}

	return stat;
}


struct exita {
	int	rval;
};

/*
 *
 * void rexit(struct exita *uap, rval_t *rvp)
 *	This is the exit(2) system call.
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *	This function does not return.
 *
 */
/* ARGSUSED */
void
rexit(struct exita *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	exit(CLD_EXITED, uap->rval);
}


/*
 *
 * void exit(int why, int what)
 *	Cause the calling process to exit for the indicated reason.
 *
 * Calling/Exit State:
 *	No locks held upon entry.
 *
 *	The 'why' parameter identifies the reason for exiting:
 *	CLD_EXITED, CLD_DUMPED, CLD_KILLED. 
 *
 *	The 'what' parameter identifies information dependent upon
 *	the 'why' parameter as follows:
 *		CLD_EXITED: what contains the exit code given to exit(2).
 *		CLD_DUMPED: contains the signal number causing the core dump.
 *		CLD_KILLED: contains the signal that killed the process.
 *
 * Remarks:
 *	This function proceeds through several phases:
 *	1. Destroy all other LWPs in the process (unless another racing
 *	   exit or exec operation has marked us for destruction first,
 *	   in which case exit() calls lwp_exit()).
 *	2. Release resources.
 *	3. Disinherit children.
 *	4. Enter the zombie state.
 *	5. Coordinate our exit with our parent.
 *
 */
void
exit(int why, int what)
{
	proc_t		*p = u.u_procp;		/* exiting (calling) process */
	proc_t		*cp;			/* child process */
	proc_t		*nxtsib;		/* next sibling */
	proc_t		*q;			/* distant relative */
	lwp_t		*lwpp = u.u_lwpp;	/* exiting LWP */
	sess_t		*sessp;			/* session of process */
	vnode_t		*cdir_vp;		/* process current dir vp */
	vnode_t		*rdir_vp;		/* process root dir vp */
	vnode_t		*lrdir_vp;		/* LWP root directory vp */
	execinfo_t	*eip;			/* execinfo for process */
	boolean_t	ignored_zombies = B_FALSE;
	extern 		int in_shutdown;	/* shutting down system */

	ASSERT(KS_HOLD0LOCKS());

	DPRINTF2("DBG: exit: Begin.  pid=%d\n", p->p_pidp->pid_id);

	if (p == proc_init && (!in_shutdown)) {
		/*
		 *+ The system init process has exit'ed for some reason.
		 */
		cmn_err(CE_PANIC, "Init died! (why = %d, what = %d)",
			why, what);
	}

	/*
	 * Destroy all of the other LWPs in the process, and also
	 * indicate that the calling LWP will eventually be destroyed
	 * as well.  However, if some other LWP has already requested
	 * that all other LWPs in the process be destroyed, then we
	 * defer to the operation already in progress.
	 */
	if (!destroy_proc(B_TRUE)) {
		/*
		 * Another LWP in the process has requested that we be
		 * destroyed.  Exit as though we had called lwp_exit().
		 */
		lwp_exit();
		/* No return */
	}

	timer_cancel();				/* Cancel all LWP timers. */

	/*
	 * The successful invocation of the destroy_proc() primitive above
	 * waited for all on-going I/O via /proc to the exiting process to
	 * complete.  Also, all subsequent I/O operations upon the process
	 * have been prevented.
	 *
	 * Deinitialize the process r/w lock, and deallocate the system
	 * call tracing sets of the process now that all other LWPs are
	 * destroyed, and new /proc operations beyond those supported for
	 * a zombie process are prohibited.
	 */
	RWSLEEP_DEINIT(&p->p_rdwrlock);	/* /proc r/w lock now unused */
	if (p->p_entrymask != NULL) {
		DPRINTF3("DBG: exit: freeing p_entrymask: 0x%x; pid=%d\n",
			(int)&p->p_entrymask, p->p_pidp->pid_id);
		kmem_free(p->p_entrymask, sizeof (k_sysset_t));
	}
	if (p->p_exitmask != NULL) {
		DPRINTF3("DBG: exit: freeing p_exitmask: 0x%x; pid=%d\n",
			(int)&p->p_exitmask, p->p_pidp->pid_id);
		kmem_free(p->p_exitmask, sizeof (k_sysset_t));
	}

	/*
	 * Generate process accounting record; this must
	 * be done before the execinfo structure is freed.
	 */
	acct((char)(wstat(why, what) & 0xff));

	/*
	 * Ignore all signals, and clear the signal trace mask to protect
	 * us from all future signals.
	 */
	(void)LOCK(&p->p_mutex, PLHI);

	sigfillset(&p->p_sigignore);
	sigemptyset(&p->p_sigtrmask);

	UNLOCK(&p->p_mutex, PLBASE);

	/*
	 * Discard process and LWP root directory, and the process
	 * current directory.  Pointers for these are NULLed while holding
	 * p_dir_mutex to prevent races with dofusers().
	 */
	(void)CUR_ROOT_DIR_LOCK(p);
	rdir_vp = p->p_rdir;
	lrdir_vp = lwpp->l_rdir;
	p->p_rdir = lwpp->l_rdir = NULL;

	cdir_vp = p->p_cdir;
	p->p_cdir = NULL;
	CUR_ROOT_DIR_UNLOCK(p, PLBASE);

	if (cdir_vp != NULL)
		VN_RELE(cdir_vp);
	if (rdir_vp != NULL) {				/* usually NULL */
		if (rdir_vp == lrdir_vp) {
			VN_RELEN(rdir_vp, 2);
		} else {
			VN_RELE(rdir_vp);
			if (lrdir_vp != NULL) {		/* could be NULL */
				VN_RELE(lrdir_vp);
			}
		}
	}

	/*
	 * In destroy_proc(), p_nlwpdir was set to zero making us invisible
	 * to any subsequent processor bind/unbind requests.  Remove any
	 * processor binding against us now.
	 */
	bind_exit(lwpp);

	/*
	 * Shed any attachment to user-mode synchronization objects.
	 */
	sq_exit(lwpp);

	/*
	 * If per-process hash list was allocated, deallocate it.
	 */
	sq_hashfree(p);

	/*
	 * Discard all pending process instance signals, and all current
	 * and pending LWP instance signals for the last LWP in the process.
	 */
	discard_procsigs();
	discard_lwpsigs(lwpp);

	closeall(p);			/* Close all open file descriptors. */

	/*
	 * Since we are the only context, we can read the p_sessp and p_nshmseg
	 * without the protection of any locks.
	 * Also nobody can manipulate my address space.
	 */
	ASSERT(p->p_sessp != NULL);
	sessp = p->p_sessp;

	/*
	 * If we are the leader of our session, detach the session from
	 * the controlling terminal.
	 */
	if (sessp->s_sidp == p->p_pidp && sessp->s_vp != NULL) {
		ADT_LOGOFF_REC();
		freectty();
	}

	keyctl_exit();

	if (p->p_semundo != NULL)	/* SystemV IPC: semaphores */
		semexit();

	if (p->p_nshmseg)
		shmexit(p);

	if (p->p_sdp)
		xsdexit();		/* XENIX shared data exit */

	/*
	 * Disassociate the final LWP's ublock from the process.
	 */
	ublock_lwp_detach(lwpp);

	(void)LOCK(&p->p_mutex, PLHI);

	/*
	 * Discard process execinfo structure.  The pointer from the
	 * proc structure to its execinfo is NULLed while holding p_mutex
	 * to interlock with dofusers().
	 */
	eip = p->p_execinfo;
	p->p_execinfo = NULL;

	/*
	 * Free the address space.  Before freeing the address space,
	 * set P_NOSEIZE.  After p_mutex is dropped, no one else will
	 * be able to seize us.  However, we must honor any pending
	 * seize request.
	 */
	p->p_flag |= P_NOSEIZE;		/* no more seize after unlock */
	if (p->p_flag & P_SEIZE) {
		UNLOCK(&p->p_mutex, PLBASE);
		CL_PREEMPT(lwpp, lwpp->l_cllwpp); /* honor pending seize */
	} else {
		UNLOCK(&p->p_mutex, PLBASE);
	}

	relvm(p);

	/*
	 * Freeing the execinfo structure does a VN_RELE()
	 * on the a.out vnode associated with the process.
	 */
	if (eip != NULL)
		eifree(eip);

	/* Free audit process structure and record event */
	ADTEXIT(p->p_auditp, why);

	/*
	 * Orphan ({grand})child processes whose resource usage statistics
	 * we were to have inherited, to our next of kin (usually our parent).
	 */
	(void)RW_WRLOCK(&proc_list_mutex, PL_PROCLIST);
	if ((q = p->p_orphan) != NULL) {
		proc_t *nxtkp = p->p_nextofkin;

		DPRINTF2("DBG: exit: orphaning next-of-kin processes; pid=%d\n",
			 p->p_pidp->pid_id);

		for (;;) {
			(void)LOCK(&q->p_mutex, PLHI);
			q->p_nextofkin = nxtkp;
			UNLOCK(&q->p_mutex, PLHI);
			if (q->p_nextorph == NULL)
				break;
			q = q->p_nextorph;
		}

		nxtkp->p_orphan->p_prevorph = q;
		q->p_nextorph = nxtkp->p_orphan;
		nxtkp->p_orphan = p->p_orphan;
		p->p_orphan = NULL;
	}
	RW_UNLOCK(&proc_list_mutex, PLBASE);

	/*
	 * If we are ignoring exiting children, prevent concurrently
	 * exiting child processes from changing our child-sibling list.
	 */
	if (p->p_flag & P_NOWAIT) {
		(void)LOCK(&p->p_mutex, PLHI);
		p->p_flag |= P_CLDWAIT;
		UNLOCK(&p->p_mutex, PLBASE);
	}

	/*
	 * Exit from our process group, and SIGHUP/SIGCONT all process
	 * group(s) orphaned by our departure (the child list is walked).
	 */
	(void)LOCK(&sessp->s_mutex, PL_SESS);

	pgdetach();

	for (cp = p->p_child; cp != NULL; cp = nxtsib) {

		DPRINTF2("DBG: exit: disinheriting children to init; pid=%d\n",
			 p->p_pidp->pid_id);

		nxtsib = cp->p_nextsib;
		(void)LOCK(&cp->p_mutex, PLHI);
		if (cp->p_nlwp == 0 && cp->p_wcode == 0) {
			ignored_zombies = B_TRUE;
			UNLOCK(&cp->p_mutex, PL_SESS);
			continue;
		}

		/*
		 * The child process is either a zombie that
		 * we haven't waited for (and we're not
		 * ignoring exiting children), or it is not
		 * a zombie process.  In either case, init(1)
		 * gets it now.
		 *
		 * Insert our child into init's child-sibling
		 * list, while holding our childs p_mutex lock.
		 * Once we release the disinherited childs p_mutex
		 * lock, it could exit.  If init were ignoring its
		 * children at that time, then corruption would occur
		 * if the child exited and freeproc()ed itself
		 * (though it's really hard to imagine a useful
		 * version of init(1) that ignores children).
		 *
		 * We insert the child at the beginning
		 * of init's child-sibling list.
		 * Anywhere else would be disastrous,
		 * since init may be concurrently walking its
		 * child-sibling list in waitid().
		 */
		(void)LOCK_SH(&proc_init->p_mutex, PLHI);
		cp->p_ppid = 1;
		cp->p_parent = proc_init;

		cp->p_nextsib = proc_init->p_child;
		if (proc_init->p_child != NULL)
			proc_init->p_child->p_prevsib = cp;
		cp->p_prevsib = NULL;
		proc_init->p_child = cp;

		/*
		 * If init is not a zombie, signal it that it has new
		 * children, but only when the last child is disinherited.
		 */
		if (nxtsib == NULL && proc_init->p_nlwp != 0)
			sigcld_l(cp);

		UNLOCK(&proc_init->p_mutex, PLHI);
		UNLOCK(&cp->p_mutex, PL_SESS);
	}

	UNLOCK(&sessp->s_mutex, PLBASE);

	/*
	 * If we have any ignored zombie processes, clean
	 * them up now.
	 */
	if (ignored_zombies) {
		ASSERT(p->p_zombies != NULL);
		(void)LOCK(&p->p_mutex, PLHI);
		freeprocs();			/* releases p_mutex @ PLBASE */
	}

	/*
	 * Now that all child processes have been disinherited to init(1),
	 * discard any sigqueue_t structure allocated for SIGCLD delivery.
	 * However, the init process must always keep this structure.
	 */
	if (p != proc_init && p->p_sigcldinfo != NULL) {
		DPRINTF3("DBG: exit: freeing p_sigcldinfo: 0x%x; pid=%d\n",
			 (int)&p->p_sigcldinfo, p->p_pidp->pid_id);
		kmem_free(p->p_sigcldinfo, sizeof (sigqueue_t));
		p->p_sigcldinfo = NULL;
	}

	/*
	 * Discard the profil(2) structure allocated for the process, if any.
	 */
	if (p->p_profp != NULL) {
		SLEEP_DEINIT(&p->p_profp->pr_lock);
		DPRINTF3("DBG: exit: freeing p_profp: 0x%x; pid=%d\n",
			 (int)&p->p_profp, p->p_pidp->pid_id);
		kmem_free(p->p_profp, sizeof (struct prof));
		p->p_profp = NULL;
	}

	/*
	 * Release process and LWP resource limits reference.
	 * NOTE: No locking of p_rlimits, since /proc does not presume
	 *	 access when P_DESTROY is set (from /proc perspective,
	 *	 the process is already a zombie).
	 */
	if (p->p_rlimits == u.u_rlimits) {	/* very likely */
		rlfreen(p->p_rlimits, 2);
	} else {
		rlfree(p->p_rlimits);
		rlfree(u.u_rlimits);
	}
	p->p_rlimits = u.u_rlimits = NULL;

	/*
	 * Release LWP credential reference.  The process
	 * credentials cannot be released until the process is
	 * freeproc()ed.  Other processes may find this process via
	 * the practive list, the procent directory, or a /proc vnode
	 * and examine its credentials (e.g., /proc or dofusers()).
	 *
	 * NOTE: No locking of l_cred, since /proc does not presume
	 *	 access when P_DESTROY is set (from /proc perspective,
	 *	 the process/LWP is already a zombie, and dofusers()
	 *	 only looks at the process credentials).
	 */
	crfree(lwpp->l_cred);
	lwpp->l_cred = NULL;

	/*
	 * Save the "why" and "what" parameters away in a temporary
	 * location in the proc structure before we step onto the private
	 * stack, once on the private stack, the parameters are inaccessible.
	 * After we acquire our p_mutex lock (and parents p_mutex lock), set
	 * the process wait and data code from the values saved away here.
	 */
	p->p_tmp_wcode = (char)why;
	p->p_tmp_wdata = what;

	/*
	 * Tell the scheduling class that we are exiting
	 * (can't sleep from now on).
	 */
	DPRINTF2("DBG: exit: calling CL_EXITCLASS and use_private; pid=%d\n",
		 p->p_pidp->pid_id);
	(void)LOCK(&lwpp->l_mutex, PLHI);
	CL_EXITCLASS(lwpp, lwpp->l_cllwpp);

	/*
	 * At this point, the only way the lwp can be "found" is by
	 * taking a clock interrupt and having the clock handler perform
	 * per-lwp services against this lwp.
	 *
	 * Notify the dispatcher that this context is no longer a
	 * viable lwp (preventing the clock handler from performing
	 * per-lwp services against the remains of this context).
	 */
	dispnolwp();

	/*
	 * Switch to per-processor private stack.
	 */
	DPRINTF3("DBG: exit: calling use_private(); pid=%d; lwpp=0x%x\n",
		 p->p_pidp->pid_id, (int)lwpp);
	use_private(lwpp, exit_final, lwpp);
	/* NOT REACHED */
}

/*
 * void exit_final(lwp_t *lwpp)
 *	Perform the "last rituals" for the exiting process. All the 
 *	final exit processing after the context has stepped onto the 
 *	per-engine stack are performed here.
 *
 * Calling/Exit State:
 *	No locks can be held on entry. The function does not return.
 *
 * Remarks:
 *	This function is invoked by use_private() after stepping onto the 
 *	per-engine stack.
 */
STATIC void
exit_final(lwp_t *lwpp)
{
	proc_t	*p = lwpp->l_procp;
	proc_t	*pp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Deallocate the per-scheduling class data associated with this
	 * lwp. 
	 */
	CL_DEALLOCATE(&class[lwpp->l_cid], lwpp->l_cllwpp);
	lwpp->l_cllwpp = NULL;

	DPRINTF3("DBG: exit: final exit sequence; p_mutex: 0x%x; pid=%d\n",
		 (int)&p->p_mutex, p->p_pidp->pid_id);

	/* Do family-specific LWP teardown. */
	lwp_cleanup_f(lwpp);

	/*
	 * Deallocate the ublock and associated structures.
	 * Call ublock_proc_deinit before ublock_lwp_free so ublock_lwp_free
	 * can avoid unnecessary work.
	 */
	ublock_proc_deinit(p);
	ublock_lwp_free(lwpp);

	(void)LOCK(&p->p_mutex, PLHI);
	pp = p->p_parent;		/* pointer to our parent process */
	p->p_lwpp = NULL;		/* record no more LWPs under lock */
	p->p_utime = ladd(p->p_utime, p->p_cutime);
	p->p_stime = ladd(p->p_stime, p->p_cstime);
	(void)LOCK_SH(&pp->p_mutex, PLHI);
	p->p_nlwp = 0;			/* Mark ourself as a zombie */

	/* Wake up anyone waiting for this process to become stopped. */
	if (p->p_trace)
		prwakeup(p->p_trace);

	if (pp->p_flag & P_NOWAIT) {	/* Parent is ignoring child exits */
		PROC_NOWAIT(p);		/* Mark ourself as non-waitable */
		pp->p_flag |= P_CLDEVT;	/* Tell parent to recheck our state */
		if (pp->p_flag & P_CLDWAIT) {
			/*
			 * Our parent is walking its child-sibling list.
			 * Put ourselves on our parents ignored zombie
			 * list to be cleaned up later.
			 */
			p->p_nextzomb = pp->p_zombies;
			pp->p_zombies = p;
			UNLOCK(&pp->p_mutex, PLHI);
			UNLOCK(&p->p_mutex, PLBASE);
			DPRINTF2("DBG: exit: P_NOWAIT/waitid; zomb; pid=%d\n",
				 p->p_pidp->pid_id);
		} else {
			/*
			 * Our parent is not walking its child-sibling
			 * list.  Freeproc() ourselves now.
			 *
			 * NOTE: If any LWPs are blocked in the parent in the
			 *	 wait(2)/waitid(2)/waitpid(2) system calls,
			 *	 we must "nudge" them here.  Otherwise, they
			 *	 may not properly recognize ECHILD, or when
			 *	 waiting on us specifically (admittedly a
			 *	 weird thing to do if you're ignoring your
			 *	 child exits), that we have exited.
			 */
			if (SV_BLKD(&pp->p_waitsv))
				SV_BROADCAST(&pp->p_waitsv, 0);

			freeproc(p);	/* all locks released upon return */
					/* The proc structure is gone! */
			DPRINTF1("DBG: exit: P_NOWAIT/freeproc\n");
		}
	} else {
		/*
		 * Our parent is not ignoring exiting children.
		 * Mark state appropriately as waitable (now that we hold
		 * both our p_mutex lock, and our parents p_mutex lock),
		 * and signal our parent.
		 */
		p->p_wcode = p->p_tmp_wcode;
		p->p_wdata = p->p_tmp_wdata;
		sigcld_l(p);
		UNLOCK(&pp->p_mutex, PLHI);
		UNLOCK(&p->p_mutex, PLBASE);
		DPRINTF2("DBG: exit: P_NOWAIT clear/sigcld_l(); pid=%d\n",
			 p->p_pidp->pid_id);
	}

	/*
	 * We delay releasing the lwp structure until now, since we were
	 * not able to set p_lwpp to NULL under p_mutex lock until just above.
	 */
	LOCK_DEINIT(&lwpp->l_mutex);
	DPRINTF2("DBG: exit: freeing lwp_t 0x%x\n", (long)lwpp);
	kmem_free((void *)lwpp, sizeof *lwpp);

	DPRINTF1("DBG: exit: calling swtch()\n");

	swtch((lwp_t *)NULL);		/* Find something useful to do. */
	/* NOTREACHED */
}


/*
 *
 * Epilogue code macro for returning from waitid().
 *
 * The following macro is executed upon any return from waitid() where
 * ignored zombie processes could have been accumulated, and hence
 * (among other things) cleans up any child processes that exited while
 * the process was ignoring exited children.
 *
 * Locking requirements:
 *	The p_mutex lock of the invoking process must be held upon entry.
 *	It is released upon return.
 *
 */
#define WAIT_EPILOGUE(p)						\
	(void)	(ASSERT(LOCK_OWNED(&(p)->p_mutex)),			\
		(p)->p_flag &= ~P_CLDWAIT,				\
		/* Check for ignored zombies. */			\
		(((p)->p_zombies != NULL)				\
			? freeprocs() : UNLOCK(&(p)->p_mutex, PLBASE)),	\
		/* If another LWP is waiting to wait, let it in. */	\
		SV_SIGNAL(&(p)->p_wait2wait, 0))
/*
 *
 * boolean_t
 * waitfor_nonzomb(proc_t *cp, int wcode, int opts,
 *		   id_t pgid, k_siginfo_t *ip)
 *	Wait for a non-zombie process.
 *
 * Calling/Exit State:
 *    Parameters:
 *	cp:	 child process to be waited for.
 *	wcode:	 desired wait code of the target child.
 *	opts:    waitid(2) wait options.
 *	pgid:	 non-zero iff the child must be found in the given pgrp (pgid).
 *	ip:	 wait information buffer.
 *
 *    Locking requirements:
 *	No locks can be held at entry.
 *	No locks remain held at exit.
 *
 *    Return value:
 *	The function returns B_TRUE if the target process is found in
 *	the expected state after the appropriate locks are acquired.
 *	Otherwise, B_FALSE is returned.
 *
 */
STATIC boolean_t
waitfor_nonzomb(proc_t *cp, int wcode, int opts, id_t pgid, k_siginfo_t *ip)
{
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * If waiting for processes in a specific process group, 
	 * make sure that the child is still in the target process group.
	 */
	(void)LOCK(&cp->p_mutex, PLHI);
	if (pgid && pgid != cp->p_pgid) {
		UNLOCK(&cp->p_mutex, PLBASE);
		return B_FALSE;
	}
	if (cp->p_wcode != wcode) {	/* raced with child state change */
		UNLOCK(&cp->p_mutex, PLBASE);
		return B_FALSE;
	}
	winfo(cp, ip, (opts & WNOWAIT) ? B_FALSE : B_TRUE);
	UNLOCK(&cp->p_mutex, PLBASE);
	return B_TRUE;
}


/*
 *
 * int waitid(idtype_t idtype, id_t id, k_siginfo_t *ip, int opts)
 *	Wait for a child process to enter a waitable state.
 *
 * Calling/Exit State:
 *    Parameters:
 *	idtype:	 one of: P_PID, P_PGID, P_ALL.
 *	id:	 process-ID if idtype = P_PID;
 *		 process group-ID if idtype = P_PGID.
 *		 Otherwise unused.
 *	ip:	 pointer to wait information buffer.
 *	opts:    waitid(2) options:
 *		   WEXITED, WTRACED, WSTOPPED, WCONTINUED, WNOHANG, WNOWAIT.
 *
 *    Return value:
 *	If a waitable child process is found, this function returns zero.  
 *	Otherwise, the non-zero errno code identifying the cause of 
 *	the failure is returned.
 *
 * Remarks:
 *	This function searches for a terminated (zombie) child.
 *	If one is found, it is laid to rest and its status is returned.
 *	A stopped or continued child will also be noticed as appropriate.
 *
 */
int
waitid(idtype_t idtype, id_t id, k_siginfo_t *ip, int opts)
{
	proc_t	*const p = u.u_procp;	/* calling process */
	proc_t	*cp;			/* child process */
	id_t	pid;
	id_t	pgid;
	int	wcode;
	boolean_t	found;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * If opts is not set, or if opts has an invalid value set,
	 * return EINVAL.
	 */
	if ((opts == 0) || (opts & ~WOPTMASK))
		return EINVAL;

	pid = pgid = 0;

	switch (idtype) {
	case P_PID:
	case P_PGID:
		if (id <= 0 || id >= MAXPID) {
			if (id == 0)
				return ECHILD;	/* SVR4 compatible */
			return EINVAL;
		}
		if (idtype == P_PID)
			pid = id;
		else
			pgid = id;
		break;

	case P_ALL:
		break;

	default:
		return EINVAL;
	}

	struct_zero(ip, sizeof *ip);
	cp = NULL;

	(void)LOCK(&p->p_mutex, PLHI);

again:
	ASSERT(LOCK_OWNED(&p->p_mutex));

	if (cp == NULL) {
		/*
		 * It is not yet known if there are children, or the
		 * arbitration between competing LWPs in the calling
		 * process is not complete.
		 */
		cp = p->p_child;
		while (cp != NULL && (p->p_flag & P_CLDWAIT) != 0) {
			/*
			 * Wait for the other LWP(s) in the
			 * process that are in waitid() to finish.
			 */
			DPRINTF2("DBG: Waitid: wait to wait; pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			if (!SV_WAIT_SIG(&p->p_wait2wait, PRIWAIT,
					 &p->p_mutex)) {
				DPRINTF3("DBG: Waitid: EINTR; sig=%d pid=%d\n",
					(int)u.u_lwpp->l_cursig,
					u.u_procp->p_pidp->pid_id);
				return EINTR;		/* signalled */
			}
			DPRINTF2("DBG: Waitid: Out of wait; pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			(void)LOCK(&p->p_mutex, PLHI);
			cp = p->p_child;
		}
		if (cp == NULL) {
			/*
			 * Awaken the next LWP waiting to 
			 * wait (if there is one).
			 */
			if (SV_BLKD(&p->p_wait2wait)) {
				SV_SIGNAL(&p->p_wait2wait, 0);
			}
			UNLOCK(&p->p_mutex, PLBASE);
			return ECHILD;
		}
	}

	/*
	 * The P_CLDWAIT flag prevents:
	 *   1) Multiple LWPs in the caller from racing	
	 *      with each other (both in waitid(), and fork1()).
	 *   2)	Any child processes that are exiting at
	 *	a time when the caller is ignoring exiting
	 *	children, from changing the parent-child-sibling
	 *	chain.
	 */
	p->p_flag |= P_CLDWAIT;
	p->p_flag &= ~P_CLDEVT;	/* clear child event flag */

	UNLOCK(&p->p_mutex, PLBASE);

	found = B_FALSE;

	if (pid) {		/* If specific pid, ask once */
		DPRINTF3("DBG: Waitid: find process %d; from pid=%d\n",
			 pid, u.u_procp->p_pidp->pid_id);
		while (pid != cp->p_pidp->pid_id && (cp=cp->p_nextsib) != NULL)
			continue;
	}

	DPRINTF2("DBG: Waitid: top of inner-loop; pid=%d\n",
		 u.u_procp->p_pidp->pid_id);
	while (cp != NULL) {
		if (pgid && pgid != cp->p_pgid) {
			cp = cp->p_nextsib;
			continue;
		}

		/*
		 * The setting of p_wcode always occurs while p_mutex
		 * is held.  Hence, the code below always checks
		 * p_wcode while unlocked, and then locks and checks
		 * again to avoid needless lock acquisitions.
		 */
		switch (wcode = cp->p_wcode) {

		case CLD_EXITED:
		case CLD_DUMPED:
		case CLD_KILLED:
			/*
			 * NOTE: If a child process exits while the
			 * parent is ignoring exited children, its
			 * p_wcode field will be zero and it will
			 * be added to p_zombies.  As a result, it
			 * will not be seen here. 
			 */
			if (!(opts & WEXITED)) {
				DPRINTF2("DBG: Waitid: W/X; pid=%d\n",
					 u.u_procp->p_pidp->pid_id);
				break;
			}
			/*
			 * If we are only waiting for processes in a
			 * specific process group, then reverify
			 * while holding the p_mutex lock.
			 */
			(void)LOCK(&cp->p_mutex, PLHI);
			if (pgid && pgid != cp->p_pgid) {
				UNLOCK(&cp->p_mutex, PLBASE);
				DPRINTF2("DBG: Waitid: X/pg; pid=%d\n",
					 u.u_procp->p_pidp->pid_id);
				break;
			}
			(void)LOCK_SH(&p->p_mutex, PLHI);
			if (cp->p_wcode == 0) {
				/*
				 * Another LWP just set the
				 * SA_NOCLDWAIT flag for SIGCLD (and
				 * added the child to the ignored
				 * zombie list).
				 */
				UNLOCK(&p->p_mutex, PLHI);
				UNLOCK(&cp->p_mutex, PLBASE);
				DPRINTF2("DBG: Waitid: X/race;pid=%d\n",
					u.u_procp->p_pidp->pid_id);
				break;
			}
			if (opts & WNOWAIT) {
				winfo(cp, ip, B_FALSE);
				UNLOCK(&cp->p_mutex, PLHI);
				/* still holding p_mutex of parent */
			} else {
				winfo(cp, ip, B_TRUE);
				/*
				 * The freeproc() function releases
				 * all locks and returns at PLBASE.
				 */
				freeproc(cp);
				(void)LOCK(&p->p_mutex, PLHI);
			}
			WAIT_EPILOGUE(p);
			DPRINTF2("DBG: Waitid: success EXITED pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			return 0;

		case CLD_TRAPPED:
		case CLD_STOPPED:
		case CLD_CONTINUED:
		{
			int	optchk;

			found = B_TRUE;
			optchk = ((wcode==CLD_TRAPPED) ? WTRAPPED :
				 ((wcode==CLD_STOPPED) ? WSTOPPED :
							 WCONTINUED));
			if ((opts & optchk) &&
			    waitfor_nonzomb(cp, wcode, opts, pgid, ip)) {
				LOCK(&p->p_mutex, PLHI);
				WAIT_EPILOGUE(p);
				return 0;
			}
			break;
		}

		default:
			DPRINTF2("DBG: Waitid: nowait yet; pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			/*
			 * The target may yet enter a wait state of 
			 * interest.
			 */
			found = B_TRUE;
		}

		if (pid) 
			break;
		cp = cp->p_nextsib;
	}

	DPRINTF2("DBG: Waitid: at bottom of inner-loop; pid=%d\n",
		 u.u_procp->p_pidp->pid_id);
	(void)LOCK(&p->p_mutex, PLHI);
	if (p->p_flag & P_CLDEVT) {
		/*
		 * One or more children changed state while we walked
		 * the child list.  It is possible that our search
		 * criteria will be matched on the next scan.
		 */
		cp = p->p_child;
		DPRINTF2("DBG: Waitid: P_CLDEVT seen; pid=%d\n",
			 u.u_procp->p_pidp->pid_id);
	} else if (!found) {
		/*
		 * No children were found to match the search criteria.
		 */
		WAIT_EPILOGUE(p);
		DPRINTF2("DBG: Waitid: returning ECHILD; pid=%d\n",
			 u.u_procp->p_pidp->pid_id);
		return ECHILD;
	} else if (opts & WNOHANG) {
		/*
		 * We did find processes of interest. However, 
		 * the option WNOHANG indicates that we return
		 * immediately (ip->si_pid == 0).
		 */
		WAIT_EPILOGUE(p);
		DPRINTF2("DBG: Waitid: returning via WNOHANG; pid=%d\n",
			 u.u_procp->p_pidp->pid_id);
		return 0;
	} else {
		/*
		 * None of the processes matching the search criteria
		 * were in a waitable state.  However, one or more
		 * targets did exist.  Block waiting for a child to
		 * change state.
		 */

		if (p->p_zombies != NULL) {
			/*
			 * One or more children exited (and we were
			 * ignoring SIGCLD), while we were walking the
			 * child chain above.  Free them now.
			 */
			DPRINTF2("DBG: Waitid: freeprocs; pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			freeprocs();		/* All locks released */
			(void)LOCK(&p->p_mutex, PLHI);
			if (p->p_flag & P_CLDEVT) {

				/*
				 * A child changed state while we were
				 * freeing ignored zombies.  Loop back.
				 */
				DPRINTF2(
				"DBG: Waitid: P_CLDEVT seen; pid=%d\n",
					 u.u_procp->p_pidp->pid_id);
				goto again;

			}
		}

                p->p_flag &= ~P_CLDWAIT;

		/*
		 * If other LWPs are waiting to wait, let the next
		 * one in now, and then block waiting for a child
		 * event.
		 */
		if (SV_BLKD(&p->p_wait2wait)) {
			DPRINTF2(
			"DBG: Waitid: signalling wait2wait; pid=%d\n",
				 u.u_procp->p_pidp->pid_id);
			SV_SIGNAL(&p->p_wait2wait, 0);
		}
		DPRINTF2("DBG: Waitid: blocking on p_waitsv; pid=%d\n",
			 u.u_procp->p_pidp->pid_id);
		if (!SV_WAIT_SIG(&p->p_waitsv, PRIWAIT, &p->p_mutex)) {
			DPRINTF3("DBG: Waitid: EINTR; sig=%d pid=%d\n",
				(int)u.u_lwpp->l_cursig,
				u.u_procp->p_pidp->pid_id);
			return EINTR;		/* signalled */
		}
		DPRINTF2("DBG: Waitid: parent unblocked; pid=%d\n",
			 u.u_procp->p_pidp->pid_id);
		(void)LOCK(&p->p_mutex, PLHI);
		/*
		 * Loop back to to trigger arbitration
		 * checks and child existence checks.
		 */
		cp = NULL;
	}
	goto again;
}


struct waita {
	int *stat_loc; /* the library function copies this value from r_val2 */
};

/*
 *
 * void wait(struct exita *uap, rval_t *rvp)
 *      This is the wait(2) system call.
 *
 * Calling/Exit State:
 *      No locks held on entry.  No locks held upon return.
 *
 * Remarks:
 *	For implementations that don't require binary compatibility,
 *	the wait(2) system call can be made into a library call to the
 *	waitid(2) system call.
 *
 */
/* ARGSUSED */
int
wait(struct waita *uap, rval_t *rvp)
{
	int error;
	k_siginfo_t info;

	ASSERT(KS_HOLD0LOCKS());

#ifdef WAIT_F
	WAIT_F(uap, rvp);
#endif
	error = waitid(P_ALL, (id_t)0, &info, WEXITED|WTRAPPED);

	if (error)
		return error;
	rvp->r_val1 = info.si_pid;
	rvp->r_val2 = wstat(info.si_code, info.si_status);
	return 0;
}


struct waitida {
	idtype_t idtype;
	id_t     id;
	siginfo_t *infop;
	int      options;
};

/*
 *
 * void waitsys(struct waitida *uap, rval_t *rvp)
 *      This is the waitid(2) system call.
 *
 * Calling/Exit State:
 *      No locks held on entry.  No locks held upon return.
 *
 */
/* ARGSUSED */
int
waitsys(struct waitida *uap, rval_t *rvp)
{
	int error;
	k_siginfo_t info;

	ASSERT(KS_HOLD0LOCKS());

	if ((error = waitid(uap->idtype, uap->id, &info, uap->options)) != 0)
		return error;
	if (copyout(&info, (caddr_t)uap->infop, sizeof info))
		return EFAULT;
	return 0;
}

/*
 *
 * void freeproc(proc_t *p)
 *	Remove the zombie process 'p' from its parents list of
 *	child processes.
 *
 * Calling/Exit State:
 *    Parameters:
 *	The zombie process 'p' must not be in the zombie list
 *	of the parent process.
 *
 *    Locking requirements:
 *	The p_mutex lock of both the child and parent process must be
 *	held on entry.  Upon return, both locks are released at PLBASE.
 *
 * Remarks:
 *	It is important to hold the child p_mutex lock when calling
 *	freeproc() from exit(), since:
 *	     1)	The p_parent field for the child is only stable if
 *		this lock is held when freeproc() is called from
 *		exit().  (Consider that the parent can be exiting
 *		simultaneously with the child, but the parent does
 *		not (cannot) hold its own p_mutex lock while orphaning
 *		its children.)
 *	     2) Like the p_parent field, the p_nextofkin field for the
 *		child is also only stable when the child p_mutex lock
 *		is held for the case of calling freeproc() from exit().
 *	     3)	A fundamental rule is that no process can be freeproc()ed
 *		while its p_mutex lock is held, unless the holder of the
 *		p_mutex lock is calling freeproc() against the corresponding
 *		process.  In addition, process lookup operations by PID
 *		are synchronized with pid_procp, with pid_procp cleared
 *		only while the p_mutex lock of the referenced process
 *		is held.
 *
 */
void
freeproc(proc_t *p)
{
	proc_t *pp;
	proc_t *nxtkp;
	int	holdcnt;

	ASSERT(p->p_nlwp == 0);
	ASSERT(p->p_zombies == NULL);
	ASSERT(LOCK_OWNED(&(p)->p_mutex));

	DPRINTF3("DBG: freeproc child (%d): Begin; pid=%d\n",
		p->p_pidp->pid_id, u.u_procp->p_pidp->pid_id);
	/*
	 * Remove child from its parents' child-sibling list.
	 */
	pp = p->p_parent;
	ASSERT(LOCK_OWNED(&(pp)->p_mutex));
	RM_CHILD_FROM_PARENT(p, pp);

	/*
	 * Update resource statistics.
	 */
	nxtkp = p->p_nextofkin;
	if (nxtkp != pp) {			/* orphaned by parent */
		UNLOCK(&pp->p_mutex, PLHI);
		(void)LOCK_SH(&nxtkp->p_mutex, PLHI);
	}

	INHERIT_RESOURCE_USAGE(p, nxtkp);
	UNLOCK(&nxtkp->p_mutex, PLHI);

	/*
	 * While holding p_mutex, make the child invisible to prfind()
	 * and /proc.
	 */
	RM_PROC_FROM_SYSTEM(p);

	if ((holdcnt = p->p_holdcnt) != 0)
		p->p_flag |= P_GONE;

	UNLOCK(&p->p_mutex, PLBASE);

	/*
	 * If there is a hold count on the proc structure,
	 * the last holder will notice that the process is
	 * marked P_GONE and call pid_exit().
	 */
	if (holdcnt == 0)
		pid_exit(p);
}


/*
 *
 * void freeprocs(void)
 *	Remove all ignored zombie processes from the list of children
 *	for the calling process.
 *
 * Calling/Exit State:
 *    Locking requirements:
 *	The p_mutex lock of the calling process must be held on entry.
 *	The p_mutex lock is released at PLBASE upon exit.
 *
 * Remarks:
 *	Only the parent process is allowed to remove its ignored zombies,
 *	otherwise certain locking assumptions will be broken.
 *	This function should only be called if the process has ignored
 *	zombies (p_zombies != NULL), however, this function will do
 *	nothing gracefully if the ignored zombie list is NULL.
 */
void
freeprocs(void)
{
	proc_t * const p = u.u_procp;
	proc_t *cp;
	int holdcnt;

	DPRINTF2("DBG: freeprocs: Begin; pid=%d\n", p->p_pidp->pid_id);
	ASSERT(LOCK_OWNED(&(p)->p_mutex));

	for (cp = p->p_zombies; cp != NULL; cp = cp->p_nextzomb) {
		ASSERT(cp->p_nlwp == 0 && cp->p_wcode == 0);

		/* Remove child from the callers child-sibling list. */
		RM_CHILD_FROM_PARENT(cp, p);

		/*
		 * Update resource statistics:
		 * NOTES:
		 *   The child p_mutex lock is not held below when updating
		 *   the resource statistics via p_nextofkin.  This is because
		 *   the 'p_nextofkin' pointer MUST equal 'p_parent' when this
		 *   function is called, because only the parent process can
		 *   call freeprocs() (only processes created by the
		 *   parent that have exited while the parent was ignoring
		 *   exited children can be in the zombie list).  Hence, the
		 *   parent process is not in the exit() code where it's
		 *   changing the 'p_nextofkin' pointer for all of its
		 *   children to point to the "grandparent" process.
		 */
		ASSERT(cp->p_nextofkin == p);
		INHERIT_RESOURCE_USAGE(cp, p);
	}

	cp = p->p_zombies;
	p->p_zombies = NULL;
	UNLOCK(&p->p_mutex, PLBASE);

	for( ; cp != NULL; cp = cp->p_nextzomb) {
		LOCK(&cp->p_mutex, PLHI);
		/*
		 * While holding p_mutex, make the child invisible to prfind()
		 * and /proc.
		 */
		RM_PROC_FROM_SYSTEM(cp);
		if ((holdcnt = cp->p_holdcnt) != 0)
			cp->p_flag |= P_GONE;		/* virtually gone */
		UNLOCK(&cp->p_mutex, PLBASE);
		if (holdcnt == 0)
			pid_exit(cp);			/* really gone */
	}
}

/*
 * void
 * proc_hold(proc_t *p)
 *	Apply a hold to the proc structure given by 'p'.
 *	This prevents the proc structure from being deallocated,
 *	the process may still exit.
 *
 * Calling/Exit State:
 *	The p_mutex of the passed in proc structure must be held
 *	by the caller on entry, p_mutex remains held on return.
 */
void
proc_hold(proc_t * const p)
{
	ASSERT(LOCK_OWNED(&p->p_mutex));

	p->p_holdcnt++;
}

/*
 * void
 * proc_rele(proc_t *p)
 *	Release a hold on the proc structure given by 'p'.
 *
 * Calling/Exit State:
 *	The p_mutex of the passed in proc structure must be held
 *	by the caller on entry, p_mutex is released at PLBASE.
 *
 * Remarks:
 *	The last holder will call pid_exit() if the process has
 *	already been freeproc()ed, this is indicated by P_GONE.
 */
void
proc_rele(proc_t * const p)
{
	ASSERT(KS_HOLD1LOCK() && LOCK_OWNED(&p->p_mutex));
	ASSERT(p->p_holdcnt > 0);

	if ((--p->p_holdcnt == 0) && (p->p_flag & P_GONE)) {
		UNLOCK(&p->p_mutex, PLBASE);
		pid_exit(p);
	} else
		UNLOCK(&p->p_mutex, PLBASE);
}
