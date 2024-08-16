/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/event.c	1.16"
#ident	"$Header: $"

/*
 * kernel synchronization primitives:  events.
 */

#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>


/*
 * void
 * EVENT_INIT(event_t *eventp)
 *	Initializes an event.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to be initialized.  Returns:  none.
 */
void 
EVENT_INIT(event_t *eventp)
{
	FSPIN_INIT(&eventp->ev_lock);

	/*
	 * doubly linked circular sleep queue architecture: set
	 * to empty
	 */
	INITQUE(&eventp->ev_list);
	eventp->ev_state = 0;			/* initialize the state */
}

/*
 * void
 * EVENT_CLEAR(event_t *eventp)
 *	Clear an event.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to be cleared.  Returns:  none.
 *
 * Remarks:
 *	No error is asserted if the event had not previously been posted.
 */
void 
EVENT_CLEAR(event_t *eventp)
{
	FSPIN_LOCK(&eventp->ev_lock);

#ifdef _LOCKTEST
	if (! EMPTYQUE(&eventp->ev_list)) {
		/*
		 *+ An lwp attempted to clear an event which had lwp's
		 *+ blocked on it.  This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "active event structure being cleared");
	}
#endif /* _LOCKTEST */

        eventp->ev_state = 0;   		/* clear the state */
	FSPIN_UNLOCK(&eventp->ev_lock);
}

/*
 * event_t *
 * EVENT_ALLOC(int flags)
 *	Allocates and initializes an event.
 *
 * Calling/Exit State:
 *	flags are passed to kma for allocation of memory for
 *	the event, and should be KM_SLEEP or KM_NOSLEEP.  Returns:
 *	a pointer to the new event, or NULL if no memory could be
 *	allocated.
 */
event_t	*
EVENT_ALLOC(int km_flags)
{
	event_t *eventp;
	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	/* allocate an event structure */
	eventp = (event_t *)kmem_alloc(sizeof(*eventp), km_flags);
	if (eventp == NULL) {
		return (NULL);
	}
	EVENT_INIT(eventp);
	return (eventp);
}

/*
 * void
 * EVENT_DEALLOC(event_t *eventp)
 *	Deallocates an event.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to be deallocated.  Returns: none.
 */
void 
EVENT_DEALLOC(event_t *eventp)
{
#ifdef _LOCKTEST
        if (! EMPTYQUE(&eventp->ev_list)) {
		/*
		 *+ An lwp attempted to deallocate an event that had other
		 *+ lwp's blocked on it.  This indicates a kernel software
		 *+ problem.
		 */
                cmn_err(CE_PANIC, "freeing active event structure");
		/*NOTREACHED*/
	}
#endif /* _LOCKTEST */

	/* get rid of the structure */
	kmem_free(eventp, sizeof(*eventp));
}

/*
 * void
 * EVENT_WAIT(event_t *, int pri)
 * 	Block the caller until the event is signaled.  The potential
 *	sleep is not interruptable.
 *	
 * Calling/Exit State:
 *	pri is the priority at which the caller would like to sleep
 *	and be dispatched at, but it is only a hint to the class-specific
 *	scheduler.  If the event has been posted, the caller is not blocked.
 */
void 
EVENT_WAIT(eventp, priority)
event_t *eventp;		/* the event to wait on */
int priority;			/* dispatch/sleep priority hint */
{
	lwp_t *lwpp = u.u_lwpp;
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Peek to see if the event has been posted.
	 */ 
	if (eventp->ev_state > 0) {

		/*
		 * We may be able to consume this event.
		 */
		FSPIN_LOCK(&eventp->ev_lock);
		if (eventp->ev_state == 1) {
			--eventp->ev_state;		/* consume the event */
			FSPIN_UNLOCK(&eventp->ev_lock);
			return;
		}
		FSPIN_UNLOCK(&eventp->ev_lock);		/* lost the race */
	}

	/* 
	 * Event has not occurred.  May have to put the caller to sleep.
	 * Need to acquire the lwp state lock and the ev_lock.
	 */
	(void) LOCK(&lwpp->l_mutex, PLHI);
	FSPIN_LOCK(&eventp->ev_lock);

	/* 
	 * Check to see if we raced with event_signal()  
	 * or event_broadcast() 
	 */
	if (--eventp->ev_state == 0) {
		/* raced with a event posting */
                FSPIN_UNLOCK(&eventp->ev_lock);
		UNLOCK(&lwpp->l_mutex, PLBASE);
                return;
        }

	/*
	 * Event has not occurred; put the caller to sleep; give control
	 * to the class specific code to enqueue the lwp.
	 */
	CL_INSQUE(lwpp, &eventp->ev_list, priority);
	FSPIN_UNLOCK(&eventp->ev_lock);

	/* Update the the lwp state */
	lwpp->l_flag |= L_NWAKE;	/* don't wake me */
	lwpp->l_stat = SSLEEP;
	lwpp->l_syncp = eventp;		
	lwpp->l_slptime = 0;		/* init the start of sleep time */
	lwpp->l_stype = ST_EVENT;	/* blocked on an event */

	/*
	 * swtch() called with the lwp state lock held. swtch() will 
	 * release the state lock at PLBASE
	 * when it is safe.
	 */
	swtch(lwpp);
}

/* 
 * boolean_t
 * EVENT_WAIT_SIG(event_t *eventp, int priority)
 *	Blocks the caller until an event is signaled.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to block on.  priority is given
 *	to the class-specific code as a hint as to our sleep/dispatch
 *	priority.  B_TRUE is returned unless the caller is caused to sleep,
 *	in which case B_TRUE is returned for normal wakeup, and B_FALSE if
 *	the lwp receives a signal.
 *
 * Description:
 *	If the event has already been signaled, the caller is not blocked.
 *	Note that if the lwp is stopped and continued, we retry for the
 *	event transparently to the calling code.
 *
 * Remarks:
 *	In this routine, we peek to see if the event has been posted, and
 *	consume it if it has without checking for pending signals.  The
 *	theory is that we can maximize system throughput at the expense of
 *	signal latency in this way, but no performance measurement has been
 *	done to support this theory.
 */
boolean_t 
EVENT_WAIT_SIG(event_t *eventp, int priority)
{
	lwp_t 	*lwpp = u.u_lwpp;
	proc_t 	*p = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	/*
	 * Peek to see if the event has been posted.
	 */ 
	if (eventp->ev_state > 0) {

		/*
		 * We may be able to consume this event.
		 */
		FSPIN_LOCK(&eventp->ev_lock);
		if (eventp->ev_state == 1) {
			--eventp->ev_state;		/* consume the event */
			FSPIN_UNLOCK(&eventp->ev_lock);
			return (B_TRUE);
		}
		FSPIN_UNLOCK(&eventp->ev_lock);		/* lost the race */
	}

again:
	(void) LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_flag &= ~L_SIGWOKE;

	if (QUEUEDSIG(lwpp)) {
		UNLOCK(&lwpp->l_mutex, PLBASE);	

		/* 
		 * call issig to determine if there are any pending signals.
		 *
		 * Neither the l_mutex or p_mutex lock of the respective calling
		 * lwp or process can be held.
		 */
		switch (issig((lock_t *)NULL)) {
		case ISSIG_NONE:
			/*	
			 * No signals or other update events posted; the
			 * l_mutex is held.
			 * 
			 * Go for the event.
			 */
			break;
		case ISSIG_SIGNALLED:
			/*
			 * The LWP has a signal to process.  l_mutex is
			 * not held.
			 */
			return (B_FALSE);
		case ISSIG_STOPPED:
			/*
			 * Stopped.  Go for the event. l_mutex is not held,
			 * so we need to check for signals again.
			 */
			goto again;
		default:
			/* this should never happen. */
			/*
			 *+ The issig() function returned an unexpected status.
			 *+ This indicates a kernel software problem.
			 */
			cmn_err(CE_PANIC, "unexpected return from issig");
		}
	}
acquire:
	/*
	 * Go for the event, if it's available.
	 * Note that the l_mutex is held at this point. We may 
	 * block if the event is not available. 
	 * Check to see if we are the last context to 
	 * block interruptibly.
	 */

	FSPIN_LOCK(&p->p_niblk_mutex);
	if ((++p->p_niblked >= p->p_nlwp)
	    && (p->p_sigwait) && CAN_SEND(SIGWAITING)) {
		/*
	 	 * There is a possibility that we may be the last 
	 	 * context in the process to block interruptibly. 
	 	 * Send the SIGWAITING signal. It is possible
	 	 * that we may not block or a different context 
	 	 * in the process could wakeup even as we are  
	 	 * preparing to send the SIGWAITING signal. 
		 * This is a harmless race
	 	 * and cannot be closed; the worst 
		 * that can happen is that 
	 	 * a signal will be sent when none should have been
	 	 * sent. 
	 	 *
		 * We send the signal only if it is not masked (by this 
		 * context and is not ignored or defaulted.
		 */
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);
		UNLOCK(&lwpp->l_mutex, PLBASE);
		sigtolwp(lwpp, SIGWAITING, (sigqueue_t *)NULL);
		goto again;
	}
	FSPIN_UNLOCK(&p->p_niblk_mutex);
	FSPIN_LOCK(&eventp->ev_lock);
	if (--eventp->ev_state == 0) {		   
		/* the event has occurred */
		FSPIN_UNLOCK(&eventp->ev_lock);
		UNLOCK(&lwpp->l_mutex, PLBASE);
		/*
		 * We did not block; decrement the p_niblked field.
		 */
		FSPIN_LOCK(&p->p_niblk_mutex);
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);
		return (B_TRUE);
	}

	/*
	 * The event has not occurred, put the caller to sleep; Give control
	 * to the class specific code to enqueue the lwp.
	 */
	CL_INSQUE(lwpp, &eventp->ev_list, priority);
	FSPIN_UNLOCK(&eventp->ev_lock);

	/* Update the the lwp state */
	lwpp->l_stat = SSLEEP;
	lwpp->l_flag &= ~L_NWAKE;	/* clear NWAKE; do wake me */
	lwpp->l_syncp = eventp;
	lwpp->l_slptime = 0;		/* init the sleep start time */
	lwpp->l_stype = ST_EVENT;	/* blocked on a event */

	/*
	 * swtch() is called with the lwp state lock held at PLHI. This lock
	 * is released at PLBASE when safe in swtch()
	 */
	swtch(lwpp);
	FSPIN_LOCK(&p->p_niblk_mutex);
	--p->p_niblked;
	FSPIN_UNLOCK(&p->p_niblk_mutex);

	if (lwpp->l_flag & L_SIGWOKE) {
		/* awakened because of a signal or other update event */
		switch (issig((lock_t *)NULL)) {
		case ISSIG_NONE:
			/*
			 * a debugger had cleared the signal or if a job control
			 * stop signal was posted that was discarded by a
			 * subsequent SIGCONT. Go for the event. Note that
			 * the l_mutex lock is held.
			 */
			lwpp->l_flag &= ~L_SIGWOKE;
			goto acquire;
		case ISSIG_SIGNALLED:
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_flag &= ~L_SIGWOKE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
			return (B_FALSE);
		case ISSIG_STOPPED:
			/*
			 * Go for the event transparently. Since issig()
			 * returns with l_mutex dropped, need to start all
			 * over. Note that we could directly go for the event
			 * if issig() had not opened the window.
			 */
			goto again;
		default:
			/* this should never happen. */
			/*
			 *+ The issig() function returned an unexpected status.
			 *+ This indicates a kernel software problem.
			 */
			cmn_err(CE_PANIC, "unexpected return from issig");
			break;
		}
	}
	/* got the event */
	return (B_TRUE);
}

/*
 * void
 * EVENT_SIGNAL(event_t *eventp, int flags)
 * 	Signals an event.
 *
 * Calling/Exit State: 
 *	eventp is a pointer to the event to be signaled.  flags 
 *	can have the KS_NOPRMPT bit set to request non-preemptive wakeup.
 *
 *	The ipl is the same upon return as it was at the time of the
 *	call.
 *
 *	Returns:  none.
 *
 * Description:
 *	If there are waiting lwps, the one at the head of the list is
 *	dequeud and scheduled to run.  If there are no waiters, the event
 *	is posted.  
 */ 
void 
EVENT_SIGNAL(event_t *eventp, int flags)
{
	pl_t s;
	lwp_t *lwpp;

	FSPIN_LOCK(&eventp->ev_lock);
	if (eventp->ev_state == 1) {
		/*
		 * the event has already been posted, so we don't need
		 * to do anything.
		 */
		FSPIN_UNLOCK(&eventp->ev_lock);
		return;
	}

	if (++eventp->ev_state == 1) {
		/*
		 * the state was zero before, so we just post it (there
		 * are no waiters.
		 */
		FSPIN_UNLOCK(&eventp->ev_lock);
		return;
	}

	/*
	 * if the ev_state is negative, there are blocked lwp's.  ev_state
	 * should never be greater than 1.  Equal to 1 case is handled
	 * above.
	 */

	/* need to wake up a waiting lwp; pull the first one from the queue */
	lwpp = (lwp_t *)eventp->ev_head;
	remque(lwpp);
	FSPIN_UNLOCK(&eventp->ev_lock);

	/* Update the lwp state */
	s = LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SRUN;
	lwpp->l_syncp = NULL;
	lwpp->l_flag &= ~L_SIGWOKE;
	lwpp->l_stype = ST_NONE;	


	/*
	 * Call class-specific code to swap-in and place the lwp on
	 * the run queue.
	 */
	CL_WAKEUP(lwpp, lwpp->l_cllwpp, flags);
	UNLOCK(&lwpp->l_mutex, s);
}

/*
 * boolean_t
 * EVENT_UNSLEEP(event_t *eventp, lwp_t *lwpp)
 *	Remove an lwp from an event's sleep queue.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to manipulate.  lwpp is a pointer
 *	to the lwp to remove.  Returns:  B_TRUE if the lwp was removed, B_FALSE
 *	otherwise.
 *
 *	Should be called with the lwp state lock (l_mutex) held at
 *	PLHI.  This lock is still held upon return.
 */
boolean_t 
EVENT_UNSLEEP(event_t *eventp, struct lwp *lwpp)
{
	boolean_t dequeued;	/* was the lwp found? */

	ASSERT(LOCK_OWNED(&lwpp->l_mutex));

	/* Remove the lwp from the sleep queue */
	FSPIN_LOCK(&eventp->ev_lock);

	if (dequeued = slpdeque(&eventp->ev_list, lwpp)) {
		/* there is one less waiter; update the state */
		++eventp->ev_state;

		/*
		 * we've removed a waiter, which may leave 0 left.  In
		 * that case the state is 0, but the state should never
		 * be greater than 1.  (state == 1 is a posted event.)
		 */
	}

	/* release the state interlock */
	FSPIN_UNLOCK(&eventp->ev_lock);
	return (dequeued);
}

/*
 * void
 * EVENT_BROADCAST(event_t *eventp, int flags)
 *	Wake all the lwps blocked on an event.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to broadcast.  flags can have
 *	the KS_NOPRMPT bit set to request non-preemptive wakeup.
 *
 * 	Upon return, the ipl is the same as at the time of the call.
 *
 * Remarks:
 *	If no lwp's are blocked on the event, it is posted.
 */
void 
EVENT_BROADCAST(event_t *eventp, int flags)
{
        pl_t s;
        lwp_t *lwpp, *clwpp;

        FSPIN_LOCK(&eventp->ev_lock);
        if (eventp->ev_state == 1)  { 
		/* an event has already been posted, so we do nothing */
                FSPIN_UNLOCK(&eventp->ev_lock);
                return;
	}
        if (++eventp->ev_state == 1) {
		/* the state was zero, so no waiters.  Post the event. */
                FSPIN_UNLOCK(&eventp->ev_lock);
                return;
	}

        /*
	 * wake up all waiting lwp's
	 */

	lwpp = (lwp_t *)eventp->ev_head;	/* get list of lwp's */
	eventp->ev_tail->flink = NULL;		/* null terminated list */
	INITQUE(&eventp->ev_list);		/* empty sleep queue */
	eventp->ev_state = 0;			/* no waiters, not posted */

        FSPIN_UNLOCK(&eventp->ev_lock);
	while (lwpp != NULL) {
		clwpp = lwpp;
		lwpp = (lwp_t *)lwpp->l_flink;

        	/* Update the lwp state */
        	s = LOCK(&clwpp->l_mutex, PLHI);

        	clwpp->l_stat = SRUN;
        	clwpp->l_syncp = NULL;
        	clwpp->l_flag &= ~L_SIGWOKE;
		clwpp->l_stype = ST_NONE;

		/*
		 * Call the class specific code to  enqueue to the run-queue 
		 * and make the necessary arrangements for swapin.
		 */
        	CL_WAKEUP(clwpp, clwpp->l_cllwpp, flags);
        	UNLOCK(&clwpp->l_mutex, s);
	}
}

#undef EVENT_BLKD
/*
 * boolean_t
 * EVENT_BLKD(event_t *eventp)
 *	See if there are any lwps blocked on an event.
 *
 * Calling/Exit State:
 *	eventp is a pointer to the event to examine.  Returns:  B_TRUE
 *	if there are lwp's blocked, B_FALSE otherwise.
 *
 * Remarks:
 * 	The return value is stale data, unless the caller has taken steps
 *	to ensure its freshness.
 */
boolean_t
EVENT_BLKD(event_t *eventp)
{
	if (eventp->ev_state < 0) {
		return (B_TRUE);
	}
	return (B_FALSE);
}
