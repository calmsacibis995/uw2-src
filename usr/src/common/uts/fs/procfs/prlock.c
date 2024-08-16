/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prlock.c	1.17"

#include <util/debug.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <fs/vnode.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <io/poll.h>


/*
 * boolean_t pr_p_rdwr(prcommon_t *prcp, boolean_t writer)
 * 	Establish reader/writer access to the process associated with
 * 	the given prcommon structure.  As long as the target process
 *	is not a zombie (or as long as the target LWP exists, if the
 *	prcommon structure refers to an LWP), then an I/O reference is
 * 	established against the prcommon structure, and the reader/writer
 *	lock of the process is acquired in the designated mode (read or
 *	write).
 *
 * Remarks:
 *	This is used whenever the caller needs to prevent the process
 *	or LWP from exiting and must block before the process can be
 *	released.  If the caller can complete its chores without blocking,
 *	then the process spin-lock p_mutex can be acquired instead (using
 *	the function pr_p_mutex).  Successful application of pr_p_rdwr
 *	prevents the process or LWP from exiting until pr_v_rdwr is applied.
 *
 *	This works by incrementing a count of I/O operations pending
 *	against the process or LWP, and acquiring the process reader-writer
 *	lock to prevent simultaneous destructive operations (exit, lwp_exit,
 *	exec) from proceeding.  However, concurrent address-space reads
 *	(through fork, or /proc) can proceed if the lock is acquired in
 *	the read mode.
 *
 *	Holding the process reader/writer lock by itself does not
 *	prevent the target process or LWP from exiting (the I/O reference
 *	accomplishes that function), nor does holding the process
 *	reader/writer lock prevent new LWPs from being created.
 *	The only guarantee provided by simply holding the process
 *	reader/writer lock in either mode with respect to LWP
 *	creation and destruction, is that the last LWP in the process
 *	will not exit or complete an exec(2) operation while the lock
 *	is held (hence the address space of the process remains stable
 *	with respect to these operations while the lock is held).
 *
 * Calling/Exit State:
 *	Returns B_TRUE iff an I/O reference has been established against the
 *	prcommon structure, and the reader/writer lock of the target
 *	process (or of the process containing the target LWP) has been
 *	acquired, and no other competing destructive operations against
 *	the process (and its LWPs) exist.
 *
 *	Returns B_FALSE otherwise (the reader/writer lock was not acquired
 *	because the target process or LWP has been or is being destroyed).
 */
boolean_t
pr_p_rdwr(prcommon_t *prcp, boolean_t writer)
{
	register proc_t *p;
	pl_t pl;

	pl = LOCK(&prcp->prc_mutex, PLHI);
	if (prcp->prc_flags & PRC_DESTROY) {
		/*
		 * The target object (process or LWP) no longer exists,
		 * or else the target object is a process and the process
		 * is a zombie.
		 */
		UNLOCK(&prcp->prc_mutex, pl);
		return B_FALSE;
	}
	p = prcp->prc_proc;
	/*
	 * Increment the reader/writer I/O reference count to interlock with
	 * prrdwrwait().  While this count is non-zero, the prrdwrwait()
	 * primitive (called from exit() and lwp_exit()) cannot proceed to
	 * completion, until we release the reference using pr_v_rdwr().
	 */
	prcp->prc_rdwriters++;	/* one more r/w I/O reference */
	UNLOCK(&prcp->prc_mutex, pl);
	/*
	 * Acquire the process reader or writer lock in the designated
	 * mode.
	 */
	if (writer) {
		/*
		 * All subsequent reader operations (e.g., fork(2),
		 * vfork(2), forkall(2), core dump generation, reads
		 * via /proc), and all subsequent writer operations not
		 * resulting in the destruction of the process (i.e.,
		 * exec(2), other writes via /proc, etc.) will be
		 * blocked, * until we release the lock using pr_v_rdwr().
		 * All other non-zombie writer operations resulting in
		 * the destruction of the process or an LWP in the
		 * process (e.g.:, _exit(2), and _lwp_exit(2), etc.),
		 * are blocked from completing by prc_rdwriters being
		 * non-zero (see above).
		 */
		RWSLEEP_WRLOCK(&p->p_rdwrlock, PRIMED);
	} else {
		/*
		 * Same as writer, except that multiple reader operations
		 * (e.g., fork(2)/vfork(2)/forkall(2), core dump
		 * generation, and reads via /proc) are allowed.
		 */
		RWSLEEP_RDLOCK(&p->p_rdwrlock, PRIMED);
	}
	/*
	 * We may have slept to acquire the lock, so check again for
	 * destruction of the process or LWP.  Paranoid.
	 */
	if (prcp->prc_flags & PRC_DESTROY) {
		pr_v_rdwr(prcp);
		return B_FALSE;
	}
	return B_TRUE;
}


/*
 * void pr_v_rdwr(prcommon_t *prcp)
 * 	Release the I/O reference and process reader/writer lock,
 *	acquired via pr_p_rdwr(), against the process or LWP to which
 *	the given prcommon structure refers.
 *
 * Calling/Exit State:
 *	Reader-writer lock is held by the caller and will be released
 *	by this routine.
 */
void
pr_v_rdwr(prcommon_t *prcp)
{
	register proc_t *p;		/* target process */
	register pl_t pl;

	p = prcp->prc_proc;
	RWSLEEP_UNLOCK(&p->p_rdwrlock);
	pl = LOCK(&prcp->prc_mutex, PLHI);
	prcp->prc_rdwriters--;
	if (prcp->prc_rdwriters == 0 && SV_BLKD(&prcp->prc_rdwrdone))
		SV_SIGNAL(&prcp->prc_rdwrdone, 0);
	UNLOCK(&prcp->prc_mutex, pl);
}


/*
 * boolean_t pr_p_mutex(prcommon_t *prcp)
 * 	Acquire the p_mutex lock of the process to which the given
 *	prcommon structure refers (or of the process containing the
 *	LWP to which the prcommon structure refers).
 *
 * Remarks:
 *	This is used when the caller needs to prevent the process from
 *	exiting or from changing data that is being examined or modified.
 *	Lock ordering considerations complicate the acquisition of
 *	p_mutex since we must first acquire prc_mutex in order to find
 *	a stable proc pointer in the prcommon structure (hence the use
 *	of TRYLOCK and code to handle the fallback case when that fails).
 *
 *	When the caller is done, p_mutex must be released via UNLOCK().
 *
 * Calling/Exit State:
 *	Returns B_TRUE if p_mutex was successfully acquired, and B_FALSE
 *	if the process or LWP no longer exists.
 */
boolean_t
pr_p_mutex(prcommon_t *prcp)
{
	register proc_t *p;

	(void)LOCK(&prcp->prc_mutex, PLHI);
	if (prcp->prc_flags & PRC_LWP) {
		if (prcp->prc_flags & PRC_DESTROY) {
			UNLOCK(&prcp->prc_mutex, PLBASE);
			return B_FALSE;
		}
		/*
		 * It is not possible for prc_proc to be NULL and not have
		 * PRC_DESTROY set, when the common file object refers to an
		 * LWP.
		 */
		p = prcp->prc_proc;
	} else if ((p = prcp->prc_proc) == NULL) {
		/*
		 * The target process no longer exists.
		 */
		UNLOCK(&prcp->prc_mutex, PLBASE);
		return B_FALSE;
	}
	/*
	 * We use TRYLOCK() to try to acquire the p_mutex lock of
	 * the process in reverse lock hierarchy.
	 */
	if (TRYLOCK(&p->p_mutex, PLHI) == INVPL) {
		/*
		 * Too bad: couldn't get p_mutex.
		 * Let go of prc_mutex, and get the process the hard way.
		 */
		UNLOCK(&prcp->prc_mutex, PLBASE);
		if ((p = prfind(prcp->prc_pid)) == NULL) {
			/*
			 * Lost the race with prexit().
			 */
			return B_FALSE;
		}
		if (p != prcp->prc_proc) {
			/*
			 * Lost the race with prexit(), and
			 * the process-ID was reallocated to
			 * a new process!
			 */
			UNLOCK(&p->p_mutex, PLBASE);
			ASSERT(prcp->prc_proc == NULL);
			return B_FALSE;
		}
	} else {
		/*
		 * We got p_mutex.  Drop prc_mutex.
		 * Leave at PLHI since we hold p_mutex.
		 */
		UNLOCK(&prcp->prc_mutex, PLHI);
	}
	return B_TRUE;
}


/*
 * void prdestroy(vnode_t *vp)
 *	The process or LWP associated with this vnode is exiting.
 *	Mark the prcommon structure to indicate this, and wake up
 *	anyone waiting for this specific process or LWP.  This is
 *	called from post_destroy() and lwp_exit().
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held by the
 *	caller.  It remains held upon return.
 */
void
prdestroy(vnode_t *vp)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;
	register pl_t pl;

	pl = LOCK(&prcp->prc_mutex, PLHI);
	prcp->prc_flags |= PRC_DESTROY;
	if (prcp->prc_flags & PRC_LWP) {
		prcp->prc_lwp->l_trace = NULL;
		prcp->prc_lwp = NULL;
	}
	UNLOCK(&prcp->prc_mutex, pl);

	/* Wake up WSTOP sleepers */
	if (SV_BLKD(&prcp->prc_stopsv))
		SV_BROADCAST(&prcp->prc_stopsv, 0);

	/* Wake up POLLWRNORM pollers */
	if (prcp->prc_pollhead)
		pollwakeup(prcp->prc_pollhead, POLLWRNORM);
}


/*
 * void prrdwrwait(vnode_t *vp)
 *	Wait for all outstanding /proc I/O operations against the
 *	target process or LWP (registered via pr_p_rdwr) to complete
 *	before proceeding to exit or lwp_exit.  Called from
 *	destroy_proc() and lwp_exit().
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held by the
 *	caller and remains held on return (though it may be dropped
 *	for a time by this routine).
 */
void
prrdwrwait(vnode_t *vp)
{
	register proc_t *p;
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;

	ASSERT(prcp->prc_flags & PRC_DESTROY);

	(void)LOCK(&prcp->prc_mutex, PLHI);
	if (prcp->prc_rdwriters > 0) {
		p = u.u_procp;
		UNLOCK(&p->p_mutex, PLHI);
		(void)SV_WAIT(&prcp->prc_rdwrdone, PRIMED, &prcp->prc_mutex);
		/*
		 * Note that prcp is a dangling reference after the return
		 * from SV_WAIT.
		 */
		(void)LOCK(&p->p_mutex, PLHI);
	} else
		UNLOCK(&prcp->prc_mutex, PLHI);
}


/*
 * void prexit(proc_t *p)
 * 	The denoted process no longer exists, even as a zombie.  Revoke
 * 	all /proc access.  Clear the proc-table pointers of all associated
 * 	prcommon structures.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held by the
 *	caller.  It remains held upon return.
 */
void
prexit(proc_t *p)
{
	register prcommon_t *prcp;
	register vnode_t *vp;
	register prnode_t *pnp;
	register pl_t pl;

	ASSERT(p->p_trace);
	for (vp = p->p_trace; vp != NULL; vp = pnp->pr_next) {
		pnp = VTOP(vp);
		prcp = pnp->pr_common;
		ASSERT(prcp->prc_flags & PRC_DESTROY);
		ASSERT(prcp->prc_proc != NULL);
		pl = LOCK(&prcp->prc_mutex, PLHI);
		prcp->prc_proc = NULL;
		UNLOCK(&prcp->prc_mutex, pl);
	}
}


/*
 * void prinvalidate(proc_t *p)
 *	An open process is execing a setuid or setgid program.
 *	Invalidate all extant /proc files referring to the process.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process is held on entry and
 *	remains held on exit.
 */
void
prinvalidate(proc_t *p)
{
	vnode_t *vp = p->p_trace, *cvp;
	register lwp_t *lwp;
	prnode_t *pnp;
	register int i;

	/*
	 * Run down the hierarchy and mark all the relevant vnodes.
	 * First mark process-level files.
	 */
	ASSERT(vp != NULL);
	pnp = VTOP(vp);
	(void)LOCK(&pnp->pr_mutex, PLHI);	/* Locks child pr_flags */
	if (pnp->pr_files)
		for (i = 0; i < npdent; i++)
			if (pdtable[i].prn_ftype == VREG &&
			    (cvp = pnp->pr_files[i]) != NULL)
				VTOP(cvp)->pr_flags |= PR_INVAL;
	UNLOCK(&pnp->pr_mutex, PLHI);
	for (lwp = p->p_lwpp; lwp != NULL; lwp = lwp->l_next) {
		if ((vp = lwp->l_trace) == NULL)
			continue;
		pnp = VTOP(vp);
		(void)LOCK(&pnp->pr_mutex, PLHI);
		if (pnp->pr_files)
			for (i = 0; i < nldent; i++)
				if ((cvp = pnp->pr_files[i]) != NULL)
					VTOP(cvp)->pr_flags |= PR_INVAL;
		UNLOCK(&pnp->pr_mutex, PLHI);
	}
}


/*
 * void prwakeup(vnode_t *vp)
 *	An LWP in the specified process is stopping or exiting and
 *	the process is being traced by /proc.  This may mean that the
 *	process is now "stopped" and anyone waiting for that to happen
 *	should wake up and check.
 *
 *	This is called from prstopped() and lwp_exit() when the number of
 *	LWPs or number of stopped LWPs has changed, possibly resulting in
 *	nprstopped == nlwp.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must be held by the
 *	caller.  The p_mutex lock remains held upon return.
 */
void
prwakeup(vnode_t *vp)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;

	/* Wake up WSTOP sleepers */
	if (SV_BLKD(&prcp->prc_stopsv))
		SV_BROADCAST(&prcp->prc_stopsv, 0);

	/* Wake up POLLWRNORM pollers */
	if (prcp->prc_pollhead)
		pollwakeup(prcp->prc_pollhead, POLLWRNORM);
}


/*
 * void prstopped(lwp_t *lwp)
 *	The specified LWP is stopping on a /proc "event of interest" and
 *	the process is being traced by /proc.
 *	This is called from stop() and dbg_restop() -- by the target LWP
 *	when it is stopping, or by some other LWP when it is
 *	"restopping" the target lwp.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must be held by the
 *	caller.  The p_mutex lock remains held upon return.
 */
void
prstopped(lwp_t *lwp)
{
	lwp_t *lwpp;
	proc_t *p = lwp->l_procp;

	ASSERT(LOCK_OWNED(&p->p_mutex));

	/* Wake up anyone waiting for this specific LWP to stop. */
	if (lwp->l_trace) {
		prcommon_t *lprcp = VTOP(lwp->l_trace)->pr_common;

		/* Wake up WSTOP sleepers */
		if (SV_BLKD(&lprcp->prc_stopsv))
			SV_BROADCAST(&lprcp->prc_stopsv, 0);

		/* Wake up POLLWRNORM pollers */
		if (lprcp->prc_pollhead)
			pollwakeup(lprcp->prc_pollhead, POLLWRNORM);
	}

	/*
	 * If we are stopping ourself in stop(), we must abort any
	 * on-going rendezvous.  Otherwise, deadlock could occur.  When
	 * all of the LWPs are released from all /proc stops, the
	 * rendezvous will be reentered.  We must do this before posting
	 * a stop request to the LWP which started the rendezvous, or it
	 * will return to user mode with the rendezvous potentially
	 * still in progress.
	 */
	if (lwp == u.u_lwpp)
		abort_rendezvous();

	/*
	 * In "synchronous stop" mode (the default), we have to post
	 * a stop request to all other LWPs in the process (more
	 * specifically, to all LWPs not already stopped on an event
	 * of interest and not marked EVF_PL_PRSTOP).
	 */
	if (!(p->p_flag & P_PRASYNC) && lwp->l_whystop != PR_REQUESTED) {
		lwpp = lwp;
		while ((lwpp = lwpp->l_prev) != lwp) {
			if (!ISTOP(lwpp)) {
				if (lwpp->l_stat == SSTOP)
					dbg_restop(lwpp);
				else if (!(lwpp->l_trapevf & EVF_PL_PRSTOP)) {
					LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf |= EVF_PL_PRSTOP;
					trapevnudge(lwpp, B_FALSE);
					UNLOCK(&lwpp->l_mutex, PLHI);
				}
			}
		}
	}

	if (p->p_trace == NULL)		/* process is not traced by /proc */
		return;

	/*
	 * In addition, if someone is waiting for the whole process
	 * (not just a specific LWP) to stop, then a wakeup is
	 * necessary if all LWPs have finally stopped.
	 */

	lwpp = lwp;
	while ((lwpp = lwpp->l_prev) != lwp)
		if (!ISTOP(lwpp))
			return;

	prwakeup(p->p_trace);

	/* Finally, if we are emulating ptrace, wake up the parent. */
	if (p->p_flag & P_TRC) {
		/*
		 * A process undergoing ptrace can only
		 * have one LWP, so notify the parent.
		 */
		p->p_wcode = CLD_TRAPPED;
		p->p_wdata = (lwp->l_whystop == PR_SIGNALLED) ?
			lwp->l_whatstop : SIGTRAP;
		sigcld(p);
	}
}
