/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/sv.c	1.17"
#ident	"$Header: $"

/*
 * kernel synchronization:  synchronization variables. 
 */

#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>


/*
 * Macro to release the entry lock. The lock is released at "ipl".
 * lkp points to the spin lock to be relaesed and cflags encode the 
 * compilation options under which the lock was initially acquired.
 */
#define RELEASE_ENTRYLOCK(lkp, cflags, ipl)		\
{							\
	if (cflags == KSVUNIPROC) {			\
		/* UNIPROC option */			\
		splx(ipl);				\
		ENABLE_PRMPT();				\
	} else if (cflags == KSVMPDEBUG)		\
		/* !UNIPROC && (DEBUG || SPINDEBUG)*/	\
		unlock_dbg(lkp, ipl);			\
	else {						\
		/* !UNIPROC && (!DEBUG && !SPINDEBUG)*/	\
		ASSERT(cflags == KSVMPNODEBUG);		\
		unlock_nodbg(lkp, ipl);			\
	}						\
}

/*
 * void
 * sv_init(sv_t *svp)
 *	Initialize a synchronization variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to be
 *	initialized.  Returns:	none.
 */
void 
sv_init(svp)
sv_t *svp;		/* pointer to the sync variable */
{
	/* init the foundation mutex */
	FSPIN_INIT(&svp->sv_lock);

	/*
	 * the sleep queue is be a doubly-linked list with the forward pointer
	 * of the last element and the backward pointer of the first element
	 * set to point to the synch object.  Make it empty.
	 */
	INITQUE(&svp->sv_list);
}

/*
 * sv_t *
 * sv_alloc(int km_flags)
 *	Allocates and initializes a synchronization variable.
 *
 * Calling/Exit State:
 *	km_flags tells whether the caller is willing to sleep while
 *	allocating memory or not.  It should be KM_SLEEP or KM_NOSLEEP.
 *	Returns:  a pointer to the new sv.
 *	
 */
sv_t *
sv_alloc(int km_flags)
{
	sv_t *svp;

	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));
	/* allocate an sv */
	svp = (sv_t *)kmem_alloc(sizeof(*svp), km_flags);
	if (svp == NULL) {
		return (NULL);
	}

	/* init it. */
	SV_INIT(svp);
	return (svp);
}

/*
 * void
 * sv_dealloc(sv_t *svp)
 *	Deallocate a syncronization variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to be
 *	deallocated.
 *
 *	Returns: none.
 */
void
sv_dealloc(sv_t *svp)
{
	kmem_free(svp, sizeof(*svp));
}

/*
 * void
 * sv_dealloc_dbg(sv_t *svp)
 *	Deallocate a syncronization variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to be
 *	deallocated.
 *
 *	Returns: none.
 */
void 
sv_dealloc_dbg(sv_t *svp)
{
	if (!EMPTYQUE(&svp->sv_list)) {
		/*
		 *+ An lwp attempted to deallocate a synchonization
		 *+ variable which was in use.	This indicates a kernel
		 *+ software problem.
		 */
		cmn_err(CE_PANIC, "active sv being freed");
	}
	kmem_free(svp, sizeof(*svp));
}

/*
 * void
 * sv_wait(sv_t *svp, int priority, lock_t *lkp, int cflags)
 *	Block on a synchronization variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to block on.
 *	priority is given to the class-specific code as a hint as to our
 *	sleep/dispatch priority.  lkp is a pointer to a lock held on entry
 *	which will be released prior to context switching. The cflags 
 *	argument defines the compilation options that were in effect 
 *	in the caller.
 *
 *	lkp should be held on entry.
 *
 *	Returns at PLBASE with the entry spin lock released.
 */
void 
sv_wait(sv_t *svp, int priority, lock_t *lkp, int cflags)
{
	register lwp_t *lwpp = u.u_lwpp;

	/* Put the caller to sleep. */

	(void) LOCK(&lwpp->l_mutex, PLHI);
	FSPIN_LOCK(&svp->sv_lock);

	/*
	 * give control to the class specific code to enqueue the lwp
	 */
	CL_INSQUE(lwpp, &svp->sv_list, priority);
	FSPIN_UNLOCK(&svp->sv_lock);
	/*
	 * unlock the entry lock.  Since the l_mutex is held at PLHI,
	 * we stay at that ipl. Since the entry lock may be held by the 
	 * driver code which could potentially have been compiled with
	 * compilation options different from that of the kernel, release
	 * the lock based on the cflags. 
	 */
	RELEASE_ENTRYLOCK(lkp, cflags, PLHI);
	/* Update the the lwp state */
	lwpp->l_flag |= L_NWAKE;	/* don't wake us */
	lwpp->l_slptime = 0;		/* init start time for sleep */
	lwpp->l_stat = SSLEEP;		/* going to sleep */
	lwpp->l_syncp = svp;		/* sleeping on this sv */
	lwpp->l_stype = ST_COND;	/* blocked on a synch variable*/

	/*
	 * swtch() is called with the lwp state lock held at PLHI. The
	 * state lock will be released at PLBASE when it is safe. swtch()
	 * returns at PLBASE.
	 */
	swtch(lwpp);
}


/*
 * STATIC boolean_t sv_wait_issig(void)
 *	Subroutine specific to sv_wait_sig returns a value conditioned on
 *	calling issig.	sv_wait_sig is expected to return the value without
 *	further interpretation.
 *
 * Calling/Exit State:
 *	No locks should be held on entry.  Should be called only at
 *	PLBASE.  Returns with no locks held, and at PLBASE.
 */
STATIC boolean_t
sv_wait_issig(void)
{
	switch (issig((lock_t *)NULL)) {
	case ISSIG_NONE:
		/*
		 * A debugger cleared the signal or a job control
		 * stop signal was posted that was discarded by a
		 * subsequent SIGCONT.	l_mutex is held here.
		 * Since we dropped the entry lock prior to 
		 * calling issig(), we cannot go ahead and block
		 * on the synch object (we may have already 
		 * missed the wakeup!). Since the clients of 
		 * SV_WAIT_X() are expected to handle premature 
		 * wakeups, we can safely return as if the condition
		 * occurred.
		 */
		UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
		return (B_TRUE);
	case ISSIG_SIGNALLED:
		/*
		 * a signal is pending.	 Neither l_mutex nor lkp
		 * are held.
		 */
		return (B_FALSE);
	case ISSIG_STOPPED:
		/*
		 * Return as if this was a normal wakeup; The calling
		 * code will check the condition anyway.
		 * Neither l_mutex nor lkp are held.
		 */
		return (B_TRUE);
	default:
		/* this should not happen */
		/*
		 *+ The issig function returned an unexpected value.
		 *+ This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "unexpected return from issig");
		break;
	}
}

/*
 * boolean_t
 * sv_wait_sig(sv_t *svp, int priority, lock_t *lkp, int cflags)
 *	Block on a synchronization variable and be awakened by signals.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to block on.
 *	priority is given to the class-specific code as a hint as to our
 *	sleep/dispatch priority.  lkp is a pointer to the entry lock.
 *	Returns: B_TRUE for normal wakeup, B_FALSE if awakened because of
 *	signals. The cflags parameter encodes the compilation options that
 *	were in effect in the caller.
 *
 *	lkp should be held on entry.
 *
 *	The function returns at PLBASE with the entry lock unlocked. 
 */
boolean_t 
sv_wait_sig(sv_t *svp, int priority, lock_t *lkp, int cflags)
{
	pl_t	s;
	register lwp_t *lwpp = u.u_lwpp;
	proc_t	*p = u.u_procp;

	s = LOCK(&lwpp->l_mutex, PLHI);
	if (QUEUEDSIG(lwpp)) {
		/* 
		 * we're going to call issig(), so we drop l_mutex
		 * and the entry spin lock. 
		 */
		UNLOCK(&lwpp->l_mutex, s);
		RELEASE_ENTRYLOCK(lkp, cflags, PLBASE);
		return (sv_wait_issig());
	}

	/*
	 * No signal is pending.  There is a possibility that we may be the
	 * last context in the process to block interruptibly.	Send SIGWAITING
	 * (but only if that signal is being caught by the process, and the
	 * signal has not yet been posted).  It is possible that we may not
	 * block or that a different context in the process could wake up even
	 * as we are preparing to send SIGWAITING.  This is a harmless race and
	 * cannot be closed; the worst that can happen is that a signal will be
	 * sent when none should have been.  Note that we can also send the
	 * signal repeatedly if we race back here with an lwp that handles
	 * the signal.
	 */
	FSPIN_LOCK(&p->p_niblk_mutex);
	++p->p_niblked;
	if ((p->p_niblked >= p->p_nlwp) && p->p_sigwait &&
	    !sigismember(&p->p_sigignore, SIGWAITING) &&
	    !sigismember(&p->p_sigs, SIGWAITING)) {
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);
		UNLOCK(&lwpp->l_mutex, s);
		RELEASE_ENTRYLOCK(lkp, cflags, PLBASE);
		sigtoproc(p, SIGWAITING, (sigqueue_t *)NULL);

		/* If we now have a signal, deal with it. */
		if (QUEUEDSIG(lwpp))
			return (sv_wait_issig());

		/*
		 * We posted SIGWAITING to some lwp other than this one.
		 * Because we dropped lkp, we have to return to our caller
		 * so that it can reexamine the sync condition.	 We will
		 * either continue on our way, if the wanted condition has
		 * happened, or will call here again, which could result
		 * in another shot at posting SIGWAITING.
		 */
		return (B_TRUE);
	}
	FSPIN_UNLOCK(&p->p_niblk_mutex);

	FSPIN_LOCK(&svp->sv_lock);

	/* give control to the class specific code to enqueue the lwp */
	CL_INSQUE(lwpp, &svp->sv_list, priority);

	FSPIN_UNLOCK(&svp->sv_lock);

	/*
	 * release the entry lock, but stay at PLHI because we hold
	 * l_mutex
	 */
	RELEASE_ENTRYLOCK(lkp, cflags, PLHI);

	/* Update the the lwp state */
	lwpp->l_stat = SSLEEP;			/* going to sleep */
	lwpp->l_flag &= ~(L_NWAKE|L_SIGWOKE);	/* do wake us */
	lwpp->l_slptime = 0;			/* init sleep start time */
	lwpp->l_syncp = svp;			/* blocked on this object */
	lwpp->l_stype = ST_COND;		/* sleeping on an sv */

	/*
	 * swtch is called with lwp state lock held at PLHI. It returns
	 * at PLBASE after dropping l_mutex.
	 */
	swtch(lwpp);

	FSPIN_LOCK(&p->p_niblk_mutex);
	--p->p_niblked;
	FSPIN_UNLOCK(&p->p_niblk_mutex);
	if (lwpp->l_flag & L_SIGWOKE) {
		/* awakened because of a signal or other update event */
		switch (issig((lock_t *)NULL)) {
		case ISSIG_NONE:
			/* The only way we could have gotten here is if a
			 * debugger had cleared the signal or if a job control
			 * stop signal was posted that was discarded by a
			 * subsequent SIGCONT. Return as if this was a normal
			 * wakeup.  The l_mutex lock is held on return from
			 * issig()
			 */
			lwpp->l_flag &= ~L_SIGWOKE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
			return (B_TRUE);	
		case ISSIG_SIGNALLED:
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_flag &= ~L_SIGWOKE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
			return (B_FALSE);
		case ISSIG_STOPPED:
			/*
			 * Return as if this was a normal wakeup.  Let the
			 * calling code check the condition.  lkp is no
			 * longer held.
			 */
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_flag &= ~L_SIGWOKE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
			return (B_TRUE);
		default:
			/*
			 *+ issig returned an unexpected value.	 This
			 *+ indicates a kernel software problem.
			 */
			cmn_err(CE_PANIC, "unexpected return from issig");
			break;
		}
	}

	/* normal wakeup */
	return (B_TRUE);
}


/*
 * void
 * sv_signal(sv_t *svp, int flags)
 *	Signal a synchronization variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to signal.  Flags
 *	is given to the class-specific code, and may have the KS_NOPRMPT
 *	bit set to indicate that the wakeup should be non-preemptive.
 *
 *	Returns:  none.
 *
 * Description:
 *	If there are waiting lwp's, SV_SIGNAL dequeues the lwp at the
 *	head of the list and schedules it to run.  If there are no lwp's
 *	on the list, does nothing.
 */ 
void 
sv_signal(sv_t *svp, int flags)
{
	pl_t s;
	register lwp_t *lwpp;

	FSPIN_LOCK(&svp->sv_lock);
	if (EMPTYQUE(&svp->sv_list)) {
		/* no waiters */
		FSPIN_UNLOCK(&svp->sv_lock);
		return;
	}

	/*
	 * need to wake up a waiting lwp; get the first one from the
	 * queue.
	 */
	lwpp = (lwp_t *)svp->sv_head;
	remque(lwpp);
	FSPIN_UNLOCK(&svp->sv_lock);

	s = LOCK(&lwpp->l_mutex, PLHI);

	/* Update the lwp state */
	lwpp->l_stat = SRUN;
	lwpp->l_syncp = NULL;
	lwpp->l_flag &= ~L_SIGWOKE;
	lwpp->l_stype = ST_NONE; 

	/*
	 * Call the class specific code to  enqueue to the run-queue 
	 * and do the necessary arrangements for swapin.  l_mutex is
	 * held.
	 */
	CL_WAKEUP(lwpp, lwpp->l_cllwpp, flags);
	UNLOCK(&lwpp->l_mutex, s);
}

/*
 * boolean_t
 * sv_unsleep(sv_t *svp, lwp_t *lwpp)
 *	Remove a specified lwp from the sleep queue.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable from whose
 *	sleep queue the lwp pointed to by lwpp is to be removed.
 *
 *	Returns: B_TRUE if the specified lwp was dequeued,
 *		 B_FALSE if the specified lwp is not on the sleep queue.
 *
 *	This function should be called with the lwp state lock held.
 */
boolean_t 
sv_unsleep(sv_t *svp, struct lwp *lwpp)
{
	boolean_t dequeued;

	/* remove the lwp from the sleep queue */
	ASSERT((LOCK_OWNED(&lwpp->l_mutex)));

	FSPIN_LOCK(&svp->sv_lock);
	dequeued = slpdeque(&svp->sv_list, lwpp);
	FSPIN_UNLOCK(&svp->sv_lock);
	return (dequeued);
}

/*
 * void
 * sv_broadcast(sv_t *svp, int flags)
 *	Awakens all the lwps blocked on the given synchronization
 *	variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable on which to
 *	look for lwps on.  flags is passed to the class-specific code,
 *	and may have the KS_NOPRMPT bit set to request non-preemptive
 *	wakeup.
 *
 *	Returns:  none.
 */
void 
sv_broadcast(sv_t *svp, int flags)
{
	pl_t s;
	register lwp_t *lwpp, *clwpp;

	FSPIN_LOCK(&svp->sv_lock);
	if (EMPTYQUE(&svp->sv_list)) {
		/* no one is blocked, just return */
		FSPIN_UNLOCK(&svp->sv_lock);
		return;		
	}

	/* wake up all the waiting lwps */
	lwpp = (lwp_t *)svp->sv_head;		/* get the list to wake */
	svp->sv_tail->flink = NULL;		/* list ends at the tail */
	INITQUE(&svp->sv_list);			/* sv list empty */

	FSPIN_UNLOCK(&svp->sv_lock);

	while (lwpp != NULL) {
		clwpp = lwpp;
		lwpp = (lwp_t *)lwpp->l_flink;
		s = LOCK(&clwpp->l_mutex, PLHI);

		/* Update the lwp state */
		clwpp->l_stat = SRUN;
		clwpp->l_syncp = NULL;
		clwpp->l_flag &= ~L_SIGWOKE;
		clwpp->l_stype = ST_NONE;

		/*
		 * Call the class specific code to  enqueue to the run-queue 
		 * and make the necessary arrangements for swapin.  The
		 * l_mutex is held at PLHI.
		 */
		CL_WAKEUP(clwpp, clwpp->l_cllwpp, flags);
		UNLOCK(&clwpp->l_mutex, s);
	}
}

#undef SV_BLKD
/*
 * boolean_t
 * SV_BLKD(sv_t *svp)
 *	Checks to see if there are lwps blocked on a synchronization
 *	variable.
 *
 * Calling/Exit State:
 *	svp is a pointer to the synchronization variable to examine.
 *	Returns:  B_TRUE if there are lwps blocked, B_FALSE otherwise.
 *
 * Remarks:
 *	The data returned is stale, unless the caller has taken steps
 *	to preserve its consistency.
 */
boolean_t
SV_BLKD(sv_t *svp)
{
	return(!EMPTYQUE(&svp->sv_list));
}
