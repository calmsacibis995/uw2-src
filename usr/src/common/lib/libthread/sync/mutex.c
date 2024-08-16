/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/mutex.c	1.3.12.11"


/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file contains all functions associated with mutex variables
 */

#include <libthread.h>
#include <trace.h>
#include <sys/usync.h>

/*
 * macros that define lock and unlock operations on the interal lwp
 * mutex used to synchronize access to the sleep queue.
 */

#define LOCK_MUTEX_SYNCLOCK(mutex)      _lwp_mutex_lock(&((mutex)->m_sync_lock))
#define UNLOCK_MUTEX_SYNCLOCK(mutex)    _lwp_mutex_unlock(&((mutex)->m_sync_lock))


/*
 * mutex_init(mutex_t *mutex, int type, void *arg)
 *	initializes a mutex to the specified type
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	first argument is a pointer to a mutex
 *	second argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *	third argument is not currently used
 *
 *	no locks are acquired during processing
 *
 * Return Values/Exit State:
 *	Returns 0 on success or EINVAL if type is invalid.
 *	On success, mutex is initialized to the specified type.
 */

/* ARGSUSED2 */
int
mutex_init(mutex_t *mutex, int type, void *arg)
{
	static lwp_mutex_t lminitval;

        switch (type) {
            case USYNC_THREAD: 
		mutex->m_type = USYNC_THREAD;
		break;
            case USYNC_PROCESS:
		mutex->m_type = USYNC_PROCESS;
		break;
            default:
		TRACE4(0, TR_CAT_MUTEX, TR_EV_MINIT, TR_CALL_ONLY,
		   mutex, type, arg, EINVAL);
		return(EINVAL);
	}
	mutex->m_lmutex = lminitval;
	mutex->m_sync_lock = lminitval;
	THRQ_INIT(&(mutex->m_sleepq));
	TRACE4(0, TR_CAT_MUTEX, TR_EV_MINIT, TR_CALL_ONLY, mutex, type, arg, 0);
	return(0);
}



/*
 * mutex_lock(mutex_t *mutex)
 *	obtains the specified mutex lock, blocking if necessary
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	The lock of the calling thread and the sync_lock of the
 *	desired mutex are acquired during processing; also the
 *	desired mutex is locked.
 *
 * Return Values/Exit State:
 *	Returns zero and acquires the specified mutex on success; 
 *	otherwise, returns an appropriate errno value.
 */

int
mutex_lock(mutex_t *mutex)
{
	int ret=0;
	thread_desc_t *t;
	TRACE1(0, TR_CAT_MUTEX, TR_EV_MLOCK, TR_CALL_FIRST, mutex);

	switch (mutex->m_type) {
	case USYNC_THREAD:
		if (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex) != 0)  {
		/* didn't get the lock */
			t = curthread;
			if (ISBOUND(t)) {
				ret = _lwp_mutex_lock(&(mutex->m_lmutex));
			} else {
				/* thread is multiplexed */
				while (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex)!= 0){
					/* multiplexed thread loop start */
					_thr_sigoff(t);
					LOCK_THREAD(t);
					LOCK_MUTEX_SYNCLOCK(mutex);
					if (mutex->m_type != USYNC_THREAD) {
        				/*
         				 * the mutex may have been destroyed 
					 * via mutex_destroy() before the
         				 * synclock was acquired; return
					 * EINVAL if this happened.
         				 */

						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						ret = EINVAL;
						_thr_sigon(t);
						break;
					}
					t->t_sync_addr = (void *)mutex;
					t->t_sync_type = TL_MUTEX;
					_thrq_prio_ins((thrq_elt_t *)
					(&(mutex->m_sleepq)), t);
					/*
				 	 * WARNING: assumes STORE-ORDERED memory
				 	 */
					if (LWP_MUTEX_TRYLOCK
					(&mutex->m_lmutex) == 0) {
        				/*
         				 * the mutex may have been unlocked 
					 * while enqueuing the calling thread;
         				 * now that the calling thread is on
					 * the sleep queue, one last check is 
					 * necessary to make sure the mutex
         				 * hasn't been unlocked.  If it has 
					 * been unlocked, dequeue the calling 
					 * thread and loop * again.  If not, 
					 * subsequent unlocking operations 
					 * will find the thread on the queue
         				 * so there is no risk of the calling 
					 * thread being stranded.
         				 */
						_thrq_rem_from_q(t);
						t->t_sync_addr = NULL;
						t->t_sync_type = TL_NONE;
						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						_thr_sigon(t);
						break;
					}
					UNLOCK_MUTEX_SYNCLOCK(mutex);
					t->t_state = TS_SLEEP;
					_thr_swtch(1, t);
					_thr_sigon(t);
				} /* end while */
			} /* end else */
		} else {
			/* got the lock on first try */
			TRACE2(0, TR_CAT_MUTEX, TR_EV_MLOCK, TR_CALL_SECOND, 
			   ret, TR_DATA_NOBLOCK);
			return(ret);
		}
		break;
       	case USYNC_PROCESS:
		if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
			ret=_lwp_mutex_lock(&(mutex->m_lmutex));
		} else { /* else, we got the lock and can return */
			TRACE2(0, TR_CAT_MUTEX, TR_EV_MLOCK, TR_CALL_SECOND, 
			   ret, TR_DATA_NOBLOCK);
			return(ret);
		}
               	break;
       	default:
		ret=EINVAL;
		TRACE2(0, TR_CAT_MUTEX, TR_EV_MLOCK, TR_CALL_SECOND,
		   ret, TR_DATA_NOBLOCK);
		return(ret);
	}
	TRACE2(0, TR_CAT_MUTEX, TR_EV_MLOCK, TR_CALL_SECOND,
	   ret, TR_DATA_BLOCK);
	return(ret);
}



/*
 * mutex_trylock(mutex_t *mutex)
 *	makes a single attempt to obtain a mutex
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	Returns zero if the lock was available and acquires the
 *	lock; otherwise returns EBUSY and doesn't acquire the lock.
 */

int
mutex_trylock(mutex_t *mutex)
{
	if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
		TRACE2(0, TR_CAT_MUTEX, TR_EV_MTRYLOCK, TR_CALL_ONLY, 
		   mutex, EBUSY);
		return(EBUSY);
	}

	TRACE2(0, TR_CAT_MUTEX, TR_EV_MTRYLOCK, TR_CALL_ONLY, mutex, 0);
	return(0);
}



/*
 * mutex_unlock(mutex_t *mutex)
 *	unlocks a mutex and awakens a waiting thread if one exists.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	During processing, the sync_lock of the mutex may be acquired and
 *	the thread lock of a thread that is awakened may be acquired.
 *
 * Return Values/Exit State:
 *	Returns 0 on success; otherwise, an errno value.  On success, if 
 *	a thread was waiting for the mutex, exactly one thread will be 
 *	awakened to try to obtain the mutex.
 */

int
mutex_unlock(mutex_t *mutex)
{
	int rval, needlwp;
#ifdef TRACE
	int awakened_someone = TR_DATA_DONTKNOW;
#endif /* TRACE */

        switch (mutex->m_type) {
            case USYNC_THREAD:
		/* first, clear the lock */
		_lock_clear(&(mutex->m_lmutex.lock));
		/*
		 * WARNING: assumes STORE-ORDERED memory
		 */
	        /*
	         * Next, we check if a bound thread is waiting for the mutex
	         * because bound threads are favored over multiplexed threads.
	         * mutex->m_lmutex.wanted is set if a bound thread may be
	         * waiting for the mutex; if so call unblock().  If
	         * unblock() doesn't return EINVAL, it means an LWP was found
	         * blocked in the kernel and we can return.  If unblock()
	         * returns EINVAL or mutex->m_lmutex.wanted wasn't set, no LWP
	         * (and so no bound thread) was found waiting for the mutex so
	         * we need to check if a multiplexed thread is waiting.
	         */
		if (mutex->m_lmutex.wanted) {
        		rval = unblock((vaddr_t)&mutex->m_lmutex, 
				(char *)&(mutex->m_lmutex.wanted), UNBLOCK_ANY);
			if (rval != EINVAL) {
#ifdef TRACE
				/* woke someone up */
				awakened_someone = TR_DATA_WAITER;
#endif /* TRACE */
				break;
			}
		}
	        /*
	         * If we get here, it's because no bound thread was found
	         * waiting for the mutex so we want to see if there's a
	         * multiplexed thread waiting for the mutex.
	         *
	         * First, set rval to 0 because it may have been set to EINVAL
	         * in the above code.
	         */
		rval = 0;
		if (!(THRQ_ISEMPTY((&mutex->m_sleepq)))) {
			thread_desc_t *temp;
			thread_desc_t *curtp = curthread;

			_thr_sigoff(curtp);
			LOCK_MUTEX_SYNCLOCK(mutex);
			temp = _thrq_rem_first((thrq_elt_t *)&mutex->m_sleepq);
			UNLOCK_MUTEX_SYNCLOCK(mutex);
			if (temp != NULL) { 
		        /*
		         * If temp in non-null, we found a thread on the 
			 * mutex sleep queue.  In that case, make it 
			 * runnable and activate an LWP for it.  It's 
			 * possible that temp can be null because the
		         * last thread may have been removed before we 
			 * acquired the mutex sync lock.
		         */
				int prio;
#ifdef TRACE
				awakened_someone = TR_DATA_WAITER;
#endif /* TRACE */
				LOCK_THREAD(temp);
				if (THRQ_ISEMPTY(&temp->t_thrq_elt)) {
					temp->t_sync_addr = NULL;
					temp->t_sync_type = TL_NONE;
					needlwp = _thr_setrq(temp, 0);
				} else {
					needlwp = INVALID_PRIO;
				}
				UNLOCK_THREAD(temp);
				if (needlwp != INVALID_PRIO) {
					_thr_activate_lwp(needlwp);
				}
			}
			_thr_sigon(curtp);
		}
		break;
            case USYNC_PROCESS:
		rval=_lwp_mutex_unlock(&mutex->m_lmutex);
#ifdef TRACE
		/* can't tell if anyone was awakened */
		awakened_someone = TR_DATA_DONTKNOW;
#endif /* TRACE */
                break;
            default:
		rval=EINVAL;
	}
#ifdef TRACE
	TRACE3(0, TR_CAT_MUTEX, TR_EV_MUNLOCK, TR_CALL_ONLY, 
	   mutex, rval, awakened_someone);
#endif /* TRACE */
	return(rval);
}



/*
 * mutex_destroy(mutex_t *mutex)
 *	destroys a mutex
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	During processing, the mutex lock may be acquired and the sync_lock
 *	of the mutex may also be acquired.
 *
 * Return Values/Exit State:
 *	Returns zero and leaves the mutex in an invalid state on success; 
 *	otherwise an errno value.
 */

int
mutex_destroy(mutex_t *mutex)
{
        switch (mutex->m_type) {
            case USYNC_THREAD:
            case USYNC_PROCESS:
		/* First, make sure nobody holds the mutex */
		if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
			TRACE2(0, TR_CAT_MUTEX, TR_EV_MDESTROY, TR_CALL_ONLY, 
			   mutex, EBUSY);
			return(EBUSY); /* mutex is held */
		}
        	/*
        	 * Now make sure that no thread is waiting for the mutex.
        	 * This is necessary because we may have grabbed the mutex
        	 * while other threads were blocked waiting for it.  This
        	 * is a consequence of the way we unlock mutexes.
        	 */
		LOCK_MUTEX_SYNCLOCK(mutex);
		if (mutex->m_lmutex.wanted || 
					!THRQ_ISEMPTY((&mutex->m_sleepq))) {
			UNLOCK_MUTEX_SYNCLOCK(mutex);
			_THR_MUTEX_UNLOCK(mutex);
			TRACE2(0, TR_CAT_MUTEX, TR_EV_MDESTROY, TR_CALL_ONLY, 
			   mutex, EBUSY);
			return(EBUSY);  /* threads are waiting */
		}
		mutex->m_type = USYNC_DESTROYED;
		UNLOCK_MUTEX_SYNCLOCK(mutex);
	        /*
	         * We return with the mutex locked.  This ensures that anyone
	         * who tries to acquire the mutex via mutex_lock() will go
	         * through the appropriate code and find that the type of
	         * the mutex is neither USYNC_PROCESS nor USYNC_THREAD.
	         */
		TRACE2(0, TR_CAT_MUTEX, TR_EV_MDESTROY, TR_CALL_ONLY, mutex, 0);
		return(0);
            default:
		TRACE2(0, TR_CAT_MUTEX, TR_EV_MDESTROY, TR_CALL_ONLY, 
		   mutex, EINVAL);
		return(EINVAL);
	}
}
		


/*
 * int _thr_remove_from_mutex_queue()
 *	Removes the specified thread from the mutex sleepq it is on.
 *
 * Calling/Exit state:
 *      On entry, the thread lock of tp is held, and signal handlers are
 *      disabled.
 *
 *      During processing, the sleepq lock is obtained and released.
 *
 *      On exit, the thread lock of tp is still held and signal handlers are
 *      still disabled. 
 *
 * Return Values/Exit State:
 *      returns 1, if the thread is removed from the queue, otherwise 0.
 */

int
_thr_remove_from_mutex_queue(thread_desc_t *tp)
{
	int rval = 0;
	mutex_t *mutex;
	
	ASSERT(tp != (thread_desc_t *)NULL);
	ASSERT(THR_ISSIGOFF(curthread));

	mutex = (mutex_t *)tp->t_sync_addr;
	ASSERT(mutex != (mutex_t *)NULL);

	LOCK_MUTEX_SYNCLOCK(mutex);
	if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
		_thrq_rem_from_q(tp);
		tp->t_sync_addr = NULL;
		tp->t_sync_type = TL_NONE;
		rval = 1;
	}
	UNLOCK_MUTEX_SYNCLOCK(mutex);
	return(rval);
}

/*
 * void _thr_requeue_mutex()
 *	Changes the priority of the specified thread and, if the thread
 *	is on a mutex sleep queue, removes it from the mutex sleepq and
 *      replaces it onto the queue based on its new priority.
 *	If the mutex was not on a sleep queue, it is not placed back onto 
 *	a sleep queue.
 *
 * Calling/Exit state:
 *      On entry, the thread lock of tp is held, and signal handlers are
 *      disabled.
 *
 *      During processing, the sleepq lock is obtained and released.
 *
 *      On exit, the thread lock of tp is still held and signal handlers are
 *      still disabled.
 *
 * Return Values/Exit State:
 *      returns no value; on exit, the thread's priority has been changed
 *	and the thread has been repositioned on its mutex sleep queue, if
 *	it was on a mutex sleep queue on entry.
 */

void
_thr_requeue_mutex(thread_desc_t *tp, int prio)
{
        mutex_t *mutex;

        ASSERT(tp != (thread_desc_t *)NULL);
        ASSERT(THR_ISSIGOFF(curthread));

        mutex = (mutex_t *)tp->t_sync_addr;
        ASSERT(mutex != (mutex_t *)NULL);

        LOCK_MUTEX_SYNCLOCK(mutex);
	tp->t_pri = prio;
        if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
                _thrq_rem_from_q(tp);
		_thrq_prio_ins((thrq_elt_t *)(&(mutex->m_sleepq)), tp);
        }
        UNLOCK_MUTEX_SYNCLOCK(mutex);
}


/*
 * _thr_mutex_lock_sigoff(mutex_t *mutex, thread_desc_t *curtp)
 *	obtains the specified mutex lock, blocking if necessary; assumes
 *	signal handlers are disabled upon entry, re-enables them if
 *	blocking is necessary, but returns with them disabled
 *
 * Parameter/Calling State:
 *	No locks are held on entry but signal handlers are disabled
 *
 *	First argument is a pointer to a mutex; second argument is a
 *	pointer to the calling thread's descriptor.
 *
 *	The lock of the calling thread and the sync_lock of the
 *	desired mutex are acquired during processing; also the
 *	desired mutex is locked.
 *
 * Return Values/Exit State:
 *	Returns zero and acquires the specified mutex on success; 
 *	otherwise, returns an appropriate errno value.  Signal handlers
 *	are disabled on return.
 */

int
_thr_mutex_lock_sigoff(mutex_t *mutex, thread_desc_t *t)
{
	int ret=0;

	ASSERT(THR_ISSIGOFF(t));
	switch (mutex->m_type) {
	case USYNC_THREAD:
		if (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex) != 0)  {
		/* didn't get the lock */
			if (ISBOUND(t)) {
				ret = _thr_lwp_mutex_lock_sigoff
				      (&(mutex->m_lmutex), t);
			} else {
				/* thread is multiplexed */
				while (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex)!= 0){
					/* multiplexed thread loop start */
					LOCK_THREAD(t);
					LOCK_MUTEX_SYNCLOCK(mutex);
					if (mutex->m_type != USYNC_THREAD) {
        				/*
         				 * the mutex may have been destroyed 
					 * via mutex_destroy() before the
         				 * synclock was acquired; return
					 * EINVAL if this happened.
         				 */

						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						ret = EINVAL;
						break;
					}
					t->t_sync_addr = (void *)mutex;
					t->t_sync_type = TL_MUTEX;
					_thrq_prio_ins((thrq_elt_t *)
					(&(mutex->m_sleepq)), t);
					/*
				 	 * WARNING: assumes STORE-ORDERED memory
				 	 */
					if (LWP_MUTEX_TRYLOCK
					(&mutex->m_lmutex) == 0) {
        				/*
         				 * the mutex may have been unlocked 
					 * while enqueuing the calling thread;
         				 * now that the calling thread is on
					 * the sleep queue, one last check is 
					 * necessary to make sure the mutex
         				 * hasn't been unlocked.  If it has 
					 * been unlocked, dequeue the calling 
					 * thread and loop * again.  If not, 
					 * subsequent unlocking operations 
					 * will find the thread on the queue
         				 * so there is no risk of the calling 
					 * thread being stranded.
         				 */
						_thrq_rem_from_q(t);
						t->t_sync_addr = NULL;
						t->t_sync_type = TL_NONE;
						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						break;
					}
					UNLOCK_MUTEX_SYNCLOCK(mutex);
					t->t_state = TS_SLEEP;
					_thr_swtch(1, t);
					/*
					 * we must enable signal handlers to
					 * ensure we handle signals that arrive
					 * while we're blocked
					 */
					_thr_sigon(t);
					_thr_sigoff(t);
				} /* end while */
			} /* end else */
		} else {
			/* got the lock on first try */
			return(ret);
		}
		break;
       	case USYNC_PROCESS:
		if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
			ret=_thr_lwp_mutex_lock_sigoff(&(mutex->m_lmutex), t);
		} else { /* else, we got the lock and can return */
			return(ret);
		}
               	break;
       	default:
		ret=EINVAL;
		return(ret);
	}
	return(ret);
}


/*
 * int _thr_lwp_mutex_lock_sigoff(lwp_mutex_t *lmp, thread_desc_t *curtp)
 *      Acquire an LWP mutex pointed to by lmp; assume signal handlers
 *	are disabled upon entry and leave them disabled on return but
 *	allow signals to interrupt the thread if it blocks.
 *
 *	Note: this is simply _lwp_mutex_lock() with changes to accommodate
 *	the signal handling requirements for threads!!
 *
 * Parameter/Calling State:
 *	No locks held on entry.
 *
 *	first argument is a pointer to an LWP mutex.
 *
 *	second argument is a pointer to the calling thread descriptor
 *
 * Return Values/Exit State:
 *      0:      the LWP mutex was acquired.
 *              The LWP mutex pointed to by lmp is in locked state.
 *      EFAULT or SIGSEGV:
 *              lmp points outside the process's allocated address space.
 */
int
_thr_lwp_mutex_lock_sigoff(lwp_mutex_t *lmp, thread_desc_t *curtp)
{
        int rval;
loop:
	ASSERT(THR_ISSIGOFF(curtp));
        /*
         * Attempt to acquire the mutex.
         */
        if (_lock_try(&lmp->lock)) {
                /*
                 * We got the mutex.
                 */
                return (0);
        }

        /*
         * We didn't get the mutex, re-enable signal handlers
	 * and call prepblock to place the calling LWP
         * on the sync-queue associated with lmp.
         * Prepblock will set lmp->wanted.
         */
	_thr_sigon(curtp);
        while ((rval = prepblock((vaddr_t)lmp, (char *)&lmp->wanted,
                                 PREPBLOCK_WRITER)) == EINVAL) {
                /*
                 * The calling LWP is placed on another sync-queue.
                 * Remove it from the sync-queue and prepblock again.
                 */
                cancelblock();
                continue;
        }
        if (rval != 0) {
		_thr_sigoff(curtp);
                return (rval); /* EFAULT */
        }

        /*
         * Attempt to acquire the mutex which may be released in the meantime.
         * This attempt closes the window between _lock_try and prepblock.
         */
        if (_lock_try(&lmp->lock)) {
                /*
                 * We got the mutex.
                 * Remove the calling LWP from the sync-queue, disable
		 * signal handlers, and return.
                 */
                cancelblock();
		_thr_sigoff(curtp);
                return (0);
        }

        /*
         * Give up the processor and wait for the mutex to be released.
         */
        while ((rval = block((const timestruc_t *)0)) != 0) {
                switch (rval) {
                case EINTR:
                case ERESTART:
                        /*
                         * Block was interrupted by a signal or a forkall.
                         * The calling thread may be placed back on the
                         * sync-queue after handling the signal.  Call 
			 * block again.
                         */
                        continue;
                case ENOENT:
                        /*
                         * Prepblock was canceled by a signal handler or a
                         * forkall, so that the calling LWP was removed from
                         * the sync-queue.  Try again.
                         */
                        goto loop;
                default:
                        break;
                }
		_thr_sigoff(curtp);
                return (rval);
        }

        /*
         * Try to acquire the mutex again.
         */
	_thr_sigoff(curtp);
        goto loop;
}



/*
 * The following routines are identical to the external mutex interfaces
 * and are provided to allow the library to obtain the functionality of
 * those interfaces without recording trace data.  These routines must
 * be kept in sync with the external interfaces, i.e., _thr_notrace_mutex_init
 * must remain identical to mutex_init except for the calls to TRACE.
 */

#ifdef TRACE
/*
 * _thr_notrace_mutex_init(mutex_t *mutex, int type, void *arg)
 *	initializes a mutex to the specified type
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	first argument is a pointer to a mutex
 *	second argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *	third argument is not currently used
 *
 *	no locks are acquired during processing
 *
 * Return Values/Exit State:
 *	Returns 0 on success or EINVAL if type is invalid.
 *	On success, mutex is initialized to the specified type.
 */

/* ARGSUSED2 */
int
_thr_notrace_mutex_init(mutex_t *mutex, int type, void *arg)
{
	static lwp_mutex_t lminitval;

        switch (type) {
            case USYNC_THREAD: 
		mutex->m_type = USYNC_THREAD;
		break;
            case USYNC_PROCESS:
		mutex->m_type = USYNC_PROCESS;
		break;
            default:
		return(EINVAL);
	}
	mutex->m_lmutex = lminitval;
	mutex->m_sync_lock = lminitval;
	THRQ_INIT(&(mutex->m_sleepq));
	return(0);
}



/*
 * _thr_notrace_mutex_lock(mutex_t *mutex)
 *	obtains the specified mutex lock, blocking if necessary
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	The lock of the calling thread and the sync_lock of the
 *	desired mutex are acquired during processing; also the
 *	desired mutex is locked.
 *
 * Return Values/Exit State:
 *	Returns zero and acquires the specified mutex on success; 
 *	otherwise, returns an appropriate errno value.
 */

int
_thr_notrace_mutex_lock(mutex_t *mutex)
{
	int ret=0;
	thread_desc_t *t;

	switch (mutex->m_type) {
	case USYNC_THREAD:
		if (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex) != 0)  {
		/* didn't get the lock */
			t = curthread;
			if (ISBOUND(t)) {
				ret = _lwp_mutex_lock(&(mutex->m_lmutex));
			} else {
				/* thread is multiplexed */
				while (LWP_MUTEX_TRYLOCK(&mutex->m_lmutex)!= 0){
					/* multiplexed thread loop start */
					_thr_sigoff(t);
					LOCK_THREAD(t);
					LOCK_MUTEX_SYNCLOCK(mutex);
					if (mutex->m_type != USYNC_THREAD) {
        				/*
         				 * the mutex may have been destroyed 
					 * via mutex_destroy() before the
         				 * synclock was acquired; return
					 * EINVAL if this happened.
         				 */

						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						ret = EINVAL;
						_thr_sigon(t);
						break;
					}
					t->t_sync_addr = (void *)mutex;
					t->t_sync_type = TL_MUTEX;
					_thrq_prio_ins((thrq_elt_t *)
					(&(mutex->m_sleepq)), t);
					/*
				 	 * WARNING: assumes STORE-ORDERED memory
				 	 */
					if (LWP_MUTEX_TRYLOCK
					(&mutex->m_lmutex) == 0) {
        				/*
         				 * the mutex may have been unlocked 
					 * while enqueuing the calling thread;
         				 * now that the calling thread is on
					 * the sleep queue, one last check is 
					 * necessary to make sure the mutex
         				 * hasn't been unlocked.  If it has 
					 * been unlocked, dequeue the calling 
					 * thread and loop * again.  If not, 
					 * subsequent unlocking operations 
					 * will find the thread on the queue
         				 * so there is no risk of the calling 
					 * thread being stranded.
         				 */
						_thrq_rem_from_q(t);
						t->t_sync_addr = NULL;
						t->t_sync_type = TL_NONE;
						UNLOCK_MUTEX_SYNCLOCK(mutex);
						UNLOCK_THREAD(t);
						_thr_sigon(t);
						break;
					}
					UNLOCK_MUTEX_SYNCLOCK(mutex);
					t->t_state = TS_SLEEP;
					_thr_swtch(1, t);
					_thr_sigon(t);
				} /* end while */
			} /* end else */
		} else {
			/* got the lock on first try */
			return(ret);
		}
		break;
       	case USYNC_PROCESS:
		if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
			ret=_lwp_mutex_lock(&(mutex->m_lmutex));
		} else { /* else, we got the lock and can return */
			return(ret);
		}
               	break;
       	default:
		ret=EINVAL;
		return(ret);
	}
	return(ret);
}



/*
 * _thr_notrace_mutex_trylock(mutex_t *mutex)
 *	makes a single attempt to obtain a mutex
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	Returns zero if the lock was available and acquires the
 *	lock; otherwise returns EBUSY and doesn't acquire the lock.
 */

int
_thr_notrace_mutex_trylock(mutex_t *mutex)
{
	if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
		return(EBUSY);
	}

	return(0);
}



/*
 * _thr_notrace_mutex_unlock(mutex_t *mutex)
 *	unlocks a mutex and awakens a waiting thread if one exists.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	During processing, the sync_lock of the mutex may be acquired and
 *	the thread lock of a thread that is awakened may be acquired.
 *
 * Return Values/Exit State:
 *	Returns 0 on success; otherwise, an errno value.  On success, if 
 *	a thread was waiting for the mutex, exactly one thread will be 
 *	awakened to try to obtain the mutex.
 */

int
_thr_notrace_mutex_unlock(mutex_t *mutex)
{
	int rval, needlwp;

        switch (mutex->m_type) {
            case USYNC_THREAD:
		/* first, clear the lock */
		_lock_clear(&(mutex->m_lmutex.lock));
		/*
		 * WARNING: assumes STORE-ORDERED memory
		 */
	        /*
	         * Next, we check if a bound thread is waiting for the mutex
	         * because bound threads are favored over multiplexed threads.
	         * mutex->m_lmutex.wanted is set if a bound thread may be
	         * waiting for the mutex; if so call unblock().  If
	         * unblock() doesn't return EINVAL, it means an LWP was found
	         * blocked in the kernel and we can return.  If unblock()
	         * returns EINVAL or mutex->m_lmutex.wanted wasn't set, no LWP
	         * (and so no bound thread) was found waiting for the mutex so
	         * we need to check if a multiplexed thread is waiting.
	         */
		if (mutex->m_lmutex.wanted) {
        		rval = unblock((vaddr_t)&mutex->m_lmutex, 
				(char *)&(mutex->m_lmutex.wanted), UNBLOCK_ANY);
			if (rval != EINVAL) {
				break;
			}
		}
	        /*
	         * If we get here, it's because no bound thread was found
	         * waiting for the mutex so we want to see if there's a
	         * multiplexed thread waiting for the mutex.
	         *
	         * First, set rval to 0 because it may have been set to EINVAL
	         * in the above code.
	         */
		rval = 0;
		if (!(THRQ_ISEMPTY((&mutex->m_sleepq)))) {
			thread_desc_t *temp;
			thread_desc_t *curtp = curthread;

			_thr_sigoff(curtp);
			LOCK_MUTEX_SYNCLOCK(mutex);
			temp = _thrq_rem_first((thrq_elt_t *)&mutex->m_sleepq);
			UNLOCK_MUTEX_SYNCLOCK(mutex);
			if (temp != NULL) { 
		        /*
		         * If temp in non-null, we found a thread on the 
			 * mutex sleep queue.  In that case, make it 
			 * runnable and activate an LWP for it.  It's 
			 * possible that temp can be null because the
		         * last thread may have been removed before we 
			 * acquired the mutex sync lock.
		         */
				int prio;
				LOCK_THREAD(temp);
				if (THRQ_ISEMPTY(&temp->t_thrq_elt)) {
					temp->t_sync_addr = NULL;
					temp->t_sync_type = TL_NONE;
					needlwp = _thr_setrq(temp, 0);
				} else {
					needlwp = INVALID_PRIO;
				}
				UNLOCK_THREAD(temp);
				if (needlwp != INVALID_PRIO) {
					_thr_activate_lwp(needlwp);
				}
			}
			_thr_sigon(curtp);
		}
		break;
            case USYNC_PROCESS:
		rval=_lwp_mutex_unlock(&mutex->m_lmutex);
                break;
            default:
		rval=EINVAL;
	}
	return(rval);
}



/*
 * _thr_notrace_mutex_destroy(mutex_t *mutex)
 *	destroys a mutex
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a mutex.
 *
 *	During processing, the mutex lock may be acquired and the sync_lock
 *	of the mutex may also be acquired.
 *
 * Return Values/Exit State:
 *	Returns zero and leaves the mutex in an invalid state on success; 
 *	otherwise an errno value.
 */

int
_thr_notrace_mutex_destroy(mutex_t *mutex)
{
        switch (mutex->m_type) {
            case USYNC_THREAD:
            case USYNC_PROCESS:
		/* First, make sure nobody holds the mutex */
		if (LWP_MUTEX_TRYLOCK(&(mutex->m_lmutex)) != 0) {
			return(EBUSY); /* mutex is held */
		}
        	/*
        	 * Now make sure that no thread is waiting for the mutex.
        	 * This is necessary because we may have grabbed the mutex
        	 * while other threads were blocked waiting for it.  This
        	 * is a consequence of the way we unlock mutexes.
        	 */
		LOCK_MUTEX_SYNCLOCK(mutex);
		if (mutex->m_lmutex.wanted || 
					!THRQ_ISEMPTY((&mutex->m_sleepq))) {
			UNLOCK_MUTEX_SYNCLOCK(mutex);
			_THR_MUTEX_UNLOCK(mutex);
			return(EBUSY);  /* threads are waiting */
		}
		mutex->m_type = USYNC_DESTROYED;
		UNLOCK_MUTEX_SYNCLOCK(mutex);
	        /*
	         * We return with the mutex locked.  This ensures that anyone
	         * who tries to acquire the mutex via mutex_lock() will go
	         * through the appropriate code and find that the type of
	         * the mutex is neither USYNC_PROCESS nor USYNC_THREAD.
	         */
		return(0);
            default:
		return(EINVAL);
	}
}

#endif /* TRACE */
