/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/fork.c	1.58"
#ident	"$Header: $"

#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/ublock.h>
#include <proc/acct.h> 
#include <proc/bind.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/usync.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
/* #include <acc/audit/audit.h> */
/* #include <fs/procfs/procfs.h> */



/* 
 *
 * int fork1(char *uap, rval_t *rvp)
 * 	This is the traditional fork(2) system call.  If all goes well,
 *	a child process with one LWP is created.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and no locks will be held on exit.
 *	Upon successful completion, this system call returns a value of 0
 *	in the LWP in the child process and the process ID of the child
 *	process in the LWP that invoked this function.  Otherwise, the
 *	non-zero errno code identifying the cause of the failure is
 *	returned to the original calling LWP.
 *
 */

/* ARGSUSED */
int
fork1(char *uap, rval_t *rvp)
{
	int	error;
	pid_t	newpid;

	error = spawn_proc(NP_FAILOK | NP_FORK, &newpid, NULL, NULL);
	rvp->r_val1 = (int)newpid;
	rvp->r_val2 = 0;
	return (error);
}


/*
 *
 * int vfork(char *uap, rval_t *rvp)
 * 	Traditional vfork(2) system call.  Create a
 *	child process with one LWP.  The child process
 *	uses the callers address space until it exit's or
 *	exec's.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and no locks will be held on exit.
 *	Upon successful completion, this system call returns a value of 0
 *	in the LWP in the child process, and the process ID of the child
 *	process in the LWP that invoked this function.  Otherwise, the
 *	non-zero errno code identifying the cause of the failure is
 *	returned to the original calling LWP.
 * 
 * Remarks:
 * 	Only single-threaded processes (processes with one LWP) are
 *	permitted to vfork().
 */

/* ARGSUSED */
int
vfork(char *uap, rval_t *rvp)
{
	int	error;
	pid_t	newpid;

	if (u.u_procp->p_nlwp != 1)
		return (EINVAL);
	error = spawn_proc(NP_FAILOK | NP_VFORK, &newpid, NULL, NULL);
	rvp->r_val1 = (int)newpid;
	rvp->r_val2 = 0;
	return (error);
}


/*
 *
 * int forkall(char *uap, rval_t *rvp) 
 * 	Forkall(2) system call.  Create a child process
 * 	with the same number of LWPs as the calling process.
 * 
 * Calling/Exit State:
 *	No locks to be held on entry and no locks will be held on exit.
 *	Upon successful completion, this system call returns a value of 0 in
 *	the LWP in the child process and the process ID of the child process
 *	in the LWP that invoked this function.  Otherwise, the non-zero
 *	errno code identifying the cause of the failure is returned to the
 *	original calling LWP.
 */

/* ARGSUSED */
int
forkall(char *uap, rval_t *rvp)
{
	int	error;
	pid_t	newpid;

	/*
	 * Wedge all other LWPs in the process.
	 */
	if (!SINGLE_THREADED()) {
		if (!rendezvous()) {
			/*
		 	 * The calling LWP is being destroyed.
		 	 * Alternatively, there exists a pending
		 	 * job control stop, pending /proc stop, or
		 	 * previous rendezvous request against the
		 	 * calling LWP, and the process is not being
		 	 * destroyed.
		 	 *
		 	 * Return from the system call, letting
		 	 * systrap() deal with the various requests.
		 	 * If the LWP is not to be destroyed, the
		 	 * forkall(2) system call will be restarted.
		 	 */
			return (ERESTART);
		}
	}

	error = spawn_proc(WAS_MT()?(NP_FAILOK | NP_FORKALL):(NP_FAILOK 
	                  | NP_FORK), 
		          &newpid, NULL, NULL);
	rvp->r_val1 = (int)newpid;
	rvp->r_val2 = 0;
	if ((!SINGLE_THREADED()))
		release_rendezvous();	
	return (error);
}


/*
 *
 * 
 * int spawn_proc(int cond, pid_t *pidp, void (*func)(void *), void *arg)
 * 	This is the internal form of fork(). 
 *
 * Calling/Exit State:
 *    Locking:
 *	No locks to be held on entry and no locks will be held on exit.
 *    Return value:
 *	 0: Spawn_proc() was successful. 
 *	 Non-zero: Spawn_proc() failed; the return value is the errno.
 *
 * Description:
 * 	This function is called by the fork() wrapper 
 *	functions to create a child
 *	process with the appropriate number of LWPs.  If all goes well, a
 *	child process with the appropriate number of LWPs is created. 
 *	If "func" is specified, the child process will execute "func" with 
 *	the specified argument. 
 *	If "func" is not specified, the child process will return to user level.
 *
 * Remarks:
 *	The historical implementation of newproc() used to place
 *	the system init(1) process in the correct scheduling class.  In
 *	this implementation, we simply inherit the scheduling class of the
 *	creating context for all created contexts including the init(1)
 *	process.  It is the responsibility of the init(1) process to then
 *	change its scheduling class.
 */
int
spawn_proc(int cond, pid_t *pidp, void (*func)(void *), void *arg)
{
	proc_t 	*pp;
	proc_t	*cp;
	pid_t   newpid;
	lwp_t	*cwalkp;
	lwp_t	*pwalkp;
	int	error;

	pp = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT((cond & (NP_FORK|NP_VFORK|NP_FORKALL)) != 0);

	/*
	 * Make sure that only one of the possible fork options is set.
	 */
	ASSERT(!((cond & NP_FORK) && (cond & NP_VFORK)) && 
	       !((cond & NP_FORK) && (cond & NP_FORKALL)) && 
	       !((cond & NP_VFORK) && (cond & NP_FORKALL)));
	/*
	 * If it is a system process a transfer function must be 
	 * specified.
	 */

	ASSERT((func) || (!(cond & NP_SYSPROC)));

	/*
	 * Acquire the process reader/writer lock as a reader to prevent
	 * simultaneous operations via /proc against the process
	 * (we need a stable copy of the parent from which to create the child
	 * process).
	 */
	RWSLEEP_RDLOCK(&pp->p_rdwrlock, PRIMED);

	if ((newpid = proc_setup(cond, &cp, &error)) == -1) {

		/* no proc table entry is available */
		if (cond & NP_FAILOK) {
			RWSLEEP_UNLOCK(&pp->p_rdwrlock);
			return (error);	/* out of memory or proc slot */
		} else {
			RWSLEEP_UNLOCK(&pp->p_rdwrlock);
			/*
			 *+ spawn_proc() has failed, when the caller did not 
			 *+ expect spawn_proc() to fail. 
			 *+ Corrective action: Check the kernel 
			 *+ configuration for excessive static 
			 *+ data allocation or increase the amount of memory
			 *+ on the system.
			 */
			cmn_err(CE_PANIC, "spawn_proc - no procs");
		}
	}

	/*
	 * In the vfork case, we need to suppress shredding of the parent's
	 * address space. We must wait until after the child is visible
	 * to the swapper, since the pages must remain shreddable at all
	 * times in order to avoid memory overcommitment deadlocks.
	 *
	 * After this point, the parent can still be swapped, only the
	 * address space shredding portion is disabled.
	 */
	if (cond & NP_VFORK) {
		(void)LOCK(&pp->p_mutex, PLHI);
		pp->p_flag |= P_AS_ONLOAN;
		pp->p_nonlockedrss = 0;
		UNLOCK(&pp->p_mutex, PLBASE);
	}

	/*
	 * Give life to the child! We fake a trap frame for the children. 
	 */
	lwp_dup(pp->p_lwpp, cp->p_lwpp, 
		   (cond & NP_FORKALL)?DUP_FORKALL:DUP_FORK1, func, arg);

	ublock_unlock(cp, UB_NOSWAP);

	/* 
	 * If NP_FORKALL was specified, then all 
	 * LWPs in the parent (except the forking context)
	 * must have already rendezvoused.
	 * Get all the child's LWPs going.
	 */
	cwalkp = cp->p_lwpp;
	if (cond & NP_FORKALL)
		pwalkp = pp->p_lwpp;
	else {
		/*
		 * fork1() or vfork().
		 */
		pwalkp = u.u_lwpp;
	}
	(void)LOCK(&cp->p_mutex, PLHI);
	while (cwalkp != NULL) {
		(void)LOCK(&pwalkp->l_mutex, PLHI);
		(void)LOCK_SH(&cwalkp->l_mutex, PLHI);
		CL_FORK(pwalkp, pwalkp->l_cllwpp, 
			cwalkp, &cwalkp->l_stat, &cwalkp->l_pri,
			&cwalkp->l_flag, &cwalkp->l_cred,
			&cwalkp->l_cllwpp);
		if (pwalkp->l_trapevf & EVF_L_SCHEDPARM) {
			/*
			 * Parent LWP has a scheduling class change
			 * pending.  Need to call parmsprop() to
			 * propagate the change to the child LWP.
			 */
			parmsprop(cwalkp);
		}
		if (pwalkp == u.u_lwpp)
			cwalkp->l_flag |= L_INITLWP;			
		bind_create(pwalkp, cwalkp);	/* inherit bindings */
		UNLOCK(&cwalkp->l_mutex, PLHI);
		UNLOCK(&pwalkp->l_mutex, PLHI);
		cwalkp = cwalkp->l_next;
		pwalkp = pwalkp->l_next;
	}
	UNLOCK(&cp->p_mutex, PLBASE);

	RWSLEEP_UNLOCK(&pp->p_rdwrlock);

	if (pidp)
		*pidp = newpid;
	MET_FORK();
	MET_LWP_INUSE(cp->p_nlwp);
	return (0);
}


/*
 * void relvm(proc_t *p)
 *	Release virtual memory for the specified process.
 *	Called by exit and exec.
 *
 * Calling/Exit State:
 *	The caller must be single threaded, and cannot be holding
 *	any spin locks.  This function can block.
 *
 * Remarks:
 *	Appropriate actions must be taken by the caller of this function
 *	to prevent /proc from accessing the address space of the
 *	specified process.
 */
void
relvm(proc_t *p)
{
	pl_t pl;
	lwp_t *lwpp;
	struct as *as;
	proc_t *prp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(p->p_nlwp == 1);		/* caller must be single threaded */

	if ((p->p_flag & P_VFORK) == 0) {
		if (p->p_as != NULL) {
			/*
			 * We are about to unload our address space. Mark
			 * the engine non-preemptable until the
			 * process can be marked as having no user
			 * address space.
			 */
			DISABLE_PRMPT();
			hat_asunload(p->p_as, B_TRUE);
			pl = LOCK(&p->p_mutex, PLHI);
			as = p->p_as;
			p->p_as = NULL;
			UNLOCK(&p->p_mutex, pl);
			ENABLE_PRMPT();
			as_free(as);
		}
	} else {
		prp = p->p_parent;
		as = p->p_as;

		ASSERT(as != NULL);
		ASSERT(prp != NULL);
		ASSERT(as == prp->p_as);

		/*
		 * We are about to unload our address space. Mark the
		 * engine non-preemptable until our parent safely owns the
		 * shared address space.
		 */
		DISABLE_PRMPT();
		hat_asunload(as, B_TRUE);
		pl = LOCK(&p->p_mutex, PLHI);
		p->p_as = NULL;	/* no longer using parents addr space */
		UNLOCK(&p->p_mutex, PLHI);

		/*
		 * The following makes the address space visible to the
		 * swapper through the parent. This must be done before
		 * the ENABLE_PRMPT() in order to avoid memory consumption
		 * deadlocks.
		 *
		 * However, if the parent has already been swapped out (or
		 * is being swapped out) then the address space will
		 * effectively be invisible to the swapper (by virtue of it
		 * being too late for the swapper to shred it). In this case,
		 * therefore, we complete the swapout of the parent by
		 * shredding the address space now.
		 */
		(void)LOCK(&prp->p_mutex, PLHI);
		ASSERT(prp->p_flag & P_AS_ONLOAN);
		if (prp->p_flag & P_SEIZE) {
			prp->p_swrss = as->a_rss - as->a_lockedrss;
			prp->p_nonlockedrss = prp->p_swrss;
			UNLOCK(&prp->p_mutex, pl);
			as_ageswap(as, AS_SWAP);
			(void)LOCK(&prp->p_mutex, PLHI);
		}
		prp->p_flag &= ~P_AS_ONLOAN;
		UNLOCK(&prp->p_mutex, PLHI);
		ENABLE_PRMPT();

		/* Wait for parent to set P_VFDONE in vfwait(). */
		(void)LOCK(&p->p_mutex, PLHI);
		p->p_flag &= ~P_VFORK;	/* no longer a vforked process */
		if (SV_BLKD(&p->p_vfork))
			SV_SIGNAL(&p->p_vfork, 0);	/* wake up parent */
		while ((p->p_flag & P_VFDONE) == 0) {
			SV_WAIT(&p->p_vfork, PRIMED, &p->p_mutex);
			(void)LOCK(&p->p_mutex, PLHI);
		}
		p->p_flag &= ~P_VFDONE;		/* continue on to exit/exec */

		/*
		 * Since pending job control stop signals are ignored while
		 * a vfork()ed child is using its parents address space, we
		 * set the EVF_PL_SIG bit here to catch a pending job control
		 * stop.
		 */
		lwpp = u.u_lwpp;
		(void)LOCK(&lwpp->l_mutex, PLHI);
		if (!sigisempty(&lwpp->l_sigs))
			lwpp->l_trapevf |= EVF_PL_SIG;
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&p->p_mutex, pl);
	}
}

/*
 *
 * void vfwait(pid_t pid)
 *	Wait for the child process given by 'pid' to exec or exit.
 *	Called by the parent of a vfork'd process to reclaim its
 *	address space from the child.
 *
 * Calling/Exit State:
 *	No locks can be held on entry, none are held
 *	on return.
 *
 * Remarks:
 *	This function is called from the system call return
 *	code when the caller is returning from a vfork(2)
 *	system call.
 *
 */
void
vfwait(pid_t pid)
{
	proc_t * const p = u.u_procp;
	proc_t *cp;

	ASSERT(KS_HOLD0LOCKS());

	/* Caller of vfork must be single threaded. */
	ASSERT(p->p_nlwp == 1);

	cp = prfind(pid);	/* returns with cp->p_mutex held */
	ASSERT(cp != NULL && cp->p_parent == p);

	/*
	 * Wait for child to give up the usage of its parents
	 * address space by calling relvm() from exec or exit.
	 */
	while (cp->p_flag & P_VFORK) {
		SV_WAIT(&cp->p_vfork, PRIMED, &cp->p_mutex);
		(void)LOCK(&cp->p_mutex, PLHI);
	}

	/*
	 * Copy back sizes to parent; child may have grown.
	 * We hope that this is the only info outside the
	 * "as" struct that needs to be shared like this!
	 */
	if (p->p_brkbase == cp->p_brkbase)
		p->p_brksize = cp->p_brksize;
	if (p->p_stkbase == cp->p_stkbase)
		p->p_stksize = cp->p_stksize;

	p->p_nonlockedrss = cp->p_nonlockedrss;

	/* Wake up child, send it on its way. */
	cp->p_flag |= P_VFDONE;
	UNLOCK(&cp->p_mutex, PLBASE);
	SV_SIGNAL(&cp->p_vfork, 0);
}
