/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/lwp.c	1.4.8.8"
#include <libthread.h>
#include <trace.h>
#include <sys/lwp.h>

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * This file contains _thr_new_lwp() which obtains a new LWP
 * and the interfaces thr_setconcurrency(), which changes the
 * application-requested concurrency, and thr_getconcurrency(),
 * which returns the application-requested concurrency.
 */


#define GET_FUNC_START_ADDR(func) ((int)func - 8)



/*
 * int
 * _thr_new_lwp(thread_desc_t *t, void *(*func)(), void *arg)
 *	used by thread library to create more LWPs.  If the LWP is
 *	to be added to the multiplexing pool, an idle thread is
 *	also created to provide it with a context.  Otherwise, the
 *	LWP will be associated with a bound thread and will use
 *	that thread's context.
 *
 * Parameter/Calling State:
 *	Signal handlers are disabled upon entry.
 *
 *	first argument is a pointer to a thread
 *	second argument is the address of the function the new LWP will run
 *	third argument is a pointer to the argument to be passed to the 
 *	new function as	an argument
 *
 *	During processing, _thr_preemptlock is acquired.  Also, 
 *	_thr_lwpprivatelock may be acquired indirectly via a call 
 *	to _thr_lwpcreate().
 *
 * Return Values/Exit State:
 *	Returns 0 on success, non-zero on failure.  On exit, signal 
 *	handlers are still disabled.
 */

int
_thr_new_lwp(thread_desc_t *t, void *(*func)(), void *arg)
{
	int nlwpflag = 0; /* set if making a multiplexing LWP */
	int error;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(func != NULL);

	if (t == NULL) {
		/*
		 * If t is NULL, the LWP is a multiplexing LWP to be added
		 * to the pool and it needs an idle thread, which we create
		 * here and assign its address to t.
		 * If t is not NULL, the LWP will be associated with a bound
		 * thread; in this case, t points at the thread.
		 */
		if ((t = _thr_idle_thread_create()) == NULL)
			return (EAGAIN);
		nlwpflag = 1;
	}
	error = _thr_lwpcreate(t, (long)(GET_FUNC_START_ADDR(thr_exit)), 
	   (caddr_t)t->t_sp, func, arg, LWP_SUSPENDED);
	if (error != 0) { /* LWP creation failed */
		if (nlwpflag) { /* clean up idle thread's stack */
			ASSERT(t->t_flags & T_ALLOCSTK);
			_thr_free_stack(t->t_stk, (int)t->t_stksize);
		}
		return(error);
	} else if (nlwpflag) {
		/*
        	 * Because this is a multiplexing LWP, we have to increment
        	 * _thr_nlwps and place the idle thread on the list of
        	 * idle threads.
		 *
		 * Note that _thr_idle_thread_create() doesn't place the
		 * new idle thread on the list.  It is done here in order
		 * to save a locking round trip for _thr_preemptlock.
		 */
 		LOCK_THREAD(t);
		LOCK_PREEMPTLOCK;
		_thr_nlwps++;
		_thr_enq_thread_to(&_thr_idlethreads, t);
		/*
		 * The thread lock of t need not be held here because t is
		 * an idle thread not yet known to the library.  Therefore,
		 * no thread can change t's thread structure and there is
		 * no need to hold the lock that protects that structure.
		 * However, we acquire the lock here in order to be able
		 * to provide an ASSERT in _thr_enq_thread_to() that the
		 * thread lock is held; this provides more confidence that
		 * locking protocols in the library are being observed.
		 * The cost of this locking round trip is not great; it is
		 * performed only when an idle thread is created (i.e., when
		 * a multiplexing LWP is added to the pool) and there will
		 * never be contention for the thread lock in these cases.
		 */
		UNLOCK_PREEMPTLOCK;
 		UNLOCK_THREAD(t);
	}

#ifdef TRACE
	/* open a trace file for the new LWP if trace is enabled */
        _thr_open_tracefile(t);
#endif /* TRACE */

	/*
         * If t is an idle thread, its state will be TS_RUNNABLE and 
         * the LWP must be continued.  Otherwise, t is a bound thread
         * and will be continued via thr_create() or thr_continue().
	 */
	if (t->t_state == TS_RUNNABLE)
		_lwp_continue(LWPID(t));
	return (0);
}


/*
 * int
 * _thr_getconcurrency(void)
 *      returns the application requested concurrency.
 *
 * Parameter/Calling State:
 *
 *      No locks are held on entry -- user interface.
 *
 *      During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *      Returns the application requested concurrency which is the sum
 *      of the value requested by the last call to thr_setconcurrency()
 *      (or zero if there has been no call) added to the number of
 *      thr_create()s performed with the flag THR_NEW_LWP since the
 *      last call to thr_setconcurrency().
 */

int
thr_getconcurrency(void)
{
	TRACE1(0, TR_CAT_THREAD, TR_EV_THR_GETCONC, TR_CALL_ONLY, 
	    _thr_minlwps);
	return(_thr_minlwps);
}


/*
 * int
 * _thr_setconcurrency(int new_level)
 *      adjusts the application requested concurrency.
 *
 * Parameter/Calling State:
 *
 *      No locks are held on entry -- user interface.
 *
 *      During processing, _thr_counterlock is acquired.
 *
 * Return Values/Exit State:
 *      On success, this interface returns zero and sets the
 *      application requested concurrency to the specified
 *      value and take steps to adjust the real concurrency
 *      to the requested value.
 *      If an invalid argument is passed, EINVAL is returned
 *      and no change is made to concurrency.
 */

int
thr_setconcurrency(int new_level)
{
	thread_desc_t *tp = curthread;
	int wanted;

	if (new_level < 0) {
		TRACE2(tp, TR_CAT_THREAD, TR_EV_THR_SETCONC, TR_CALL_ONLY, 
		   new_level, EINVAL);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	LOCK_COUNTER;
	_thr_minlwps = new_level;
	UNLOCK_COUNTER;
	LOCK_RUNQ;
	if (new_level == _thr_nlwps) {
		/*
		 * We don't need to create or remove any LWPs but we
		 * set _thr_dontkill_lwp to exempt currently aging LWPs
		 * from terminating; this protects against the case
		 * where all the aging LWPs time out immediately after
		 * the call to thr_setconcurrency().
		 */
		_thr_dontkill_lwp = _thr_nage;
		UNLOCK_RUNQ;
		TRACE2(tp, TR_CAT_THREAD, TR_EV_THR_SETCONC, TR_CALL_ONLY, 
		   new_level, 0);
		_thr_sigon(tp);
		return(0);
	}
	if (new_level < _thr_nlwps) {
		/*
		 * The requested level is lower than the actual level.
		 * Adjusting concurrency downward is done in extremely
		 * conservative fashion.  LWPs will be terminated only
		 * after they have been idle for sufficiently long time,
		 * even though the application has indicated that these
		 * LWPs are not needed.
		 *
		 * In this case we set _thr_dontkill_lwp so that if all 
		 * aging LWPs timed out immediately after this function 
		 * returned, there would still be at least new_level 
		 * multiplexing LWPs left in the process.
		 */
		_thr_dontkill_lwp = 
		   MAX((new_level - (_thr_nlwps - _thr_nage)), 0);
		UNLOCK_RUNQ;
	} else {
		/*
		 * The requested level is higher than the actual level.
		 * Adjusting concurrency upward is also done in a rather
		 * conservative fashion.  LWPs are not created beyond the
		 * point where the number of LWPs exceeds the number of
		 * multiplexed threads.
		 *
		 * Adjusting concurrency upward is done via the housekeeping
		 * thread.  This ensures that the newly created LWPs share
		 * certain characteristics because they inherit them from
		 * another pooled LWP (since the housekeeping thread is
		 * multiplexed).  In addition, this allows a faster return
		 * from this function.
		 *
		 * The one drawback this approach has is that we can't
		 * detect if LWP creation fails (e.g., if the user limit
		 * on LWPs has been reached).
		 *
		 * In this case we set _thr_dontkill_lwp so that currently 
		 * aging LWPs will not be terminated if they time out.
		 */
		_thr_dontkill_lwp = _thr_nage;
		wanted = (MIN(new_level, _thr_nthreads)) - _thr_nlwps;
		if (wanted < 0) {
			wanted = 0;
		}
		_thr_lwpswanted = wanted;
		UNLOCK_RUNQ;
		if (wanted > 0) {
			_thr_wakehousekeeper();
		}
	}
	TRACE2(tp, TR_CAT_THREAD, TR_EV_THR_SETCONC, TR_CALL_ONLY, 
	   new_level, 0);
	_thr_sigon(tp);
	return(0);
}
