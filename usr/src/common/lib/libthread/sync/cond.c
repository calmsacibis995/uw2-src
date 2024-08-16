/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/cond.c	1.1.2.27"

#include <libthread.h>
#include <trace.h>
#include <memory.h>
#include <stdlib.h>

#define LWP_MUTEX_ISLOCKED(/* lwp_mutex_t * */ mp) ((mp)->lock) /* XXX */

#ifdef DEBUG
extern void _thr_lock_cond(cond_t *);
extern void _thr_unlock_cond(cond_t *);
#endif

/*
 * LOCK_COND_SYNCLOCK(cond_t *cond) 
 *	Lock the internal LWP mutex lock for a thread condition variable.
 *
 * Return Values/Exit State:
 *	0:	The lwp mutex variable pointed to by cond is locked.
 *	
 */

#ifdef DEBUG
#define LOCK_COND_SYNCLOCK(cond)	_thr_lock_cond(cond)
#else
#define LOCK_COND_SYNCLOCK(cond) _lwp_mutex_lock(&((cond)->c_sync_lock))
#endif

/*
 * UNLOCK_COND_SYNCLOCK(cond_t *cond) 
 *	Unlock the internal LWP mutex lock for a thread condition variable.
 *
 * Return Values/Exit State:
 *	0:	The lwp mutex variable pointed to by cond is unlocked.
 *	
 */

#ifdef DEBUG
#define UNLOCK_COND_SYNCLOCK(cond)  _thr_unlock_cond(cond)
#else
#define UNLOCK_COND_SYNCLOCK(cond) _lwp_mutex_unlock(&((cond)->c_sync_lock))
#endif

/*
 * int cond_init(cond_t *cond, int type, void *arg)
 *	Initialize a thread condition variable.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *      third argument is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	No new locks are held on exit.
 *	
 */
/* ARGSUSED */
int
cond_init(cond_t *cond, int type, void *arg)
{
	PRINTF("Entering cond_init\n");
	if (cond == NULL)
		return(EINVAL);

	/*
	 * zero fill structure, since this is equivalent to a
	 * USYNC_THREAD condition variable by definition.
	 */
	(void) memset((void *)cond, '\0', sizeof(*cond)); 

	switch(type)
	{
	case USYNC_PROCESS:
		cond->c_type = type;
		/* FALLTHROUGH */
	case USYNC_THREAD:
		break;
	default:
		TRACE4(0, TR_CAT_COND, TR_EV_CINIT, TR_CALL_ONLY,
		   cond, type, arg, EINVAL);
		return(EINVAL);
	}
        TRACE4(0, TR_CAT_COND, TR_EV_CINIT, TR_CALL_ONLY, cond, type, arg, 0);
	return(0);
}


/*
 * int cond_destroy(cond_t *cond)
 *	Destroy a thread condition variable.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *      During processing, the synchronization lock of the condition 
 *	variable sleep queue is acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable type is invalidated so it
 *		can no longer be used.
 *	EINVAL: The value of c_type is neither USYNC_THREAD nor USYNC_PROCESS.
 *	EBUSY:	The condition variable is currently in use by other threads.
 *
 *	No locks held on exit.
 */

int
cond_destroy(cond_t *cond)
{
	int rval;
	thread_desc_t *curtp = curthread;

	PRINTF("Entering cond_destroy\n");
	if (cond == NULL) {
		return(EINVAL);
	}

	switch(cond->c_type)
	{
	case USYNC_THREAD:
	case USYNC_PROCESS:
		_thr_sigoff(curtp);	 /* turn off signals */
		/*
		 * check wanted flag for USYNC_PROCESS condition
		 * variables or bound threads. Check the sleep queue
		 * for multiplexed threads.
		 */
		if (cond->c_lcond.wanted) {
			_thr_sigon(curtp);
			rval = EBUSY;
			break;
		}

		LOCK_COND_SYNCLOCK(cond);/* lock condition varible */

		if (cond->c_syncq == NULL) {
			cond->c_type = USYNC_DESTROYED; 
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			rval = 0;
			break;
		}

		if (!THRQ_ISEMPTY(cond->c_syncq)) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			rval = EBUSY;
			break;
		}

		/* invalidate condition variable */
		cond->c_type = USYNC_DESTROYED; 
		free(cond->c_syncq);
		cond->c_syncq = NULL;
		UNLOCK_COND_SYNCLOCK(cond);
		_thr_sigon(curtp);
		rval = 0;
		break;
	default:
		rval = EINVAL;
		break;
	}
	TRACE2(curtp, TR_CAT_COND, TR_EV_CDESTROY, TR_CALL_ONLY, cond, rval);
	return rval;
}

/*
 * int cond_timedwait(cond_t *cond, mutex_t *mutex, timestruc_t *abstime)
 *	Wait for the occurance of condition. If the amount of time specified
 *	in abstime passes the thread is awakened and ETIME is returned.
 *
 *
 * Parameter/Calling State:
 *      mutex passed as the second arguement is assumed to be locked.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is a pointer to a locked mutex
 *      third argument is a pointer to a timestruc_t
 *
 *	During processing indirectly through the call to _thr_cond_wait,
 *	the thread lock of the calling thread,
 *      the synchronization lock of the condition variable sleep queue
 *	are acquired. In addition, the lock on the mutex passed in as
 *	the second argument is dropped and reacquired.
 *
 * Return Values/Exit State:
 *	0:	The condition was signaled by cond_signal.
 *	EINVAL:	The value of type is neither USYNC_THREAD nor USYNC_PROCESS
 *		or the time value specified by abstime is not valid.
 *	ETIME:	The timer has expired.
 *	EINTR:	The thread was awakened by a signal.
 *
 *	On exit the mutex lock is held.
 */

int
cond_timedwait(cond_t *cond, mutex_t *mutex, timestruc_t *abstime)
{
	int rval;

	TRACE4(0, TR_CAT_COND, TR_EV_CTIMEDWAIT, TR_CALL_FIRST,
	   cond, mutex, abstime->tv_sec, abstime->tv_nsec);
	ASSERT(LWP_MUTEX_ISLOCKED((lwp_mutex_t *) mutex));

	PRINTF("Entering cond_timedwait.\n");

	/* Check that time is acceptable */
	if (abstime == NULL || abstime->tv_sec < 0 || 
		abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
		TRACE1(0, TR_CAT_COND, TR_EV_CTIMEDWAIT, TR_CALL_SECOND,
		   EINVAL);
		return(EINVAL);
	}

	if (cond == NULL) {
		TRACE1(0, TR_CAT_COND, TR_EV_CTIMEDWAIT, TR_CALL_SECOND,
		   EINVAL);
		return(EINVAL);
	}
	rval = _thr_cond_wait(cond, mutex, (lwp_mutex_t *) NULL, 
			 (const timestruc_t *) abstime, B_TRUE, B_TRUE);

        TRACE1(0, TR_CAT_COND, TR_EV_CTIMEDWAIT, TR_CALL_SECOND, rval);
	return(rval);
}

/*
 * int cond_wait(cond_t *cond, mutex_t *mutex)
 *	Wait for the occurance of condition. 
 *
 * Parameter/Calling State:
 *	The lock of the mutex passed in as the second parameter is assumed
 *	to be held.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is a pointer to a locked mutex
 *
 *	During processing indirectly through the call to _thr_cond_wait,
 *	the thread lock of the calling thread,
 *      the synchronization lock of the condition variable sleep queue
 *	are acquired. In addition, the lock on the mutex passed in as
 *	the second argument is dropped and reacquired.
 *
 * Return Values/Exit State:
 *	0:	The condition was signaled by cond_signal.
 *	EINVAL:	The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *		or the time value specified by abstime is not valid.
 *	EINTR:	The thread was awakened by a signal.
 *
 *	On exit the mutex passed in as the second argument is locked.
 */

int
cond_wait(cond_t *cond, mutex_t *mutex)
{
	int rval;

	TRACE2(0, TR_CAT_COND, TR_EV_CWAIT, TR_CALL_FIRST, cond, mutex);
	PRINTF("Entering cond_wait\n");

	if (cond == NULL) {
		return(EINVAL);
	}
	if (mutex != NULL && ((mutex->m_type != USYNC_PROCESS) &&
	   (mutex->m_type != USYNC_THREAD))) {
		TRACE1(0, TR_CAT_COND, TR_EV_CWAIT, TR_CALL_SECOND, EINVAL);
	        return(EINVAL);
	}

	rval = _thr_cond_wait(cond, mutex, (lwp_mutex_t *)NULL,
		(const timestruc_t *) NULL, B_TRUE, B_TRUE);
	TRACE1(0, TR_CAT_COND, TR_EV_CWAIT, TR_CALL_SECOND, rval);
	return rval;
}

/*
 * int cond_signal(cond_t *cond)
 *	Signal the occurrance of the condition, awakening a waiting
 * 	thread if one is sleeping on the queue.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing the synchronization lock of the condition 
 *	variable sleep queue and the thread lock of the first thread
 *	on the sleep queue are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	On exit no locks are held.
 *	
 */

int
cond_signal(cond_t *cond)
{
	thread_desc_t *curtp = curthread;
	thread_desc_t *thread;
	int rval = 0;			/* return value */
	int needlwp;

	switch (cond->c_type) {
	case USYNC_PROCESS:
		rval = _lwp_cond_signal(&cond->c_lcond);
		break;

	case USYNC_THREAD:
		_thr_sigoff(curtp);
		/*
		 * If a bound thread is waiting, awaken it first
		 * since it is holding kernel resources and is
		 * likely more important that a multiplexed
		 * thread.
		 */

		if (cond->c_lcond.wanted) {
			rval = unblock((vaddr_t)&(cond->c_lcond),
				(char *) &(cond->c_lcond.wanted),
				UNBLOCK_ANY);

			if (rval != EINVAL) {
				_thr_sigon(curtp);
                                TRACE3(curtp, TR_CAT_COND, TR_EV_CSIGNAL,
					TR_CALL_ONLY,  cond, rval,
					TR_DATA_WAITER);
				return(rval);
			}
		}

		LOCK_COND_SYNCLOCK(cond);

		/* no syncq, no waiters */
		if (cond->c_syncq == NULL) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			TRACE3(curtp, TR_CAT_COND, TR_EV_CSIGNAL,
			   TR_CALL_ONLY, cond, 0, TR_DATA_NOWAITER);
			return(0);
		}
		/*
		 * Set the first thread on the sleep queue
		 * of multiplexed threads runable.
		 */
		if ((thread = (thread_desc_t *)_thrq_rem_first(cond->c_syncq))
		    != NULL) {
			UNLOCK_COND_SYNCLOCK(cond);
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
			TRACE3(curtp, TR_CAT_COND, TR_EV_CSIGNAL,
			   TR_CALL_ONLY, cond, 0, TR_DATA_WAITER);
		} else {
			UNLOCK_COND_SYNCLOCK(cond);
			TRACE3(curtp, TR_CAT_COND, TR_EV_CSIGNAL,
			   TR_CALL_ONLY, cond, 0, TR_DATA_NOWAITER);
		}
		_thr_sigon(curtp);
		return 0;
	default:
		rval = EINVAL;
		break;
	}
	TRACE3(curtp, TR_CAT_COND, TR_EV_CSIGNAL, TR_CALL_ONLY,
	   cond, rval, TR_DATA_DONTKNOW);
	return rval;
}


/*
 * int cond_broadcast(cond_t *cond)
 *	Signal the occurrance of the condition, awakening all waiting threads.
 *
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing the synchronization lock of the condition 
 *	variable sleep queue and the thread lock of the each thread
 *	on the sleep queue are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	
 */

int
cond_broadcast(cond)
	cond_t *cond;
{
	thread_desc_t	*curtp = curthread;
	thread_desc_t	*thread;
	thrq_elt_t	c_tempque;
	int		rval;
	/* LINTED */
	int		prio, needlwp;


	if (cond == NULL) {
		TRACE3(curtp, TR_CAT_COND, TR_EV_CBROADCAST, TR_CALL_ONLY,
		   cond, EINVAL, TR_DATA_NOWAITER);
		return(EINVAL);
	}

	switch (cond->c_type) {
	case USYNC_PROCESS:
		rval = _lwp_cond_broadcast(&cond->c_lcond);
		break;

	case USYNC_THREAD:
		_thr_sigoff(curtp);
		/*
		 * If bound threads are waiting, awaken them first
		 * since they are holding kernel resources and are
		 * likely more important than multiplexed threads.
		 */

		if (cond->c_lcond.wanted) {
			(void) unblock((vaddr_t) &(cond->c_lcond),
			(char *) &(cond->c_lcond.wanted),
			UNBLOCK_ALL);
		}
		
		/*
		 * Move any threads on the queue to a temproary
		 * queue headed by c_tempque, if we don't move them
		 * because of the locking hierarchy we will have
		 * to repeatedly lock and unlock the queue lock
		 * for each thread.
		 */
		LOCK_COND_SYNCLOCK(cond);

		/* no syncq, no waiters */
		if (cond->c_syncq == NULL) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			/*
			 * for trace, we can't say for sure that
			 * no waiter was found because we may have
			 * awakened a bound thread (above)
			 */
			TRACE3(curtp, TR_CAT_COND, TR_EV_CBROADCAST,
			   TR_CALL_ONLY, cond, 0, TR_DATA_DONTKNOW);
			return(0);
		}

		if (THRQ_ISEMPTY(cond->c_syncq)) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			TRACE3(curtp, TR_CAT_COND, TR_EV_CBROADCAST,
			   TR_CALL_ONLY, cond, 0, TR_DATA_DONTKNOW);
			return(0);
		} else if (_THRQ_ONLY_MEMBER(cond->c_syncq)) {
			thread = _thrq_rem_first(cond->c_syncq);
			UNLOCK_COND_SYNCLOCK(cond);
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
			_thr_sigon(curtp);
			 TRACE3(curtp, TR_CAT_COND, TR_EV_CBROADCAST,
			   TR_CALL_ONLY, cond, 0, TR_DATA_WAITER);
			return(0);
		}
		
		
		c_tempque.thrq_next = cond->c_syncq->thrq_next;
		c_tempque.thrq_prev = cond->c_syncq->thrq_prev;
		cond->c_syncq->thrq_prev->thrq_next = &c_tempque;
		cond->c_syncq->thrq_next->thrq_prev = &c_tempque;
		/*
		 * At this point it is safe to reinitialize the
		 * condition variable sleep queue and unlock it.
		 */
		THRQ_INIT(cond->c_syncq);
		UNLOCK_COND_SYNCLOCK(cond);

		while ((thread = (thread_desc_t *)_thrq_rem_first(
			&c_tempque)) != NULL) { 
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
		}
		_thr_sigon(curtp);
		rval = 0;
		break;
	default:
		rval = EINVAL;
		break;
	}
	TRACE3(curtp, TR_CAT_COND, TR_EV_CBROADCAST, TR_CALL_ONLY,
	   cond, rval, TR_DATA_DONTKNOW);
	return rval;
}

/*
 * void _thr_cond_time_handler(thread_t tid)
 *	Notify a thread sleeping for a cond_timedwait that the
 *	timer has expired.
 *
 * Parameter/Calling State:
 *      No locks are held on entry, signal handlers are assumed to be off.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing the tidvec lock, the condition variable 
 *	synchronization lock and the thread lock of the thread to
 *	be awakend are acquired.
 *
 * Return Values/Exit State:
 *
 *	On exit no locks are held.
 *	
 */

void
_thr_cond_time_handler(int tid)
{
	thread_desc_t	*thread;	/* pointer to thread structure */
	int		found = INVALID_PRIO;
	/* LINTED */
	int		prio;

	/*
	 * Lock the tidvec and validate the thread id to be sure the
	 * thread still exists. Assuming it does lock the thread lock
	 * to stabilize the thread structure. Once you have the thread
	 * lock you can release the tidvec.
	 */

	PRINTF1("_thr_cond_time_handler for tid %d\n", tid);

	if ((thread = _thr_get_thread(tid)) == NULL) {
		return;
	}

	/*
	 * we know the thread exists, check if sleeping on a condtion variable
	 */
	if (thread->t_state == TS_SLEEP && thread->t_sync_type == TL_COND) {
		if ((found = _thr_remove_from_cond_queue(thread))) {
			PRINTF("_thr_cond_time_handler,calling _thr_setrq\n");
			found = _thr_setrq(thread, 0);
		}
	}
	UNLOCK_THREAD(thread);
	if (found != INVALID_PRIO) {
		_thr_activate_lwp(found);
	}
}

/*
 * int 
 * _thr_remove_from_cond_queue(thread_desc_t *tp)
 *
 * Parameter/Calling State:
 *	The thread lock of the calling thread is held.
 *
 *      first argument is a pointer to thread
 *
 *      During processing, the synchronization lock of the condition variable
 *	queue is acquired.
 *
 * Return Values/Exit State:
 *	Returns if the thread is found and 0 otherwise.
 *
 *	On exit the thread lock remains held.
 */

int 
_thr_remove_from_cond_queue(thread_desc_t *tp)
{
	int rval = 0;
	cond_t	*cond;

	PRINTF("Entering _thr_remove_from_cond_queue\n");
	cond = (cond_t *)tp->t_sync_addr;
	LOCK_COND_SYNCLOCK(cond);

	if (cond->c_syncq == NULL) {
		UNLOCK_COND_SYNCLOCK(cond);
		return(rval);
	}

	if (!THRQ_ISEMPTY(&(tp->t_thrq_elt)) &&
	_thrq_is_on_q(cond->c_syncq,tp)) {
		_thrq_rem_from_q(tp);
		tp->t_sync_addr = NULL;
		tp->t_sync_type = TL_NONE;
		rval = 1;
	}
	UNLOCK_COND_SYNCLOCK(cond);
	return(rval);
}

/*
 * void
 * _thr_requeue_cond(thread_desc_t *tp, int prio)
 *	Repositions specified thread on the condition variable sleep queue
 *	according to its new priority.
 *
 * Parameter/Calling State:
 *	The thread lock of the calling thread is held.
 *	First argument is a pointer to the thread descriptor.
 *	Second argument is new priority for thread tp.
 *
 *	During processing, the synchronization lock of the condition
 *	variable queue is acquired.
 *
 * Return Values/Exit State:
 *	On exit the thread lock remeins held.
 */

void
_thr_requeue_cond(thread_desc_t *tp, int prio)
{
	cond_t *cond;

	PRINTF("Entering _thr_requeue_cond\n");
	cond = (cond_t *)tp->t_sync_addr;
	LOCK_COND_SYNCLOCK(cond);
	tp->t_pri = prio;
	if (!THRQ_ISEMPTY(&(tp->t_thrq_elt)) &&
	_thrq_is_on_q(cond->c_syncq,tp)) {
		_thrq_rem_from_q(tp);
		_thrq_prio_ins(cond->c_syncq,tp);
	}
	UNLOCK_COND_SYNCLOCK(cond);
}

/*
 * int _thr_cond_wait(cond_t *cond, mutex_t *mutex, lwp_mutex_t *lmutex,
 *	const timestruc_t *abstime, boolean_t signal_return)
 *
 *	Wait for the occurrance of condition, does the real work.
 *
 * Parameter/Calling State:
 *	The lock of the mutex passed in as the second parameter is assumed
 *	to be held if mutex is non-NULL otherwise the lock of the lwp
 *	mutex passed in as the third argument is assumed to be held.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is a pointer to a locked mutex or NULL
 *      third argument is a pointer to a locked lwp mutex or NULL
 *      fourth argument is a pointer to a valid timestruc for cond_timed_wait
 *		and NULL for cond_wait.
 *	fifth argument is a boolean which indicate whether cond_wait will
 *	loop and sleep or return when interupted by an internally generated
 *	pending signal or suspend. Since condition variables are used by
 *	semaphores, they need to see any break in the wait to guarantee
 *	that no wakeups are ever lost.  When this argument is true, interrupts
 *	return EINTR; otherwise they return 0.
 *	sixth argument is a boolean which indicates whether cond_wait
 *	should re-acquire the mutex prior to return; this allows an internal
 *	use of condition variables that doesn't re-acquire the mutex -- this
 *	is used to optimize performance of barriers.
 *
 *	During processing the thread lock of the calling thread,
 *      the synchronization lock of the condition variable sleep queue
 *	are acquired. In addition, the lock on the mutex passed in as
 *	the second argument is dropped and reacquired.
 *
 * Return Values/Exit State:
 *	0:	The condition was signaled by cond_signal.
 *	EINVAL:	The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *		or the time value specified by abstime is not valid.
 *	EINTR:	The thread was awakened by a signal.
 *
 *	On exit the mutex passed in as the second argument is locked.
 */

int
_thr_cond_wait(cond_t *cond, mutex_t *mutex, lwp_mutex_t *lmutex,
	   const timestruc_t *abstime, boolean_t signal_return,
	   boolean_t reacquire_mutex)
{
	thread_desc_t	*curtp = curthread;	/* calling thread pointer */
	int rval;			/* return value */
	int rval2 = 0;			/* return value */

	ASSERT((mutex != NULL) || (lmutex != NULL));
	ASSERT((mutex == NULL) || (lmutex == NULL));
	PRINTF("Entering _thr_cond_wait\n");

#ifdef DEBUG
	if (mutex)
		ASSERT(LWP_MUTEX_ISLOCKED((lwp_mutex_t *) mutex));
	else
		ASSERT(LWP_MUTEX_ISLOCKED(lmutex));
#endif

	if ((cond->c_type != USYNC_PROCESS) && (cond->c_type
	    != USYNC_THREAD)) {
	        return(EINVAL);
	}

	/*
	 * _thr_cond_wait handles cond_timed_wait, cond_wait and
	 * _thr_condwait_lwpmutex for internal library code. 
	 * For bound threads and for USYNC_PROCESS we need to suspend the
	 * lwp and the thread using the lwp blocking
	 * operations. We cannot use lwp_cond_wait or lwp_cond_timedwait
	 * directly since the mutex is type USYNC_THREAD
	 * and must be unlocked. For multiplexed threads we put it on the
	 * internal sleep queue.
	 */
	if ((ISBOUND(curtp)) || (cond->c_type == USYNC_PROCESS)) {
		/*
		 * Call prepblock to place the calling LWP 
		 * on the sync-queue associated with c_lcond.
		 *  Prepblock may set c_lcond.wanted.
		 */
		while ((rval = prepblock((vaddr_t)&cond->c_lcond,
		    (char *)&cond->c_lcond.wanted,
		     PREPBLOCK_WRITER)) == EINVAL) {
			/*
			 * The calling LWP is placed on another
			 * sync-queue. Remove it from the 
			 * sync-queue and prepblock again.
			 */
			cancelblock();
			continue;
		}
		if (rval != 0) {
			_thr_sigon(curtp);
			return(rval);
		}
			
		/*
		 * Release the mutex.
		 */
		if (mutex)
			rval = _THR_MUTEX_UNLOCK(mutex);
		else
			rval = _lwp_mutex_unlock(lmutex);
			
		if (rval != 0) {
			/*
			 * mutex_unlock failed.
			 * Remove the calling LWP from the 
			 * sync-queue, and return.
			 */
			cancelblock();
			_thr_sigon(curtp);
			return(rval);
		}
		/*
		 * Give up a processor and wait for 
		 * being awakened.
		 */
		if ((rval = block(abstime)) != 0) {
			switch (rval) {
			case ERESTART:
				/*
				 * Block was interrupted by a 
				 * signal or a forkall.
				 */
				rval = EINTR;
				/*FALLTHROUGH*/
			case EINTR:
				/*
				 * Block was interrupted by a signal.
				 */
			case EFAULT:	/* abstime is invalid */
			case EINVAL:	/* abstime is invalid */
				/*
				 * The calling LWP may be still
				 * placed on the sync-queue. Remove it.
				 */
				cancelblock();
				break;
			
			case ETIME:	/* abstime has passed */
				/*
				 * The calling LWP is not on the sync-queue.
				 */
				break;
			
			case ENOENT:
				/*
				 * Prepblock was canceled by a 
				 * signal handler, or the
				 * calling LWP was in a child 
				 * process created by a
				 * forkall, so that the calling
				 * LWP was removed from
				 * the sync-queue.
				 */
				rval = EINTR;
				break;

			default:
				break;
			}
		}
			
		/*
		 * Re-acquire the mutex, if necessary.
		 */
		if (reacquire_mutex == B_TRUE) {
			if (mutex) {
				rval2 = _THR_MUTEX_LOCK(mutex);
			} else {
				rval2 = _lwp_mutex_lock(lmutex);
			}
		}

		/*
		 * The break was caused by a pending signal, if the caller
		 * wants to see all breaks in the wait return and send
		 * back EINVAL, otherwise return 0 and force the caller
		 * to recheck the condition.
		 */
		if (signal_return && rval == EINTR)
			return(rval);
		else if (rval != ETIME)
			rval = 0;

		if (rval2 != 0) {
			/*
			 * mutex_lock failed. Replace a return value 
			 * with rval2 only if block returned 0 or ETIME.
			 */
			if (rval == 0 || rval == ETIME)
				rval = rval2;
		}
		return(rval);
	}

	/*
	 * Multiplexed threads, condition variable sync queue for
	 * for blocking and internal library function and locks.
	 */
	_thr_sigoff(curtp);

	/*
	 * if abstime != 0 then this is cond_timed_wait so
	 * set the condition variable callout on the
	 * global timeout queue.
	 *
	 */
	if (abstime &&
	    _thr_setcallout(&curtp->t_callo_cv, abstime, (const timestruc_t *)0,
			    (void (*)(void *))_thr_cond_time_handler,
			    (void *)curtp->t_tid)) {
		_thr_panic("cond_timedwait:mux timer failed");
	}

	LOCK_THREAD(curtp);	  /* lock thread */

	/* make sure timer hasn't already expired if this is a timed wait! */
	if (abstime && curtp->t_callo_cv.co_stat != CO_TIMER_ON) {
		UNLOCK_THREAD(curtp);
		rval = ETIME;
		if (reacquire_mutex != B_TRUE) {
			if (mutex) {
				(void) _THR_MUTEX_UNLOCK(mutex);
			} else {
				(void) _lwp_mutex_unlock(lmutex);
			}
		}
		_thr_sigon(curtp);
		return(rval);
	}

	LOCK_COND_SYNCLOCK(cond); /* lock condition variable */

	if (cond->c_syncq == NULL) {
		UNLOCK_THREAD(curtp);	/* must unlock thread before malloc */
		if ((cond->c_syncq = 
		    (thrq_elt_t *)malloc(sizeof(thrq_elt_t))) == NULL)
			_thr_panic("_thr_cond_wait: malloc failed no space");

		memset(cond->c_syncq,'\0',sizeof(thrq_elt_t));

		/*
		 * we can't call LOCK_THREAD while holding the cond synclock
		 * since it could lead to deadlock; this would be a violation
		 * of the locking hierarchy.  Therefore we try the lock
		 * first.  If we get it, we can continue; otherwise we must
		 * release the cond synclock and then obtain both locks in
		 * the correct order.
		 */

		if (TRYLOCK_THREAD(curtp) == EBUSY) {
			UNLOCK_COND_SYNCLOCK(cond);
			LOCK_THREAD(curtp);
			LOCK_COND_SYNCLOCK(cond);
		}
	}

	/* make sure timer hasn't already expired if this is a timed wait! */
	if (abstime && curtp->t_callo_cv.co_stat != CO_TIMER_ON) {
		UNLOCK_COND_SYNCLOCK(cond);
		UNLOCK_THREAD(curtp);
		rval = ETIME;
		if (reacquire_mutex != B_TRUE) {
			if (mutex) {
				(void) _THR_MUTEX_UNLOCK(mutex);
			} else {
				(void) _lwp_mutex_unlock(lmutex);
			}
		}
		_thr_sigon(curtp);
		return(rval);
	}

	/* suspend the thread and put it on sleep queue */
	curtp->t_sync_addr = (void *) cond;
	curtp->t_sync_type = TL_COND;
	_thrq_prio_ins(cond->c_syncq,curtp);
	curtp->t_state = TS_SLEEP;
	UNLOCK_COND_SYNCLOCK(cond); /* unlock condition var */

	if (mutex)
		_THR_MUTEX_UNLOCK(mutex);
	else
		_lwp_mutex_unlock(lmutex);

	
	_thr_swtch(1,curtp);		/* give up the lwp */
	ASSERT(curtp->t_state == TS_ONPROC);

	/*
	 * Determine why we are woke up. It could be caused
	 * by the timer expiring, a pending signal or suspend
	 * or by cond_signal where we have acquired the
	 * condition we have been waiting for. Even in the
	 * case of timer expiration, it is impossible to
	 * determine if the  timeout occurred before or
	 * after we were possibly awakend by a pending signal
	 * or suspend. The opposite is also true.
	 */
	rval = 0;

	if (abstime || signal_return) {
		LOCK_THREAD(curtp);	  /* lock thread to check state */
	 	if (abstime) {
			if (curtp->t_callo_cv.co_stat != CO_TIMER_ON) {
				UNLOCK_THREAD(curtp);
				rval = ETIME;
			} else {
	 			if (signal_return && curtp->t_sig)
                        		rval = EINTR;
				UNLOCK_THREAD(curtp);
				(void)_thr_rmcallout(&curtp->t_callo_cv);
			}
	 	} else {
			if (curtp->t_sig) {
                        	rval = EINTR;
			}
			UNLOCK_THREAD(curtp);
		}

	}

	/*
	 * Re-acquire the mutex, if necessary.
	 */
	if (reacquire_mutex == B_TRUE) {
		if (mutex) {
			(void) _THR_MUTEX_LOCK(mutex);
		} else {
			(void) _lwp_mutex_lock(lmutex);
		}
	}

	_thr_sigon(curtp);
	return rval;
}

/*
 * int cond_dump(cond_t *cond)
 *	Dump a thread condition variable.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *	NONE
 */

#ifdef DEBUG
void
cond_dump(cond)
	cond_t *cond;
{
	int i;
	thrq_elt_t	*qp;

	printf("Condition Variable address = %x",cond);
	switch(cond->c_type)
	{
		case USYNC_THREAD:
			printf("Type = USYNC_THREAD\n");
			break;
		case USYNC_PROCESS:
			printf("Type = USYNC_PROCESS\n");
			break;
		default:
			printf("Type = INVALID\n");
			break;
	}

	printf("Multiplexed Threads Queued:");
	
	if (THRQ_ISEMPTY(cond->c_syncq))
		printf("NONE\n");
	else {
		i = 0;
		qp = cond->c_syncq->thrq_next;
		printf("\n");

		while (i != 0 && (i % 4 != 0)) {
			printf("  thread = %x  ",qp);
			if ((qp = qp->thrq_next) == cond->c_syncq)
				break;
		}
	}

	printf("Bound Threads Queued:");

	if (cond->c_lcond.wanted)
		printf("  Yes\n");
	else
		printf("  No\n");
}

#endif
#ifdef DEBUG
/*
 * lock statistic structures for condition variables.
 */

thr_lckstat_t	cond_stats;

/*
 * lwp mutex lock to protect multiple accesses to thread locks since
 * there are many.
 */
lwp_mutex_t	_thr_lckstat_cond;

void
_thr_lock_cond(cond_t *cond)
{
	_lwp_mutex_lock(&_thr_lckstat_cond);
	cond_stats.lckstat_locks++;
	_lwp_mutex_unlock(&_thr_lckstat_cond);

	if (_lwp_mutex_trylock(&((cond)->c_sync_lock)) == EBUSY) {
		_lwp_mutex_lock(&_thr_lckstat_cond);
		cond_stats.lckstat_waits++;
		_lwp_mutex_unlock(&_thr_lckstat_cond);
		_lwp_mutex_lock(&((cond)->c_sync_lock));
	}
	PRINTF2("@@@ Thread %d locked CONDition variable %x\n",
		 curthread->t_tid, cond);
}

void
_thr_unlock_cond(cond_t *cond)
{
	_lwp_mutex_lock(&_thr_lckstat_cond);
	cond_stats.lckstat_unlocks++;
	_lwp_mutex_unlock(&_thr_lckstat_cond);
	PRINTF2("@@@ Thread %d UNlocked CONDition Variable %x\n",
		 curthread->t_tid, cond);
	_lwp_mutex_unlock(&((cond)->c_sync_lock));
}
#endif




/*
 * The following interfaces allow the library to obtain
 * condition variable functionality without recording the data
 * during trace.  These interfaces must be kept in sync with
 * their external counterparts (i.e., _thr_notrace_cond_init must be
 * an exact copy of cond_init minus the calls to TRACE, etc.).
 */

#ifdef TRACE
/*
 * int _thr_notrace_cond_init(cond_t *cond, int type, void *arg)
 *	Initialize a thread condition variable.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *      third argument is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	No new locks are held on exit.
 *	
 */
/* ARGSUSED */
int
_thr_notrace_cond_init(cond_t *cond, int type, void *arg)
{
	PRINTF("Entering cond_init\n");
	if (cond == NULL)
		return(EINVAL);

	/*
	 * zero fill structure, since this is equivalent to a
	 * USYNC_THREAD condition variable by definition.
	 */
	(void) memset((void *)cond, '\0', sizeof(*cond)); 

	switch(type)
	{
	case USYNC_PROCESS:
		cond->c_type = type;
		/* FALLTHROUGH */
	case USYNC_THREAD:
		break;
	default:
		return(EINVAL);
	}
	return(0);
}


/*
 * int _thr_notrace_cond_destroy(cond_t *cond)
 *	Destroy a thread condition variable.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *      During processing, the synchronization lock of the condition 
 *	variable sleep queue is acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable type is invalidated so it
 *		can no longer be used.
 *	EINVAL: The value of c_type is neither USYNC_THREAD nor USYNC_PROCESS.
 *	EBUSY:	The condition variable is currently in use by other threads.
 *
 *	No locks held on exit.
 */

int
_thr_notrace_cond_destroy(cond_t *cond)
{
	int rval;
	thread_desc_t *curtp = curthread;

	PRINTF("Entering cond_destroy\n");
	if (cond == NULL) {
		return(EINVAL);
	}

	switch(cond->c_type)
	{
	case USYNC_THREAD:
	case USYNC_PROCESS:
		_thr_sigoff(curtp);	 /* turn off signals */
		/*
		 * check wanted flag for USYNC_PROCESS condition
		 * variables or bound threads. Check the sleep queue
		 * for multiplexed threads.
		 */
		if (cond->c_lcond.wanted) {
			_thr_sigon(curtp);
			rval = EBUSY;
			break;
		}

		LOCK_COND_SYNCLOCK(cond);/* lock condition varible */

		if (cond->c_syncq == NULL) {
			cond->c_type = USYNC_DESTROYED; 
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			rval = 0;
			break;
		}

		if (!THRQ_ISEMPTY(cond->c_syncq)) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			rval = EBUSY;
			break;
		}

		/* invalidate condition variable */
		cond->c_type = USYNC_DESTROYED; 
		free(cond->c_syncq);
		cond->c_syncq = NULL;
		UNLOCK_COND_SYNCLOCK(cond);
		_thr_sigon(curtp);
		rval = 0;
		break;
	default:
		rval = EINVAL;
		break;
	}
	return rval;
}

/*
 * int _thr_notrace_cond_timedwait(cond_t *cond, mutex_t *mutex, timestruc_t *abstime)
 *	Wait for the occurance of condition. If the amount of time specified
 *	in abstime passes the thread is awakened and ETIME is returned.
 *
 *
 * Parameter/Calling State:
 *      mutex passed as the second arguement is assumed to be locked.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is a pointer to a locked mutex
 *      third argument is a pointer to a timestruc_t
 *
 *	During processing indirectly through the call to _thr_cond_wait,
 *	the thread lock of the calling thread,
 *      the synchronization lock of the condition variable sleep queue
 *	are acquired. In addition, the lock on the mutex passed in as
 *	the second argument is dropped and reacquired.
 *
 * Return Values/Exit State:
 *	0:	The condition was signaled by cond_signal.
 *	EINVAL:	The value of type is neither USYNC_THREAD nor USYNC_PROCESS
 *		or the time value specified by abstime is not valid.
 *	ETIME:	The timer has expired.
 *	EINTR:	The thread was awakened by a signal.
 *
 *	On exit the mutex lock is held.
 */

int
_thr_notrace_cond_timedwait(cond_t *cond, mutex_t *mutex, timestruc_t *abstime)
{
	int rval;

	ASSERT(LWP_MUTEX_ISLOCKED((lwp_mutex_t *) mutex));

	PRINTF("Entering cond_timedwait.\n");

	/* Check that time is acceptable */
	if (abstime == NULL || abstime->tv_sec < 0 || 
		abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
		return(EINVAL);
	}

	if (cond == NULL) {
		return(EINVAL);
	}
	rval = _thr_cond_wait(cond, mutex, (lwp_mutex_t *) NULL, 
			 (const timestruc_t *) abstime, B_TRUE, B_TRUE);

	return(rval);
}

/*
 * int _thr_notrace_cond_wait(cond_t *cond, mutex_t *mutex)
 *	Wait for the occurance of condition. 
 *
 * Parameter/Calling State:
 *	The lock of the mutex passed in as the second parameter is assumed
 *	to be held.
 *
 *      first argument is a pointer to a condition variable
 *      second argument is a pointer to a locked mutex
 *
 *	During processing indirectly through the call to _thr_cond_wait,
 *	the thread lock of the calling thread,
 *      the synchronization lock of the condition variable sleep queue
 *	are acquired. In addition, the lock on the mutex passed in as
 *	the second argument is dropped and reacquired.
 *
 * Return Values/Exit State:
 *	0:	The condition was signaled by cond_signal.
 *	EINVAL:	The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *		or the time value specified by abstime is not valid.
 *	EINTR:	The thread was awakened by a signal.
 *
 *	On exit the mutex passed in as the second argument is locked.
 */

int
_thr_notrace_cond_wait(cond_t *cond, mutex_t *mutex)
{
	int rval;

	PRINTF("Entering cond_wait\n");

	if (cond == NULL) {
		return(EINVAL);
	}
	if (mutex != NULL && ((mutex->m_type != USYNC_PROCESS) &&
	   (mutex->m_type != USYNC_THREAD))) {
	        return(EINVAL);
	}

	rval = _thr_cond_wait(cond, mutex, (lwp_mutex_t *)NULL,
		(const timestruc_t *) NULL, B_TRUE, B_TRUE);
	return rval;
}

/*
 * int _thr_notrace_cond_signal(cond_t *cond)
 *	Signal the occurrance of the condition, awakening a waiting
 * 	thread if one is sleeping on the queue.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing the synchronization lock of the condition 
 *	variable sleep queue and the thread lock of the first thread
 *	on the sleep queue are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	On exit no locks are held.
 *	
 */

int
_thr_notrace_cond_signal(cond_t *cond)
{
	thread_desc_t *curtp = curthread;
	thread_desc_t *thread;
	int rval = 0;			/* return value */
	int needlwp;

	switch (cond->c_type) {
	case USYNC_PROCESS:
		rval = _lwp_cond_signal(&cond->c_lcond);
		break;

	case USYNC_THREAD:
		_thr_sigoff(curtp);
		/*
		 * If a bound thread is waiting, awaken it first
		 * since it is holding kernel resources and is
		 * likely more important that a multiplexed
		 * thread.
		 */

		if (cond->c_lcond.wanted) {
			rval = unblock((vaddr_t)&(cond->c_lcond),
				(char *) &(cond->c_lcond.wanted),
				UNBLOCK_ANY);

			if (rval != EINVAL) {
				_thr_sigon(curtp);
				return(rval);
			}
		}

		LOCK_COND_SYNCLOCK(cond);

		/* no syncq, no waiters */
		if (cond->c_syncq == NULL) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			return(0);
		}
		/*
		 * Set the first thread on the sleep queue
		 * of multiplexed threads runable.
		 */
		if ((thread = (thread_desc_t *)_thrq_rem_first(cond->c_syncq))
		    != NULL) {
			int prio;

			UNLOCK_COND_SYNCLOCK(cond);
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
		} else {
			UNLOCK_COND_SYNCLOCK(cond);
		}
		_thr_sigon(curtp);
		return 0;
	default:
		rval = EINVAL;
		break;
	}
	return rval;
}


/*
 * int _thr_notrace_cond_broadcast(cond_t *cond)
 *	Signal the occurrance of the condition, awakening all waiting threads.
 *
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a condition variable
 *
 *	During processing the synchronization lock of the condition 
 *	variable sleep queue and the thread lock of the each thread
 *	on the sleep queue are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread condition variable is initialized to the
 *		type specified as the second argument.
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	
 */

int
_thr_notrace_cond_broadcast(cond)
	cond_t *cond;
{
	thread_desc_t	*curtp = curthread;
	thread_desc_t	*thread;
	thrq_elt_t	c_tempque;
	int		rval;
	int		prio, needlwp;


	if (cond == NULL) {
		return(EINVAL);
	}

	switch (cond->c_type) {
	case USYNC_PROCESS:
		rval = _lwp_cond_broadcast(&cond->c_lcond);
		break;

	case USYNC_THREAD:
		_thr_sigoff(curtp);
		/*
		 * If bound threads are waiting, awaken them first
		 * since they are holding kernel resources and are
		 * likely more important than multiplexed threads.
		 */

		if (cond->c_lcond.wanted) {
			(void) unblock((vaddr_t) &(cond->c_lcond),
			(char *) &(cond->c_lcond.wanted),
			UNBLOCK_ALL);
		}
		
		/*
		 * Move any threads on the queue to a temproary
		 * queue headed by c_tempque, if we don't move them
		 * because of the locking hierarchy we will have
		 * to repeatedly lock and unlock the queue lock
		 * for each thread.
		 */
		LOCK_COND_SYNCLOCK(cond);

		/* no syncq, no waiters */
		if (cond->c_syncq == NULL) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			/*
			 * for trace, we can't say for sure that
			 * no waiter was found because we may have
			 * awakened a bound thread (above)
			 */
			return(0);
		}

		if (THRQ_ISEMPTY(cond->c_syncq)) {
			UNLOCK_COND_SYNCLOCK(cond);
			_thr_sigon(curtp);
			return(0);
		} else if (_THRQ_ONLY_MEMBER(cond->c_syncq)) {
			thread = _thrq_rem_first(cond->c_syncq);
			UNLOCK_COND_SYNCLOCK(cond);
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
			_thr_sigon(curtp);
			return(0);
		}
		
		
		c_tempque.thrq_next = cond->c_syncq->thrq_next;
		c_tempque.thrq_prev = cond->c_syncq->thrq_prev;
		cond->c_syncq->thrq_prev->thrq_next = &c_tempque;
		cond->c_syncq->thrq_next->thrq_prev = &c_tempque;
		/*
		 * At this point it is safe to reinitialize the
		 * condition variable sleep queue and unlock it.
		 */
		THRQ_INIT(cond->c_syncq);
		UNLOCK_COND_SYNCLOCK(cond);

		while ((thread = (thread_desc_t *)_thrq_rem_first(
			&c_tempque)) != NULL) { 
			/*
			 * Lock the thread, cancel any pending
			 * timers and start the thread running.
			 */
			LOCK_THREAD(thread);
			if (THRQ_ISEMPTY(&thread->t_thrq_elt)) {
				thread->t_sync_addr = NULL;
				thread->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(thread, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(thread);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
		}
		_thr_sigon(curtp);
		rval = 0;
		break;
	default:
		rval = EINVAL;
		break;
	}
	return rval;
}

#endif /* TRACE */
