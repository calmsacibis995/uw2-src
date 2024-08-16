/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/tidvec.c	1.1.5.4"

#include <stdio.h>
#include <string.h>
#include <libthread.h>
#include <sys/usync.h>

tidvec_t *_thr_freetid = NULL;         /* next free thread ID on the free list*/
tidvec_t *_thr_last_freetid = NULL;     /* last thread ID on the free list */
tidvec_t _thr_tid0[2];
/*
 * _thr_tidvec is an array used to map thread IDs to thread structures.
 * The index into the array corresponds to a thread ID and
 * the array element is the address of the thread structure corresponding
 * to that thread ID, if any, or the address of the next element in
 * the free list, _thr_freetid, if no thread is associated with
 * that array element.
 * _thr_last_tid_index points to the last entry in the _thr_tidvec array
 */
tidvec_t *_thr_tidvec = &_thr_tid0[0];
tidvec_t *_thr_last_tid_index = &_thr_tid0[1];
int _thr_total_tids;                    /* current size of tidvec in tids */
int _thr_tidpages = 0;			/* current size of tidvec in pages */

/*
 *_thr_tidveclock is an LWP mutex lock that must be held by a thread
 * before it changes the thread ID array.
 */
lwp_mutex_t _thr_tidveclock;

/*
 * _thr_alloc_tid(thread_desc_t *t)
 *	Allocates a unique thread ID (tid) and assigns it
 *	to the thread identified by the argument to the function.
 *
 *  Parameter/Calling State:
 *	No locks are held on entry.
 *
 *	t - pointer to the thread structure for which tid is to be allocated.
 *
 *	During processing the tidvec lock and the thread lock of 
 *	the calling thread are acquired.
 *	Signal handlers are assumed to be disabled.
 * 
 *  Return Values/Exit State:
 *	t->t_tid is initialized with an allocated thread ID.
 *	returns 0 on success and -1 on failure
 *
 *	On return, the thread lock of the calling thread remains locked.
 */

int
_thr_alloc_tid(thread_desc_t *t)
{
	tidvec_t *newid;
	int oldsize, newsize;
	int newtid;

	ASSERT(THR_ISSIGOFF(curthread));

	LOCK_TIDVEC;

	/*
	 * Try to Allocate a tid from the head of the free list (_thr_freetid).
 	 * If the free list is empty, then another list, larger by PAGESIZE 
         * bytes is allocated. The old entries are copied to the new
 	 * _thr_tidvec[] and the new entries are put onto the free list.
 	 */
	if (_thr_freetid == NULL) {
		oldsize = _thr_tidpages * PAGESIZE;
		newsize = ++_thr_tidpages * PAGESIZE;

		if (!_thr_alloc_chunk(0, newsize, (caddr_t *)&newid)) {
			UNLOCK_TIDVEC;
			return(-1);
		}

		/*
		 * _thr_total_tids is used only as an optimization
		 * in THREAD macro.
		 */

		_thr_total_tids = newsize/sizeof(tidvec_t);

		if (oldsize) {
			memmove(newid, _thr_tidvec, oldsize);
			_thr_free_chunk((caddr_t)_thr_tidvec, oldsize);
		} else {
			*newid =  _thr_tidvec[0];
			*(newid+1) = _thr_tidvec[1];
			oldsize = sizeof (_thr_tid0);
		}
		_thr_tidvec = newid;
		/*
		 * put allocated tids onto its free list.
		 */

		_thr_freetid  = (newid + oldsize/sizeof(tidvec_t));
		_thr_last_tid_index = (_thr_tidvec + newsize/sizeof(tidvec_t));

		for (newid = _thr_freetid; newid < (_thr_last_tid_index - 1); newid++) {
			newid->thr_ptr = (thread_desc_t *)(newid+1);
			newid->id_gen = 0;
		}
		newid->thr_ptr = NULL;
		newid->id_gen = 0;
		_thr_last_freetid = newid;
	}

	newid = _thr_freetid;

	_thr_freetid = (tidvec_t *) newid->thr_ptr;
	newid->thr_ptr = t;
	newtid = newid - _thr_tidvec;

	/*
	 * Check for tidvec overflow.
	 */


	ASSERT(newtid <= (~(~0 << THR_ID_SIZE) -1) || newid->id_gen <=  ~(~0 << THR_GEN_SIZE));

	t->t_tid = (newtid | (newid->id_gen << THR_ID_SIZE));

	LOCK_THREAD(t);
	UNLOCK_TIDVEC;
	PRINTF1("Allocated TID = %ld\n", (t->t_tid & THR_TID_MASK));
	PRINTF1("Gen of TID = %ld\n", newid->id_gen);
	return(0);
}

/*
 *  _thr_free_tid(thread_t tid)
 *	Puts a thread ID (tid) on the free list of thread IDs
 *	Freed tid is placed at the end of the free list.
 *
 *  Parameter/Calling State:
 *	No locks are held on entry and signal handlers must be disabled.
 *
 *	tid - is the thread ID to be freed.
 *
 *	During processing the tidvec lock is acquired.
 *
 *  Return Values/Exit State:
 *	returns no value
 *
 *	On return, no locks are held.
 */

void
_thr_free_tid(thread_t tid)
{

	ASSERT(THR_ISSIGOFF(curthread));
	LOCK_TIDVEC;

	/* if _thr_last_freetid is not set, then no new threads are created,
  	 * and there is not tid to free.
 	 */
	if (_thr_last_freetid == NULL) {
		UNLOCK_TIDVEC;
		return;
	}

	_thr_last_freetid->thr_ptr = (thread_desc_t *) &_thr_tidvec[tid & THR_TID_MASK];
	_thr_last_freetid = &_thr_tidvec[tid & THR_TID_MASK];
	_thr_last_freetid->thr_ptr = NULL;
	_thr_last_freetid->id_gen = (_thr_last_freetid->id_gen <  ~(~0 << THR_GEN_SIZE)) ? ++_thr_last_freetid->id_gen : 0;

	if (_thr_freetid == NULL) {
		/* This is the case then the last tid was taken from 
		 * the free list, and now one tid is released 
		 * therefore a free tid list has exactly one tid.
		 * Let's reset _thr_freetid to it and see what will
		 * happen next !
		 */
		_thr_freetid = _thr_last_freetid;
	}
	UNLOCK_TIDVEC;
	PRINTF1("Freed TID = %ld\n", (tid & THR_TID_MASK));
}


/*
 *  _thr_get_thread(thread_t tid)
 *      Find the thread assocatied with the tid.
 *
 *  Parameter/Calling State:
 *	On entry no locks are held and signal handlers are disabled.
 *
 *	tid - thread id of the thread to be found.
 *
 *      During processing the tidvec lock is acquired. If the thread exists,
 *	it's thread lock is also acquired.
 *
 *  Return Values/Exit State:
 *	Returns a pointer to the thread structure on success, NULL on failure
 *
 *	On return, the thread lock remains held if the thread exists.
 */


thread_desc_t *
_thr_get_thread(thread_t tid)
{
	thread_desc_t *tp;
	
	ASSERT(THR_ISSIGOFF(curthread));

	LOCK_TIDVEC;
	if ((tp = THREAD(tid)) != NULL) {
		LOCK_THREAD(tp);
	}
	UNLOCK_TIDVEC;
	return(tp);
}
