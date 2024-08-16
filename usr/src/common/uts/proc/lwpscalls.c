/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/lwpscalls.c	1.74"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/lock.h>
#include <mem/ublock.h>
#include <proc/bind.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/uidquota.h>
#include <proc/user.h>
#include <proc/usync.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

extern void fix_retval(rval_t *);
extern void strlwpclean(lwp_t *);
extern void lwp_cleanup_f(lwp_t *);
extern void timer_cancel(void);
STATIC void lwp_finalexit(lwp_t *);

/*
 * The _lwp_create() system call.
 */
struct lwp_createa {
	ucontext_t *ucp;
	u_long	   flags;
};

/*
 *
 * int _lwp_create(struct lwp_createa *uap, rval_t *rvp)
 *	This function creates an LWP within the process of the calling
 *	context, implementing the _lwp_create(2) system call.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *
 */
int
_lwp_create(struct lwp_createa *uap, rval_t *rvp)
{
	k_lwpid_t	newid;
	ucontext_t	*ucp;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	MET_LWPCREATE();
	if (u.u_procp->p_flag & P_TRC)
		return EINVAL;
	ucp = kmem_alloc(sizeof(ucontext_t), KM_SLEEP);
	if (copyin(uap->ucp, ucp, sizeof(ucontext_t))) {
		kmem_free(ucp, sizeof(ucontext_t));
		return EFAULT;
	}
	if (error = spawn_lwp(NP_FAILOK, &newid, uap->flags, 
		         ucp, NULL, NULL)) {
		kmem_free(ucp, sizeof(ucontext_t));
		return error;
	}
	rvp->r_val1 = (int)newid;
	rvp->r_val2 = 0;
	return error;
}


/*
 *
 * int spawn_lwp(int cond, k_lwpid_t *idp, u_long flags, ucontext_t *ucp 
 *		    void (*func)(void *), void *argp)
 *	Create a new LWP in the calling context's process.
 * 	This is the internal version of _lwp_create(). 
 *
 * Calling/Exit State:
 *    Locking:
 *	No locks held on entry and none held on return.
 *    Parameters:
 *	(idp) if non-NULL identifies where the LWP-ID of the created LWP
 *	should be returned. The flags parameter indicate the creation flags 
 *	specified by the creating context. (ucp) if non-NULL specifies the 
 *	user level context for the new LWP. If (func) is specified, then the 
 *	sibling LWP will execute "func" on creation with the specified
 *	argument (arg). Only system LWPs should specify "func".
 *
 *    Return value:
 *	 0: is returned  on success.
 *	 Non-zero: is returned on failure (errno is returned).
 *
 * Remarks:
 *	It is assumed that the containing process is fully initialized.
 *	This function performs all the necessary quota checks prior to 
 *	creating the new LWP.
 */
int
spawn_lwp(int cond, k_lwpid_t *idp, u_long flags, ucontext_t *ucp,
	  void (*funcp)(void *), void *argp)
{	
	static uidres_t	uidresobject;
	lwp_t		*lwpp = u.u_lwpp;	
	lwp_t		*newlwpp;
	proc_t		*procp = lwpp->l_procp;
	k_lwpid_t	newslot;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(!(cond & (NP_FORK|NP_VFORK|NP_FORKALL)));
	
	/*
	 * System LWPs can only be created in a system process.
	 * Non-system LWPs can only be created in a non-system process.
	 */
	ASSERT(((cond & NP_SYSPROC) && (procp->p_flag & P_SYS)) ||
	       (!(cond & NP_SYSPROC) && !(procp->p_flag & P_SYS)));

	/*
	 * System LWPs must specify a transfer function.
	 */
	ASSERT((funcp) || (!(cond & NP_SYSPROC)));

	/* 
	 * Allocate the LWP structure and all relevant resources.
	 * The allocated LWP structure will be fully initialized.
	 */
	if ((newlwpp = lwp_setup(lwpp, cond, procp)) == NULL) 
		return EAGAIN;

	newlwpp->l_ucp = ucp;

	uidresobject.ur_lwpcnt = 1;

	/*
	 * The function lwp_dirslot() allocates the LWP-ID for the new LWP,
	 * and consequently reserves and initializes the LWP directory slot 
	 * position as well (returning the allocated LWP-ID which equals the
	 * slot number + 1).  This function may sleep if it needs to grow the
	 * LWP directory, and manages its own concurrency control.
	 *
	 * If successful, lwp_dirslot() returns the non-zero LWP-ID given
	 * to the new LWP with the p_mutex lock of the calling process held,
	 * in which case we perform the quota checks. If we get past the 
	 * quota check, we will attempt to reserve memory for the newly 
	 * created LWP if the process it is joining is already in core. 
	 * However, if lwp_dirslot() is unsuccessful, p_mutex is not held,
	 * and no uidquota check or memory reservation is performed.
	 */
	if ((newslot = lwp_dirslot(procp, newlwpp)) == 0 ||
	    !uidquota_incr(procp->p_uidquotap, &uidresobject, B_FALSE)) {
		/*
		 * Either lwp_dirslot() failed (very unusual since this means
		 * that the calling process attempted to create more LWPs than
		 * the kernel can possibly support in a single process: i.e.,
		 * more than USHRT_MAX LWPs) or (most likely), we have failed
		 * the quota check.
		 *
		 * In either case, rollback state and return failure (if
		 * failure is tolerated by the caller).
		 */
		if (cond & NP_FAILOK) {
			if (newslot != 0) {	/* lwp_dirslot() succeeded */
				/*
				 * Free the LWP directory slot that has been
				 * reserved for us.  Note that we don't attempt
				 * to shrink the LWP directory resources. 
				 */
				if (procp->p_nlwpdir <= NBITPW) {
					BITMASK1_CLR1(&procp->p_small_lwpidmap,
						      newslot - 1);
				} else {
					BITMASKN_CLR1(procp->p_large_lwpidmap,
						      newslot - 1);
				}
				procp->p_lwpdir[newslot - 1] = NULL;
				UNLOCK(&procp->p_mutex, PLBASE);
			}
			lwp_cleanup(newlwpp);
			MET_LWP_FAIL();
			return EAGAIN;

		} else if (newslot != 0) {
			UNLOCK(&procp->p_mutex, PLBASE);
			/*
			 *+ Newlwp() failed because we would have
			 *+ exeeded the per-uid quota limits.
			 *+ The quotas may be increased to avoid
			 *+ the problem.
			 */
			cmn_err(CE_PANIC, "newlwp(): Quotas exceeded");
		} else {
			/*
			 *+ Newlwp() has failed because the calling
			 *+ process would have more than USHRT_MAX
			 *+ LWPs if the operation was successful, and
			 *+ the caller did not expect newlwp() to
			 *+ fail.  The number of LWPs in the process
			 *+ must be decreased, as the kernel cannot
			 *+ support more than USHRT_MAX LWPs per
			 *+ process.
			 */
			cmn_err(CE_PANIC,
				"newlwp(): Max LWPs per process limit reached");
		} 
	}

	/* Initialize the ID of the new LWP; it is not that of the creator! */
	newlwpp->l_lwpid = newslot;

	/*
	 * Inherit the callers l_trapevf and l_flag atomically
	 * w.r.t. making the new LWP globally visible.
	 * Note: If /proc mucks with l_trapevf or l_flag and
	 *	 expects them to be carried over to a sibling
	 *	 LWP which is being created then the target LWP
	 *	 must either be stopped, or /proc must be holding
	 *	 the p_mutex of the containing process.
	 */
	newlwpp->l_trapevf = lwpp->l_trapevf & (INHERIT_FLAGS|
						EVF_PL_RENDEZV| EVF_PL_SEIZE|
						EVF_PL_SYSENTRY|EVF_PL_SYSEXIT);
	/*
	 * If the LWP we are creating has inherited EVF_PL_SEIZE,
	 * increment the p_nseized field. We do this because the new 
	 * LWP will not be put on the run queue.
	 */
	if (newlwpp->l_trapevf & EVF_PL_SEIZE) {
		(void)LOCK(&procp->p_seize_mutex, PLHI);
		procp->p_nseized++;
		UNLOCK(&procp->p_seize_mutex, PLHI);	/*p_mutex still held*/
	}
	newlwpp->l_flag = lwpp->l_flag & ~(L_DETACHED | L_INITLWP);

	/* Deal with creation flags. */
	if (flags & LWP_DETACHED)
		newlwpp->l_flag |= L_DETACHED;

	if (procp->p_flag & P_PRSTOP) {
		/* /proc is trying to stop this whole process. */
		lwpp->l_trapevf |= EVF_PL_PRSTOP;
	}

	/* Link up the new LWP */
	procp->p_lwpp->l_prev->l_next = newlwpp;
	newlwpp->l_next = NULL;
	newlwpp->l_prev = procp->p_lwpp->l_prev;
	procp->p_lwpp->l_prev = newlwpp;

	procp->p_nlwp++;		/* bump the process LWP count */
	procp->p_ntotallwp++;		/* one more lwp in the process */

	if (flags & LWP_SUSPENDED) {
		newlwpp->l_trapevf |= EVF_PL_SUSPEND;
		/*
	 	 * Since the newly created LWP cannot be the last context
		 * to block, for the purpose of SIGWAITING, we need only 
		 * to increment the number of blocked LWPs.
	 	 */
		FSPIN_LOCK(&procp->p_niblk_mutex);
		++procp->p_niblked;
		FSPIN_UNLOCK(&procp->p_niblk_mutex);
	}

	UNLOCK(&procp->p_mutex, PLBASE);

	/* Acquire reference counts on all the shared attributes */

	crhold(lwpp->l_cred);
	rlhold(u.u_rlimits);
	if (lwpp->l_rdir != NULL)
		VN_HOLD(lwpp->l_rdir);
	if (idp)
		*idp = newslot;

	/*
	 * Duplicate the LWP. 
	 */ 
	lwp_dup(lwpp, newlwpp, DUP_LWPCR, funcp, argp);

	(void)LOCK(&lwpp->l_mutex, PLHI);
	(void)LOCK_SH(&newlwpp->l_mutex, PLHI);
	CL_FORK(lwpp, lwpp->l_cllwpp,
		newlwpp, &newlwpp->l_stat, &newlwpp->l_pri,
		&newlwpp->l_flag, &newlwpp->l_cred,
		&newlwpp->l_cllwpp);
	if (lwpp->l_trapevf & EVF_L_SCHEDPARM) {
		/*
		 * Parent lwp has a scheduling class change pending.
		 * Need to call into the parms* layer to propagate
		 * the change to the child lwp.
		 */
		parmsprop(newlwpp);
	}
	bind_create(u.u_lwpp, newlwpp);
	UNLOCK(&newlwpp->l_mutex, PLHI);
	UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
	MET_LWP_INUSE(1);
	return 0;
}


/*
 *
 * void _lwp_exit(void *uap, rval_t rvp)
 * 	_lwp_exit(2) system call. Terminate the execution of calling LWP.
 *	If this LWP was created with the LWP_DETACHED option then this 
 *	LWP cannot be waited for and hence all of its state is cleaned up.
 *	In the absence of the LWP_DETACHED option, a zombie LWP will be 
 *	created.
 *
 * Calling/Exit State:
 *	No locks held on entry. This function does not return.
 *
 */

/* ARGSUSED */

void
_lwp_exit(void *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	ADTLWPEXIT(u.u_lwpp->l_auditp);
	lwp_exit();				/* no return */

	/* NOT REACHED */
}


/*
 *
 * void lwp_exit(void)
 * 	Internal implementation of the _lwp_exit(2) system call. 
 *
 * Calling/Exit State:
 *	This function does not return. No locks can be held on entry.
 *
 */

void
lwp_exit(void)
{
	proc_t *procp = u.u_procp;	/* calling process */
	lwp_t *lwpp = u.u_lwpp;		/* calling LWP */
	vnode_t *vp;			/* /proc vnode for LWP */
	vnode_t *rdir;
	boolean_t destroyed;
	uint_t *lwpidmap;

	ASSERT(KS_HOLD0LOCKS());

	/* Free audit lwp structure */
	if (lwpp->l_auditp) { 
		alwp_t *alwp = lwpp->l_auditp;
		lwpp->l_auditp = NULL;
		adt_free(alwp);
	}

	/*
	 * Discard LWP concept of root directory, resource limits, and
	 * credentials.
	 */

	(void)CUR_ROOT_DIR_LOCK(procp);
	rdir = lwpp->l_rdir;
	lwpp->l_rdir = NULL;
	(void)CUR_ROOT_DIR_UNLOCK(procp,PLBASE);

	if (rdir != NULL) {
		VN_RELE(rdir);
	}

	/*
	 * Cleanup POLL related STREAMS state. 
	 */
	strlwpclean(lwpp);

	/*
	 * Shed any attachment to user-mode synchronization objects.
	 */
	sq_exit(lwpp);

#ifdef MERGE386
	/*
	 * If a merge process, call merge hook to clean up.
	 */
	if (lwpp->l_special & SPECF_VM86)
	      (*(void (*)())((vaddr_t *)u.u_vm86p)[MRG_LWPEXIT_OFF])(u.u_vm86p);
#endif

	/*
	 * Detach ublock from the process, since we will be unlinking it
	 * from p_lwpp and the swapper will not be able to get at it.
	 */
	ublock_lwp_detach(lwpp);

	/*
	 * Mark LWP as destroyed to prevent subsequent efforts by /proc
	 * to open the LWP.
	 */

	destroyed = B_TRUE;
	(void)LOCK(&procp->p_mutex, PLHI);
	(void)LOCK(&lwpp->l_mutex, PLHI);
	if (!(lwpp->l_trapevf & EVF_PL_DESTROY)) {
		lwpp->l_trapevf |= EVF_PL_DESTROY;
		destroyed = B_FALSE;		/* not previously destroyed */
	} else {
		/*
		 * We were asked to self-destruct!  In this case we will leave
		 * no state behind.  Set the L_DETACHED flag since nobody
		 * should wait for us. 
		 */
		lwpp->l_flag |= L_DETACHED;
	}
	UNLOCK(&lwpp->l_mutex, PLHI);   	/* p_mutex is held */

	/*
	 * Allow all in-progress operations upon the exiting LWP via
	 * the /proc file system to complete now, but abort all queued
	 * readers/writers that have not yet begun their operations.
	 */
	if ((vp = lwpp->l_trace) != NULL) {
		/*
		 * The LWP is open via /proc.  Prevent any future /proc
		 * operations upon the LWP, and wait for all on-going /proc
		 * activity against the LWP to complete before continuing.
		 */
		if (!destroyed)			/* not previously destroyed */
			prdestroy(vp);		/* mark as destroyed */
		prrdwrwait(vp);		/* wait for ops to finish */
	}				/* (p_mutex dropped and reacquired) */
	if ((vp = procp->p_trace) != NULL) {
		/*
		 * Even if the LWP is not open via /proc, it may previously
		 * have been "chosen" as part of a process-level /proc
		 * operation.  We need to block until any such operation
		 * has completed.  We accomplish this through a bit of
		 * overkill: we acquire the process reader-writer lock
		 * in exclusive mode (thus using it as a synchronization
		 * variable).  Once we have it we know that any pending
		 * operations against this LWP (in fact against any LWP in
		 * the process) have completed, and we know that this LWP
		 * will not be chosen again (because we marked it as
		 * destroyed at the beginning of lwp_exit()).  Then we
		 * immediately release it.
		 */
		UNLOCK(&procp->p_mutex, PLBASE);
		RWSLEEP_WRLOCK(&procp->p_rdwrlock, PRIMED);
		RWSLEEP_UNLOCK(&procp->p_rdwrlock);
		LOCK(&procp->p_mutex, PLHI);
	}

	/*
	 * Comply with rendezvous requests for the purposes of
	 * writing a core file.
	 *
	 * NOTES
	 * 	We could wait until much later (after the LWP was
	 *	removed from the list of LWPs in the process) to check
	 *	for rendezvous requests.  However, whenever a fatal
	 *	signal is received by the process (or an LWP within the
	 *	process), we try to grab all of the LWPs as soon as
	 *	possible, so that the core file represents the most
	 *	accurate picture of the process possible at the time
	 *	of the fatal signal delivery.
	 *
	 *	In addition, we do NOT want to rendezvous here for the
	 *	purposes of forkall(2)!  When a forkall(2) rendezvous
	 *	request exists, we do not want to enter the rendezvous
	 *	until the LWP is removed from the process, so that the
	 *	exiting LWP is not copied in the child process to be
	 *	produced by forkall(2).
	 */
	if ((lwpp->l_trapevf & EVF_PL_RENDEZV) && (procp->p_flag & P_DESTROY)) {
		/*
		 * Another LWP in the process has requested that all
		 * LWPs in the process reach rendezvous, and then be
		 * destroyed.  The only possible example of such a
		 * situation occurs in the writing of a core dump file
		 * after a fatal signal has been delivered.
		 *
		 * Increment the number of LWPs that have rendezvoused,
		 * and wakeup the requesting LWP if we are the last LWP
		 * to enter the rendezvous.  Then block on p_rendezvoused
		 * awaiting release from rendezvous.
		 */
		ASSERT(lwpp->l_trapevf & EVF_PL_DESTROY);
		if (++procp->p_nrendezvoused == procp->p_nlwp - 1) {
			SV_SIGNAL(&procp->p_rendezvous, 0);
		}
		SV_WAIT(&procp->p_rendezvoused, PRIMED, &procp->p_mutex);
		(void)LOCK(&procp->p_mutex, PLHI);
		if ((lwpp->l_trapevf & EVF_PL_RENDEZV) != 0) {
			/*
			 *+ An LWP that reached rendezvous in lwp_exit()
			 *+ for the purposes of writing a core dump, was
			 *+ released from the rendezvous state, yet the
			 *+ EVF_PL_RENDEZV flag remained set. User cannot take
			 *+ any corrective action.
			 */
			cmn_err(CE_PANIC,
				"Improper rendezvous release in lwp_exit()");
		}
	}

	/*
	 * Cleanup timer related state; cancel all LWP timers.
	 */
	timer_cancel();

	/*
	 * We may have gotten called at the behest of _lwp_exit(2), in
	 * which case we could be the last LWP in the process, and
	 * should actually invoke exit().
	 */
	if (procp->p_nlwp == 1) {
		/*
		 * We are the only LWP in the process.
		 * Act as though _exit(2) were called.
		 */
		UNLOCK(&procp->p_mutex, PLBASE);
		exit(CLD_EXITED, 0);		/* no return */
	}

	/*
	 * Remove ourselves from the LWP chain and directory maintained for
	 * the process.  This means that we will be invisible to any agent
	 * in the system, unless that agent inspects the LWP-ID map, in which
	 * case they will only be able to discern our presence, but not
	 * establish a pointer to us.  [NOTE: we cannot be the only LWP in
	 * the process.]
	 */
	if (lwpp->l_next == NULL) {
		lwpp->l_prev->l_next = NULL;
		procp->p_lwpp->l_prev = lwpp->l_prev;
	} else if (lwpp != procp->p_lwpp) {
		lwpp->l_prev->l_next = lwpp->l_next;
		lwpp->l_next->l_prev = lwpp->l_prev;
	} else {	
		procp->p_lwpp = lwpp->l_next;
		lwpp->l_next->l_prev = lwpp->l_prev;
	}
	procp->p_lwpdir[lwpp->l_lwpid - 1] = NULL;

	/*
	 * one less LWP
	 *	procp->p_nlwp must be decremented after the call to
	 *	lwpunlock(), as lwpunlock() examines this field.
	 */
	procp->p_nlwp--;

	/* Wake up anyone waiting for this process to become stopped. */
	if (procp->p_trace)
		prwakeup(procp->p_trace);

	/*
	 * Wakeup all contexts that may be waiting for us to be 
	 * suspended.
	 */
	if (SV_BLKD(&procp->p_suspsv))
		SV_BROADCAST(&procp->p_suspsv, 0);

	/*
	 * Deposit accounting statistics in process.
	 */
	procp->p_acflag |= u.u_acflag;

	/*
	 * Deposit I/O counts.  Note that CPU usage
	 * (p_utime, p_stime) is updated by the
	 * clock handler.
	 */
	procp->p_ior = ladd(procp->p_ior, u.u_ior);
	procp->p_iow = ladd(procp->p_iow, u.u_iow); 
	procp->p_ioch = ladd(procp->p_ioch, u.u_ioch);

	/*
	 * Take all process instance signals accepted by the LWP, and push
	 * them back to the process level where the other LWPs will take
	 * them.
	 */
	if (!sigisempty(&lwpp->l_procsigs)) {
		/*
		 * There exist non-defaulted process instance
		 * signals (i.e., process instance signals whose
		 * disposition is set to catch), to push back to
		 * the process.  We do this by pretending to mask
		 * these signals.
		 */
		mask_signals(lwpp, lwpp->l_procsigs);
	}

	/*
	 * Synchronize with any agent in the system trying to seize us.
	 * NOTES:
	 *	Since we've removed ourselves from the list of LWPs in
	 *	the process (and pushed all process instance signals
	 *	back to the process level), it is safe to inform any
	 *	LWPs attempting to seize the process that all LWPs in
	 *	the process are now quiescent (since we're no longer in
	 *	the process)!
	 *
	 *	Also note that it is not possible to set EVF_PL_SEIZE
	 *	without holding p_mutex....
	 */
	if (LWP_SEIZED(lwpp)) {
		/*
		 * Note that we do not have to hold the p_seize_mutex
		 * as we want to just sample the current value. We assume 
		 * atomic reads.
		 */
		if (procp->p_nseized == procp->p_nlwp) {
			/*
			 * Since all remaining LWPs in the process are
			 * seized, the process is now seized.
			 */
			EVENT_SIGNAL(&procp->p_seized, 0);
		}

		/* clear seize flag because if we preempt we can
		 * incorrectly trigger a call to become_seized() in swtch()
		 */
                lwpp->l_trapevf &= ~EVF_PL_SEIZE;
	}

	/*
	 * If the exiting LWP is detached, then finish freeing the LWP now.
	 * Otherwise, it continues to consume an LWP-ID, a uidquota LWP
	 * counter, as well as appearing to exist as a zombie LWP from the
	 * point-of-view of /proc.
	 *
	 * NOTE #1:
	 *	It is sufficient to check for L_DETACHED while only holding
	 *	p_mutex, since the LWP was either originally created with
	 * 	L_DETACHED set, or set its own L_DETACHED flag up above
	 *	when discerning that it was to be destroyed.  In no other
	 *	circumstances is it possible for L_DETACHED to become set.
	 *
	 * NOTE #2:
	 *	If the LWP is detached, it must be reaped here _before_
	 *	letting any rendezvous() request below for forkall(2)
	 *	succeed.  Otherwise, the LWP calling forkall(2) could think
	 *	a zombie LWP existed in the child when this was not the case.
	 */
	if (lwpp->l_flag & L_DETACHED) {
		if (procp->p_nlwpdir <= NBITPW)
			lwpidmap = &procp->p_small_lwpidmap;
		else
			lwpidmap = procp->p_large_lwpidmap;
		BITMASKN_CLR1(lwpidmap, lwpp->l_lwpid - 1);
		freelwp(procp);
	} else {
		/*
		 * We're not detached; wake up other LWPs blocked in
		 * _lwp_wait(2).
		 */
		if (SV_BLKD(&procp->p_lwpwaitsv))
			SV_BROADCAST(&procp->p_lwpwaitsv, 0);
	}

	/*
	 * It is possible that a forkall(2) rendezvous request is pending
	 * (rendezvous requests for core dump were handled earlier before
	 * taking the LWP out of the process).  Comply with forkall(2)
	 * rendezvous requests now.
	 */
	if (lwpp->l_trapevf & EVF_PL_RENDEZV) {

		if ((procp->p_flag & P_DESTROY) != 0) {
			/*
			 *+ A rendezvous request was encountered in lwp_exit()
			 *+ when the P_DESTROY flag was also set (a core dump
			 *+ operation is pending).  Yet all such core dump
			 *+ operations should have already been dealt with.
			 */
			cmn_err(CE_PANIC,
				"Missed core dump rendezvous in lwp_exit()");
		}

		/*
		 * Another LWP in the process has requested that all
		 * LWPs in the process reach rendezvous for the purposes
		 * of forkall(2).
		 *
		 * Wakeup the requesting LWP(s) if our departure from
		 * the process means all LWPs in the process are
		 * rendezvoused.  Note however that we do not block
		 * on p_rendezvoused awaiting release from rendezvous,
		 * since we have already removed ourself from the LWP
		 * chains of the process, and modified all of the
		 * process state information that we will modify as
		 * part of our exit sequence.
		 */
		if (procp->p_nrendezvoused == procp->p_nlwp - 1) {
			SV_SIGNAL(&procp->p_rendezvous, 0);
		}
	}

	/*
	 * Handle destroy requests in which another LWP in the process
	 * has requested that the calling LWP be destroyed.
	 */
	if (procp->p_nlwp == 1 && SV_BLKD(&procp->p_destroy)) {
		SV_SIGNAL(&procp->p_destroy, 0);
	}

	DISABLE_PRMPT();	/* disable till we hold lwp's mutex */

	UNLOCK(&procp->p_mutex, PLBASE);

	/*
	 * WARNING: No reference to the proc structure must occur past this
	 * point, since we could be racing with exit(2), exec(2), or
	 * forkall(2).
	 *
	 * Now, discard all LWP instance signals pending to the LWP.  We
	 * have chosen to ignore all such signals as has been traditionally 
	 * done.  Note that the assumption here is that no further signals 
	 * will be posted to the LWP via the _lwp_kill() interface.
	 */

	discard_lwpsigs(lwpp);

	bind_exit(lwpp);		/* unbind if we were bound */

	rlfree(u.u_rlimits);

	crfree(lwpp->l_cred);

	/*
	 * Tell the scheduling class that we are exiting
	 * (Can't sleep from now on).
	 */
	(void)LOCK(&lwpp->l_mutex, PLHI);

	ENABLE_PRMPT();		/* end of race window with exit() */

	CL_EXITCLASS(lwpp, lwpp->l_cllwpp);

	/*
	 * At this point, the only way the lwp can be "found" is by
	 * taking a clock interrupt and having the clock handler perform
	 * per-lwp services against this lwp.
	 *
	 * Notify the dispatcher this context can no longer be
	 * considered a viable lwp (preventing the clock handler from
	 * performing per-lwp services against the remains of this context).
	 */
	dispnolwp();

	/*
	 * Switch to per-processor private stack. 
	 */
	use_private(lwpp, lwp_finalexit, lwpp); 
	/* NOT REACHED */
}

/*
 * void lwp_finalexit(lwp_t *lwpp)
 *	This function performs all the final (exit) cleanup that has to 
 *	to be performed on the per-engine stack.
 *
 * Calling/Exit State:
 *	No locks can be held on entry. The function does not return.
 *
 * Remarks:
 *	This function is invoked directly from the use_private() function
 *	after the calling context has stepped onto the per-engine stack
 *	and u area.
 */
STATIC void
lwp_finalexit(lwp_t *lwpp)
{
	ASSERT(KS_HOLD0LOCKS());

	CL_DEALLOCATE(&class[lwpp->l_cid], lwpp->l_cllwpp);
	LOCK_DEINIT(&lwpp->l_mutex);

	/* Do family-specific teardown. */
	lwp_cleanup_f(lwpp);

	ublock_lwp_free(lwpp);

	kmem_free(lwpp, sizeof(lwp_t));

	swtch((lwp_t *)NULL);
	/* NOTREACHED */
}




/*
 * int  __lwp_self(void *uap, rval_t *rvp)
 *	The function returns the ID of the calling context in r_val1.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on return.
 */

/* ARGSUSED */
int
__lwp_self(void *uap, rval_t *rvp)

{
	ASSERT(KS_HOLD0LOCKS());
	rvp->r_val1 = u.u_lwpp->l_lwpid;
	return 0;
}


/*
 * The _lwp_wait() system call.
 */
struct lwpwaita {
	k_lwpid_t	wait_for;
	k_lwpid_t	*departed_lwp;	/* lib stub gets this in r_val2 */
};


/*
 *
 * int _lwp_wait(strct lwpwaita *uap, rval_t *rvp)
 *	This function waits for the termination of a sibling LWP.  This
 *	function can be used to wait either for a specific sibling LWP (by
 *	specifying its ID) or any LWP in the process (by specifying an ID
 *	of 0). 
 *
 * Calling/Exit State:
 *	No locks to be held on entry and no locks will be held on return.
 *	If the "wait for any" option is specified, then the id of the waited
 *	for lwp is returned in r_val2 field. The library stub should copy
 *	this to the user specified address. The function returns 0 to indicate
 *	success.
 *
 * Remarks:
 *	The ID of the LWP that was waited for is returned in an out parameter.
 *	Note that the function will block only if thetarget LWP has not exited.
 *
 */

int
_lwp_wait(struct lwpwaita *uap, rval_t *rvp)
{
	proc_t		*pp = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	if (uap->wait_for) {		/* need to wait for a specific LWP */
		if (uap->wait_for == u.u_lwpp->l_lwpid) {
			return EDEADLK;
		}

		for (;;) {
			switch(lwp_reapid(pp, uap->wait_for)) {
			case DIR_ACTIVE:
				/*
				 * The requested ID is still active; need to
				 * block.  In this case, lwp_reapid() returns 
				 * with the p_mutex lock held.
				 */
				ASSERT(LOCK_OWNED(&pp->p_mutex));
				if (!SV_WAIT_SIG(&pp->p_lwpwaitsv,
						 PRIWAIT, &pp->p_mutex)) {
					/* we have been signaled */
					return EINTR;
				}
				/* some LWP has exited */
				break;

			case DIR_ESEARCH:
				/*
				 * The requested ID is not known to the system.
				 * This could be the result of either a user
				 * error (specifying an ID that is not valid)
				 * or a race in which a different context in
				 * this process has succeeded in "waiting" for
				 * the target ID.  In either case, the ID is
				 * not known to the system and hence the error
				 * return.
				 * No locks are held on return in this case.
				 */
				ASSERT(KS_HOLD0LOCKS());
				return ESRCH;

			case DIR_EXITED:
				/*
				 * The requested ID has exited.  In this case,
				 * the function lwp_reapid() does the necessary 
				 * cleanup.
				 * No locks are held on return in this case.
				 */
				ASSERT(KS_HOLD0LOCKS());
				rvp->r_val2 = uap->wait_for;
				return 0;

			case DIR_INVAL:
				/*
				 * The specified ID cannot be waited for as it
				 * is in a detached state.
				 */
				ASSERT(KS_HOLD0LOCKS());
				return ESRCH;
			}
		}
	}

	/*
	 * Wait for any non-detached zombie LWP.
	 */
	for (;;) {
		switch(lwp_reapany(pp, (lwpid_t *)&rvp->r_val2)) {
		case DIR_ACTIVE:
			/*
			 * No id can be freed up; need to block.  In this 
			 * case lwp_reapany() returns with p_mutex held.
			 */
			ASSERT(LOCK_OWNED(&pp->p_mutex));
			if (!SV_WAIT_SIG(&pp->p_lwpwaitsv, PRIWAIT,
					 &pp->p_mutex)) {
				/* we have been signalled */
				return EINTR;
			}
			/* Some LWP has exited */
			break;

		case DIR_EXITED:
			/*
			 * Some ID has exited and has been cleaned up.
			 * The ID has been returned in r_val2.  No locks
			 * are held.
			 */
			ASSERT(KS_HOLD0LOCKS());
			return 0;

		case DIR_INVAL:
			/*
			 * The caller is the only active context in the
			 * process and there are no zombie LWPs.
			 */
			ASSERT(KS_HOLD0LOCKS());
			return EDEADLK;
		}
	}
	/* NOTREACHED */
}

/*
 * The _lwp_info() system call.
 */

struct lwp_infoa {
	lwpinfo_t	*infop;
};

/*
 *
 * int _lwp_info(struct lwp_infoa *uap, rval_t *rvp)
 *	Returns the user and system times of the LWP.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *
 */

/*ARGSUSED*/
int
_lwp_info(struct lwp_infoa *uap, rval_t *rvp)
{
	lwpinfo_t	lwpinfo;
	time_t		seconds;
	long		nanosec;

	ASSERT(KS_HOLD0LOCKS());

	/* Get the user time */

	seconds = (time_t)((u.u_lwpp->l_utime)/HZ);
	nanosec = (long)((u.u_lwpp->l_utime - seconds * HZ) * TICK);
	lwpinfo.lwp_utime.tv_sec = seconds;
	lwpinfo.lwp_utime.tv_nsec = nanosec;

	/* Get the system time */
	seconds = (time_t)((u.u_lwpp->l_stime)/HZ);
	nanosec = (long)((u.u_lwpp->l_stime - seconds * HZ) * TICK); 
	lwpinfo.lwp_stime.tv_sec = seconds;
	lwpinfo.lwp_stime.tv_nsec = nanosec;

	if (copyout(&lwpinfo, uap->infop, sizeof(lwpinfo_t)))
		return EFAULT;

	return 0;
}

/*
 * The _lwp_suspend() system call.
 */

struct lwpsuspenda {
	lwpid_t	suspend_id;
};

/*
 * _lwp_suspend(struct lwpsuspenda *uap, rval_t *rvp)
 *	Suspend the specified  context.
 *
 * Calling/Exit State:
 *	No locks can be held on entry and none will be held on return.
 */

/* ARGSUSED */
int
_lwp_suspend(struct lwpsuspenda *uap, rval_t *rvp)

{
	proc_t		*procp = u.u_procp;
	lwp_t		*lwpp;
	
	ASSERT(KS_HOLD0LOCKS());

	if ((u.u_lwpp->l_lwpid == uap->suspend_id) && (procp->p_nlwp == 1)) {
		/*
		 * The caller is attempting to suspend itself!
		 */
		return EDEADLK;
	}



	(void)LOCK(&procp->p_mutex, PLHI);
	/*
	 * Check the validity of the ID passed in.
	 */

	if ((uap->suspend_id < (lwpid_t)1) || 
	     (uap->suspend_id > (lwpid_t)(procp->p_nlwpdir))) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return ESRCH;
	}

checkagain:
	ASSERT(LOCK_OWNED(&procp->p_mutex));
	if ((lwpp = procp->p_lwpdir[uap->suspend_id - 1]) == NULL) {
		UNLOCK(&procp->p_mutex, PLBASE);
		/*
		 * The LWP corresponding to the specified ID has exited.
		 * We could have either raced with an exiting LWP, or the 
		 * user gave us an invalid ID.
		 */
		return ESRCH;
	}

	/*
	 * If the target LWP is already in a suspended state; return 
	 * success.
	 */

	(void)LOCK(&lwpp->l_mutex, PLHI);
	if ((lwpp->l_flag & L_SUSPENDED) && 
	    (!(lwpp->l_trapevf & EVF_PL_SUSPEND))) {
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&procp->p_mutex, PLBASE);
		return 0;
	}

	/*
	 * If LWP is not marked for suspension, mark it to be suspended.
	 */

	if (!(lwpp->l_trapevf & EVF_PL_SUSPEND)) {
		lwpp->l_trapevf |= EVF_PL_SUSPEND;
		if (!(lwpp->l_flag & L_SUSPENDED)) {
			FSPIN_LOCK(&procp->p_niblk_mutex);
			++procp->p_niblked;
			FSPIN_UNLOCK(&procp->p_niblk_mutex);
		}
		if (u.u_lwpp->l_lwpid != uap->suspend_id)
			trapevnudge(lwpp, B_FALSE);
	}
	if (u.u_lwpp->l_lwpid == uap->suspend_id) {
		FSPIN_LOCK(&procp->p_niblk_mutex);
		if ((procp->p_niblked >= procp->p_nlwp) && procp->p_sigwait &&
		    !sigismember(&procp->p_sigignore, SIGWAITING) &&
		    !sigismember(&procp->p_sigs, SIGWAITING)) {
			/*
			 * There is a possibility that we may be the last
			 * context in the process to block interruptibly.  Send
			 * SIGWAITING (but only if that signal is being caught
			 * by the process).  
			 * In this case, we post the signal and return ERESTART,
			 * so that the stub will do the "right" thing.
			 */
			--procp->p_niblked;
			FSPIN_UNLOCK(&procp->p_niblk_mutex);
			lwpp->l_trapevf &= ~EVF_PL_SUSPEND;
			UNLOCK(&lwpp->l_mutex, PLHI);
			UNLOCK(&procp->p_mutex, PLBASE);
			sigtoproc(procp, SIGWAITING, (sigqueue_t *)NULL);
			return ERESTART;
		}
		FSPIN_UNLOCK(&procp->p_niblk_mutex);
	}
	UNLOCK(&lwpp->l_mutex, PLHI);
	/*
	 * Wait for some lwp to become suspended. _lwp_suspend() system call 
	 * is not interruptible. However, if we block here non-interruptibly,
	 * we have a deadlock situation if a rendezvous request is pending.
	 * Note that an LWP will not be suspended if there is a pending
	 * rendezvous request. We solve the problem by sleeping 
	 * interruptibly and returning ERESTART if we are interrupted
	 * and expect the system call stub to restart the call.
	 */ 
	if (!SV_WAIT_SIG(&procp->p_suspsv, PRIMED, &procp->p_mutex)) {
		return ERESTART;	
	} 

	/*
	 * If the lwp was suspending itself, then it was
	 * suspended within the call to SV_WAIT_SIG(). In
	 * this case, return success as it must have been
	 * continued by another lwp within the 
	 * containing process.
	 */
	if (u.u_lwpp->l_lwpid == uap->suspend_id) {
		return 0;
	}
	/*
	 * Some LWP in the process has suspended itself. Go back and 
	 * check if we got our target LWP in the suspended state.
	 */
	(void)LOCK(&procp->p_mutex, PLHI);
	goto checkagain;
}

/*
 * _lwp_continue() system call.
 */

struct lwpcontinuea {
	lwpid_t	contid;
};

/*
 * _lwp_continue(lwpcontinuea *uap, rval_t *rvp)
 *	Get the target lwp going.
 *
 * Calling/Exit State:
 *	No locks can be held on entry and none will be held on return.
 */

/* ARGSUSED */
int
_lwp_continue(struct lwpcontinuea *uap, rval_t *rvp)

{
	proc_t		*procp = u.u_procp;
	lwp_t		*lwpp;

	ASSERT(KS_HOLD0LOCKS());

	/* Continuing ourself, return success! */
	if (u.u_lwpp->l_lwpid == uap->contid)
		return 0;

	(void)LOCK(&procp->p_mutex, PLHI);
	if ((uap->contid < (lwpid_t)1) || 
	     (uap->contid > (lwpid_t)(procp->p_nlwpdir))
	     || ((lwpp = procp->p_lwpdir[uap->contid - 1]) == NULL)) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return ESRCH;
	}

	/*
	 * If the target LWP is not in a suspended state; return success.
	 */
	(void)LOCK(&lwpp->l_mutex, PLHI);
	if (!(lwpp->l_trapevf & EVF_PL_SUSPEND) && 
	     (!(lwpp->l_flag & L_SUSPENDED))) {
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&procp->p_mutex, PLBASE);
		return 0;
	}

	/*
	 * Get the specified LWP going! Setrun() the LWP only if 
	 * it is suspended and presently not executing. Note that L_SUSPENDED
	 * will only be set if the LWP was suspended and remains set as long
	 * as the LWP remains suspended. However, an LWP with L_SUSPENDED
	 * flag set may enter the SONPROC state if it was forced to run
	 * to satisfy a rendezvous request.
	 */
	if ((lwpp->l_flag & L_SUSPENDED) && (lwpp->l_stat == SSTOP))
		setrun(lwpp);
	/*
	 * Decrement the number of stopped LWPs only if the target LWP
	 * had stopped to begin with!  Also, if a /proc stop has taken
	 * effect, don't decrement nstopped.
	 */
	if (lwpp->l_flag & L_SUSPENDED)
		if (lwpp->l_whystop == PR_SUSPENDED)
			procp->p_nstopped--; /* 1 less stopped LWP */
	FSPIN_LOCK(&procp->p_niblk_mutex);
	--procp->p_niblked;
	FSPIN_UNLOCK(&procp->p_niblk_mutex);

	lwpp->l_trapevf &= ~EVF_PL_SUSPEND;
	lwpp->l_flag &= ~L_SUSPENDED;
	UNLOCK(&lwpp->l_mutex, PLHI);
	UNLOCK(&procp->p_mutex, PLBASE);
	/*
	 * Wakeup all contexts that may be waiting for us to be
         * suspended.
         */
	if (SV_BLKD(&procp->p_suspsv))
		SV_BROADCAST(&procp->p_suspsv, 0);
	return 0;
}


STATIC event_t		spawn_sys_lwp_spawnevent;
STATIC event_t		spawn_sys_lwp_waitevent;
STATIC sleep_t		spawn_mutex;
STATIC LKINFO_DECL(spawn_lkinfo, "spawn_mutex", 0);

/* spawn_action controls what the daemon does when it wakes up */
enum spawn_action {
        SP_SPAWN,       /* spawn a new daemon LWP */
        SP_WAIT        /* wait for a previously spawned LWP to exit*/
};
STATIC enum spawn_action spawn_sys_lwp_action;

STATIC u_long		spawn_sys_lwp_flags;
STATIC void 		(* spawn_sys_lwp_func)();
STATIC void *		spawn_sys_lwp_argp;

STATIC k_lwpid_t 	spawn_sys_lwp_lwpid;

STATIC int 		spawn_sys_lwp_return;

/*
 * void
 * spawn_sys_lwp_daemon(void)
 *      System LWP to spawn other lwp's of proc 0,
 *	and to kill them off.
 *
 * Calling/Exit State:
 *      The LWP blocks until signalled that there is
 *      work for it to do.  The routine spawn_sys_lwp
 *	will pass us its arguments and then signal us
 *	to call spawn_lwp. We store the return value
 * 	for caller, and signal them to continue.
 *
 *    Return value:
 *	Never returns.
 */
void
spawn_sys_lwp_daemon(void)
{
	struct lwpwaita waita;
	rval_t          rv;

        u.u_lwpp->l_name = "spawn_lwp";
	EVENT_INIT(&spawn_sys_lwp_spawnevent);
	EVENT_INIT(&spawn_sys_lwp_waitevent);
	SLEEP_INIT(&spawn_mutex, 0, &spawn_lkinfo, KM_SLEEP);

        for(;;) {
                EVENT_WAIT(&spawn_sys_lwp_spawnevent, PRIMED);

		switch (spawn_sys_lwp_action) {
		case SP_SPAWN:
			ASSERT(spawn_sys_lwp_func);
			spawn_sys_lwp_return = spawn_lwp(NP_SYSPROC|NP_FAILOK,
				&spawn_sys_lwp_lwpid, spawn_sys_lwp_flags,
				NULL, spawn_sys_lwp_func,
				spawn_sys_lwp_argp);
			break;

		case SP_WAIT:
			ASSERT(spawn_sys_lwp_lwpid);

			waita.wait_for = spawn_sys_lwp_lwpid;
			spawn_sys_lwp_return = _lwp_wait(&waita, &rv);
			break;
		}

		EVENT_SIGNAL(&spawn_sys_lwp_waitevent, 0);
	}
}

/*
 *
 * int spawn_sys_lwp(k_lwpid_t *idp, u_long flags, 
 *		    void (*func)(void *), void *argp)
 *	Create a new LWP in the context of process 0.
 *
 * Calling/Exit State:
 *	Same as for spawn_lwp, above.
 *
 *    Return value:
 *	Same as for spawn_lwp, above.
 *
 * Remarks:
 *	Callers will be single threaded
 *	thru this routine.
 */
int
spawn_sys_lwp(k_lwpid_t *idp, u_long flags, void (*funcp)(void *), void *argp)
{	

	int error;
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Set up arguments, signal the daemon
	 * to start up a new lwp, and return
	 * the results to our caller.
	 */
	SLEEP_LOCK(&spawn_mutex, PRIMED);

	spawn_sys_lwp_action = SP_SPAWN;
	spawn_sys_lwp_flags = flags;
	spawn_sys_lwp_func = funcp;
	spawn_sys_lwp_argp = argp;
	EVENT_SIGNAL(&spawn_sys_lwp_spawnevent, 0);

	EVENT_WAIT(&spawn_sys_lwp_waitevent, PRIMED);
	error = spawn_sys_lwp_return;
	if (idp)
		*idp = spawn_sys_lwp_lwpid; 

	SLEEP_UNLOCK(&spawn_mutex);
	return error;
}

/*
 *
 * int wait_sys_lwp(k_lwpid_t lwpid)
 *	Wait for LWP lwpid in process 0 to exit.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *
 *    Return value:
 *	Same as for _lwp_wait, above.
 *
 * Remarks:
 *	Callers will be single threaded
 *	thru this routine.
 */
int
wait_sys_lwp(k_lwpid_t lwpid)
{	

	int error;
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Set up arguments, signal the daemon
	 * to start up a new lwp, and return
	 * the results to our caller.
	 */
	SLEEP_LOCK(&spawn_mutex, PRIMED);

	spawn_sys_lwp_action = SP_WAIT;
	spawn_sys_lwp_lwpid = lwpid;
	EVENT_SIGNAL(&spawn_sys_lwp_spawnevent, 0);

	EVENT_WAIT(&spawn_sys_lwp_waitevent, PRIMED);
	error = spawn_sys_lwp_return;

	SLEEP_UNLOCK(&spawn_mutex);
	return error;
}
