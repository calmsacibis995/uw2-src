/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/lock.c	1.2"

#include <thread.h>
#include <libthread.h>

#ifdef DEBUG
/*
 * lock statistic structures for main locks.
 */

thr_lckstat_t	tidvec_stats;
thr_lckstat_t	counter_stats;
thr_lckstat_t	thread_stats;
thr_lckstat_t	runq_stats;
thr_lckstat_t	preemptlock_stats;

/*
 * lwp mutex lock to protect multiple accesses to thread locks since
 * there are many.
 */
lwp_mutex_t	_thr_lckstat_threadlock;

/*
 * void _thr_lock_tidvec(void)
 *
 * 	Locks the tidvec lock.
 *
 * Parameter/Calling State:
 *	Any locks may be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Locks the tidvec lock during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the tidvec lock locked. 
 */

void
_thr_lock_tidvec(void)
{
	if (_lwp_mutex_trylock(&_thr_tidveclock) == EBUSY) {
		_lwp_mutex_lock(&_thr_tidveclock);
		tidvec_stats.lckstat_waits++;
	}
	tidvec_stats.lckstat_locks++;
	PRINTF1("@@@ Thread %d locked TIDVEC\n",curthread->t_tid);
}

/*
 * void _thr_unlock_tidvec(void)
 *
 * 	Unlocks the tidvec lock.
 *
 * Parameter/Calling State:
 *	The tidvec lock must be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Acquires no new locks during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the tidvec lock unlocked. 
 */

void
_thr_unlock_tidvec(void)
{
	tidvec_stats.lckstat_unlocks++;
	PRINTF1("@@@ Thread %d UNlocked TIDVEC\n",curthread->t_tid);
	_lwp_mutex_unlock(&_thr_tidveclock);
}


/*
 * void _thr_lock_counter(void)
 *
 * 	Locks the counter lock.
 *
 * Parameter/Calling State:
 *	Any locks may be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Locks the counter lock during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the counter lock locked. 
 */

void
_thr_lock_counter(void)
{
	if (_lwp_mutex_trylock(&_thr_counterlock) == EBUSY) {
		_lwp_mutex_lock(&_thr_counterlock);
		counter_stats.lckstat_waits++;
	}
	counter_stats.lckstat_locks++;
	PRINTF1("@@@ Thread %d locked COUNTER\n",curthread->t_tid);
}


/*
 * void _thr_unlock_counter(void)
 *
 * 	Unlocks the counter lock.
 *
 * Parameter/Calling State:
 *	The counter lock must be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Acquires no new locks during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the counter lock unlocked. 
 */

void
_thr_unlock_counter(void)
{
	counter_stats.lckstat_unlocks++;
	_lwp_mutex_unlock(&_thr_counterlock);
	PRINTF1("@@@ Thread %d UNlocked COUNTER\n",curthread->t_tid);
}

/*
 * void _thr_lock_runq(void)
 *
 * 	Locks the runq lock.
 *
 * Parameter/Calling State:
 *	Any locks may be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Locks the runq lock during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the runq lock locked. 
 */

void
_thr_lock_runq(void)
{
	if (_lwp_mutex_trylock(&_thr_runnableqlock) == EBUSY) {
		_lwp_mutex_lock(&_thr_runnableqlock);
		runq_stats.lckstat_waits++;
	}
	runq_stats.lckstat_locks++;
	PRINTF1("@@@ Thread %d locked RUNQ\n",curthread->t_tid);
}

/*
 * void _thr_unlock_runq(void)
 *
 * 	Unlocks the runq lock.
 *
 * Parameter/Calling State:
 *	The runq lock must be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Acquires no new locks during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the runq lock unlocked. 
 */

void
_thr_unlock_runq(void)
{
	runq_stats.lckstat_unlocks++;
	PRINTF1("@@@ Thread %d UNlocked RUNQ\n",curthread->t_tid);
	_lwp_mutex_unlock(&(_thr_runnableqlock));
}


/*
 * void _thr_lock_thread(thread_desc_t *t)
 *
 * 	Locks the thread lock of the thread pointed to by t.
 *
 * Parameter/Calling State:
 *	Any locks may be held on entry -- debug only routine.
 *
 *	Argument - pointer to the thread lock to be locked.
 *
 * 	Locks the thread lock and the thr_lckstat_threadlock
 *      which serializes access to the lock statistics
 *      structure during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the thread lock locked. 
 */

void
_thr_lock_thread(thread_desc_t *t)
{
	_lwp_mutex_lock(&_thr_lckstat_threadlock);
	thread_stats.lckstat_locks++;
	_lwp_mutex_unlock(&_thr_lckstat_threadlock);

	if (_lwp_mutex_trylock(&(t->t_lock)) == EBUSY) {
		_lwp_mutex_lock(&_thr_lckstat_threadlock);
		thread_stats.lckstat_waits++;
		_lwp_mutex_unlock(&_thr_lckstat_threadlock);
		_lwp_mutex_lock(&(t->t_lock));
	}
	PRINTF2("@@@ Thread %d locked Thread lock %d\n",curthread->t_tid, t->t_tid);
}

/*
 * void _thr_unlock_thread(thread_desc_t *t)
 *
 * 	Unlocks the thread lock of the thread pointed to by t.
 *
 * Parameter/Calling State:
 *	The thread lock of t must be held on entry -- debug only routine.
 *
 *	Argument - pointer to the thread lock to be unlocked.
 *
 * 	Locks thr_lckstat_threadlock which serializes access to the 
 *	lock statistics structure during processing.
 *
 * Return Values/Exit State:
 * 	Returns with no new locks held.
 */

void
_thr_unlock_thread(thread_desc_t *t)
{
	_lwp_mutex_lock(&_thr_lckstat_threadlock);
	thread_stats.lckstat_unlocks++;
	_lwp_mutex_unlock(&_thr_lckstat_threadlock);
	PRINTF2("@@@ Thread %d UNlocked Thread lock %d\n",curthread->t_tid, t->t_tid);
	_lwp_mutex_unlock(&(t->t_lock));
}


/*
 * void _thr_lock_preemptlock(void)
 *
 * 	Locks the preempt lock.
 *
 * Parameter/Calling State:
 *	Any locks may be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Locks the preempt lock during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the preempt lock locked. 
 */

void
_thr_lock_preemptlock(void)
{
        if (_lwp_mutex_trylock(&_thr_preemptlock) == EBUSY) {
                _lwp_mutex_lock(&_thr_preemptlock);
                preemptlock_stats.lckstat_waits++;
        }
        preemptlock_stats.lckstat_locks++;
	PRINTF1("@@@ Thread %d locked PREEMPT\n",curthread->t_tid);
}

/*
 * void _thr_unlock_preemptlock(void)
 *
 * 	Unlocks the preempt lock.
 *
 * Parameter/Calling State:
 *	The preempt lock must be held on entry -- debug only routine.
 *
 *	Takes no arguments
 *
 * 	Acquires no new locks during processing.
 *
 * Return Values/Exit State:
 * 	Returns with the preempt lock unlocked. 
 */

void
_thr_unlock_preemptlock(void)
{
        preemptlock_stats.lckstat_unlocks++;
	PRINTF1("@@@ Thread %d UNlocked PREEMPT\n",curthread->t_tid);
        _lwp_mutex_unlock(&_thr_preemptlock);
}


/*
 * void _thr_lckstats_init(void)
 *	Zeroes the lock statistics structures.
 *
 * Parameter/Calling State:
 *	No locks are held on entry.
 *
 *	Takes no arguments
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *  	No locks are held on return.
 */

void
_thr_lckstats_init(void)
{
	memset(&tidvec_stats,0,sizeof(thr_lckstat_t));
	memset(&counter_stats,0,sizeof(thr_lckstat_t));
	memset(&thread_stats,0,sizeof(thr_lckstat_t));
	memset(&runq_stats,0,sizeof(thr_lckstat_t));
	memset(&preemptlock_stats,0,sizeof(thr_lckstat_t));
}

/*
 * void _thr_lckstats_print(void)
 *	Prints the lock statistics structures.
 *
 * Parameter/Calling State:
 *	No locks are held on entry.
 *
 *	Takes no arguments
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *  	No locks are held on return.
 */

void
_thr_lckstats_print(void)
{
	printf("TIDVEC: locks %d waits %d unlocks %d\n",
		tidvec_stats.lckstat_locks, tidvec_stats.lckstat_waits,
		tidvec_stats.lckstat_unlocks);
	printf("COUNTER: locks %d waits %d unlocks %d\n",
		counter_stats.lckstat_locks, counter_stats.lckstat_waits,
		counter_stats.lckstat_unlocks);
	printf("THREAD: locks %d waits %d unlocks %d\n",
		thread_stats.lckstat_locks, thread_stats.lckstat_waits,
		thread_stats.lckstat_unlocks);
	printf("RUNQ: locks %d waits %d unlocks %d\n",
		runq_stats.lckstat_locks, runq_stats.lckstat_waits,
		runq_stats.lckstat_unlocks);
	printf("PREEMPT: locks %d waits %d unlocks %d\n",
		preemptlock_stats.lckstat_locks,
		preemptlock_stats.lckstat_waits,
		preemptlock_stats.lckstat_unlocks);
}

#endif
