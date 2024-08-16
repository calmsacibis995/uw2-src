/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/slp.c	1.3.9.5"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file contains functions that are used when manipulating
 * sleeping threads and the queues associated with them.
 */

#include <libthread.h>

/*
 * int
 * _thr_setrun(thread_desc_t *tp)
 *	Removes the specified thread from its sleep queue
 *      and makes the specified thread runnable.
 *
 * Parameter/Calling State:
 *	The thread lock of tp is held and signal handlers are disabled
 *	upon entry to this function.
 *	
 *      The address of a thread structure is passed to this function.
 *
 *	During processing, the lock of the queue on which the thread is
 *	sleeping is acquired and the lock of the runnable queue is
 *	acquired via a call to _thr_setrq().
 *
 * Return Values/Exit State:
 *      Removes the thread from its sleep queue, makes it runnable,
 *	and returns INVALID_PRIO or a priority value on success.  Otherwise,
 *	causes the library to panic.
 *
 *	A return value other than INVALID_PRIO indicates that the calling 
 *	thread should activate an LWP for the newly runnable thread as 
 *	soon as it releases the lock of that thread.
 */
int
_thr_setrun(thread_desc_t *tp)
{
	int ret = 0;
	int rval = INVALID_PRIO;

	ASSERT(tp->t_state == TS_RUNNABLE || tp->t_state == TS_SLEEP);

	switch(tp->t_sync_type) {
	case TL_MUTEX:
		ret = _thr_remove_from_mutex_queue(tp);
		break;
	/*
	 * cases for other sync sleep queues will be added as
	 * those sync primitives are added to the library.
	 */
	case TL_COND:
		ret = _thr_remove_from_cond_queue(tp);
		break;

	case TL_LIBC:
		ret = _thr_remove_from_libc_queue(tp);
		break;

	case TL_NONE:
	default:
		PRINTF3("_thr_setrun: target tid: %d; t_state: 0x%x t_sync_type: 0x%x\n",
			tp->t_tid, tp->t_state, tp->t_sync_type);
		_thr_panic("_thr_setrun: Illegal list type");
		break;
	}
	if (ret == 1) {
		rval = _thr_setrq(tp, 0);
		/*
		 * _thr_activate_lwp() is not called here because that
		 * function should not be called while holding
		 * tp's thread lock.  
		 */
	}
	return(rval);
}

/*
 * int
 * _thr_requeue(thread_desc_t *tp, int prio)
 *	It removes the specified thread from its current position 
 *      on a priority based sleep queue, changes the priority of the
 *	thread to prio, and places is back on the same sleep queue in
 *	a new position determined by the thread's new priority value.
 *
 * Parameter/Calling State:
 *      The thread lock of tp is held and signal handlers are disabled
 *      upon entry to this function.
 *
 *      The address of a thread structure and the new thread priority 
 *	are passed to this function.
 *
 *      During processing, the lock of the queue on which the thread is
 *      sleeping is acquired.
 *
 * Return Values/Exit State:
 *	Returns 0 if no LWP activation is required; returns 1 if an LWP
 *	activation is required.  (LWP activation may be necessary when
 *	the target thread is on the runnable queue and its priority is
 *	increased.)
 *
 *      Repositions the thread on its sleep queue according to 
 *	its new priority.
 */

int
_thr_requeue(thread_desc_t *tp, int prio)
{
	int rval = 0;

	/* extern int _thr_requeue_libc(thread_desc_t *, int prio); */
	ASSERT(tp->t_state == TS_RUNNABLE || tp->t_state == TS_SLEEP);

	if (tp->t_state == TS_RUNNABLE) {
		ASSERT(tp->t_sync_type == TL_NONE);
		rval = _thr_requeue_runq(tp, prio);
	} else {
		switch(tp->t_sync_type) {
		case TL_MUTEX:
			_thr_requeue_mutex(tp, prio);
			break;

		case TL_COND:
			_thr_requeue_cond(tp, prio);
			break;
	
		case TL_LIBC:
			_thr_requeue_libc(tp, prio);
			break;
	
		case TL_NONE:
		default:
		PRINTF3("_thr_requeue: target tid: %d; t_state: 0x%x t_sync_type: 0x%x\n",
			tp->t_tid, tp->t_state, tp->t_sync_type);
			_thr_panic("_thr_requeue: Illegal list type");
			break;
		}
	}
	return(rval);
}

