/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/disp.c	1.5.18.24"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file contains functions used to multiplex threads and LWPs 
 * and to control the run queue
 */

#include <stdio.h>
#include <libthread.h>
#include <memory.h>

/*
 * Reserve one bucket for all threads with priorities > THREAD_MAX_PRIORITY
 * or less than THREAD_MIN_PRIORITY
 */
#define DEBUG 1

thrq_elt_t _thr_runnable[DISPQ_SIZE];/* the queue of runnable threads */

long _thr_dqactmap[(DISPQ_SIZE/BPW)];	/* bit map of priority queues */
int _thr_maxpriq = -1;			/* index of highest priority dispq */
int _thr_minpri = THREAD_MIN_PRIORITY;	/* value of lowest pri running thread */
int _thr_nrunnable = 0;			/* number of threads on the run queue */
int _thr_nthreads = 1;			/* number of unbound threads */
int _thr_totalthreads = 1;		/* total number of visible threads */
int _thr_daemonthreads = 0;		/* total daemonthreads		*/
int _thr_nage = 0;			/* number of aging lwps */
int _thr_dontkill_lwp = 0;              /* number of aging lwps to age again */
int _thr_nlwps = 1;			/* number of lwps in the pool */
int _thr_minlwps = 0;			/* min number of lwps in pool */
int _thr_reapable = 0;			/* number of unreaped detached threads*/

lwp_cond_t _thr_aging;			/* signalled via _thr_activate_lwp() */
timestruc_t _thr_idletime = {60*5, 0};	/* idle for 5 minutes then die */
/*
 * int
 * _thr_setrq(thread_desc_t *tp, int calling_thread)
 *
 *      This function either activates a thread tp that is lazy-switched
 *	on an LWP or places the thread descriptor indicated by tp on the
 *      appropriate sub-queue of the runnable queue, depending on the 
 *	priority of tp.
 *
 * Parameter/Calling State:
 *      tp - address of the thread descriptor to be made runnable.
 *	calling_thread - 0 if tp is not calling thread,
 *	                 1 if tp is calling thread, or
 *	                 2 if it's not known whether tp is calling thread
 *
 *      The thread lock of tp must be held and signal handlers
 *      must be disabled upon entry.
 *
 *      The lock that protects the runnable queue will be acquired
 *      during processing.
 *
 * Return Values/Exit State:
 *      Returns INVALID_PRIO if activated thread does not need an LWP, 
 *	otherwise returns the priority argument that should be passed
 *	to _thr_activate_lwp().  tp's lock is still held, signal handlers
 *      are still disabled, and tp is either activated on its LWP or
 *	placed on the runnable queue.
 */
int
_thr_setrq(thread_desc_t *tp, int calling_thread)
{
	register thrq_elt_t *dq;
	int qx, rval;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(IS_THREAD_LOCKED(tp));
	ASSERT((tp)->t_pri != -1);
	ASSERT(tp->t_thrq_elt.thrq_next == NULL);
	ASSERT(tp->t_thrq_elt.thrq_next == tp->t_thrq_elt.thrq_prev);
	/* ASSERT(tp->t_state != TS_RUNNABLE); */
	ASSERT(tp->t_sync_type == TL_NONE);
	ASSERT(tp->t_sync_addr == NULL);


	LOCK_RUNQ;

	/*
	 * First, check to see if the thread is lazy-switched on an LWP.
	 * If so, all that is necessary is to set the state to runnable
	 * and, possibly, signal the condition variable on which the 
	 * thread sleeps.
	 */
	if (tp->t_blockedonlwp == B_TRUE) {
		tp->t_state = TS_RUNNABLE;
		if (!(THRQ_ISEMPTY((thrq_elt_t *)&(tp->t_lazyswitch)))) {
			_thrq_elt_rem_from_q((thrq_elt_t *)&(tp->t_lazyswitch));
			(void)_lwp_cond_signal(&(tp->t_lazyswitch.condvar));
			rval = INVALID_PRIO;
		} else {
			/*
			 * If the thread wasn't on _thr_lazyq, it was
			 * activated for another thread.  However, since it
			 * is now runnable, it will remain on its LWP.  
			 * Therefore, we must activate another LWP for the 
			 * thread for whom this thread was originally activated.
			 * Since we don't know what priority that thread was,
			 * we must assume the current highest priority on the
			 * runnable queue.
			 */
			rval = (((thread_desc_t *)
			   (_thr_runnable[_thr_maxpriq].thrq_next))->t_pri);
		}
		UNLOCK_RUNQ;
		PRINTF1("_thr_setrq:  returning priority value %d\n", rval);
		return(rval);
	}

	/*
	 * Check to see if tp is on an LWP.  If it is on an LWP and
	 * it's not the calling thread, it already has an LWP and we
	 * can return.  This check depends on _thr_resume zeroing out
	 * the t_lwpp field in non-zombie user threads during context
	 * switch.
	 */
	if ((tp->t_lwpp != 0) && 
	    ((calling_thread == 0) || 
	     ((calling_thread == 2) && (tp != curthread)))) {
		UNLOCK_RUNQ;
		return(INVALID_PRIO);
	}

	/*
	 * If we get here, we'll need an LWP since the thread was not
	 * lazy-switched and so must go on the runnable queue.
	 */
	PRINTF1("_thr_setrq: placing thread %d on _thr_runnableq\n", tp->t_tid);
	rval = tp->t_pri;

        /*
         * HASH_PRIORITY assigns to qx the index of the subqueue of
         * the runnable queue on which tp should be placed.
         */
	HASH_PRIORITY(tp->t_pri, qx);
	tp->t_state = TS_RUNNABLE;
	PRINTF4("_thr_setrq: tp->t_tid = %d; tp->t_pri = %d; maxpriq = %d; nrun = %d\n",
	tp->t_tid, tp->t_pri, _thr_maxpriq, _thr_nrunnable);
	if (qx > _thr_maxpriq) {
		/* then tp is the new highest priority runnable thread */
		_thr_maxpriq = qx;
	}
	dq = &_thr_runnable[qx];
	if (THRQ_ISEMPTY(dq)) {
		/* queue empty */
		_thrq_add_end(dq, tp);
		_thr_dqactmap[(qx/BPW)] |= 1<<(qx & (BPW-1));
	} else {
		if (qx < DISPQ_SIZE - 1) {
			/* priorities within normal range map to FIFO queues */
			_thrq_add_end(dq, tp);
		} else {
			/*
			 * priorities outside normal range map to last
			 * priority bucket and are inserted in priority
			 * order.
			 */
			_thrq_prio_ins(dq, tp);
		}
	}
	_thr_nrunnable++;
	UNLOCK_RUNQ;
	PRINTF2("_thr_setrq on return: maxpriq = %d; nrun = %d\n",
	_thr_maxpriq, _thr_nrunnable);
	return(rval);
}

/*
 * thread_desc_t *
 * _thr_disp(void)
 *
 *      This function removes the thread from the front of the
 *      highest priority non-empty subqueue on the runnable queue
 *      and returns a pointer to that thread.  If no threads are
 *      on the runnable queue, a pointer to the calling LWP's
 *      idle thread is returned.
 *
 * Parameter/Calling State:
 *      The thread lock of calling thread must be held and signal
 *      handlers must be disabled upon entry.
 *
 *      The lock that protects the runnable queue will be acquired
 *      during processing.
 *
 * Return Values/Exit State:
 *      A pointer to the thread removed from the runnable queue is
 *      returned, the calling thread's lock is still held, and signal
 *      handlers are still disabled.
 */
thread_desc_t *
_thr_disp(thread_desc_t *ctp)
{
	register thread_desc_t *t=NULL;
	register thrq_elt_t *dq;
	register volatile int runword, hb;
	register volatile long bitmap;
	volatile int done = 0;
	volatile int rval;
	int hashprio;

	ASSERT(IS_THREAD_LOCKED(ctp));
	ASSERT(THR_ISSIGOFF(ctp));
	LOCK_RUNQ;
	/*
	 * The following loop allows for possible corruption in the
	 * runnable queue.  If a thread is expected to be found on
	 * a subqueue of the runnable queue but no thread is actually
	 * found, the runnable queue, the bitmap, and _thr_maxpriq
	 * are updated by the function _thr_fix_runq() and the loop is
	 * repeated.  
	 */
	while (!done) {

		/*
		 * The next block of code is the lazy switch code.
		 * Though the calling thread currently doesn't need its
		 * LWP, it's desirable to stay on the LWP unless another
		 * thread needs it.
		 * 
		 * If the thread is in the sleep state and there are no
		 * runnable threads, we place the thread on the lazy 
		 * switch queue, increment the lazy switch counter and block
		 * on our LWP, except in the case of condition variables.
		 *
		 * We don't switch for threads blocking on condition
		 * variables because if a lazy-switched thread were
		 * awakened during a cond_broadcast, there would be no
		 * way to remove it from the temporary queue that it would
		 * have been placed on by cond_broadcast; thus, if the
		 * thread tried to block before it was removed from the
		 * temporary queue, we'd have a problem since we wouldn't
		 * be able to put it on the new queue until it was removed
		 * from the old one.
		 *
		 * We don't lazy switch for threads in the suspended state
		 * because we don't want them to process signals; it's
		 * possible that process directed signals could put them
		 * into a signal handler.  We don't lazy switch for zombie
		 * threads since we want to be able to reclaim their stacks.
		 */
		while ((!_thr_nrunnable) && (ctp->t_state == TS_SLEEP) &&
			(ctp->t_sync_type != TL_COND)) {

			ctp->t_blockedonlwp = B_TRUE;
			UNLOCK_THREAD(ctp);
			ASSERT(THRQ_ISEMPTY((thrq_elt_t *)&(ctp->t_lazyswitch)));
			/*
			 * It's safe to release the thread lock before
			 * enqueueing the thread since we still hold the
			 * RUNQ lock and the queue can't be changed by
			 * another thread that doesn't hold the RUNQ lock.
			 */
			_thrq_elt_add_end(&_thr_lazyq, 
			    (thrq_elt_t *)&ctp->t_lazyswitch);
			rval = _lwp_cond_wait(&(ctp->t_lazyswitch.condvar), 
			        &_thr_runnableqlock);
			PRINTF1("_thr_disp: %d lazy_switched thread awakened\n",
			   ctp->t_tid);
			/*
			 * Reacquire the thread lock.
			 * We can't block for the thread lock while holding
			 * the runnable queue lock since it could lead to
			 * deadlock; this would be a violation of the locking
			 * hierarchy.  Therefore, we test the lock first.  If
			 * it's already held, we release the runnable queue
			 * lock, then get both locks in the correct order.
			 */
			if (TRYLOCK_THREAD(ctp) == EBUSY) {
				UNLOCK_RUNQ;
				LOCK_THREAD(ctp);
				LOCK_RUNQ;
			}
			ctp->t_blockedonlwp = B_FALSE;

			/* remove ctp from lazyq if it's still on it */
			if (!(THRQ_ISEMPTY(
			    (thrq_elt_t *)&(ctp->t_lazyswitch)))){
				_thrq_elt_rem_from_q(
				   (thrq_elt_t *)&(ctp->t_lazyswitch));
			}

			/*
			 * If the calling thread is runnable, return
			 * immediately.  This could result from getting
			 * the sync object we want or from receiving a
			 * thread-directed signal.
			 *
			 * If this thread was activated because another 
			 * thread became runnable, the other thread will
			 * get an LWP as a result of action taken by the
			 * thread that made this thread runnable.
			 */
			if (ctp->t_state == TS_RUNNABLE) {
				if (_thr_nrunnable == 0) {
					UNLOCK_RUNQ;
					return(ctp);
				}
				/*
				 * If we get here, it means that
				 * another thread is also runnable; if that
				 * thread is higher priority than this one,
				 * we give up our LWP to it.
				 */
				HASH_PRIORITY(ctp->t_pri, hashprio);
				if (hashprio >= _thr_maxpriq) {
					UNLOCK_RUNQ;
					return(ctp);
				}
				/*
				 * If we get here, the other thread has a
				 * higher priority.   Since we're now
				 * runnable, we must place ourself on the
				 * runnable queue before yielding our LWP.
				 * 
				 * The following is streamlined code from
				 * _thr_setrq().
				 */
				dq = &_thr_runnable[hashprio];
				if (THRQ_ISEMPTY(dq)) {
					_thrq_add_end(dq, ctp);
					_thr_dqactmap[(hashprio/BPW)] |=
					   1<<(hashprio & (BPW - 1));
				} else {
					if (hashprio < DISPQ_SIZE - 1) {
						_thrq_add_end(dq, ctp);
					} else {
						_thrq_prio_ins(dq, ctp);
					}
				}
				_thr_nrunnable++;
				break;
			}

			/*
			 * If we received a signal, we must return to base
			 * code to process the signal.  Since we are not
			 * runnable, this means we received a process-directed
			 * signal or SIGWAITING.  Thread-directed signals
			 * would have made us runnable.
			 */
			if (rval == EINTR) {
				UNLOCK_RUNQ;
				switch(ctp->t_sync_type) {
				case TL_MUTEX:
					(void)_thr_remove_from_mutex_queue(ctp);
					break;
				case TL_LIBC:
					(void)_thr_remove_from_libc_queue(ctp);
					break;
				case TL_COND:
					/*
					 * this shouldn't happen since we do not
					 * lazy switch while blocked on a
					 * condition variable.
					 */
				case TL_NONE:
				default:
					_thr_panic("_thr_disp: bad list type");
					break;
				}
				/*
				 * Our above attempt to remove ourself from
				 * our sleep queue may have failed if another
				 * thread is in the process of waking us up
				 * for the related synchronaization event.
				 * Normally, we'd like to wait for the
				 * thread that dequeued us to change 
				 * our state to runnable but that is 
				 * awkward here since it's not clear 
				 * what we should do in the meantime.
				 * If we go back and sleep on the
				 * condition variable, another thread
				 * may request our LWP; if we don't,
				 * the dequeuing thread won't know that
				 * we're on our LWP and will try to put
				 * us on the runnable queue.
				 * When the dequeuing thread tries to make us
				 * runnable, it will find we are already on
				 * an LWP and it will leave us alone.
				 */
				ctp->t_state = TS_RUNNABLE;
				return(ctp);
			}

			/*
			 * If we get here, the thread was awakened by an
			 * _lwp_cond_signal, which is the notification for
			 * it to give up its LWP to a runnable thread.
			 * We'll fall out of this while-loop if the runnable
			 * thread is still waiting for an LWP; however, it's
			 * possible that another LWP came along in the
			 * meantime, so we may end up repeating this loop.
			 */
		}
		if (!_thr_nrunnable) {	
			/*
			 * Runnable queue is empty; return LWP's idle thread.
			 * This occurs if the thread is in the zombie state
			 * or the suspended state.
			 */
			UNLOCK_RUNQ;
			PRINTF3("disp:curthread=0x%x,lwpp=0x%x,l_idle=0x%x\n", 
			ctp, ctp->t_lwpp, ctp->t_idle);
			ASSERT(ctp->t_idle != NULL);
			return(ctp->t_idle);
		}

		/*
		 * Find the runnable thread with the highest priority.
		 * _thr_dqactmap[] is a bit map of priority queues. This
		 * loop looks through this map for the highest priority
		 * queue with runnable threads.
		 */
		if (_thr_maxpriq < 0) {
			/* 
			 * _thr_runnableq and _thr_maxpriq are out 
			 * of sync; re-sync them and start over
			 */
			PRINTF1("calling _thr_fix_runq: _thr_maxpriq = %d\n",
				_thr_maxpriq);
			_thr_fix_runq();
			continue;
		}
		runword = _thr_maxpriq/BPW; 
		dq = &_thr_runnable[_thr_maxpriq];
		t = _thrq_rem_first((thrq_elt_t *)dq);
		if (t == NULL) { /* no thread found on queue */
			/* 
			 * _thr_runnableq and _thr_dqactmap are out 
			 * of sync; re-sync them and start over
			 */
			PRINTF1("_thr_disp: no thread found on queue %d\n",
			         _thr_maxpriq);
			PRINTF3("runword=%d; _thr_maxpriq=%d; BPW=%d\n",
				runword, _thr_maxpriq, BPW);
			PRINTF("calling _thr_fix_runq: 3\n");
			_thr_fix_runq();
			continue;
		}
		/*
		 * dequeued a thread; set done to 1 to exit the loop;
		 * then update _thr_dqactmap and _thr_maxpriq.
		 */
		done = 1; 
		if (--_thr_nrunnable == 0) {
			_thr_dqactmap[runword] = 0;
			_thr_maxpriq = THREAD_MIN_PRIORITY - 1;
		} else if (THRQ_ISEMPTY(dq)) {
			bitmap = _thr_dqactmap[runword];
			hb = _thr_hibit(bitmap) - 1;
			_thr_dqactmap[runword] &= ~(1<<hb);
			/* re-adjust _maxpriq */
			do {
				bitmap = _thr_dqactmap[runword];
				hb = _thr_hibit(bitmap) - 1;
				if (hb >= 0)
					break;
			} while (runword--);
			if (hb < 0) {
				/* 
				 * This shouldn't happen since _thr_nrunnable
				 * indicates there are runnable threads.
			 	 * _thr_nrunnable and _thr_dqactmap must be 
			 	 * out of sync; re-sync them.
			 	 */
				PRINTF1("call _thr_fix_runq: hb = %d\n", hb);
				_thr_fix_runq();
			} else {
				_thr_maxpriq = (hb + (BPW * runword));
			}
		}
		ASSERT(t->t_state == TS_RUNNABLE);
	}
		
	UNLOCK_RUNQ;
	ASSERT(t != NULL);
	PRINTF1("_thr_disp:  dispatching thread %d\n", t->t_tid);
	return(t);
}


/*
 * void
 * _thr_swtch(boolean_t savecontext, thread_desc_t *t)
 *
 *      This function causes a multiplexed thread to give up
 *      its LWP.  If savecontext is 'true', the context of
 *      the calling thread is saved.
 *
 * Parameter/Calling State:
 *      The thread lock of calling thread must be held and signal
 *      handlers must be disabled upon entry.
 *
 *	First argument indicates whether to save the calling thread's
 *	context.  Second argument is a pointer to the calling thread.
 *
 *      The lock that protects the runnable queue will be acquired
 *      during processing via a call to _thr_disp().
 *      The lock of a dispatched runnable thread will be acquired.
 *
 * Return Values/Exit State:
 *      Nothing is returned, no locks are held, and signal
 *      handlers are still disabled.
 */
void 
_thr_swtch(boolean_t savecontext, thread_desc_t *t)
{
	thread_desc_t *next;
	int sigflag, hashprio;
	int save_errno = errno;
	int rval;

	ASSERT(!ISBOUND(t));
	ASSERT(IS_THREAD_LOCKED(t));
	ASSERT(THR_ISSIGOFF(t));

	if ((savecontext)&&(t->t_state == TS_RUNNABLE)) {
		/*
		 * If the calling thread is runnable and the calling thread is
		 * not an idle thread, then it is calling _thr_swtch()  because
		 * it has been preempted or has called thr_yield(); this also
		 * means that it has not yet been placed on the runnable queue.
		 * In the case of preemption, the thread that caused the
		 * preemption may have already been scheduled; in the case of
		 * thr_yield(), there may not be a thread of equal or higher
		 * priority that is runnable.  Therefore we check to see if the
		 * priority of the calling thread is higher than or equal to
		 * that of the highest priority runnable thread (or higher
		 * than, in the case of thr_yield())  and return if it is, as
		 * we don't want to give up the LWP to an equal or lower
		 * priority thread (lower in the case of thr_yield()).  If it
		 * is not, we place the calling thread on the runnable queue.
		 */
		HASH_PRIORITY(t->t_pri, hashprio);
		if (((hashprio >= _thr_maxpriq) && (t->t_flags & T_PREEMPT)) ||
 		    (hashprio > _thr_maxpriq)) {
			 if (t->t_flags & T_PREEMPT) {
			 	t->t_flags &= ~T_PREEMPT;
			 }
			t->t_state = TS_ONPROC;
			UNLOCK_THREAD(t);
			errno = save_errno;
			return;
		} else {
			(void)_thr_setrq(t, 1);

		}
	}

	PRINTF2("_thr_swtch out-thread = %d: _thr_maxpriq = %d\n", 
		t->t_tid,_thr_maxpriq);

	/* get highest priority runnable thread */

	next = _thr_disp(t);
	ASSERT(next != NULL);
	ASSERT((t->t_tid == -1) || (t->t_state != TS_ONPROC));
	PRINTF2("_thr_swtch in-thread = %d: _thr_maxpriq = %d\n", 
		next->t_tid, _thr_maxpriq);
	if (next == t) {
		/*
		 * It's possible the calling thread could have dispatched 
		 * itself if it is runnable or if the calling thread is an 
		 * idle thread or if it was lazy-switched out and then
		 * awakened; if so, we simply change our state to 
		 * TS_ONPROC, release our lock, and return.
		 */
		if (t->t_flags & T_PREEMPT) {
			t->t_flags &= ~T_PREEMPT;
		}
		t->t_state = TS_ONPROC;
		/*
		 * must call _thr_resendsig to process any signals received
		 * in critical sections
		 */
		(void)_thr_resendsig(B_FALSE, NULL, t);
		UNLOCK_THREAD(t);
		PRINTF2("_thr_swtch: LWP %d, thread %d  next == t\n", LWPID(t),
			t->t_tid);
		errno = save_errno;
		return;
	}

	/*
	 * We obtain the thread lock of the newly dispatched thread
	 * while holding the lock of the calling thread.  This is safe
	 * as explained by locking rule 2 in the library design document,
	 * since we also know from the previous test that we're not trying
	 * to reacquire our own lock.
	 */
	ASSERT(THR_ISSIGOFF(next));
	LOCK_THREAD(next);
	PRINTF3("swtch1:curthread=0x%x,t_lwpp=0x%x,t_idle=0x%x\n", t,
		t->t_lwpp, t->t_idle);

	PRINTF4("swtch: calling _thr_resume -- next=0x%x, t=0x%x, _thr_sigcmpset(&t->t_hold, &next->t_hold) =0x%x, savecontext=0x%x\n", next, t, _thr_sigcmpset(&t->t_hold, &next->t_hold), savecontext);

	/*
	 * if the old and new masks are different, then mask all signals
	 * while we switch from the old to new so that signals don't get
	 * sent to the wrong guy
	 * the actual mask for the new thread will be loaded in _thr_resume()
	 */
	sigflag = _thr_sigcmpset(&t->t_hold, &next->t_hold);

	/*
	 * Only call _sys_sigprocmask if the masks of the two threads are
	 * different (i.e. sigflag != 0) and no signal was received in 
	 * the critical section (i.e., t->t_sig == 0).
	 * If a signal was received in the critical section, the global
	 * signal handler would already be blocking all signals.
	 */
        if (sigflag && t->t_sig == 0) {
		(void)(*_sys_sigprocmask)(SIG_SETMASK, &_thr_sig_allmask, NULL);
        }

	_thr_resume(next, t, (sigflag ? B_TRUE : B_FALSE), 
		savecontext, _thr_debug.thr_debug_on);
        /*
         * _thr_resume replaces the calling thread with the dispatched
         * thread on the current LWP and then releases both the lock of
         * the calling thread and the lock of the dispatched thread.
         */

	/*
	 * if t_exitval is non-zero, its value represents the priority
	 * argument that should be given to _thr_activate_lwp().  This
	 * is set because the thread previously on this LWP received a
	 * signal in a critical section and _thr_resendsig was unable to
	 * call _thr_activate_lwp() because it held our thread lock.
	 * Now we can call _thr_activate_lwp() since our lock is released.
	 */
	if (t->t_exitval) {
		rval = (int)t->t_exitval;
		t->t_exitval = 0;
		_thr_activate_lwp(rval);
	}

	/*
	 * The following is reached only when the
	 * savecontext value is non-zero.  When savecontext is
	 * 0, the context of the calling thread is not saved so
	 * this point is not returned to if the thread is
	 * rescheduled.
	 */

	if (_thr_preempt_ok == B_TRUE) {
		HASH_PRIORITY(t->t_pri, hashprio);
		if (hashprio < _thr_maxpriq) {
			/*
		 	 * There is a runnable thread with priority higher 
		 	 * than t.  It's possible that thread could be stranded
		 	 * if we don't give up our LWP to it, since the 
		 	 * preemption search may not have realized t would be 
		 	 * running if it looked at the previous thread that was
		  	 * on the LWP at the time.  Therefore, in order to 
		 	 * ensure the correctness of the preemption mechanism,
 			 * we have to offer our LWP to that thread. (Note that
		 	 * this could only be the case if the higher priority
		 	 * thread is already on the runnable queue, since
		 	 * the search occurs after the thread is placed on
		 	 * the runnable queue.  Therefore, we don't have to
		   	 * worry about threads placed on the runnable queue
		 	 * after the above check, i.e., there is no fear
		 	 * of a race condition here.
		 	 */
		 	LOCK_THREAD(t);
			HASH_PRIORITY(t->t_pri, hashprio);
			if (hashprio < _thr_maxpriq) {
			 	PREEMPT_SELF(t);
			} else {
				UNLOCK_THREAD(t);
			}
		}
	}
	PRINTF3("swtch2:curthread tid = %d,t_lwpp=0x%x,t_idle=0x%x\n", t->t_tid,
				t->t_lwpp, t->t_idle);
	errno = save_errno;
}


/*
 * void *
 * _thr_age(void)
 *
 *      This function is executed by the idle thread of a multi-
 *      plexing LWP when there are no other threads to run on the
 *      LWP.  It causes the LWP to block on an LWP condition
 *      variable via _lwp_cond_timedwait().  From there it can
 *      be activated via _lwp_cond_signal() if a thread becomes
 *      runnable.  If the timer expires on the wait, the LWP may
 *      be terminated, depending on concurrency requirements of
 *      the process.
 *
 * Parameter/Calling State:
 *      No locks are held upon entry; all signals are blocked by
 *	idle threads.
 *
 *      The counter lock, the preemption lock, and the calling 
 *	thread's lock will be acquired during processing.
 *
 * Return Values/Exit State:
 *      This function doesn't return.  If a thread becomes
 *      runnable, the function causes the calling thread to
 *      give up the LWP via _thr_swtch() and the context of
 *      the thread is not saved.  If the timer expires and
 *      the LWP is terminated, processing ends with the
 *      termination of the LWP.
 */
void *
_thr_age(void)
{
	volatile int timedout;
	timestruc_t tval;
	thread_desc_t *t = curthread;

	ASSERT(!ISBOUND(t));
	ASSERT(t->t_tid == -1);
	ASSERT(t->t_nosig == 1);

	/*
	 * An idle thread holds its thread lock throughout execution
	 * of this function.  This is safe because idle threads block
	 * all signals and are unavailable to the user.  Therefore,
	 * there is no other thread that will attempt to acquire an
	 * idle thread's lock.  Holding the lock throughout execution
	 * avoids awkward procedures that would otherwise be needed
	 * to prevent locking hierarchy violations.
	 */
	LOCK_THREAD(t);
        ASSERT(IS_THREAD_LOCKED(t));
	LOCK_RUNQ;
	_thr_nage++;
	for (; ;) {
		timedout = 0;
		while (!_thr_nrunnable && timedout == 0) {
			/*
			 * The next two lines obtain the current time
			 * and add the LWP sleep interval to it.
			 */
			hrestime(&tval);
			tval.tv_sec += _thr_idletime.tv_sec;
			timedout = _lwp_cond_timedwait(&_thr_aging,
					&_thr_runnableqlock, &tval);
		}
		if (_thr_nrunnable) {
		/*
		 * This function generously gives up its LWP to a runnable
		 * thread even if it timed out.  This potentially slows down
		 * the rate at which LWPs are removed from the process.
		 */
			_thr_nage--;
			if (_thr_dontkill_lwp > 0) {
				_thr_dontkill_lwp--;
			}
			UNLOCK_RUNQ;
			/*
			 * The argument to _thr_swtch() is 0 since we don't
			 * need to save the context of the idle thread.
			 *
			 * The thread's lock is released on return from
			 * _thr_swtch().
			 */
			_thr_swtch(0, t);
			ASSERT(t->t_state == TS_ONPROC);
			/*
			 * It's possible that _thr_swtch will not
			 * switch out the idle thread if there is no
			 * real thread to run, i.e., any runnable
			 * thread that caused this thread to return
			 * may already have been scheduled on another
			 * LWP.  In this case, it is simplest to
			 * continue from here -- but we must relock
			 * the thread, first, since return from
			 * _thr_swtch will leave the thread unlocked.
			 */
			LOCK_THREAD(t);
			LOCK_RUNQ;
			_thr_nage++;
		} else if (timedout == ETIME){
		/*
		 * We must make sure the wait timed out because it's
		 * possible the LWP could have been signalled, which would
		 * cause timedout to be non-zero.  If that happened, we
		 * simply loop again.
		 */
			if (_thr_dontkill_lwp > 0) {
				/*
				 * thr_setconcurrency was called after
				 * this LWP started aging; since that call
				 * may have been just a microsecond ago, we
				 * let the LWP age again before terminating it
				 */
				_thr_dontkill_lwp--;
			} else {
				/*
				 * make sure there will be enough LWPs in
				 * the process after this one terminates;
				 * if not, simply repeat the aging process
				 *
				 * The last LWP is prevented from
				 * terminating except when there are no
				 * multiplexed threads left; if the last LWP
				 * does terminate because there are no more
				 * multiplexed threads, there is no danger
				 * that new multiplexed threads will not be
				 * runnable because thr_create() will
				 * recognize this situation and create an LWP.
				 */
				if ((_thr_nlwps > 
				    MIN(_thr_minlwps, _thr_nthreads)) &&
				    (_thr_nlwps > 1 || !_thr_nthreads)) {
					/*
					 * The LWP will be terminated; we 
					 * exploit its availability to reap
					 * any reapable threads before it 
					 * is terminated.
					 */
					_thr_nage--;
					UNLOCK_RUNQ;
					if (_thr_reapable > 1) {
						_thr_reaper();
					}
					LOCK_PREEMPTLOCK;
					_thr_deq_from_idlethreads(t);
					_thr_nlwps--;
					UNLOCK_PREEMPTLOCK;
					LOCK_COUNTER;
					_thr_reapable++;
					/*
					 * To prevent reaping thread when we
					 * still hold its lock the unlocking
					 * sequence must be:
					 * (1) thread lock
					 * (2) counter lock
					 */
					_thr_enq_thread_to(&_thr_reaplist, t);
					UNLOCK_THREAD(t);
					UNLOCK_COUNTER;
					PRINTF("terminating aging LWP\n");
					_lwp_exit();
				}
			}
		}
	}
}

/*
 * void
 * _thr_enq_thread_to(thread_desc_t **q, thread_desc_t *t)
 *
 *      This function places the indicated thread onto the specified queue.
 *	It is used for queues on which the ordering of members doesn't
 *	matter.  It is called by the functions _thr_age(), _thr_new_lwp(),
 *	_thr_exit(), _thr_reaper(), and _thr_put_on_reaplist().
 *
 * Parameter/Calling State:
 *      The locks that protect the queue and the thread must
 *	be held and signal handlers must be disabled upon entry.
 *
 *	An exception to this is _thr_new_lwp(), which doesn't need
 *	to hold the thread lock because it calls this function before
 *	the thread is known to the library; therefore, holding the
 *	thread lock is unnecessary as no other thread can act on
 *	the thread being enqueued.  Another exception is _thr_reaper()
 *	which doesn't hold the thread locks of zombie threads because
 *	no other thread can obtain them after the zombies have been
 *	placed on the reaplist.
 *
 * Return Values/Exit State:
 *      Upon return, t has been added to list q.  The locks held upon
 * 	entry are held upon exit and signal handlers are still disabled.
 */
void
_thr_enq_thread_to(thread_desc_t **q, thread_desc_t *t)
{
	ASSERT(THR_ISSIGOFF(t));
	ASSERT((TNEXT(t) == t->t_prev) && (TNEXT(t) == NULL));

	if (*q == NULL) {
		*q = t;
		(*q)->t_next = (*q)->t_prev = t;
	} else {
		(*q)->t_prev->t_next = t;
		t->t_next = *q;
		t->t_prev = (*q)->t_prev;
		(*q)->t_prev = t;
	}
}

/*
 * int
 * _thr_deq_thread_from(thread_desc_t **q, thread_desc_t *t)
 *
 *      This function removes the specified thread from the specified queue.
 *      It searches the queue to make sure the thread is actually on it.
 *      It is called by the function thr_join().  If the specified
 *      thread is removed from the queue, it returns 1; if the thread was
 *      not on the queue, it returns 0.
 *
 * Parameter/Calling State:
 *      The locks that protect the queue and the thread must be held and
 *      signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *      Upon return, the specified thread has been removed from list q and
 *      1 is returned by the function or 0 is returned if the thread was not
 *      on the list.  The locks held upon entry are held upon exit and signal
 *      handlers are still disabled.
 */
int
_thr_deq_thread_from(thread_desc_t **q, thread_desc_t *t)
{
        int success = 1;
        int failure = 0;
        thread_desc_t *temp, *next;
        thread_desc_t *nextthread; /* added for redef of t_next */
        ASSERT(THR_ISSIGOFF(t));


        if (*q == NULL) {              /* list is empty */
                return(failure);
        } else {
                temp = *q;
                next = temp->t_next;
                while ((temp != t) && (next != *q)) {
                        /* search q for t */
                        temp = next;
                        next = temp->t_next;
                }
                if (temp == t) {
                        /* found t */
                        if (temp->t_next == t) {
                                PRINTF1("temp = %d only list member\n",
                                        temp->t_tid);
                                /* t is only list member */
                                *q = NULL;
                        } else {
                                nextthread = (thread_desc_t *)temp->t_next;/*redef*/
                                PRINTF2("temp = %d; temp->next = %d\n",
                                        temp->t_tid,
                                        /* temp->t_next->t_tid);   -- redef */
                                        nextthread->t_tid);     /* -- redef */
                                /* other threads on list, too */
                                /* temp->t_next->t_prev            -- redef */
                                nextthread->t_prev              /* -- redef */
                                        = temp->t_prev;
                                temp->t_prev->t_next = temp->t_next;
                                if (*q == temp)
                                        *q = temp->t_next;
                        }
                        t->t_next = NULL;
                        t->t_prev = NULL;
                        return(success);
                } else {
                        /* t is not on the list */
                        PRINTF1("couldn't find tid %d on list\n", temp->t_tid);
                        return(failure);
                }
        }
}

/*
 * thread_desc_t *
 * _thr_deq_any_from(thread_desc_t **q)
 *
 *      This function removes the first thread from the specified queue.
 *      It is used for queues on which the ordering of members doesn't
 *      matter.  It is called by the function thr_join().
 *
 * Parameter/Calling State:
 *      The lock that protects the queue must be held and signal handlers
 *      must be disabled upon entry.
 *
 * Return Values/Exit State:
 *      Upon return, the first thread has been removed from list q and its
 *      address is returned by the function; NULL is returned if the list
 *      was empty.  The locks held upon entry are held upon exit and signal
 *      handlers are still disabled.
 */
thread_desc_t *
_thr_deq_any_from(thread_desc_t **q)
{
        thread_desc_t *nextthread;   /* added because of redef of t_next */
        thread_desc_t *t;
        ASSERT(THR_ISSIGOFF(curthread));

        if (*q == NULL) {              /* list is empty */
                return((thread_desc_t *) NULL);
        } else {
                t = *q;
                if (t->t_next == t) {  /* exactly one thread on the list */
                        *q = NULL;
                } else {               /* more than one thread on the list */
                        nextthread = (thread_desc_t *)t->t_next;  /* redef */
                        /* t->t_next->t_prev = t->t_prev;            redef */
                        nextthread->t_prev = t->t_prev;           /* redef */
                        t->t_prev->t_next = t->t_next;
                        *q = t->t_next;
                }
                t->t_next = NULL;
                t->t_prev = NULL;
                return(t);
        }
}

/*
 * void
 * _thr_deq_from_idlethreads(thread_desc_t *t)
 *
 *      This function removes the indicated thread from the queue
 *	_thr_idlethreads and decrements the global variable _thr_nlwps.  
 *	It is called by _thr_age() when a multiplexing LWP is terminating 
 *	due to aging.
 *
 * Parameter/Calling State:
 *      The preempt lock must be held and signal handlers 
 *	must be disabled upon entry.
 *
 * Return Values/Exit State:
 *      Upon return, t has been removed from _thr_idlethreads.  The counter
 *	lock is still held and signal handlers are still disabled.
 */
void
_thr_deq_from_idlethreads(thread_desc_t *t)
{
	ASSERT(IS_PREEMPT_LOCKED);
	ASSERT(THR_ISSIGOFF(t));

	ASSERT(_thr_nlwps > 0);

	if (_thr_nlwps > 1) {     /* still some multiplexing LWPs remain */
		t->t_prev->t_next = TNEXT(t);
		TNEXT(t)->t_prev = t->t_prev;
		if (_thr_idlethreads == t)
			_thr_idlethreads = TNEXT(t);
	} else                      /* no multiplexing LWPs remain */
		_thr_idlethreads = NULL;
	t->t_next = t->t_prev = NULL;
}


/*
 * int _thr_remove_from_runq()
 *      Removes the specified thread from the runq sleepq it is on.
 *
 * Calling/Exit state:
 *      On entry, the thread lock of tp is held, and signal handlers are
 *      disabled.
 *
 *      During processing, the runq lock is obtained and released.
 *
 *      On exit, the thread lock of tp is still held and signal handlers are
 *      still disabled.
 *
 * Return Values/Exit State:
 *      returns 1, if the thread is removed from the queue, otherwise 0.
 */

int
_thr_remove_from_runq(thread_desc_t *tp)
{
	int rval = 0;

	ASSERT(tp->t_sync_type == TL_NONE);
	ASSERT(tp->t_sync_addr == NULL);
	LOCK_RUNQ;
        if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
                _thrq_rem_from_q(tp);
                rval = 1;
        }
	UNLOCK_RUNQ;
	return(rval);
}


/*
 * int _thr_requeue_runq()
 *	Changes the priority of a runnable thread, removes it from its
 *	subqueue on the runnable queue and replaces it on the appropriate
 *	subqueue on the runnable queue.  If the thread was not on the
 *	runnable queue, it is not replaced on the runnable queue.
 *
 * Calling/Exit state:
 *      On entry, the thread lock of tp is held, and signal handlers are
 *      disabled.
 *
 *      During processing, the runq lock is obtained and released.
 *
 *      On exit, the thread lock of tp is still held and signal handlers are
 *      still disabled.
 *
 * Return Values/Exit State:
 *      Returns INVALID_PRIO if no LWP needs to be activated for the thread; 
 *	returns a priority value if an LWP should be activated for the thread 
 *	(because its priority has been increased).
 *
 *	On return, the priority of the thread has been
 *	set to the indicated value and, if the thread was on the runnable
 *	queue on entry, it has been placed onto the appropriate subqueue
 *	of the runnable queue based on its new priority.
 */

int
_thr_requeue_runq(thread_desc_t *tp, int prio)
{
	int oldpri, qindex, runword, hb;
        register thrq_elt_t *qp;
        register volatile long bitmap;
	int rval = INVALID_PRIO;

	ASSERT(tp->t_sync_type == TL_NONE);
	ASSERT(tp->t_sync_addr == NULL);

	oldpri = tp->t_pri;
	if (prio > oldpri) {
		/*
		 * If the thread's priority is being increased, we set the
		 * return value to indicate that a call to _thr_activate_lwp()
		 * should be performed with argument "prio".  This is necessary
		 * since there's no way to know for sure if preemption is
		 * necessary.
		 *
		 * We know we don't have to activate an LWP if the priority
		 * is adjusted downward since, if we didn't get one already
		 * for this thread, we definitely won't need to get one at
		 * a lower priority.
		 */
		rval = prio;
	}
	LOCK_RUNQ;
        if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
		/*
		 * remove thread from runnable queue and update the
		 * appropriate data structures.
		 */
                _thrq_rem_from_q(tp);
		HASH_PRIORITY(oldpri, qindex);
		qp = &_thr_runnable[qindex]; /* qp is queue we took tp from */
	        if (THRQ_ISEMPTY(qp)) {
	                /* queue of old priority is now empty */
	                /*_thr_dqactmap[(qindex/BPW)] &= ~(1<<(qindex % BPW));*/
	                _thr_dqactmap[(qindex/BPW)] &= ~(1<<(qindex & (BPW-1)));

			if (qindex == _thr_maxpriq) {
				/* we must update _thr_maxpriq */

				if ((_thr_nrunnable == 1) || (prio > oldpri)) {
					/*
					 * this is the only runnable thread
					 * or we're increasing its priority;
					 * in either case set _thr_maxpriq to 
					 * its new priority value
					 */
					HASH_PRIORITY(prio, _thr_maxpriq);
				} else {
					/*
					 * there are other runnable threads and
					 * we're lowering priority, so we must
					 * find the highest priority thread
					 * and set _thr_maxpriq to its priority.
					 * If prio is higher than that thread's
					 * priority, we'll discover that after
					 * we change priority.
					 */
					for (runword = (qindex/BPW); 
					   runword >= 0; runword--) {
						bitmap = _thr_dqactmap[runword];
						hb = _thr_hibit(bitmap) - 1;
						if (hb >= 0)
							break;
					}
					if (hb < 0) {
					        /*
					         * This shouldn't happen since 
						 * _thr_nrunnable indicates 
						 * there are runnable threads.
						 * _thr_nrunnable and 
						 * _thr_dqactmap must be out of
					         * sync; re-sync them.
					         */
					        PRINTF("call _thr_fix_runq: 4\n");
					        _thr_fix_runq();
						/*
						 * _thr_fix_runq() resets the
						 * values of _thr_maxpriq and
						 * _thr_nrunnable based on the
						 * threads currently on the
						 * queue; since this thread is
						 * temporarily off the queue,
						 * we must increment the value
						 * of _thr_nrunnable.
						 */
						_thr_nrunnable++;
					} else {
					        _thr_maxpriq = 
						   (hb + (BPW * runword));
					}
				}
			}
		}

		/*
		 * now change the thread's priority
		 */
		tp->t_pri = prio;

		/*
		 * place thread back onto appropriate subqueue of runnable
		 * queue and update appropriate data structures.
		 */
		HASH_PRIORITY(prio, qindex);
		if (qindex > _thr_maxpriq) {
			_thr_maxpriq = qindex;
		}
		qp = &_thr_runnable[qindex];
	        if (THRQ_ISEMPTY(qp)) {
	                /* queue empty */
	                _thrq_add_end(qp, tp);
	                _thr_dqactmap[(qindex/BPW)] |= 1<<(qindex & (BPW-1));
	        } else {
	                if (qindex < DISPQ_SIZE - 1) {
				/* priorities in normal range use FIFO queues */
				_thrq_add_end(qp, tp);
	                } else {
				/*
				 * priorities outside normal range map to last
				 * priority bucket and are inserted in priority
				 * order.
				 */
				_thrq_prio_ins(qp, tp);
			}
		}
        } else {
		/*
		 * tp was not on a subqueue of the runnable queue so
		 * all we have to do is change its priority
		 */
		tp->t_pri = prio;
	}
	UNLOCK_RUNQ;
	return(rval);
}


/*
 * int
 * _thr_get_qlength(thrq_elt_t *q)
 *	determines the number of elements in the specified thrq_elt_t queue.
 *
 * Parameter/Calling State:
 *	called with signal handlers disabled and _thr_runnableqlock held;
 *	It is called by _thr_fix_runq() to determine the number of runnable
 *	threads in the runnable queue.
 *
 * Return Values/Exit State:
 *	Returns the number of threads in the queue with signal handlers
 *	still disabled and _thr_runnableqlock still held.
 */
int
_thr_get_qlength(thrq_elt_t *q)
{
	volatile thrq_elt_t *temp;
	int count = 0;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(IS_RUNQ_LOCKED);

	if (!THRQ_ISEMPTY(q)) {
		temp = q;
		while (temp->thrq_next != q) {
			temp = temp->thrq_next;
			count++;
		}
	}
	return(count);
}


/*
 * void
 * _thr_fix_runq();
 *	searches the runnable queues and resets _thr_maxpriq,
 *	_thr_nrunnable, and _thr_dqactmap[].
 * 
 * Parameter/Calling State:
 *	called with signal handlers disabled and _thr_runnableqlock held;
 *	It is called by _thr_disp() when it discovers an irregularity in
 *	the runnable queue.
 *
 * Return Values/Exit State:
 *	Resets the values indicated above and returns with signal handlers
 *	still disabled and _thr_runnableqlock still held.
 */
void
_thr_fix_runq()
{
	volatile int i, temp, word, offset;
	int total_runnable = 0;
	int maxpri = -1;
	long bitmap[(DISPQ_SIZE/BPW)];
	memset(bitmap, 0, sizeof(bitmap));

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(IS_RUNQ_LOCKED);

	PRINTF2("\n_thr_fix_runq: _thr_maxpriq=%d; _thr_nrunnable=%d\n",
		_thr_maxpriq, _thr_nrunnable);
#ifdef THR_DEBUG
	for (i = 0; i < (DISPQ_SIZE/BPW); i++) { 
		PRINTF2("_thr_fix_runq: initial _thr_dqactmap[%d] = 0x%x\n", 
		i, _thr_dqactmap[i]);
	}
#endif
	for (i = 0; i <= DISPQ_SIZE; i++) {
		temp = _thr_get_qlength(&_thr_runnable[i]);
		total_runnable += temp;  /* update count of runnable threads */
		if (temp > 0) {
			/*
			 * if at least one thread was in the queue, adjust
			 * maxpri and set the bitmap bit for the queue.
			 */
			PRINTF2("_thr_fix_runq:found %d thread(s) in q[%d]\n",
			temp, i);
			maxpri = i;
			word = (i / BPW);
			offset = (i % BPW);
			bitmap[word] |= (1 << offset);
		}
	}

	if (_thr_maxpriq != maxpri) {
		PRINTF2("_thr_fix_runq: _thr_maxpriq = %d; real max pri = %d\n",
		_thr_maxpriq, maxpri);
		_thr_maxpriq = maxpri;
	}
	if (_thr_nrunnable != total_runnable) {
		PRINTF2("_thr_fix_runq: _thr_nrunnable = %d; should be = %d\n", 
			_thr_nrunnable, total_runnable);
		_thr_nrunnable = total_runnable;
	}
	for (i = 0; i < ((DISPQ_SIZE/BPW)); i++) { 
		if (_thr_dqactmap[i] != bitmap[i]) {
			PRINTF4("_thr_fix_runq: _thr_dqactmap[%d]= %x; bitmap[%d]= %x\n", 
			i, _thr_dqactmap[i], i, bitmap[i]);
		}
		_thr_dqactmap[i] = bitmap[i];
	}
}
