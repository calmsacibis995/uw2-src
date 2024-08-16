/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/thread.c	1.10.23.44"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * This file contains functions used to create and control threads, including:
 *
 * thr_create
 * thr_minstack
 * thr_self
 * thr_continue
 * thr_exit
 * _thr_idle_thread_create - stub
 * _thr_reaper
 * _thr_put_on_reaplist
 * _thr_wakehousekeeper
 * _thr_housekeeper
 * _thr_activate_lwp
 * _thr_prioq_init
 * _thr_getminpri
 * _thr_sigwaiting_disable
 * _thr_sigwaiting_enable
 * _thr_dumpreaplist
 * _thr_dumpjoinlist
 * _thr_dump_thread
 * _thr_dump_allthreads
 */

#include <sys/types.h>
#include <libthread.h>
#include <sys/usync.h>
#include <stdlib.h>
#include <stdio.h>
#include <trace.h>
#include <tls.h>
#include <values.h>
#include <memory.h>

extern void _thr_binding_print(struct _thr_binding *);

/*
 * _thr_allthreads is a doubly-linked list of all non-zombie non-idle
 * threads in the process.
 */
thread_desc_t *_thr_allthreads = NULL;

/*
 * If non-NULL, points to the housekeeping thread. This thread is responsible
 * for creating multiplexed LWP's and reaping exited threads.
 */
thread_desc_t *_thr_housethread = NULL;

/*
 * _thr_daemonthreads is a count of all threads created explicitly by
 * the application with the flag THR_DAEMON.
 */
extern int _thr_daemonthreads;

/*
 * _thr_counterlock is an LWP mutex lock that must be held by a thread
 * before it changes most global counters or global lists.
 */
lwp_mutex_t _thr_counterlock;

/*
 * _thr_runnableqlock is an LWP mutex lock that must be held by a thread
 * before it changes the runnable queue.
 */
lwp_mutex_t _thr_runnableqlock;

/*
 * _thr_house_sema is a semaphore that the housekeeping thread sleeps 
 * on when it is inactive.
 */
sema_t _thr_house_sema;

/*
 * _thr_zombied is a thread level condition variable on which threads
 * sleep while waiting to join with a random thread (i.e., the first
 * argument to thr_join is 0).  It is protected by _thr_counterlock and,
 * therefore, must be waited on via the _thr_condwait_lwpmutex() function.
 */
cond_t _thr_zombied;

/*
 * _thr_zombie_waiters is a count of threads waiting on _thr_zombied.
 * It is used to determine whether cond_broadcast()s are needed to
 * wake these threads.  It is used to improve performance of any
 * functions that perform the cond_broadcast()s.  It is local to this file.
 */
int _thr_zombie_waiters = 0;

/*
 * _thr_lwpswanted is a counter that indicates how many multiplexing
 * LWPs need to be created by the housekeeping thread.
 */
int _thr_lwpswanted;

/*
 * _thr_joinable is a counter that indicates how many unjoined
 * joinable threads currently exist in the process.
 */
int _thr_joinable = 1;

/*
 *  _thr_reapable is a count of unreaped detached threads that have exited.
 *  int _thr_reapable; is already defined in disp.c
 */

/*
 *  _thr_sigwaitingset is a flag that indicates if the SIGWAITING
 * signal handler is enabled.
 */
boolean_t _thr_sigwaitingset;

/*
 * _thr_idlethreads is a linked list of idle threads currently
 * in the process.
 */
thread_desc_t *_thr_idlethreads;

/*
 * _thr_reaplist is a linked list of threads waiting to have their
 * resources reaped.
 */
thread_desc_t *_thr_reaplist;

/*
 * _thr_joinablelist is a linked list of joinable threads that have
 * exited.
 */
thread_desc_t *_thr_joinablelist;

/*
 * _thr_lazyq is a linked list of multiplexed threads that are in either
 * the TS_SLEEP or TS_SUSPENDED state and blocked on their LWPs.
 */
thrq_elt_t _thr_lazyq;

/*
 * _thr_userpricnt indicates how many different thread priorities currently
 * exist in the process for application-created multiplexed threads.
 * (This doesn't include the housekeeping thread.)
 */
int _thr_userpricnt = 1;

/*
 * _thr_userminpri indicates the lowest priority value of an application-
 * created thread currently in the process.
 */
int _thr_userminpri = DEFAULTMUXPRI;

/*
 * _thr_preempt_off indicates whether preemption is to be used in the process.
 * Preemption is enabled by default and may be disabled via a call to
 * _thr_preempt_disable().
 */
boolean_t _thr_preempt_off = B_FALSE;

/*
 * _thr_preempt_ok indicates whether conditions are right for preemption.
 * This occurs when:
 *	_thr_preempt_off == B_FALSE
 *	and either:
 *		_thr_userpricnt > 1, or
 *		_thr_userminpri == THREAD_MAX_PRIORITY
 */
boolean_t _thr_preempt_ok = B_FALSE;

/*
 * entries in _thr_userpriarray[] indicate the number of threads whose
 * priorities have a value corresponding to the offset in the array.
 * This array is only maintained when (_thr_userpricnt > 0).
 */
int _thr_userpriarray[DISPQ_SIZE];

/*
 * _thr_sigwaiting_ok is a boolean that indicates whether it is ok to
 * enable the SIGWAITING handler.  This flag is controlled by the
 * functions _thr_sigwaiting_disable() and _thr_sigwaiting_enable().
 */
boolean_t _thr_sigwaiting_ok = B_TRUE; 

/*
 * _thr_preemptlock is an LWP mutex lock that is held while searching
 * the list of idle threads, _thr_idlethreads, for a thread to preempt 
 * (this is done in the function _thr_activate_lwp()).
 * This lock protects _thr_idlethreads and the global counter _thr_nlwps.
 */
lwp_mutex_t _thr_preemptlock;


void _thr_wakehousekeeper(void);	/* wake the housekeeping thread */
void *_thr_housekeeper(void *);		/* executed by housekeeping thread */
void _thr_activate_lwp(int);		/* active an lwp for awakened thread */
void _thr_prioq_init(int, int);		/* initialize _thr_userpriarray[] */
int  _thr_getminpri(int);		/* determine lowest prio in process */
void _thr_preempt_disable(void);	/* prevents preemption */
void _thr_sigwaiting_disable(void);	/* prevents enabling SIGWAITING hndlr*/
void _thr_sigwaiting_enable(void);	/* allows enabling SIGWAITING hndlr*/
boolean_t _thr_put_on_reaplist(thread_desc_t *target);

/*
 * for use by debug
 */
struct  thread_debug    _thr_debug = {
	(void **) &(_thr_allthreads),
	(void (*)(struct thread_map *, enum thread_change))
		_thr_debug_notify,
	0
};

/*
 *  thr_create(caddr_t stack_addr, uint stack_size,
 *   void *(*start_routine)(void *arg), 
 *   void *arg, long flags, thread_t *new_thread)
 *
 *  	This function creates a thread that will execute start_routine(arg). 
 *	If stack_addr is NULL and stack_size is zero, then allocate a 
 *	default sized stack with a redzone.
 *
 *  Parameter/Calling State:
 *
 *	No locks are held on entry -- user interface.
 *
 *	stack_addr - can be 0 or the address of the stack
 *	stack_size - can be 0 or the desired stack size
 *	start_rouinte - must be the address of the function the 
 *		new thread will run
 *	arg - is a pointer to the argument to the above function
 *	flags - 0 or any combination of THR_BOUND, THR_DETACHED,
 *		THR_SUSPENDED, THR_NEW_LWP
 *	new_thread - is a pointer to the new thread_t
 *
 *	During processing, _thr_tidveclock is acquired, the lock of the
 *	new thread is acquired, and _thr_counterlock is acquired.
 *
 *  Return Values/Exit State:
 *	Creates a new thread with the specified characteristics, returns 0 
 *	and sets value of new_thread to the address of the new thread.
 *	If an error is detected, no thread is created and it returns an 
 *	appropriate errno-type value.
 *
 *	On return, no locks are held.
 */
int
thr_create(void *stack_addr, size_t stack_size,
	   void *(*start_routine)(void *arg), void *arg,
	   long flags, thread_t *new_thread)
{
	thread_desc_t *t;
	thread_desc_t *curtp;
	int rval, index;

	/* create the initial thread if it's not already there */
	if (_thr_allthreads == (void *)0) {
		_thr_init();
	}
	curtp = curthread;

	TRACE5(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_FIRST,
	   stack_addr, stack_size, start_routine, arg, flags);

	/* if stack is specified insure meets the minimum size requirements */
	if (stack_addr && (stack_size < THR_MIN_STACK)) {
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
		   EINVAL, 0); 
		return(EINVAL);
	}

	/*
	 * Any requested stack size greater than 0 (uses the default stack
	 * size) must meet the minimum size requriements.
	 */
	if (stack_size && (stack_size < THR_MIN_STACK)) {
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
		   EINVAL, 0); 
		return(EINVAL);
	}

	if (start_routine == NULL) {	/* thread must execute a function */
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
		   EINVAL, 0); 
		return(EINVAL);
	}

	_thr_sigoff(curtp);

	/* allocate thread local storage */
	/* t points to the child thread on successful return */
	if (!_thr_alloc_tls(stack_addr, stack_size, &t)) {
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
		   ENOMEM, 0); 
		_thr_sigon(curtp);
		return(ENOMEM);
	}
	
	t->t_usropts = flags;
	t->t_usingfpu = B_FALSE;	/* TEMPORARY */
	t->t_hold = curtp->t_hold;	/* inherit creator's signal mask */
	t->t_nosig = 1;


	if ((flags & THR_BOUND)) {
		/*
		 * If the thread is bound and the lwp create fails it
		 * is a fatal error unlike the case for multiplexed threads.
		 *
		 * The fields t_pri and t_cid don't have to be set for a
		 * bound thread because scheduling of bound threads is done 
		 * by the kernel based on priority and scheduling class
		 * of the LWP.  Therefore, these fields are unused in a 
		 * bound thread.
		 */
		if ((rval = _thr_new_lwp(t, start_routine, arg)) != 0) {
			/* free library allocated stack */
			if ((t->t_flags & T_ALLOCSTK))
				_thr_free_stack(t->t_stk, (int)t->t_stksize);
			TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE,
			   TR_CALL_SECOND, EAGAIN, 0); 
			_thr_sigon(curtp);
			return(rval);
		}
	} else {
		/* create thread context */
		_thr_makecontext(t, start_routine, arg);
		/*
		 * a multiplexed thread being created by a bound thread
		 * is assigned the default priority.
		 * a multiplexed parent passes its priority
		 * class on to the child.
		 */
		if (ISBOUND(curtp))
			t->t_pri = DEFAULTMUXPRI;
		else
			t->t_pri = curtp->t_pri;
	}

	if (_thr_alloc_tid(t) == -1) {	/* allocate a thread id */
		/* free library allocated stack */
		if ((t->t_flags & T_ALLOCSTK))
			_thr_free_stack(t->t_stk, (int)t->t_stksize);
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
		   ENOMEM, 0); 
		_thr_sigon(curtp);
		return(ENOMEM);
	}

	/*
	 * If _thr_alloc_tid succeeds, the thread's lock is locked
	 * on return from the function.
	 *
	 * Normally, whenever a field in a thread structure is changed, the
	 * thread lock is held. Even though the new thread's structure is 
	 * being changed throughout this function, it is not necessary to 
	 * hold the thread lock prior to when it is obtained because the 
	 * new thread is not yet known to the library.
	 *
	 * However, once the thread's new ID has been allocated, it is 
	 * necessary to hold the lock to prevent the possibility of a 
	 * thr_continue() operation prematurely starting the thread.
	 *
	 * Once the tid is allocated, the thread becomes visible so 
	 * we now update all the global counters and lists.
	 */

	if (new_thread != NULL)		/* return thread id */
		*new_thread = t->t_tid;

	LOCK_COUNTER;
	_thr_totalthreads++;
	if (flags & THR_DAEMON)
		_thr_daemonthreads++;
	if ((flags & THR_DETACHED) == 0)
		_thr_joinable++;
	if ((flags & THR_BOUND) == 0) {
		/*
		 * One approach to implementing the SIGWAITING handler 
		 * would be to enable it at this point if _thr_nthreads 
		 * exceeds _thr_nlwps. Instead, the SIGWAITING handler 
		 * is enabled only when there are threads on the runnable 
		 * queue.  This avoids allowing generation of the SIGWAITING
		 * signal when no thread can benefit from it.
		 */
		_thr_nthreads++;

		/*
		 * thr_create() checks if the concurrency has fallen below the
		 * minimum level, _thr_minlwps, due to LWP aging when a
		 * multiplexed thread is created.
		 * If it has and there are not enough LWPs to run all 
		 * multiplexed threads currently in the process, a multiplexing
		 * LWP is created for the new thread regardless of whether 
		 * the application specified THR_NEW_LWP.
		 * See thr_setconcurrency() for more discussion of 
		 * _thr_minlwps.
		 */

		if ((flags & THR_NEW_LWP) == 0) {
			if (((_thr_nlwps < _thr_minlwps)
					 && (_thr_nlwps < _thr_nthreads))
			    || (_thr_nlwps == 0)) {
				flags |= THR_NEW_LWP;
			}
		} else {
			/*
			 * the caller requested a new multiplexing LWP,
			 * so we must update _thr_minlwps
			 */
			_thr_minlwps++;
		}

		/*
		 * if preemption is enabled and more than one user priority
		 * will exist, update the priority array.
		 */
		if (_thr_preempt_off == B_FALSE) {
			if (ISBOUND(curtp)) {  /* creator is a bound thread */
				if (_thr_userpricnt > 1) {
					/* CONSTCOND */
					HASH_PRIORITY(DEFAULTMUXPRI, index);
					ADD_TO_PRIO_ARRAY(index);
				} else if (_thr_userminpri != DEFAULTMUXPRI) {
					/*
					 * when _thr_userpricnt == 1, we
					 * only update prio array if the new
					 * thread has a different priority
					 * than existing multiplexed threads
					 */
					/* CONSTCOND */
					HASH_PRIORITY(DEFAULTMUXPRI, index);
					_thr_prioq_init(_thr_userminpri, index);
				}
			} else {  /* creator is multiplexed */
				if (_thr_userpricnt > 1) {
					HASH_PRIORITY(curtp->t_pri, index);
					ADD_TO_PRIO_ARRAY(index);
				}
				/*
				 * when the creating thread is multiplexed,
				 * if _thr_userpricnt == 1, we know that the
				 * new thread has the same priority as all
				 * other multiplexed threads, so we don't
				 * have to update the prio array.
				 */
			}
		}
	} 
	if ((flags & THR_NEW_LWP) && (ISBOUND(curtp))) {
		_thr_lwpswanted++;
	}

	/* save start routine addr for debugger's - we don't really
	 * need it for the library.
	 * We set the startup flag so debuggers know that the thread
	 * has not yet reached its main routine.
	 */
	t->t_start_addr = start_routine;
	t->t_dbg_startup = 1;

	/* add thread to the _thr_allthreads */
	_thr_allthreads->t_prev->t_next = t;
	t->t_prev = _thr_allthreads->t_prev;
	_thr_allthreads->t_prev = t;
	t->t_next = _thr_allthreads;
	UNLOCK_COUNTER;

	/*
	 * threads not created in the initially suspended state are continued
	 * here.
	 */
	/* three cases for debug notify routine - in each
	 * case make sure it is called after t_state is set
	 */

	if ((flags & THR_SUSPENDED) == 0)  {
		if (ISBOUND(t)) {
			t->t_state = TS_ONPROC;
			if (_thr_debug.thr_debug_on)
				_thr_debug_notify(t, tc_thread_create);
			_lwp_continue(LWPID(t));
			if (_thr_debug.thr_debug_on)
				_thr_debug_notify(t, tc_thread_continue);
			UNLOCK_THREAD(t);
		} else {
			int prio;
			prio = _thr_setrq(t, 0);
			if (_thr_debug.thr_debug_on)
				_thr_debug_notify(t, tc_thread_create);
			UNLOCK_THREAD(t);
			_thr_activate_lwp(prio);
		}
	} else {
		t->t_state = TS_SUSPENDED;
		if (_thr_debug.thr_debug_on)
			_thr_debug_notify(t, tc_thread_create);
		UNLOCK_THREAD(t);
	}

	/*
	 * The housekeeping thread creates new multiplexing LWPs.
	 * This provides a performance benefit for thr_create() and
	 * maintains a homogeneous multiplexing LWP pool.
	 *
	 * In order to ensure homogeneity among the multiplexing LWPs in a
	 * process, it is necessary that only multiplexing LWPs create new
	 * multiplexing LWPs.
	 * This ensures that all multiplexing LWPs share the same
	 * scheduling class, signal mask, and, depending on the scheduling
	 * class, priority as long as the application uses only thread
	 * library interfaces to control the priorities and scheduling classes
	 * of its threads.
	 * 
	 * No error indication is given if the creation of a multiplexing 
	 * LWP fails. This is viewed as a failure to increase concurrency,
	 * presumably because a system limit has been reached.
	 * Therefore, it is not deemed necessary to destroy the new thread and
	 * claim failure in this case.
	 * 
	 */
	if (flags & THR_NEW_LWP) {
		if (ISBOUND(curtp)) {
			_thr_wakehousekeeper();
		} else {
			(void)_thr_new_lwp(NULL, _thr_age, NULL);
		}
	}

	TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CREATE, TR_CALL_SECOND,
	   0, t->t_tid); 
	_thr_sigon(curtp);
	return(0);
}

/*
 * thr_minstack()
 *  	returns the minimum allowable stack size which is the sum of 
 * 	the minimum stack frame size and the size of a tls structure.
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	Takes no arguments
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *  	Returns the value of THR_MIN_STACK; no locks are held on return.
 */

size_t
thr_minstack(void)
{
	TRACE0(0, TR_CAT_THREAD, TR_EV_THR_MINSTACK, TR_CALL_ONLY); 
	return(THR_MIN_STACK);
}



/*
 * thr_self()
 *  	returns the thread_id of the calling thread
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	takes no arguments
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	Returns the thread_t value that identifies the calling thread;
 *	no locks are held on return.
 */

thread_t
thr_self()
{
	TRACE0(0, TR_CAT_THREAD, TR_EV_THR_SELF, TR_CALL_ONLY); 
	return(curthread->t_tid);
}


/*
 * thr_continue(thread_t target_thread)
 *	removes the inhibition on execution of a thread
 *	This inhibition exists either because the thread was created 
 * 	in the suspended state or as the result of a thr_suspend().
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	a thread identifier is passed to the function
 *
 *	During processing, _thr_tidveclock and the lock of target_thread
 *	are acquired
 *
 * Return Values/Exit State:
 *	Returns zero if successful or ESRCH if thread identifier is 
 *	invalid; no locks are held on return.
 */
int
thr_continue(thread_t target_thread)
{
	thread_desc_t *tp;
	int rval = 0;
	thread_desc_t *curtp = curthread;
	int needlwp = 0;

	_thr_sigoff(curtp);

	if ((tp = _thr_get_thread(target_thread)) == NULL)  {
		_thr_sigon(curtp);
		return(ESRCH);
	}

	if (tp->t_state == TS_ZOMBIE) {	/* thread has already exited */
		UNLOCK_THREAD(tp);
		_thr_sigon(curtp);
		return(ESRCH);
	}

	/*
	 * If a multiplexed thread is in state TS_SUSPENDED, then it
	 * simply needs to be made runnable.
	 * A bound thread in this state needs to have its state changed
	 * and its LWP continued.
	 */

	if (_thr_debug.thr_debug_on)
		_thr_debug_notify(tp, tc_thread_continue);

	if (tp->t_state == TS_SUSPENDED) {
		if (ISBOUND(tp)) {
			tp->t_state = TS_ONPROC;
			/*
			 * we check to see if a bound target thread has ever
			 * run by examining the t_dbg_startup flag.  If it
			 * has never run, its LWP is in the suspended state
			 * and it is not sleeping on a LWP condition variable;
			 * if it has run, its LWP is not in the suspended
			 * state and it is sleeping on a LWP condition variable.
			 */
			if (tp->t_dbg_startup == 1) {
				rval = _lwp_continue(LWPID(tp));
			} else {
				rval = CONTINUE_BOUND_THREAD(tp);
			}
		} else {
			needlwp = _thr_setrq(tp, 0);
		}
	} else if (tp->t_suspend == TSUSP_PENDING) {
		/*
		 * a suspension was pending but not acted on; we clear
		 * the pending suspension and notify any waiters that
		 * the suspension is complete
		 */
		tp->t_suspend = TSUSP_CLEAR;
		(void) _THR_COND_BROADCAST(&(tp->t_join));
	}

	UNLOCK_THREAD(tp);
	if (needlwp != INVALID_PRIO) { 
		_thr_activate_lwp(needlwp);
	}
	TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_CONTINUE, TR_CALL_ONLY,
	   target_thread, rval); 
	_thr_sigon(curtp);
	return (rval);
}


/*
 * thr_exit(void *status)
 *	Current thread is turned into a zombie and placed onto _thr_deathrow. 
 *	These threads will be freed by a reaper, the reaper will discard 
 *	the thread's id if this thread is marked THR_DETACHED.
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	Takes one argument which represents the exit status of the thread
 *
 *	During processing, the calling thread's lock and _thr_counterlock
 *	are acquired.
 *
 * Return Values/Exit State:
 *	does not return to the caller
 */
/*ARGSUSED*/
void
thr_exit(void *status)
{
	thread_desc_t *t = curthread;
	boolean_t wakeflag = B_FALSE;
	int index;

	TRACE1(t, TR_CAT_THREAD, TR_EV_THR_EXIT, TR_CALL_ONLY, status); 

	PRINTF3("thr_exit: _lwp_getprivate=0x%x,curthread=0x%x, tid=%d\n", 
					_lwp_getprivate(), t, t->t_tid);
	if (_thr_debug.thr_debug_on) {
	       _thr_debug_notify(t, tc_thread_exit);
	}
	_thr_sigoff(t);
	_thr_timer_threxit(t);	/* clean up outstanding timer requests */
	if (t->t_binding.tb_data) {
		(void)_thr_key_exit(t);	/* clean up TSD bindings. */
	}

	/*
	 * A terminating thread's ID should be freed as soon as possible.
	 * In the case of a detached thread, this can be done at this point
	 * because no other thread can affect a detached thread once it
	 * enters the critical code of this function.
	 * In the case of a joinable thread, this cannot be done in this
	 * function because other threads may subsequently join with a
	 * joinable thread.
	 * Therefore, it is the thread that joins a joinable thread that
	 * frees that joinable thread's ID.
	 */

	if (t->t_usropts & THR_DETACHED) {
		_thr_free_tid(t->t_tid);
	}

	LOCK_THREAD(t);
	t->t_state = TS_ZOMBIE;
	LOCK_COUNTER;

	if (t->t_usropts & THR_DAEMON)
		_thr_daemonthreads--;
	if ((--_thr_totalthreads - _thr_daemonthreads) == 0) {
		exit((int)status);
	}

	/* remove t from _thr_allthreads list */
	t->t_prev->t_next = TNEXT(t);
	TNEXT(t)->t_prev = t->t_prev;
	if (_thr_allthreads == t) {
		_thr_allthreads = TNEXT(t);
	}
#ifdef DEBUG
	t->t_prev = t->t_next = NULL;
#endif /* DEBUG */

	/* update _thr_userpriarray[] if necessary */
	if (_thr_preempt_off == B_FALSE) {
		if (_thr_userpricnt > 1) {
			HASH_PRIORITY(t->t_pri, index);
			REM_FROM_PRIO_ARRAY(index);
		}
	}

	if (t->t_usropts & THR_DETACHED) {
		/*
		 * If the calling thread is detached, then its t_joincount
		 * field can be non-zero only if at least one thread is 
		 * trying to suspend it. In this case, the calling thread 
		 * is not placed on _thr_reaplist because that would create
		 * the potential for segmentation violation if the calling 
		 * thread is reaped while the suspending thread tries to
		 * access its thread structure.  When the last suspending 
		 * thread finds the calling thread has exited, it
		 * will place the calling thread onto _thr_reaplist and 
		 * increment _thr_reapable.
		 */

		if (t->t_joincount == 0) {
			/*
			 * The terminating thread's t_join condition variable
			 * need not be destroyed to prevent other threads from
			 * waiting on this variable because at this point,
			 * the thread is in zombie state and holds its own
			 * thread lock.  There is no operation that will
			 * cause a thread to acquire a thread's lock and
			 * block on it t_join condition variable when the
			 * locked thread is already a zombie.
			 *
			 * rval = _THR_COND_DESTROY(&t->t_join);
			 *
			 */
			wakeflag = _thr_put_on_reaplist(t);
		} else {
			/*
			 * someone's waiting on a suspend, so wake them
			 * up, destroy the condition variable, and don't
			 * put the thread on _thr_reaplist.
			 */
			PRINTF("thr_exit: not putting thread on reaplist1\n");
			(void) _THR_COND_BROADCAST(&(t->t_join));
#ifdef DEBUG
			ASSERT(_THR_COND_DESTROY(&(t->t_join)) == 0);
#else
			(void)_THR_COND_DESTROY(&(t->t_join));
#endif
		}
	} else {			/* t is joinable */
		t->t_exitval = status;
		PRINTF2("thr_exit: tid %d exitval = %d\n",
			t->t_tid, t->t_exitval);
		/*
		 * We increment t_joincount to allow for the possibility that a
		 * thread joining any thread (i.e., first argument to thr_join
		 * was 0) might dequeue t from _thr_joinablelist.  Incrementing
		 * this counter prevents placing t onto _thr_reaplist before
		 * all joining threads have acquired t's lock.  The t_joincount
		 * field is decremented in the following cases:
		 *      a) whoever dequeues t from _thr_joinablelist as soon
		 *         as caller holds t's lock (a thread waiting speci-
		 *         cally for t will hold the lock when it dequeues t;
		 *         a thread waiting for any thread will dequeue t and
		 *         then acquire t's lock).  This case negates the
		 *         incrementing done in this function.
		 *      b) any thread waiting specifically for t who previously
		 *         incremented the counter upon acquiring the thread
		 *         lock after t exits.  This case negates the incre-
		 *         menting done in either thr_join() or thr_suspend().
		 *
		 * This procedure assures us that when we finally decrement
		 * t_joincount to zero, it is safe to place the thread onto
		 * _thr_reaplist.
		 */
		t->t_joincount++;
		/* put t on _thr_joinablelist */
		_thr_enq_thread_to(&_thr_joinablelist, t);
#ifdef DEBUG
		_thr_dumpjoinlist();
#endif /* DEBUG */
		if (t->t_joincount > 1) {
			/* activate other threads waiting on t_join */
			(void) _THR_COND_BROADCAST(&(t->t_join));
		}
		_THR_COND_DESTROY(&(t->t_join));
		if (_thr_zombie_waiters) {
			/* activate threads waiting for any thread to exit */
			(void) _THR_COND_BROADCAST(&(_thr_zombied));
		}
		/*
		 * A joinable thread always notifies threads joining
		 * specifically that thread as well as threads joining any
		 * thread.  An alternative would be to skip the second
		 * notification if the first one succeeded in waking a thread.
		 * However, it is not possible to immediately determine that
		 * the first notification actually did wake a thread that
		 * will successfully join with the terminating thread.
		 * The approach used also has the advantage that it prevents
		 * deadlock in the case that a thread waiting for any thread
		 * to terminate would have no thread to join with after the
		 * current thread is gone. It is not possible to provide
		 * absolute protection against deadlock as it may occur that
		 * all joinable threads are trying to do joins.  Such
		 * deadlocks are programming errors and the cost to detect
		 * such deadlocks is too great.
		 *
		 * Note that threads waiting for the calling thread to
		 * suspend as a result of thr_suspend() are awakened by
		 * thr_exit() at no additional cost since these threads
		 * sleep on the same condition variable as threads that are
		 * thr_join()ing the thread.
		 */

	}
	
	if (ISBOUND(t)) {
		UNLOCK_COUNTER;
		if (wakeflag) {
			PRINTF1("thread %d calling _thr_reaper()1\n", t->t_tid);
			_thr_reaper();
		}
		UNLOCK_THREAD(t); 
		/*
		 * It is safe to unlock the terminating thread's lock and
		 * continue to process because there are no operations 
		 * that can be performed on the terminated thread that will
		 * interfere with further processing.  Attempts to send a 
		 * signal or to suspend the terminated thread will
		 * fail because the thread is now a zombie.
		 */

		_lwp_exit();
	} else {
		_thr_nthreads--;
		UNLOCK_COUNTER;

		if (wakeflag) {
			PRINTF1("thread %d calling _thr_reaper()2\n", t->t_tid);
			_thr_reaper();
		}
		/*
		 * The calling thread's lock is held upon entry to 
		 * _thr_swtch().  The lock is released in _thr_swtch().
		 * 
		 * The exiting thread's context does not need to be saved.
		 * This permits the faster switch function _thr_swtch() to be
		 * used.
		 */
		ASSERT(IS_THREAD_LOCKED(t));
		_thr_swtch(0, t);
		/*
		 * This function does not return so there is no need to
		 * save the context of the thread; therefore, the argument
		 * to _thr_swtch() is 0.
		 */
	}
}

/*
 * int
 * thr_join(thread_t wait_for, thread_t *departed_thread, void **status)
 *      wait for another thread to thr_exit; if 'wait_for' is 0, wait for
 *      the next joinable thread, otherwise wait for the indicated thread.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held -- user interface
 *
 *      During processing, _thr_tidveclock, _thr_counterlock and the
 *      thread lock of the thread to be joined may be acquired.
 *
 * Return Values/Exit State:
 *      if successful:
 *              1) returns 0
 *              2) if 'departed_thread' is not NULL, stores the thread ID
 *                 of the joined thread in the address indicated by
 *                 'departed_thread'
 *              3) if 'status' is not NULL, returns thr_exit status of the
 *                 departed thread in 'status'.
 *
 *      if unsuccessful, returns appropriate error value.
 *
 *      On exit, no locks are held.
 */

int
thr_join(thread_t wait_for, thread_t *departed_thread, void **status)
{
	thread_desc_t *curtp = curthread;
	thread_desc_t *tp;
	boolean_t first_time_thru_loop = B_TRUE;
	boolean_t joined_a_thread = B_FALSE;
	boolean_t flag = B_FALSE;
	int rval = 0;
	thread_t freetid_flag = (thread_t)0;
	int dq_rval;
#ifdef THR_DEBUG
	int cond_rval;
#endif

	TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN, TR_CALL_FIRST, wait_for); 
	/*
	 * Case 1:  wait for a specific thread
	 */
	if (wait_for != 0) {

		/* make sure wait_for is not the calling thread */
		if (wait_for == curtp->t_tid) {
			TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN,
			   TR_CALL_SECOND, 0, 0, EDEADLK); 
			return(EDEADLK);
		}

		_thr_sigoff(curtp);

		/* make sure wait_for exists */
		if ((tp = _thr_get_thread(wait_for)) == NULL)  {
			TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN,
			   TR_CALL_SECOND, 0, 0, ESRCH); 
			_thr_sigon(curtp);
			return(ESRCH);
		}

		/* make sure wait_for is joinable */
		if (tp->t_usropts & THR_DETACHED) {
			UNLOCK_THREAD(tp);
			TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN,
			   TR_CALL_SECOND, 0, 0, ESRCH); 
			_thr_sigon(curtp);
			return(ESRCH);
		}

		/* wait for wait_for to thr_exit() */
		while (tp->t_state != TS_ZOMBIE) {
			PRINTF2("thr_join: %d waiting for %d to exit\n",
				curtp->t_tid, tp->t_tid);
			if (first_time_thru_loop == B_TRUE) {
				tp->t_joincount++;
				first_time_thru_loop = B_FALSE;
			}
#ifdef THR_DEBUG
			cond_rval = _thr_cond_wait(&(tp->t_join),
				(mutex_t *)NULL, &(tp->t_lock),
				(const timestruc_t *) NULL, B_TRUE, B_TRUE);
			ASSERT(cond_rval != EINVAL);
#else
			(void)_thr_cond_wait(&(tp->t_join), (mutex_t *)NULL,						     &(tp->t_lock),
					     (const timestruc_t *) NULL,
					     B_TRUE, B_TRUE);
#endif
		}

		PRINTF1("thr_join: cond_rval = %d; ", cond_rval);
		PRINTF2("caller = %d, t_state = %d\n",
			curtp->t_tid, tp->t_state);

		/* at this point, wait_for has terminated */
		if (first_time_thru_loop == B_FALSE) {
			/*
			 * the following decrement of t_joincount cancels
			 * the increment done previously in this function
			 * (when we set first_time_thru_loop to B_FALSE).
			 */
			tp->t_joincount--;
		}

		/* check if thread has been joined yet */
		if ((tp->t_usropts & THR_DETACHED) == 0) {
			/*
			 * thread is still joinable; while holding just
			 * the thread lock, obtain any requested
			 * info about it, free its thread ID, and mark
			 * it as no longer joinable...
			 */
			if (departed_thread != NULL)
				*departed_thread = tp->t_tid;
			freetid_flag = tp->t_tid;
			if (status != NULL) {
				*status = tp->t_exitval;
				PRINTF1("thr_join: assigning exitvalue %d\n",
				  tp->t_exitval);
			}
			tp->t_usropts |= THR_DETACHED;

			/*
			 * ...now update _thr_joinable, dequeue the 
			 * thread from the joinable list, and put it on 
			 * _thr_reaplist if no other thread wants its 
			 * thread lock.
			 */
			LOCK_COUNTER;
			_thr_joinable--;
			if (_thr_joinable == 0) {
				/*
				 * we just joined the last
				 * joinable thread so we must
				 * wake up any threads trying
				 * to join any thread so they
				 * can realize there are no
				 * more joinable threads.
				 */
				if (_thr_zombie_waiters)
					_THR_COND_BROADCAST(&(_thr_zombied));
			}
			dq_rval = _thr_deq_thread_from(&_thr_joinablelist, tp);
			if (dq_rval) {
				/*
				 * Only check to put tp on the _thr_reaplist if
				 * we removed the thread from the _thr_joinable
				 * list; otherwise, the thread that removed it
				 * will do the check.
				 */
				if (tp->t_joincount == 1) {
					flag = _thr_put_on_reaplist(tp);
				} else {
					tp->t_joincount--;
				/*
				 * Note that even though we may have already
				 * decremented t_joincount a few lines back,
				 * we must do it again if we dequeued the
				 * thread from _thr_joinablelist but cannot
				 * yet reap it.  This second decrement cancels
				 * the increment performed in thr_exit(). This
				 * decrement is the responsibility of the thread
				 * that dequeues tp from _thr_joinablelist.
				 */
				}
			}
			UNLOCK_COUNTER;
			UNLOCK_THREAD(tp);
		} else {
			/*
			 * tp is no longer joinable; this means that another
			 * thread succeeded in joining with it before we did.
			 * In this case, we must check to see if we're the
			 * last thread waiting for tp and, if so, put tp on
			 * _thr_reaplist.  In addition, we must set the
			 * return value to ESRCH.
			 *
			 * The first check below ensures that no other threads
			 * are waiting to try to join this thread (i.e., no
			 * other thread will acquire the thread's lock).
			 *
			 * Note that we also check that first_time_thru_loop
			 * is false.  This must be done because if it is not
			 * false, it means that we didn't set t_joincount to 0.
			 * This means another thread that either joined the
			 * target thread or decremented t_joincount to 0 will
			 * place the target thread onto _thr_reaplist.
			 */
			if ((tp->t_joincount == 0) &&
			    (first_time_thru_loop == B_FALSE)) {
				LOCK_COUNTER;
				flag = _thr_put_on_reaplist(tp);
				UNLOCK_THREAD(tp);
				UNLOCK_COUNTER;
			} else {
				UNLOCK_THREAD(tp);
			}
			rval = ESRCH;
		}
	/*
	 * Case 2:  wait_for == 0, so wait for any thread
	 */
	} else {

		_thr_sigoff(curtp);
		LOCK_COUNTER;
		while (joined_a_thread == B_FALSE) {

			/*
			 * make sure there is at least one joinable thread
			 * in the process.
			 */
			if (_thr_joinable == 0) {
				UNLOCK_COUNTER;
				TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN,
				   TR_CALL_SECOND, 0, 0, EINVAL); 
				_thr_sigon(curtp);
				return(EINVAL);
			}

			/*
			 * make sure the calling thread is not the only
			 * joinable thread in the process.
			 */
			if ((_thr_joinable == 1) &&
			((curtp->t_usropts & THR_DETACHED) == 0)) {
				/* only joinable thread is calling thread */
				UNLOCK_COUNTER;
				TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN,
				   TR_CALL_SECOND, 0, 0, EINVAL); 
				_thr_sigon(curtp);
				return(EINVAL);
			}

			/*
			 * If no joinable thread has exited, sleep on the
			 * _thr_zombied condition variable and go back
			 * to the start of the loop when awakened; otherwise,
			 * dequeue the thread from the joinable list and
			 * try to join with it.
			 */
			if (_thr_joinablelist == NULL) {
				/* no joinable thread has exited yet */
				PRINTF2("thr_join: tid %d sleeping on any; joinable = %d\n",
				curtp->t_tid, _thr_joinable);
				_thr_zombie_waiters++;
				_thr_cond_wait(&_thr_zombied,
				   (mutex_t *)NULL, &_thr_counterlock,
				   (const timestruc_t *) NULL, B_TRUE,
				   B_TRUE);
				_thr_zombie_waiters--;
				PRINTF1("thr_join: tid %d waking on any\n",
				curtp->t_tid);
			} else {
				/* a joinable thread has exited */
				tp = _thr_deq_any_from(&_thr_joinablelist);
				UNLOCK_COUNTER;
				ASSERT(tp != (thread_desc_t *) NULL);
				LOCK_THREAD(tp);
				PRINTF2("thr_join: %d dequeued %d\n",
					curtp->t_tid, tp->t_tid);
				if ((tp->t_usropts & THR_DETACHED) == 0) {
					/*
					 * thread is still joinable; get any
					 * requested info and mark it as
					 * no longer joinable...
					 */
					joined_a_thread = B_TRUE;
					PRINTF2("thr_join: %d joining %d\n",
						curtp->t_tid, tp->t_tid);
					if (status != NULL) {
						*status = tp->t_exitval;
						PRINTF1("thr_join: exitval=%d\n",
							tp->t_exitval);
					}
					if (departed_thread != NULL)
						*departed_thread = tp->t_tid;
					tp->t_usropts |= THR_DETACHED;
					freetid_flag = tp->t_tid;

					/*
					 * ...now get _thr_counterlock, update
					 * _thr_joinable, and put the thread
					 * on _thr_reaplist if no other thread
					 * wants its thread lock.
					 */
					LOCK_COUNTER;
					_thr_joinable--;
					if (_thr_joinable == 0) {
						/*
						 * we just joined the last
						 * joinable thread so we must
						 * wake up any threads trying
						 * to join any thread so they
						 * can realize there are no
						 * more joinable threads.
						 */
						if (_thr_zombie_waiters)
							_THR_COND_BROADCAST
							(&(_thr_zombied));
					}
					if (tp->t_joincount == 1) {
						flag = _thr_put_on_reaplist(tp);
					} else {
						/* 
						 * Indicate we've acquired the
						 * thread lock of tp.  This
						 * cancels the increment of
						 * t_joincount performed by
						 * thr_exit().
						 */
						tp->t_joincount--;
					}

					UNLOCK_THREAD(tp);
					UNLOCK_COUNTER;
				} else {
					/*
					 * tp is no longer joinable; this
					 * can happen if another thread that
					 * was waiting specifically for tp
					 * joined with it before we acquired
					 * its lock.  In this case, we must
					 * reacquire the counter lock and
					 * check to see if any other threads
					 * are trying to join with tp.  If so,
					 * we decrement tp's t_joincount flag
					 * to indicate that the thread that
					 * dequeued tp from _thr_joinablelist
					 * has already acquired tp's lock;
					 * if not, we put tp on _thr_reaplist
					 * and repeat the loop holding the
					 * counter lock and wait for another
					 * thread.
					 */
					PRINTF2("thr_join: %d not joining %d\n",
						curtp->t_tid, tp->t_tid);
					LOCK_COUNTER;
					if (--tp->t_joincount == 0) {
						flag = _thr_put_on_reaplist(tp);
					}
					UNLOCK_THREAD(tp);
				}
			}
		}
	}
	/*
	 * check to see if a thread was actually joined; if so, its
	 * thread ID must be freed.
	 */
	if (freetid_flag != (thread_t)0) {
		_thr_free_tid(freetid_flag);
	}

	/*
	 * check to see if there are enough reapable threads to justify
	 * a call to _thr_reaper()
	 */
	if (flag) {
		_thr_reaper();
	}
	TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_JOIN, TR_CALL_SECOND, 
	   *departed_thread, *status, rval); 
	_thr_sigon(curtp);
	return(rval);
}
/*
 * _thr_idle_thread_create()
 *	create an idle thread.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held but signal handlers are disabled.
 *
 *	takes no arguments
 *
 *	During processing, _thr_stkcachelock may be acquired
 *	indirectly via the call to _thr_alloc_tls().
 *
 * Return Values/Exit State:
 *      if successful returns the thread_t value of the thread, returns
 *	NULL on failure
 *
 *	On exit, no locks are held and signal handlers are still disabled
 */

thread_desc_t *
_thr_idle_thread_create()
{
	thread_desc_t *t;
 
	ASSERT(THR_ISSIGOFF(curthread));

	if (!_thr_alloc_tls(NULL, NULL, &t))
		return(NULL);
	_thr_make_idle_context(t);
	t->t_idle = t;
	/*
	 * An idle thread is given a priority of -1 to ensure that it is
	 * always preempted before an application thread whose priority 
	 * can never be less than 0.
	 */
	t->t_pri = -1;
	t->t_state = TS_RUNNABLE;
	/*
	 * All idle threads are given the same thread ID.
	 * This is safe because idle threads are not accessible to the 
	 * application and there are no operations performed on idle threads.
	 * Idle threads may be distinguished during debugging by examining
	 *  which LWP they are associated with; an idle thread is always 
	 * associated with a single LWP throughout its existence.
	 */
	t->t_tid = IDLETHREADID;
	t->t_nosig = 2;

	sigfillset(&t->t_hold);
	_thr_sigdiffset(&t->t_hold, &_thr_sig_programerror);
	/*
	 * The idle thread is not placed on the list of idle threads by 
	 * this function.  Instead, that is done in _thr_new_lwp() when 
	 * that function obtains the _thr_counterlock.
	 * This takes advantage of the fact that this function has only one
	 * customer and is done to save a locking round trip.
	 */
	return(t);
}
	
/*
 * void
 * _thr_reaper()
 *	removes all detached threads from _thr_reaplist
 *
 * Parameter/Calling State:
 *      On entry, the calling thread's lock may be held;  this function 
 *	takes no arguments.
 *
 *      During processing, _thr_counterlock is acquired and
 *      _thr_preemptlock is acquired each time a thread stack is reaped.
 *	Also, the thread lock of the thread to be reaped is acquired via
 *	LWP_MUTEX_TRYLOCK (to avoid deadlock possibilities).
 *
 *      On exit, the same locks are held as on entry.
 *
 * Return Values/Exit State:
 *	Has no return value, on exit no locks are held
 */
void
_thr_reaper()
{
	thread_desc_t *temp, *temp2, *temp3, *next,
		      *first; /* linted - not used??? */
	thread_desc_t *curtp = curthread;
	int unreapable = 0;
	int rval;


	_thr_sigoff(curtp);
	LOCK_COUNTER;
	/*
	 * The global list of reapable threads, _thr_reaplist, is moved to
	 * a temporary list.  This minimizes the time that _thr_counterlock
	 * is held and that the global list is unavailable to threads
	 * calling thr_exit().
	 */
	temp =  _thr_reaplist;
	_thr_reaplist = NULL;
	_thr_reapable = 0;
	UNLOCK_COUNTER;
	temp2 = NULL;
	first = temp;           /* indicates when we've circled the list */
	if (temp != NULL) {
		do {
			/* free library allocated stacks */

			PRINTF1("reaping %d\n", temp->t_tid);
			next = temp->t_next;
			if (!(temp->t_flags & T_ALLOCSTK)) {
				PRINTF("stack not from library\n");
				temp = next;
				continue;
			}

			/*
			 * Before freeing the stack, we must
			 * first make sure the thread is safe
			 * to reap.  If it's not, we put it
			 * onto the temp2 list.  This list will
			 * be appended later to _thr_reaplist.
			 */

			/* 
			 * We don't want to reap current thread.
			 */

			if (temp == curtp) {
				PRINTF2("case curtp %d: t_tls = %x\n",
				   temp->t_tid, temp->t_tls);
#ifdef DEBUG
				temp->t_next = NULL;
				temp->t_prev = NULL;
#endif /* DEBUG */
				_thr_enq_thread_to
				  (&temp2, temp);
				unreapable++;
			} else { 
				/*
			 	 * Next we try to lock the thread lock.  If
			 	 * we obtain the lock, it is safe to reap
			   	 * the thread; otherwise, we check other
			 	 * cases described below.
				 * (Note that it's safe to try for this lock
				 * even though we hold our own lock since
				 * we are using a trylock operation so there
				 * is no danger of deadlock.  Also, we are
				 * trying to obtain the lock of a thread that
				 * is not known to any other thread except,
				 * possibly, the thread itself if it is also
				 * calling _thr_reaper -- that is the only
				 * possible source of contention.)
			 	 */
				if (LWP_MUTEX_TRYLOCK(&temp->t_lock) == 0) {

					/*
					 * We got the lock!  Two cases exist:  
					 * (a) thread was a bound or idle thread
					 *  - can be reaped only after its LWP
					 * has exited; 
					 * (b) if thread was a non-idle multi-
					 * plexed thread, it's off its LWP and 
					 * can be reaped.
				 	 */

					if ((temp->t_usropts & THR_BOUND) ||
				   		(temp->t_tid < 0)) {
						/* case a */
						PRINTF("checking case a\n");
						rval = _lwp_wait(LWPID(temp), NULL);
						if (rval == 0) {
							/* LWP has exited */
							PRINTF("freeing bstack\n");
							_thr_lwp_freeprivate(
							  (__lwp_desc_t *)
							  temp->t_lwpp);
							LOCK_PREEMPTLOCK;
							_thr_free_stack
						  	(temp->t_stk,
						  	(int)temp->t_stksize);
							UNLOCK_PREEMPTLOCK;
						} else if (rval == EINTR) {
							/* LWP still exists */
#ifdef DEBUG
							temp->t_next = NULL;
							temp->t_prev = NULL;
#endif /* DEBUG */
							_thr_enq_thread_to
							  (&temp2, temp);
							UNLOCK_THREAD(temp);
							unreapable++;
							PRINTF("save case b\n");
							PRINTF1("unreapable=%d\n",
							unreapable);
						} else {
							/* rval == ESRCH or EDEADLK */
							_thr_panic("_thr_reaper");
						}
					} else {
						/* case b */
						PRINTF("freeing stack\n");
						LOCK_PREEMPTLOCK;
						PRINTF2("freeing 0x%x - 0x%x\n",
					 	 temp->t_stk, 
						 temp->t_stk + temp->t_stksize);
						_thr_free_stack(temp->t_stk, 
						 temp->t_stksize);
						UNLOCK_PREEMPTLOCK;
					}
				} else {
					/*
					 * It is not safe to free thread's stack
					 * since we couldn't get the lock.
					 * (This indicates, since it was on
					 * _thr_reaplist, that it was still in
					 * the process of context switching or
					 * of being thr_join'd.)
					 */
#ifdef DEBUG
					temp->t_next = NULL;
					temp->t_prev = NULL;
#endif /* DEBUG */
					_thr_enq_thread_to (&temp2, temp);
					unreapable++;
					PRINTF1("not reaping; unreapable=%d\n",
					  unreapable);
				}
			}
			temp = next;
			PRINTF1("next = 0x%x\n", next);
		} while (next != first);

		if (unreapable) {

			/*
			 * This is where temp2 list is added to _thr_reaplist.
			 * We also have to add the length of temp2 to
			 * _thr_reapable.
			 */

			PRINTF1("adding %d threads to reaplist\n", unreapable);
			temp3 = temp2->t_prev;
			LOCK_COUNTER;
			if (_thr_reaplist == NULL) {
				_thr_reaplist = temp2;
			} else {
				_thr_reaplist->t_prev->t_next = temp2;
				temp2->t_prev = _thr_reaplist->t_prev;
				temp3->t_next = _thr_reaplist;
				_thr_reaplist->t_prev = temp3;
			}
			_thr_reapable += unreapable;
			/* _thr_dumpreaplist(); */
			UNLOCK_COUNTER;
#ifdef THR_DEBUG
		} else {
			PRINTF1("unreapable = %d\n", unreapable);
#endif
		}
	}
	PRINTF("_thr_reaper -- done\n");
	_thr_sigon(curtp);
}


/*
 * boolean_t
 * thr_put_on_reaplist(thread_desc_t *target)
 *	puts a thread on the reaplist for later resource recovery
 *
 * Parameter/Calling State:
 *	On entry, _thr_counterlock is held.
 *
 *      target - a thread identifier is passed to the function
 *
 *	During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *	Returns 1 if the reap threshold is exceeded and 0 otherwise
 *	
 *	On exit, _thr_counterlock is still held.
 */

boolean_t
_thr_put_on_reaplist(thread_desc_t *target)
{
	ASSERT(IS_COUNTER_LOCKED);
	ASSERT(THR_ISSIGOFF(curthread));

	/* append target to the rear of _thr_reaplist */
	_thr_enq_thread_to(&_thr_reaplist, target);
	return(++_thr_reapable > REAP_THRESHOLD);
}


/*
 * void
 * thr_wakehousekeeper()
 *	This function wakes up the housekeeping thread, creating it if 
 *	it does not exist.  
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and no locks need to be held,
 *	though it is acceptable to hold the lock of a thread.
 *
 *	takes no arguments
 *
 *	During processing, if the housekeeping thread is created, the lock
 *	of the housekeeping thread is acquired.
 *
 * Return Values/Exit State:
 * 	Has no return value.
 *
 *	On exit, conditions are the same as on entry.
 */

void 
_thr_wakehousekeeper(void) 
{
	thread_desc_t *t;

	ASSERT(THR_ISSIGOFF(curthread));

	if (_thr_housethread == NULL) {
		/* create the housekeeping thread */
		PRINTF("THR_WAKEHOUSEKEEPER: creating the housekeeper\n");
		LOCK_RUNQ;
		if(_thr_housethread == NULL) { /* check to avoid races */
			/* alloc thread local storage */
			if (!_thr_alloc_tls(NULL, 0, &t)) {
				_thr_panic("housekeeping thread create failed");
			}
			t->t_tid = HOUSEKEEPERID;
			t->t_nosig = 2;
			t->t_pri = MAXINT;
			t->t_cid = DEFAULTMUXCID;
			sigfillset(&t->t_hold);
			_thr_sigdiffset(&t->t_hold, &_thr_sig_programerror);
			_thr_makecontext(t, _thr_housekeeper, NULL);
			_thr_housethread = t;
			UNLOCK_RUNQ;
			LOCK_THREAD(t);
			/*
			 * don't care about return value from _thr_setrq()
			 * since we know the housekeeping thread is not
			 * lazy-switched out; therefore, _thr_activate_lwp()
			 * must be called with housekeeping thread's prio
			 * as its argument.
			 */
			(void) _thr_setrq(t, 0);
			UNLOCK_THREAD(t);
			PRINTF1("_thr_housethread=0x%x\n", t);	
			_thr_activate_lwp(t->t_pri);
		} else {
			UNLOCK_RUNQ;
			_THR_SEMA_POST(&(_thr_house_sema));
		}
	} else {
		PRINTF("THR_WAKEHOUSEKEEPER: waking the housekeeper\n");
		_THR_SEMA_POST(&(_thr_house_sema));
	}
}



/*
 * thr_housekeeper()
 *	Creates multiplexing LWPs.
 *
 * Parameter/Calling State:
 *	No locks are held on entry but all signals are blocked.
 *
 *	Takes no arguments.
 *
 *	During processing, _thr_counterlock is acquired; the housekeeping
 *	thread sleeps on the semaphore _thr_house_sema.
 *
 * Return Values/Exit State:
 *	never returns.
 */

/* ARGSUSED */
void *
_thr_housekeeper(void * notused)
{
	int counter;
	int rval;
	struct sigaction sigwaitingact;

	ASSERT(THR_ISSIGOFF(curthread));

	for (; ; ) {
		if (_thr_lwpswanted != 0) {
			LOCK_COUNTER;
			counter = _thr_lwpswanted;
			_thr_lwpswanted = 0;
			UNLOCK_COUNTER;

			while (counter-- > 0) {
				/*
				 * Create as many LWPs as requested.
				 * If creation fails, simply give up
				 * as the system limit has probably
				 * been exceeded.
				 */
				PRINTF("_THR_HOUSEKEEPER making an LWP\n");
				if (_thr_new_lwp(NULL, _thr_age, NULL) != 0)
					break;
			}

			if ((!_thr_sigwaitingset) && 
			    (counter > _thr_nrunnable)) {
				/*
				 * Re-enable SIGWAITING handler.  The handler
				 * was disabled because a SIGWAITING signal 
				 * arrived after the housekeeper was activated.
				 * Since there are runnable threads, re-enable
				 * the handler now that the previous request
				 * for an LWP has been satisfied.
				 */
				sigwaitingact.sa_flags = SA_WAITSIG | 
				   SA_RESTART | SA_NSIGACT;
				sigwaitingact.sa_handler = 
				   __thr_sigwaitinghndlr;
				sigfillset(&sigwaitingact.sa_mask);
				_thr_sigdiffset(&sigwaitingact.sa_mask, 
				   &_thr_sig_programerror);
				rval = (*_sys__sigaction)(SIGWAITING, 
				   &sigwaitingact, NULL, _thr_sigacthandler);
				if (rval < 0) {
				        _thr_panic("_thr_housekeeper sigaction failed");
				}
				_thr_sigwaitingset = B_TRUE;

			}
		}
		_THR_SEMA_WAIT(&_thr_house_sema);
	}
}


#ifdef DEBUG

/*
 * _thr_showlazyq(void)
 *	dump the threads on the lazy switch sleep queue
 *
 * Parameter/Calling State:
 *	this is for internal debugging only
 *
 * Return Values/Exit State:
 *	none
 */

void 
_thr_showlazyq(void)
{
	thrq_elt_t *next;
	printf("lazyq contains: ");
	if (_thr_lazyq.thrq_next == NULL)
		printf("nothing\n");
	else {
		printf("\n 0x%x, n=0x%x, p=0x%x", _thr_lazyq.thrq_next, _thr_lazyq.thrq_next->thrq_next, _thr_lazyq.thrq_next->thrq_prev);
		next = _thr_lazyq.thrq_next;
		while (next != _thr_lazyq.thrq_prev) {
			printf("\n 0x%x, n=0x%x, p=0x%x", next->thrq_next, next->thrq_next->thrq_next, next->thrq_next->thrq_prev);
			next = next->thrq_next;
		}
	}
	printf("\n");
	fflush(stdout);
}
#endif /* DEBUG */

/*
 * thr_activate_lwp(int prio)
 *	activate a waiting lwp otherwise
 *
 * Parameter/Calling State:
 *	On entry, no locks are held but signal handlers are disabled.
 *
 * 	prio - priority of the thread to run
 *
 *	During processing, the lock of a thread to be preempted may be
 *	 acquired.
 *
 * Return Values/Exit State:
 *	Returns no value
 *
 *	On exit, no locks are held and signal handlers are still disabled.
 */

void
_thr_activate_lwp(int prio)
{
	thread_desc_t  *next, *curtp, *lowp;
	thread_desc_t  *nextthread;
	int lowprio, hashprio;
	thread_t lowtid;
	int rval;
	thrq_elt_t *first;
	struct sigaction sigwaitingact;


	/*
	 * Look for an LWP that is currently idling first.
	 */
	ASSERT(THR_ISSIGOFF(curthread));
	LOCK_RUNQ;
	if (_thr_nage) {
		rval = unblock((vaddr_t)&_thr_aging, 
				(char *)&(_thr_aging.wanted), UNBLOCK_ANY);
		if (rval == 0) {
			/*
			 * woke an LWP that was blocked on _thr_aging
			 */
			UNLOCK_RUNQ;
			return;
		} 
	}

	/*
	 * Next, look for an LWP associated with a lazy-switched thread.
	 */
	if (!(THRQ_ISEMPTY(&_thr_lazyq))) {
		/* found a lazy-switched thread */
		first = _thrq_elt_rem_first(&_thr_lazyq);
		ASSERT (first != NULL);
		(void)_lwp_cond_signal(&((lazyblock_t *)first)->condvar);
		/*
		 * We don't care if the thread was already off the LWP
		 * condition variable (i.e, the _lwp_cond_signal failed); 
		 * either way, it will be running.
		 */
		UNLOCK_RUNQ;
		return;
	}

	/*
	 * since there is a runnable thread that doesn't have a
	 * corresponding LWP, we enable the SIGWAITING handler if
	 * it's not already enabled.
	 */
	if ((_thr_sigwaitingset == B_FALSE)&&(_thr_sigwaiting_ok == B_TRUE)) {
		PRINTF("_thr_activate_lwp: enabling sigwaitinghndlr\n");
		sigwaitingact.sa_flags = SA_WAITSIG | SA_RESTART | SA_NSIGACT;
		sigwaitingact.sa_handler = __thr_sigwaitinghndlr;
		sigfillset(&sigwaitingact.sa_mask);
		_thr_sigdiffset(&sigwaitingact.sa_mask, &_thr_sig_programerror);
		rval = (*_sys__sigaction)(SIGWAITING, &sigwaitingact, NULL,
					  _thr_sigacthandler);
		if (rval < 0) {
			_thr_panic("_thr_activate_lwp sigaction failed");
		}
		_thr_sigwaitingset = B_TRUE;
	}
	UNLOCK_RUNQ;
	/*
	 * The following code searches for the lowest priority thread that is
	 * currently on an LWP and that has a priority value less than prio.
	 * If it finds such a thread, it will cause that thread to preempt
	 * itself.
	 *
	 * We only enter this code if the application uses more than one
	 * priority and prio is not equal to the lowest priority the
	 * application is using, or if the housekeeping thread is the one being
	 * activated.
	 *
	 * We hold the preemption lock to prevent other threads from making
	 * changes to _thr_idlethreads while we search that list (which might
	 * result in a segmentation violation)  and to prevent threads from
	 * being reaped by _thr_reaper()  while we're looking at their thread
	 * descriptors (since we don't obtain thread locks while doing this
	 * search -- reaping could also lead to a segmentation violation).
	 */
	HASH_PRIORITY(prio, hashprio);
	/*
	 * only need to try preemption if prio hashes to a value greater
	 * than _thr_userminpri AND either preemption has not been disabled
	 * by the application or we're activating the housekeeping thread
	 * (whose priority == MAXINT).
	 */

	if ((hashprio > _thr_userminpri) && 
	    ((_thr_preempt_ok == B_TRUE) || (prio == MAXINT))) {
		LOCK_PREEMPTLOCK;
		PRINTF1("_thr_activate_lwp: preempting for prio %d\n", prio);

		/*
		 * find the lowest priority multiplexed thread on an LWP
		 */
		next = _thr_idlethreads;
		nextthread =  &(((struct tls *) ((__lwp_desc_t *)
		     (next->t_lwpp))->lwp_thread)->tls_thread);
		/* 
		 * first, we must make sure next doesn't point to a thread that
		 * has already been preempted
		 */
		while ((nextthread->t_flags & T_PREEMPT) && 
		  (next->t_next != _thr_idlethreads)) {
			PRINTF2("_thr_activate_lwp: tid=%d; flag=%d; looping\n",
				nextthread->t_tid, nextthread->t_flags);
			next = next->t_next;
			nextthread =  &(((struct tls *) ((__lwp_desc_t *)
			   (next->t_lwpp))->lwp_thread)->tls_thread);
		}
		if (nextthread->t_flags & T_PREEMPT) {
			/*
			 * all threads have already been preempted
			 * so we can quit as our thread is already on
			 * the runnable queue and will get a fair chance
			 * at the LWPs that become available
			 */
			UNLOCK_PREEMPTLOCK;
			PRINTF1("_thr_activate_lwp: not preempting for %d\n",
				prio);
			return;
		}

		/*
		 * find lowest priority thread that hasn't already
		 * been preempted
		 */
		PRINTF3("_thr_activate_lwp 1st try: tid=%d; pri=%d; flags=%d\n",
		   nextthread->t_tid, nextthread->t_pri, 
		   nextthread->t_flags);
		lowprio = nextthread->t_pri;
		lowtid = nextthread->t_tid;
		do {
			if ((nextthread->t_pri < lowprio) &&
			   ((nextthread->t_flags & T_PREEMPT) == 0)) {
				lowprio = nextthread->t_pri;
				lowtid = nextthread->t_tid;
			}
			if (lowprio <= _thr_userminpri) {
				break;
			}
			next = next->t_next;
			nextthread =  &(((struct tls *) ((__lwp_desc_t *)
			   (next->t_lwpp))->lwp_thread)->tls_thread);
			PRINTF4("_thr_activate_lwp next: tid=%d; pri=%d; lowpri=%d; flags=%d\n",
			    nextthread->t_tid, nextthread->t_pri,
			    lowprio, nextthread->t_flags);
		} while (next != _thr_idlethreads);

		/*
		 * if the lowest priority multiplexed thread on an LWP
		 * has lower priority than prio, preempt that thread 
		 * unless it is an idle thread, in which case 
		 * we are finished as explained below.
		 */
		if ((lowprio < prio) && (lowtid > 0)) {
			PRINTF1("_thr_activate_lwp:  preempting %d\n", lowtid);
			curtp = curthread;
			if (lowtid == curtp->t_tid) {
				PRINTF("_thr_activate_lwp: preempting self\n");
				/*
				 * we're the lowest priority thread so 
				 * we're preempting ourself; it's cheaper
				 * to do it this way than to send ourself
				 * a signal
				 */
				LOCK_THREAD(curtp);
				UNLOCK_PREEMPTLOCK;
				PREEMPT_SELF(curtp);
			} else {
				lowp = _thr_get_thread(lowtid);
				if (lowp != NULL) {
					PREEMPT_THREAD(lowp);
					UNLOCK_THREAD(lowp);
					PRINTF2("_thr_activate_lwp: just preempted %d; flags=%d\n",
						lowp->t_tid, lowp->t_flags);
				}
				UNLOCK_PREEMPTLOCK;
				/*
				 * if _thr_get_thread returned NULL, it
				 * means that the thread we wanted to give
				 * up its LWP has already done so.  In this
				 * case, we are done.
				 */
			}
		} else { /* either (lowtid <= 0) or (lowprio > prio) */
			/*
			 * lowtid is either an idle thread or a thread with
			 * priority greater than prio.  In either case we
			 * can return.  
			 *
			 * If it is an idle thread, we must mark it as
			 * preempted to make sure that the next thread
			 * through the preemption code doesn't also find
			 * this idle thread and assume that it is done.
			 * (Note that if it is an idle thread, it either
			 * got on the list after we put our thread on the
			 * runnable queue, meaning our thread must have
			 * already been picked up by an LWP, or it was
			 * on its LWP before and was in the process of
			 * switching off, i.e., it had already decremented
			 * _thr_nage and was calling _thr_swtch().  In the
			 * former case, no action is necessary; in the latter
			 * case, we just need to make sure we're the only
			 * thread that relies on this idle thread's LWP
			 * to serve our thread.  We accomplish this by
			 * setting it T_PREEMPT flag.
			 *
			 * If lowtid's priority is greater than prio there
			 * are no runnable threads of lower priority than
			 * the thread we're trying to get an LWP for, so 
			 * preemption is not possible.
			 */
			PRINTF1("_thr_activate_lwp: cannot preempt for %d\n", 
				prio);
			if (lowprio == -1) {
				/*
				 * there is no lock we can hold to protect
				 * the setting of the idle thread's T_PREEMPT
				 * flag.  This flag is cleared when a thread
				 * switches off its LWP, so if we set the
				 * flag after the switch the flag will stay
				 * set when it shouldn't be.  This is not so
				 * bad, though.  In the worst case, this LWP
				 * will be considered as already preempted
				 * in a subsequent preemption search and
				 * this will cause an unnecessary
				 * preemption if the window is caught where
				 * that particular idle thread has already
				 * decremented _thr_nage but hasn't switched
				 * off its LWP yet.
				 */ 
				nextthread =  &(((struct tls *) 
				  ((__lwp_desc_t *)
				  (next->t_lwpp))->lwp_thread)->tls_thread);
				if ((nextthread->t_tid) == -1)
					nextthread->t_flags |= T_PREEMPT;
				PRINTF1("LWP %d is still idle\n", 
				  ((__lwp_desc_t *)next->t_lwpp)->lwp_id);
#ifdef THR_DEBUG
			} else {
				PRINTF("lowprio > prio\n");
#endif
			}
			UNLOCK_PREEMPTLOCK;
		} 
#ifdef THR_DEBUG
	} else { /*
		  * either preemption is turned off or prio == _thr_userminpri
		  */
		PRINTF1("_thr_activate_lwp: not preempting for %d\n", prio);
#endif
	}
}

/*
 * void
 * _thr_prioq_init(int old, int new)
 *	initializes _thr_userpriarray[].
 *	It is called by thr_setscheduler() and thr_setprio() and thr_create().
 *	The first argument is the old priority of the thread being changed.
 *	The second argument is the new priority of the thread.
 *
 * Parameter/Calling State:
 *      On entry, _thr_counterlock is held and signal handlers are disabled.
 *
 *      During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *      On exit, _thr_counterlock is still held and signal handlers are 
 *	still disabled; no value is returned.
 *
 *	The priority array is initialized and _thr_preempt_ok is set to 
 *	true if more than 1 multiplexed thread exists.  The value of 
 *	_thr_userminpri is updated when necessary.
 */
void
_thr_prioq_init(int old, int new) 
{
	ASSERT(_thr_userpricnt == 1);
	ASSERT(_thr_userminpri == old);
	ASSERT(old != new);
	if (_thr_nthreads > 1) {
		(void)memset((void *)_thr_userpriarray, 
		    0, (DISPQ_SIZE * sizeof(int)));
		_thr_userpricnt = 2;
		_thr_userpriarray[new] = 1;
		_thr_userpriarray[old] = _thr_nthreads - 1;
		if (new < old)
			_thr_userminpri = new;
		_thr_preempt_ok = B_TRUE;
	} else { 
		/*
		 * It's possible that 'new' is the only multiplexed thread
		 * in the process (i.e., a bound thread has just created it).
		 * In this case, we only need to update _thr_userminpri.
		 */
		_thr_userminpri = new;
	}
}

/*
 * int
 * _thr_getminpri(int current) 
 *	obtains the priority of the lowest priority multiplexed thread 
 * 	in the process via a search of _thr_userpriarray[].
 *	Search of the array starts at the index above 'current'.
 *
 *	This function is called from the macro REM_FROM_PRIO_ARRAY(), which
 *	is called by thr_exit(), thr_setscheduler() and thr_setprio().
 *
 * Parameter/Calling State:
 *      _thr_counterlock is held on entry and signal handlers are disabled.
 *
 *      During processing, no locks are acquired.
 *
 *	On exit, _thr_counterlock is still held and signal handlers are
 *	still disabled.
 *
 * Return Values/Exit State:
 *      Returns the priority of the lowest priority multiplexed thread
 *	currently in the process.
 */
int
_thr_getminpri(int current) 
{
	int y;

	ASSERT(IS_COUNTER_LOCKED);
	ASSERT((current >= 0) && (current < (DISPQ_SIZE - 1)));

#ifdef DEBUG
	for (y = 0; y <= current; y++)
		ASSERT(_thr_userpriarray[y] == 0);
#endif /* DEBUG */

	for (y = (current + 1); y < (DISPQ_SIZE); y++)
		if (_thr_userpriarray[y])
			break;
	ASSERT(_thr_userpriarray[y] > 0);
	return(y);
}

/*
 *  int
 *  thr_suspend(thread_t target_thread)
 *      This function is used to suspend the execution of a thread.
 *      If the specified thread exists, this function does not return
 *      until the specified thread is suspended.
 *
 *  Parameter/Calling State:
 *      On entry, no locks are held.
 *
 *      first arguement is the thread id of the thread to be suspended.
 *
 *      During processing, _thr_tidveclock, target_thread's thread lock,
 *      and, possibly, the lock of the queue on which target_thread sleeps
 *      and the runnable queue lock are acquired.
 *
 *  Return Values/Exit State:
 *
 *      If the function was successful, 0 is returned and the thread with
 *      ID target_thread is in the suspended state.
 *      If the function was unsuccessful, it is because no thread with ID
 *      target_thread was found; in this case, ESRCH is returned.
 *
 *      On exit, no locks are held.
 */

int
thr_suspend(thread_t target_thread)
{
	thread_desc_t   *curtp = curthread;
	thread_desc_t   *susptp; /* thread to be suspended */
	int rval;
	int wakeflag = B_FALSE; /* if B_TRUE, indicates call to reaper */
	int went_through_loop;  /* used to manage join count */

	PRINTF("In thr_suspend...\n");
	_thr_sigoff(curtp);             /* disable signal handlers */

	TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_FIRST,
	   target_thread); 

	if ((susptp = _thr_get_thread(target_thread)) == NULL)  {
		TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_SECOND,
		   ESRCH); 
		_thr_sigon(curtp);
		return(ESRCH);          /* thread not found */
	}

	if (susptp->t_state == TS_ZOMBIE) { /* thread has already exited */
		UNLOCK_THREAD(susptp);
		TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_SECOND,
		   ESRCH); 
		_thr_sigon(curtp);
		return(ESRCH);
	}

	/*
	 * It is useful to determine if the specified thread happens to
	 * also be the calling thread.
	 * Much less overhead is required to suspend the calling
	 * thread than to suspend another thread as no checks need
	 * to be made concerning critical code and no signals need to be sent.
	 * Problems can develop if the calling thread suspends itself while
	 * holding a critical resource; there is no way to detect or
	 * resolve this, so care must be taken to avoid it.
	 */
	if (curtp == susptp) {
		PRINTF("In thr_suspend - same thread...\n");
		susptp->t_state = TS_SUSPENDED;
		if (_thr_debug.thr_debug_on) {
			_thr_debug_notify(susptp, tc_thread_suspend_pending);
		}
		if (susptp->t_joincount) {
			(void) _THR_COND_BROADCAST(&(susptp->t_join));
		}

		if (ISBOUND(susptp)) {
			if (_thr_debug.thr_debug_on) {
				_thr_debug_notify(susptp, tc_thread_suspend);
			}
			/*
			 * SUSPEND_BOUND_THREAD does not release the thread
			 * lock so we must do it explicitly afterwards
			 */
			SUSPEND_BOUND_THREAD(susptp);
			UNLOCK_THREAD(susptp);
		} else {
			_thr_swtch(1, curtp);
			ASSERT(curtp->t_state == TS_ONPROC);
		}
		TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_SECOND,
		   0); 
		_thr_sigon(curtp);
		return(0);
	}

	PRINTF("In thr_suspend - Not same thread...\n");
	/* target_thread is not the calling thread */
	if (susptp->t_state == TS_SUSPENDED) {
		UNLOCK_THREAD(susptp);
		TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_SECOND,
		   0); 
		_thr_sigon(curtp);
		return(0);
	}
	/*
	 * The target thread's t_suspend flag is checked because it
	 * may be TSUSP_PENDING, indicating a suspension is already pending.
	 *
	 * In the case where no suspension is pending, a SIGLWP is
	 * sent. There are three cases:
	 *
	 * TS_ONPROC -  the thread is running on an lwp, an lwp_kill will
	 *              be sent.
	 * TS_SLEEP -   the thread is sleeping on a sync primitive, post
	 *              the signal and set it running, the thread will
	 *              suspend itself and send the cond_broadcast when it
	 *              is done. The code is called from _thr_sigon.
	 * TS_RUNNABLE -The thread is runnable, when it runs _thr_sigon will
	 *              do the right thing since the call to _thr_sendsig
	 *              will just post the existence of SIGLWP.
	 */

	if (_thr_debug.thr_debug_on) {
		_thr_debug_notify(susptp, tc_thread_suspend_pending);
	}

	if (susptp->t_suspend == TSUSP_CLEAR) {
		PRINTF("In thr_suspend - Sending siglwp...\n");
		susptp->t_suspend = TSUSP_PENDING;
		_thr_sigsend(susptp, SIGLWP);
	}

	/*
	 * The variable went_through_loop is used to manage the field
	 * t_joincount.
	 */
	went_through_loop = B_FALSE;

	while ((susptp->t_suspend == TSUSP_PENDING) &&
	      (susptp->t_state != TS_ZOMBIE)) {
		if (!went_through_loop) {
			/*
			 * The variable t_joincount indicates the number
			 * of threads waiting on the thread's t_join
			 * condition variable and is used to prevent
			 * the thread from being reaped while other
			 * threads are waiting on that condition variable.
			 */
			susptp->t_joincount++;
			went_through_loop = B_TRUE;
		}


		rval = _thr_cond_wait(&(susptp->t_join), (mutex_t *)NULL,
					&(susptp->t_lock),
					(const timestruc_t *) NULL, B_TRUE,
					B_TRUE);
		/*
		 * The call to cond_wait() cannot return EINVAL.
		 * In thr_exit, the exiting thread's state is set to
		 * TS_ZOMBIE before the condition variable is destroyed
		 * and this is all done under protection of the thread's
		 * lock.  Since the thr_suspend code holds the same lock
		 * and checks that state is not zombie, it is therefore
		 * assured that the condition variable is not destroyed, too.
		 */
		ASSERT(rval != EINVAL);
	}
	/*
	 * At this point, suspension is no longer pending.
	 *
	 * If target thread's t_suspend flag was not TSUSP_PENDING,
	 * it means that the target thread completed suspending itself,
	 * the target thread has called thr_exit(), or a
	 * thr_continue() negated the effect of this thr_suspend().
	 *
	 * It is necessary to decrement the target thread's t_joincount
	 * variable if it was incremented earlier.  After that, a check
	 * is made to see if the target thread is a zombie.
	 * In this case, if the target thread is detached and the
	 * calling thread has just set its t_joincount counter to 0,
	 * then it is the responsibility of the calling thread to place
	 * the target thread on _thr_reaplist.  If the calling thread
	 * does place the target thread on _thr_reaplist, it calls
	 * _thr_reaper() if the number of reapable threads is
	 * sufficiently high, as determined by _thr_put_on_reaplist().
	 */

	if (went_through_loop) {
		susptp->t_joincount--;
	}

	/*
	 * If the thread is a zombie then suspend failed since the thread
	 * has exited.
	 */
	if (susptp->t_state == TS_ZOMBIE) {
		rval = ESRCH;

		if ((susptp->t_joincount == 0) &&
		    (susptp->t_usropts & THR_DETACHED) &&
		    went_through_loop) {
			LOCK_COUNTER;
			wakeflag = _thr_put_on_reaplist(susptp);
			UNLOCK_THREAD(susptp);
			UNLOCK_COUNTER;
		}
	} else {                          /* suspend succeeded */
		rval = 0;
		UNLOCK_THREAD(susptp);
	}
	if (wakeflag == B_TRUE) {
		_thr_reaper();
	}

	TRACE1(curtp, TR_CAT_THREAD, TR_EV_THR_SUSPEND, TR_CALL_SECOND, rval); 
	_thr_sigon(curtp);
	PRINTF1("Leaving thr_suspend rval = %d\n",rval);
	return(rval);
}

/*
 * void
 * _thr_preempt_disable()
 *      sets a flag to indicate that preemption should not be attempted.
 *	Note, turning off preemption is NOT reversible; there is no
 *	_thr_preempt_enable() function.  Once preemption is disabled,
 *	no record is kept of thread priorities in the process in order
 *	to avoid performance penalties, so re-enabling preemption would 
 *	find the library in an unknown state with regard to thread
 *	priorities.
 *
 * Parameter/Calling State:
 *      takes no arguments; no locks are needed since this is a one-time
 *	toggle.
 *
 * Return Values/Exit State:
 *      returns no value
 */

void
_thr_preempt_disable(void)
{
	_thr_preempt_off = B_TRUE;
	PRINTF("_thr_preempt_disable: disabling preemption\n");
}

/*
 * void
 * _thr_sigwaiting_disable()
 *      sets a flag to indicate that the SIGWAITING handler should not
 *	be enabled.  Note, this does NOT turn off the handler if it
 *	already is enabled.
 *
 * Parameter/Calling State:
 *      takes no arguments
 *
 * Return Values/Exit State:
 *      returns no value
 */

void
_thr_sigwaiting_disable(void)
{
	_thr_sigwaiting_ok = B_FALSE;
	PRINTF("_thr_sigwaiting_disable: disabling SIGWAITING\n");
}

/*
 * void
 * _thr_sigwaiting_enable()
 *      sets a flag to indicate that the SIGWAITING handler may
 *	be enabled.  Note, this does NOT turn on the handler if it
 *	is not currently enabled.
 *
 * Parameter/Calling State:
 *      takes no arguments
 *
 * Return Values/Exit State:
 *      returns no value
 */

void
_thr_sigwaiting_enable(void)
{
	_thr_sigwaiting_ok = B_TRUE;
	PRINTF("_thr_sigwaiting_enable: enabling SIGWAITING\n");
}


/*
 * void
 * thr_dumpreaplist()
 *      prints the IDs of all threads currently on _thr_reaplist
 *
 * Parameter/Calling State:
 *      takes no arguments
 *
 * Return Values/Exit State:
 *      returns no value
 */

void
_thr_dumpreaplist(void)
{
#ifdef THR_DEBUG
	lwppriv_block_t *lwpp;
#endif
	thread_desc_t *temp;
	int ctr = 0;

	if (_thr_reaplist != NULL) {
		PRINTF("reaplist follows:\n");
		temp = _thr_reaplist;
		do {
			ctr++;
			;
			PRINTF5(
		" thread %d = %d, tls = 0x%x, lwp_thread=0x%x, lwp_id = %d\n",
			  ctr, temp->t_tid, temp->t_tls,
	        (lwpp = (lwppriv_block_t *)temp->t_lwpp)->l_lwpdesc.lwp_thread,
			  lwpp->l_lwpdesc.lwp_id);
			temp = temp->t_next;
		} while (temp != _thr_reaplist);
#ifdef THR_DEBUG
	} else {
		PRINTF("_thr reaplist is NULL\n");
#endif
	}
}

/*
 * void
 * thr_dumpjoinlist()
 *      prints the IDs of all threads currently on _thr_joinablelist
 *
 * Parameter/Calling State:
 *      takes no arguments
 *
 * Return Values/Exit State:
 *      returns no value
 */

void
_thr_dumpjoinlist(void)
{
#ifdef THR_DEBUG
	lwppriv_block_t *lwpp;
#endif
	thread_desc_t *temp;
	int ctr = 0;

	if (_thr_joinablelist != NULL) {
		PRINTF("joinablelist follows:\n");
		temp = _thr_joinablelist;
		do {
			ctr++;
			;
			PRINTF5(
		" thread %d = %d, exitval = %d, lwp_thread=0x%x, lwp_id = %d\n",
			  ctr, temp->t_tid, temp->t_exitval,
		 (lwpp = (lwppriv_block_t *)temp->t_lwpp)->l_lwpdesc.lwp_thread,
			  lwpp->l_lwpdesc.lwp_id);
			temp = temp->t_next;
		} while (temp != _thr_joinablelist);
#ifdef THR_DEBUG
	} else {
		PRINTF("_thr_joinablelist is NULL\n");
#endif
	}
}


/*
 * thr_dump_thread(thread_desc_t *tp)
 *	prints all the fields of a thread structure
 *
 * Parameter/Calling State:
 *	tp - the thread whose fields are to be printed
 *
 * Return Values/Exit State:
 *	returns no value
 */

void
_thr_dump_thread(thread_desc_t *tp)
{
	printf("t_thrq_elt.thrq_next   = 0x%x\t", tp->t_thrq_elt.thrq_next);
	printf("t_thrq_elt.thrq_prev   = 0x%x\n", tp->t_thrq_elt.thrq_prev);
	printf("t_sync_addr            = 0x%x\t",tp-> t_sync_addr);
	printf("t_sync_type            = %d\n", tp->t_sync_type);    
	printf("t_next                 = 0x%x\t", TNEXT(tp));         
	printf("t_prev                 = 0x%x\n", tp->t_prev);         

	printf("t_ucontext    = ....\t");     
	printf("t_tls         = 0x%x\n", tp->t_tls);          
	printf("t_tid         = %d\t", tp->t_tid);          
	printf("t_lwpp        = 0x%x\n", tp->t_lwpp);	
	printf("t_state       = %d\t", tp->t_state);        
	printf("t_usropts     = %d\n", tp->t_usropts);      
	printf("t_flags       = %d\t", tp->t_flags);        
	printf("t_pri         = %d\n", tp->t_pri);          
	printf("t_cid         = %d\t", tp->t_cid);          
	printf("t_usingfpu    = %d\n", tp->t_usingfpu);     
	printf("t_suspend     = %d\t", tp->t_suspend);      
	printf("t_exitval     = 0x%x\n",tp->t_exitval);	
	printf("t_idle        = 0x%x\t", tp->t_idle);	
	printf("t_stk         = 0x%x\n", tp->t_ucontext.uc_stack.ss_sp);
	printf("t_stksize     = 0x%x\t", tp->t_ucontext.uc_stack.ss_size);
	printf("t_lwpid       = 0x%x\n", LWPID(tp));
	printf("t_sp          = 0x%x\t",tp->t_ucontext.uc_mcontext.gregs[R_ESP]);
	printf("t_pc          = 0x%x\n", tp->t_ucontext.uc_mcontext.gregs[R_EIP]);

	printf("t_lock        = 0x%x\t", &tp->t_lock);         
	printf("t_join        = 0x%x\t", &tp->t_join);         
	printf("t_joincount   = %d\n", tp->t_joincount);	

	printf("t_nosig       = %d\t", tp->t_nosig);        
	printf("t_sig         = %d\n", tp->t_sig);          
	printf("t_hold        = 0x%x and 0x%x\t", tp->t_hold.sa_sigbits[0], tp->t_hold.sa_sigbits[1]);
	printf("t_psig        = 0x%x and 0x%x\n", tp->t_psig.sa_sigbits[0], tp->t_psig.sa_sigbits[1]);
	printf("t_oldmask     = 0x%x and 0x%x\n", tp->t_oldmask.sa_sigbits[0], tp->t_oldmask.sa_sigbits[1]);

	printf("t_callo_alarm.co_stat = %d\n",tp->t_callo_alarm.co_stat);
	printf("t_callo_alarm.co_abstime = %ld and %ld\t",tp->t_callo_alarm.co_abstime.tv_sec, tp->t_callo_alarm.co_abstime.tv_nsec);
	printf("t_callo_alarm.co_interval = %ld and %ld\n",tp->t_callo_alarm.co_interval.tv_sec, tp->t_callo_alarm.co_interval.tv_nsec);

	printf("t_callo_realit.co_stat = %d\n",tp->t_callo_realit.co_stat);
	printf("t_callo_realit.co_abstime = %ld and %ld\t",tp->t_callo_realit.co_abstime.tv_sec, tp->t_callo_realit.co_abstime.tv_nsec);
	printf("t_callo_realit.co_interval = %ld and %ld\n",tp->t_callo_realit.co_interval.tv_sec, tp->t_callo_realit.co_interval.tv_nsec);

	printf("t_callo_cv.co_stat = %d\n",tp->t_callo_cv.co_stat);
	printf("t_callo_cv.co_abstime = %ld and %ld\t",tp->t_callo_cv.co_abstime.tv_sec, tp->t_callo_cv.co_abstime.tv_nsec);
	printf("t_callo_cv.co_interval = %ld and %ld\n",tp->t_callo_cv.co_interval.tv_sec, tp->t_callo_cv.co_interval.tv_nsec);
	_thr_binding_print(&tp->t_binding);
}


/*
 * thr_dump_allthreads()
 *	prints the addresses of thread structures of all threads currently
 *	in a process
 *
 * Parameter/Calling State:
 *	takes no arguments
 *
 * Return Values/Exit State:
 *	returns no value 
 */


void
_thr_dump_allthreads(void)
{
	thread_desc_t *temp;
	char ans[3];

	if (_thr_allthreads == (void *)0) {
		printf("_thr_dump_allthreads: _thr_allthreads == 0\n");
		fflush(stdout);
		return;
	}
	temp = _thr_allthreads;
	printf("_thr_dump_allthreads:curthread = 0x%x\n", curthread);
	printf("Start of _thr_allthreads list: _thr_allthreads = 0x%x\n", 
					_thr_allthreads); 
	do {
		printf("thread descriptor ptr = 0x%x\n", temp);
		printf("Display descriptor (yes/no or q to quit) ? ");
		scanf("%s", ans);
		if (ans[0] == 'y' || ans[0] == 'Y') {
		_thr_dump_thread(temp);
		} else if (ans[0] == 'q' || ans[0] == 'Q') {
			return;
		}
	} while((temp = TNEXT(temp)) != _thr_allthreads); 
	printf("End of _thr_allthreads list: _thr_allthreads = 0x%x\n", 
			_thr_allthreads);
}
