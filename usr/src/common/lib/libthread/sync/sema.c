/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/sema.c	1.2.2.10"

#include "libthread.h"
#include <trace.h>

/*
 * int sema_init(sema_t *sema, int sema_count, int type, void *arg)
 *	Initialize a thread semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *      second argument is count of number of resources protected
 *      third argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *      fourth argument is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is initialized to the
 *		type specified as the third argument. 
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	On exit no locks are held.
 */
/* ARGSUSED */
int
sema_init(sema_t *sema, int sema_count, int type, void *arg)
{
	ASSERT(sema != NULL);

	if (sema_count < 0) {
		TRACE5(0, TR_CAT_SEMA, TR_EV_SINIT, TR_CALL_ONLY, 
		   sema, sema_count, type, arg, EINVAL);
		return(EINVAL);
	}

	if ((type == USYNC_THREAD) || (type == USYNC_PROCESS)) {
		sema->s_type = type;
		/*
		 * initialize mutex and condition variable used
		 * to implement the semaphore functionality.
		*/
		_THR_MUTEX_INIT(&sema->s_mutex, type, 0);
		_THR_COND_INIT(&sema->s_cond, type, 0);
		sema->s_count = sema_count;
		sema->s_wakecnt = 0;
		TRACE5(0, TR_CAT_SEMA, TR_EV_SINIT, TR_CALL_ONLY, 
		   sema, sema_count, type, arg, 0);
		return(0);
	} else {
		TRACE5(0, TR_CAT_SEMA, TR_EV_SINIT, TR_CALL_ONLY, 
		   sema, sema_count, type, arg, EINVAL);
		return(EINVAL);
	}
}


/*
 * int sema_destroy(sema_t *sema)
 *	Invalidate a thread semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer the semaphore
 *
 *	During processing the internal synchronization locks of the
 *	condition variable and mutex are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is invalidated and can not longer
 *		be used for synchronization.
 *	EBUSY:  The semaphore is still busy.
 *
 *	On exit no locks are held.
 */

int
sema_destroy(sema_t *sema)
{
	thread_desc_t *curtp = curthread; /* holder for current thread */

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);	/* signals off */
		/*
		 * queue up any new comers on the mutex until you can 
		 * invalidate the semaphore.
		 */
		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			TRACE2(0, TR_CAT_SEMA, TR_EV_SDESTROY, TR_CALL_ONLY,
			   sema, EINVAL);
			_thr_sigon(curtp);
			return(EINVAL);
		}

		/* someone else has already destroyed it for you */
		if (sema->s_type == USYNC_DESTROYED) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			TRACE2(0, TR_CAT_SEMA, TR_EV_SDESTROY, TR_CALL_ONLY,
			   sema, EINVAL);
			_thr_sigon(curtp);
			return(EINVAL);
		}
			
		/* 
		 * If you manage to successfully destroy the condition
		 * variable, no one is waiting internally on the semaphore,
		 * however someone may be queued behind you on the mutex
		 * so immediately invalidate the semaphore to prevent any
		 * new comers. All the internal operations check for
		 * USYNC_DESTROYED after coming out of the mutex_lock
		 * to catch this condition.
		 */
		if (_THR_COND_DESTROY(&sema->s_cond) == EBUSY) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			TRACE2(0, TR_CAT_SEMA, TR_EV_SDESTROY, TR_CALL_ONLY,
			   sema, EBUSY);
			_thr_sigon(curtp);
			return(EBUSY);
		} else {
			sema->s_type = USYNC_DESTROYED;
		}

		/*
		 * At this point, you hold the mutex and have invalidated
		 * the semaphore.You just need to clear the mutex queue of 
		 * waiters but you must unlock it first to destroy it.
		 */
		(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		_thr_sigon(curtp);

		while (_THR_MUTEX_DESTROY(&sema->s_mutex) == EBUSY) {
			(void) _THR_MUTEX_LOCK(&sema->s_mutex);
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		TRACE2(0, TR_CAT_SEMA, TR_EV_SDESTROY, TR_CALL_ONLY, sema, 0);
		return(0);
	} else {
		TRACE2(0, TR_CAT_SEMA, TR_EV_SDESTROY, TR_CALL_ONLY,
		   sema, EINVAL);
		return(EINVAL);
	}
}


/*
 * int sema_wait(sema_t *sema)
 * 	Try to acquire a semaphore, if it is not available wait until it is.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *
 *	During processing the mutex protecting the semaphore count is
 *	acquired. In addition the condition variable synchronization lock
 *	and the thread lock of the current thread may be acquired
 *	indirectly via the call to cond_wait.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is initialized to the
 *		type specified as the second argument.
 *	EINTR:  The process was awakened by a signal or suspend rather than
 *		by acquisition of the semaphore.
 *	EINVAL: The semaphore was invalidated by sema_destroy
 * 		and the semaphore is no longer valid.
 *
 *	On exit no locks are held.
 */

int
sema_wait(sema_t *sema)
{
	int rval;
	thread_desc_t *curtp = curthread;

	ASSERT(sema != NULL);
	TRACE2(0, TR_CAT_SEMA, TR_EV_SWAIT, TR_CALL_FIRST, sema, sema->s_count);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT, TR_CALL_SECOND,
			   EINVAL);
			_thr_sigon(curtp);
			return(EINVAL);
		}
		
		/* semaphore was invalidated during wait on mutex */
		if(sema->s_type == USYNC_DESTROYED) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT, TR_CALL_SECOND,
			   EINVAL);
			_thr_sigon(curtp);
			return(EINVAL);
		}

		if (--sema->s_count >= 0) {		/* got one */
			/* mutex is locked, it cannot be invalidated */
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT, TR_CALL_SECOND, 0);
			_thr_sigon(curtp);
			return(0);
		}

		/*
	 	* Now we have to do a cond_wait, since condition variables
	 	* are not totally reliable, we use wakecount field to be
	 	* sure that the number of wakeups generated by sema_post is
	 	* equivalent to the number of threads that awaken here in
	 	* sema_wait. Cond_wait can return if the condition variable
	 	* has been invalidated, if the thread was awakened by a signal
	 	* or if cond_signal was actually done.
	 	*/ 
		do {
			rval = _thr_cond_wait(&sema->s_cond, &sema->s_mutex,
					 (lwp_mutex_t *) NULL, 
					 (timestruc_t *) 0, B_TRUE, B_TRUE);

			if (rval == EINVAL) {
				break;
			} else if (rval == EINTR) {
				/*
				 * cond_wait was interrupted by signal or
				 * suspend. We must recheck the condition
				 * to avoid missing a sema_post.
				 * Before we start again, we will dispense
				 * with any pending signals. This will
				 * only happen when we call sigon.
				 */
				(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
				_thr_sigon(curtp);
		
				/*
				 * Now we have to resolve the race.
				 * If we woke up because of a signal and
				 * a sema_post was never done or was done
				 * for someone else, we need to sleep
				 * again. If we also picked up the post
				 * either legitimately or not (we may have
				 * stolen someone elses byt thsi is unlikely)
				 * then we can break out at this point.
				 */
				_thr_sigoff(curtp);

				if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
					TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT,
					   TR_CALL_SECOND, EINVAL);
					_thr_sigon(curtp);
					return(EINVAL);
				}
		
				/*
				 * semaphore was invalidated during wait 
				 * on mutex
				 */
				if(sema->s_type == USYNC_DESTROYED) {
					(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
					TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT,
					   TR_CALL_SECOND, EINVAL);
					_thr_sigon(curtp);
					return(EINVAL);
				}
				
				if (sema->s_wakecnt) { /* we got it */
					rval = 0;
					break;
				}
			}
		} while (sema->s_wakecnt == 0);

		/*
	 	* If the wakeup was a result of a sema_post, decrement the
		*  s_wakecnt field.
	 	*/
		if (rval == 0)
			sema->s_wakecnt--;
	
		(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		_thr_sigon(curtp);
	} else {
		rval = EINVAL;
	}

	TRACE1(0, TR_CAT_SEMA, TR_EV_SWAIT, TR_CALL_SECOND, rval);
	return(rval);
}


/*
 * int sema_trywait(sema_t *sema)
 * 	Try to acquire a semaphore, if it is not available return immediately.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore 
 *
 *	During processing the mutex lock protecting the semaphore count
 *	is acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore was successfully acquired.
 *	EBUSY:  The semaphore is held by another thread.
 *	EINVAL: The mutex was invalidated by sema_destroy
 * 		and the semaphore is no longer valid.
 *
 *	On exit no locks are held.
 */

int
sema_trywait(sema_t *sema)
{
	int rval = 0;
	thread_desc_t *curtp = curthread;

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			rval = EINVAL;
		} else {
			/*
			 * There are three cases:
			 * 1. semaphore was invalidated during wait
			 * 2. semaphrore is available
			 * 3. semaphore is not available
			 */
			if (sema->s_type == USYNC_DESTROYED)
				rval = EINVAL;
			else if (sema->s_count > 0)
				sema->s_count--;
			else
				rval = EBUSY;

			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		_thr_sigon(curtp);
	} else
		rval = EINVAL;
	
	TRACE2(0, TR_CAT_SEMA, TR_EV_STRYWAIT, TR_CALL_ONLY, sema, rval);
	return(rval);
}


/*
 * int sema_post(sema_t *sema)
 *	Signal the availability of a resource protected by the semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *
 *	During processing the mutex lock protecting the semaphore count
 *	is acquired. The internal synchronization lock of the condition
 *	variable and the thread lock of a sleeping thread may be acquired
 *	indirectly via a call to cond_signal.
 *
 * Return Values/Exit State:
 *	0:	The semaphore s_count was incrremented another thread
 *		was successfully signaled.
 *	EINVAL: The semaphore was invalidated by sema_destroy.
 *
 *	On exit no locks are held.
 */

int
sema_post(sema_t *sema)
{
	thread_desc_t *curtp = curthread;
	int rval = 0;
#ifdef TRACE
	int original_count_value;
#endif /* TRACE */

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			rval = EINVAL;
		} else {
			/* semaphore may have been invalidated during sleep */
#ifdef TRACE
			original_count_value = sema->s_count;
#endif /* TRACE */
			if (sema->s_type == USYNC_DESTROYED)
				rval = EINVAL;
			else if (sema->s_count++ < 0) {
				sema->s_wakecnt++;
				(void) _THR_COND_SIGNAL(&sema->s_cond);
			}
			_THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		_thr_sigon(curtp);
	} else
		rval = EINVAL;
#ifdef TRACE
	TRACE3(0, TR_CAT_SEMA, TR_EV_SPOST, TR_CALL_ONLY,
	   sema, rval, original_count_value);
#endif /* TRACE */
	return(rval);
}

/*
 * int
 * _thr_remove_from_sema_queue(thread_desc_t *tp)
 *
 * Parameter/Calling State:
 *      The thread lock of the calling thread is held.
 *
 *      first argument is a pointer to thread
 *
 *      During processing, the synchronization lock of the condition variable
 *      queue and the mutex protecting the semaphore are acquired.
 *
 * Return Values/Exit State:
 *	Returns 1 if the thread is found and 0 otherwise.
 *
 *      On exit the thread lock remains held.
 */

int
_thr_remove_from_sema_queue(thread_desc_t *curtp)
{
	int rval;
	sema_t *sema;

	sema = (sema_t *)curtp->t_sync_addr;

	if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
		_thr_sigon(curtp);
		return(0);
	}

	if ((rval = _thr_remove_from_cond_queue(curtp))) {
		curtp->t_sync_addr = NULL;
		curtp->t_sync_type = TL_NONE;
	}

	(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
	return(rval);
}

#ifdef DEBUG
/*
 * int sema_dump(sema_t *sema)
 *	Dump the state of a thread semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	NONE 
 */

void
sema_dump(sema)
	sema_t		*sema;
{
	printf("Semaphore Dump sema = %x", sema);
	switch(sema->s_type)
	{
		case USYNC_THREAD:
			printf("Type: USYNC_THREAD\n");
			break;
		case USYNC_PROCESS:
			printf("Type: USYNC_PROCESS\n");
			break;
		default:
			printf("Type: INVALID\n");
			break;
	}
	printf("sema count:  %d  sema s_wakecnt:  %d\n",sema->s_count,sema->s_wakecnt);
/*	mutex_dump(&sema->s_mutex); */
	cond_dump(&sema->s_cond);
}
#endif



/*
 * The following routines provide the functionality of the external semaphore
 * interfaces but do not record trace data.  They are provided for internal
 * use of the library and must be kept in sync with the external interfaces,
 * e.g., _thr_notrace_sema_init must be identical to sema_init except for 
 * calls to TRACE.
 */

#ifdef TRACE
/*
 * int _thr_notrace_sema_init(sema_t *sema, int sema_count, int type, void *arg)
 *	Initialize a thread semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *      second argument is count of number of resources protected
 *      third argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *      fourth argument is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is initialized to the
 *		type specified as the third argument. 
 *	EINVAL: The value of type is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	On exit no locks are held.
 */
/* ARGSUSED */
int
_thr_notrace_sema_init(sema_t *sema, int sema_count, int type, void *arg)
{
	ASSERT(sema != NULL);

	if (sema_count < 0) {
		return(EINVAL);
	}

	if ((type == USYNC_THREAD) || (type == USYNC_PROCESS)) {
		sema->s_type = type;
		/*
		 * initialize mutex and condition variable used
		 * to implement the semaphore functionality.
		*/
		_THR_MUTEX_INIT(&sema->s_mutex, type, 0);
		_THR_COND_INIT(&sema->s_cond, type, 0);
		sema->s_count = sema_count;
		sema->s_wakecnt = 0;
		return(0);
	} else {
		return(EINVAL);
	}
}


/*
 * int _thr_notrace_sema_destroy(sema_t *sema)
 *	Invalidate a thread semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer the semaphore
 *
 *	During processing the internal synchronization locks of the
 *	condition variable and mutex are acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is invalidated and can not longer
 *		be used for synchronization.
 *	EBUSY:  The semaphore is still busy.
 *
 *	On exit no locks are held.
 */

int
_thr_notrace_sema_destroy(sema_t *sema)
{
	thread_desc_t *curtp = curthread; /* holder for current thread */

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);	/* signals off */
		/*
		 * queue up any new comers on the mutex until you can 
		 * invalidate the semaphore.
		 */
		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			_thr_sigon(curtp);
			return(EINVAL);
		}

		/* someone else has already destroyed it for you */
		if (sema->s_type == USYNC_DESTROYED) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			_thr_sigon(curtp);
			return(EINVAL);
		}
			
		/* 
		 * If you manage to successfully destroy the condition
		 * variable, no one is waiting internally on the semaphore,
		 * however someone may be queued behind you on the mutex
		 * so immediately invalidate the semaphore to prevent any
		 * new comers. All the internal operations check for
		 * USYNC_DESTROYED after coming out of the mutex_lock
		 * to catch this condition.
		 */
		if (_THR_COND_DESTROY(&sema->s_cond) == EBUSY) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			_thr_sigon(curtp);
			return(EBUSY);
		} else {
			sema->s_type = USYNC_DESTROYED;
		}

		/*
		 * At this point, you hold the mutex and have invalidated
		 * the semaphore.You just need to clear the mutex queue of 
		 * waiters but you must unlock it first to destroy it.
		 */
		(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		_thr_sigon(curtp);

		while (_THR_MUTEX_DESTROY(&sema->s_mutex) == EBUSY) {
			(void) _THR_MUTEX_LOCK(&sema->s_mutex);
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		return(0);
	} else {
		return(EINVAL);
	}
}


/*
 * int _thr_notrace_sema_wait(sema_t *sema)
 * 	Try to acquire a semaphore, if it is not available wait until it is.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *
 *	During processing the mutex protecting the semaphore count is
 *	acquired. In addition the condition variable synchronization lock
 *	and the thread lock of the current thread may be acquired
 *	indirectly via the call to cond_wait.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore is initialized to the
 *		type specified as the second argument.
 *	EINTR:  The process was awakened by a signal or suspend rather than
 *		by acquisition of the semaphore.
 *	EINVAL: The semaphore was invalidated by sema_destroy
 * 		and the semaphore is no longer valid.
 *
 *	On exit no locks are held.
 */

int
_thr_notrace_sema_wait(sema_t *sema)
{
	int rval;
	thread_desc_t *curtp = curthread;

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			_thr_sigon(curtp);
			return(EINVAL);
		}
		
		/* semaphore was invalidated during wait on mutex */
		if(sema->s_type == USYNC_DESTROYED) {
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			_thr_sigon(curtp);
			return(EINVAL);
		}

		if (--sema->s_count >= 0) {		/* got one */
			/* mutex is locked, it cannot be invalidated */
			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
			_thr_sigon(curtp);
			return(0);
		}

		/*
	 	* Now we have to do a cond_wait, since condition variables
	 	* are not totally reliable, we use wakecount field to be
	 	* sure that the number of wakeups generated by sema_post is
	 	* equivalent to the number of threads that awaken here in
	 	* sema_wait. Cond_wait can return if the condition variable
	 	* has been invalidated, if the thread was awakened by a signal
	 	* or if cond_signal was actually done.
	 	*/ 
		do {
			rval = _thr_cond_wait(&sema->s_cond, &sema->s_mutex,
					 (lwp_mutex_t *) NULL, 
					 (timestruc_t *) 0, B_TRUE, B_TRUE);

			if (rval == EINVAL) {
				break;
			} else if (rval == EINTR) {
				/*
				 * cond_wait was interrupted by signal or
				 * suspend. We must recheck the condition
				 * to avoid missing a sema_post.
				 * Before we start again, we will dispense
				 * with any pending signals. This will
				 * only happen when we call sigon.
				 */
				(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
				_thr_sigon(curtp);
		
				/*
				 * Now we have to resolve the race.
				 * If we woke up because of a signal and
				 * a sema_post was never done or was done
				 * for someone else, we need to sleep
				 * again. If we also picked up the post
				 * either legitimately or not (we may have
				 * stolen someone elses byt thsi is unlikely)
				 * then we can break out at this point.
				 */
				_thr_sigoff(curtp);

				if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
					_thr_sigon(curtp);
					return(EINVAL);
				}
		
				/*
				 * semaphore was invalidated during wait 
				 * on mutex
				 */
				if(sema->s_type == USYNC_DESTROYED) {
					(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
					_thr_sigon(curtp);
					return(EINVAL);
				}
				
				if (sema->s_wakecnt) { /* we got it */
					rval = 0;
					break;
				}
			}
		} while (sema->s_wakecnt == 0);

		/*
	 	* If the wakeup was a result of a sema_post, decrement the
		*  s_wakecnt field.
	 	*/
		if (rval == 0)
			sema->s_wakecnt--;
	
		(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		_thr_sigon(curtp);
	} else {
		rval = EINVAL;
	}

	return(rval);
}


/*
 * int _thr_notrace_sema_trywait(sema_t *sema)
 * 	Try to acquire a semaphore, if it is not available return immediately.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore 
 *
 *	During processing the mutex lock protecting the semaphore count
 *	is acquired.
 *
 * Return Values/Exit State:
 *	0:	The thread semaphore was successfully acquired.
 *	EBUSY:  The semaphore is held by another thread.
 *	EINVAL: The mutex was invalidated by sema_destroy
 * 		and the semaphore is no longer valid.
 *
 *	On exit no locks are held.
 */

int
_thr_notrace_sema_trywait(sema_t *sema)
{
	int rval = 0;
	thread_desc_t *curtp = curthread;

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			rval = EINVAL;
		} else {
			/*
			 * There are three cases:
			 * 1. semaphore was invalidated during wait
			 * 2. semaphrore is available
			 * 3. semaphore is not available
			 */
			if (sema->s_type == USYNC_DESTROYED)
				rval = EINVAL;
			else if (sema->s_count > 0)
				sema->s_count--;
			else
				rval = EBUSY;

			(void) _THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		_thr_sigon(curtp);
	} else
		rval = EINVAL;
	
	return(rval);
}


/*
 * int _thr_notrace_sema_post(sema_t *sema)
 *	Signal the availability of a resource protected by the semaphore.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      first argument is a pointer to a semaphore
 *
 *	During processing the mutex lock protecting the semaphore count
 *	is acquired. The internal synchronization lock of the condition
 *	variable and the thread lock of a sleeping thread may be acquired
 *	indirectly via a call to cond_signal.
 *
 * Return Values/Exit State:
 *	0:	The semaphore s_count was incrremented another thread
 *		was successfully signaled.
 *	EINVAL: The semaphore was invalidated by sema_destroy.
 *
 *	On exit no locks are held.
 */

int
_thr_notrace_sema_post(sema_t *sema)
{
	thread_desc_t *curtp = curthread;
	int rval = 0;

	ASSERT(sema != NULL);

	if((sema->s_type == USYNC_THREAD) || (sema->s_type == USYNC_PROCESS)) {
		_thr_sigoff(curtp);

		if (_THR_MUTEX_LOCK(&sema->s_mutex) == EINVAL) {
			rval = EINVAL;
		} else {
			/* semaphore may have been invalidated during sleep */
			if (sema->s_type == USYNC_DESTROYED)
				rval = EINVAL;
			else if (sema->s_count++ < 0) {
				sema->s_wakecnt++;
				(void) _THR_COND_SIGNAL(&sema->s_cond);
			}
			_THR_MUTEX_UNLOCK(&sema->s_mutex);
		}
		_thr_sigon(curtp);
	} else
		rval = EINVAL;
	return(rval);
}
#endif /* TRACE */
