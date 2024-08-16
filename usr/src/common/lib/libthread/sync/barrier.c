/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/barrier.c	1.6"

#include <libthread.h>
#include <trace.h>

/*
 * int barrier_init(barrier_t *barrier, int count, int type, void *arg)
 *	Initialize a synchronization barrier.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
 *      argument 'count' is count of number of threads
 *      argument 'type' is either USYNC_THREAD or USYNC_PROCESS
 *      argument 'arg' is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The barrier is initialized to the
 *		type specified as the 'type' argument. 
 *	EINVAL: The value of 'type' is neither USYNC_THREAD nor USYNC_PROCESS,
 *		or the value of count is negative.
 *
 *	On exit no locks are held.
 */
/* ARGSUSED */
int
barrier_init(barrier_t *barrier, int count, int type, void *arg)
{
	int rval;

	if (type != USYNC_THREAD && type != USYNC_PROCESS) {
		TRACE5(0, TR_CAT_BARRIER, TR_EV_BINIT, TR_CALL_ONLY,
		   barrier, count, type, arg, EINVAL);
		return(EINVAL);
	}
	if (count < 0) {
		TRACE5(0, TR_CAT_BARRIER, TR_EV_BINIT, TR_CALL_ONLY,
		   barrier, count, type, arg, EINVAL);
		return(EINVAL);
	}
	if (rval = _THR_MUTEX_INIT(&barrier->b_lock, type, NULL)) {
		TRACE5(0, TR_CAT_BARRIER, TR_EV_BINIT, TR_CALL_ONLY,
		   barrier, count, type, arg, rval);
		return(rval);
	}
	if (rval = _THR_COND_INIT(&barrier->b_cond, type, NULL)) {
		TRACE5(0, TR_CAT_BARRIER, TR_EV_BINIT, TR_CALL_ONLY,
		   barrier, count, type, arg, rval);
		return(rval);
	}

	barrier->b_type = type;
	barrier->b_count = count;
	barrier->b_waiting = 0;
	barrier->b_generation = 0;

	TRACE5(0, TR_CAT_BARRIER, TR_EV_BINIT, TR_CALL_ONLY,
	   barrier, count, type, arg, 0);

	return(0);
}


/*
 * int _spin_barrier_init(barrier_t *barrier, int count, void *arg)
 *	Initialize a synchronization spin barrier.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
 *      argument 'count' is count of number of threads
 *      argument 'arg' is not currently used
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The barrier is initialized.
 *	EINVAL: The value of 'count' is negative.
 *
 *	On exit no locks are held.
 */
/* ARGSUSED */
int
_barrier_spin_init(barrier_spin_t *barrier, int count, void *arg)
{
	int rval;

	if (count < 0) {
		TRACE4(0, TR_CAT_BARRIER_SPIN, TR_EV_BSINIT, TR_CALL_ONLY,
		   barrier, count, arg, EINVAL);
		return(EINVAL);
	}
	rval = _THR_SPIN_INIT(&barrier->bs_lock, NULL);
	if (rval == 0) {
		/* _spin_init succeeded */
		barrier->bs_count = count;
		barrier->bs_waiting = 0;
		barrier->bs_generation = 0;
		barrier->bs_type = USYNC_THREAD;
	}
	TRACE4(0, TR_CAT_BARRIER_SPIN, TR_EV_BSINIT, TR_CALL_ONLY,
	   barrier, count, arg, rval);
	return(rval);
}


/*
 * int barrier_destroy(barrier_t *barrier);
 *	Invalidate a synchronization barrier.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
 *
 *	During processing the internal synchronization locks of the
 *	condition variable and mutex are acquired.
 *
 * Return Values/Exit State:
 *	0:	The barrier is invalidated and can no longer
 *		be used for synchronization.
 *	EBUSY:  The barrier is still busy.
 *	EINVAL: The barrier is invalid.
 *
 *	On exit no locks are held.
 */

int
barrier_destroy(barrier_t *barrier)
{
	if (_THR_MUTEX_LOCK(&barrier->b_lock) == EINVAL) {
		/* barrier uninitialized or already destroyed */
		TRACE2(0, TR_CAT_BARRIER, TR_EV_BDESTROY, TR_CALL_ONLY,
		   barrier, EINVAL);
		return(EINVAL);
	}
	if (barrier->b_type != USYNC_THREAD && 
	    barrier->b_type != USYNC_PROCESS) {
		(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
		(void)_THR_MUTEX_DESTROY(&barrier->b_lock);
		TRACE2(0, TR_CAT_BARRIER, TR_EV_BDESTROY, TR_CALL_ONLY,
		   barrier, EINVAL);
		return(EINVAL);
	}
	if (barrier->b_waiting) {
		/* threads are waiting at the barrier */
		(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
		TRACE2(0, TR_CAT_BARRIER, TR_EV_BDESTROY, TR_CALL_ONLY,
		   barrier, EBUSY);
		return(EBUSY);
	}
	barrier->b_type = USYNC_DESTROYED;
	(void)_THR_COND_DESTROY(&barrier->b_cond);
	(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
	(void)_THR_MUTEX_DESTROY(&barrier->b_lock);
	TRACE2(0, TR_CAT_BARRIER, TR_EV_BDESTROY, TR_CALL_ONLY,
	   barrier, 0);
	return(0);
}


/*
 * int _spin_barrier_destroy(barrier_t *barrier)
 *	Invalidate a synchronization spin barrier.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
 *
 *	During processing the internal synchronization lock of the
 *	mutex is acquired.
 *
 * Return Values/Exit State:
 *	0:	The barrier is invalidated and can no longer
 *		be used for synchronization.
 *	EBUSY:  The barrier is still busy.
 *	EINVAL: The barrier is invalid.
 *
 *	On exit no locks are held.
 */

int
_barrier_spin_destroy(barrier_spin_t *barrier)
{
	int rval;

	rval = _THR_SPIN_TRYLOCK(&barrier->bs_lock);
	if (rval != 0) {
		/* spin lock busy or already destroyed */
		TRACE2(0, TR_CAT_BARRIER_SPIN, TR_EV_BSDESTROY, TR_CALL_ONLY,
		   barrier, rval);
		return(rval);
	}
	if (barrier->bs_type != USYNC_THREAD) {
		/*
		 * barrier was already destroyed 
		 */
		_THR_SPIN_UNLOCK(&barrier->bs_lock);
		TRACE2(0, TR_CAT_BARRIER_SPIN, TR_EV_BSDESTROY, TR_CALL_ONLY,
		   barrier, EINVAL);
		return(EINVAL);
	}
	if (barrier->bs_waiting) {
		 /* threads are waiting at the barrier */
		_THR_SPIN_UNLOCK(&barrier->bs_lock);
		TRACE2(0, TR_CAT_BARRIER_SPIN, TR_EV_BSDESTROY, TR_CALL_ONLY,
		   barrier, EBUSY);
		return(EBUSY);
	}
	barrier->bs_type = USYNC_DESTROYED;
	_THR_SPIN_UNLOCK(&barrier->bs_lock);
	_THR_SPIN_DESTROY(&barrier->bs_lock);
	TRACE2(0, TR_CAT_BARRIER_SPIN, TR_EV_BSDESTROY, TR_CALL_ONLY,
	   barrier, 0);
	return(0);
}


/*
 * int barrier_wait(barrier_t *barrier);
 * 	Block the calling thread if some threads have not arrived.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
 *
 *	During processing the mutex protecting the barrier count is
 *	acquired. In addition the condition variable synchronization lock
 *	and the thread lock of the current thread may be acquired
 *	indirectly via the call to cond_wait.
 *
 * Return Values/Exit State:
 *	0:	all threads have arrived at the barrier.
 *	EINVAL: The barrier was invalidated by barrier_destroy
 * 		and the barrier is no longer valid.
 *
 *	On exit no locks are held.
 */

int
barrier_wait(barrier_t *barrier)
{
	int rval;
	int my_generation;
	thread_desc_t *curtp = curthread;
	boolean_t done = B_FALSE;
	TRACE2(0, TR_CAT_BARRIER, TR_EV_BWAIT, TR_CALL_FIRST,
	   barrier, barrier->b_count);

	_thr_sigoff(curtp);
	if (rval = _thr_mutex_lock_sigoff(&barrier->b_lock, curtp)) {
		/* barrier uninitialized or destroyed */
		_thr_sigon(curtp);
		TRACE1(0, TR_CAT_BARRIER, TR_EV_BWAIT, TR_CALL_SECOND, rval);
		return(rval);
	}
	if (barrier->b_type != USYNC_THREAD && 
	    barrier->b_type != USYNC_PROCESS) {
		(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
		(void)_THR_MUTEX_DESTROY(&barrier->b_lock);
		_thr_sigon(curtp);
		TRACE1(0, TR_CAT_BARRIER, TR_EV_BWAIT, TR_CALL_SECOND, 
		   EINVAL);
		return(EINVAL);
	}
	if (barrier->b_count < 0 ) {
		_thr_sigon(curtp);
		TRACE1(0, TR_CAT_BARRIER, TR_EV_BWAIT, TR_CALL_SECOND, 
		   EINVAL);
		return(EINVAL);
	}

	barrier->b_waiting++;
	/*
 	 * we test ">" below in case barrier is statically init'ed to all 0
	 */
	if (barrier->b_waiting >= barrier->b_count) {
		/*
		 * This is the last thread; reset the waiting count to
		 * zero, increment the generation count, notify the
		 * waiters, and return.
		 */
		barrier->b_waiting = 0;
		barrier->b_generation++;
		(void)_THR_COND_BROADCAST(&barrier->b_cond);
		(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
		_thr_sigon(curtp);
	} else {
		/*
		 * This is not the last thread; save the current value
		 * of the generation count and wait on the condition.
		 */
		my_generation = barrier->b_generation;
		_thr_sigon(curtp);
		while (done == B_FALSE) {
			rval = _thr_cond_wait(&barrier->b_cond, 
					&barrier->b_lock, NULL,
					(const timestruc_t *)NULL, B_FALSE,
					B_FALSE);
			if (barrier->b_generation != my_generation) {
				done = B_TRUE;
			} else {
				(void)_THR_MUTEX_LOCK(&barrier->b_lock);
				/*
				 * must check generation again since it
				 * may have changed while we were waiting
				 * for the mutex
				 */
				if (barrier->b_generation != my_generation) {
					(void)_THR_MUTEX_UNLOCK(&barrier->b_lock);
					done = B_TRUE;
				}
			}
		}
		/*
		 * we come here only if last thread has arrived
		 * we assume cond_wait() returned 0 (ie not even EINTR) here
		 */
	}
	TRACE1(0, TR_CAT_BARRIER, TR_EV_BWAIT, TR_CALL_SECOND, rval);
	return(rval);
}

/*
 * int _spin_barrier_wait(barrier_t *barrier);
 * 	Spin the calling thread if some threads have not arrived
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'barrier' is a pointer to a barrier
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
_barrier_spin(barrier_spin_t *barrier)
{
	int my_generation;
	TRACE2(0, TR_CAT_BARRIER_SPIN, TR_EV_BSWAIT, TR_CALL_FIRST,
	   barrier, barrier->bs_count);

	if (barrier->bs_type != USYNC_THREAD) { /* not a valid barrier */
		TRACE1(0, TR_CAT_BARRIER_SPIN, TR_EV_BSWAIT, TR_CALL_SECOND,
		   EINVAL);
		return(EINVAL);
	}

	_THR_SPIN_LOCK(&barrier->bs_lock);

	if (barrier->bs_count < 0) {
		TRACE1(0, TR_CAT_BARRIER_SPIN, TR_EV_BSWAIT, TR_CALL_SECOND,
		   EINVAL);
		return(EINVAL);
	}

	barrier->bs_waiting++;
	if (barrier->bs_waiting >= barrier->bs_count) {
		/*
		 * This is the last thread; reset the waiting count 
		 * to zero, increment the generation count, and return.
		 */
		barrier->bs_waiting = 0;
		barrier->bs_generation++;
		_THR_SPIN_UNLOCK(&barrier->bs_lock);
	} else {
		/*
		 * This is not the last thread; spin until the generation
		 * count is incremented.
		 */
		my_generation = barrier->bs_generation;
		_THR_SPIN_UNLOCK(&barrier->bs_lock);
		while (barrier->bs_generation == my_generation)
			/* spin */;
	}
	TRACE1(0, TR_CAT_BARRIER_SPIN, TR_EV_BSWAIT, TR_CALL_SECOND, 0);
	return(0);
}
