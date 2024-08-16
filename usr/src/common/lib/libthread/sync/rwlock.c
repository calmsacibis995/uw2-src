/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/rwlock.c	1.13"

#include <memory.h>
#include <stdlib.h>
#include <libthread.h>
#include <trace.h>

STATIC int _thr_rwlock_wait(rwlock_t *rwlock, int rdwr);

/*
 * int rwlock_init(rwlock_t *rwlock, int type, void *arg)
 *	initialize a read-write lock.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *      argument 'type' is an int: either USYNC_THREAD or USYNC_PROCESS
 *      argument 'arg' is not currently used
 *      The rw_mutex lock in 'rwlock' is initialized by calling mutex_init().
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The 'rwlock' lock is initialized to the type specified
 *		as the 'type' argument.
 *	EINVAL: The value of 'type' is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	No new locks are held on exit.
 *	
 */

int
rwlock_init(rwlock_t *rwlock, int type, void *arg)
{
	if (type != USYNC_THREAD && type != USYNC_PROCESS) {
		TRACE4(0, TR_CAT_RWLOCK, TR_EV_RWINIT, TR_CALL_ONLY,
		   rwlock, type, arg, EINVAL);
		return(EINVAL);
	}
	/*
	 * zero fill structure, since this is equivalent to a
	 * USYNC_THREAD condition variable by definition.
	 */
	memset((void *)rwlock, 0, sizeof(rwlock_t));

	if (type == USYNC_PROCESS) {
		rwlock->rw_type = type;
		if (_THR_MUTEX_INIT(&rwlock->rw_mutex, type, arg) == EINVAL) {
			TRACE4(0, TR_CAT_RWLOCK, TR_EV_RWINIT, TR_CALL_ONLY,
			   rwlock, type, arg, EINVAL);
			return(EINVAL);
		}
	}
	TRACE4(0, TR_CAT_RWLOCK, TR_EV_RWINIT, TR_CALL_ONLY, 
	   rwlock, type, arg, 0);
	return(0);
}

/*
 * int rw_rdlock(rwlock_t *rwlock)
 *	acquire the read-write lock in read mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If the caller has to block, it allocates a condition
 *	variable if one is needed, and calls _thr_cond_wait() on the condition 
 *	variable. When it is unblocked by a cond_signal()/rw_unlock(), it 
 *	deallocates the condition variable if it is the last reader awakened.
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in read mode.
 *	EINVAL: The value of 'rw_type' in rwlock is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or if the lock has been destroyed.
 *
 *	No new locks are held on exit.
 *	
 */

int
rw_rdlock(rwlock_t *rwlock)
{
	rwcv_t	*rwcvp, *np = NULL;
	thread_desc_t	*tp = curthread;
	int rval;
#ifdef TRACE
	int		did_i_block = TR_DATA_NOBLOCK;
#endif /* TRACE */

	TRACE1(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK, TR_CALL_FIRST, rwlock);
	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block, 0);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
#ifdef TRACE
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block, 0);
#endif /* TRACE */
		_thr_sigon(tp);
		return(EINVAL);
	}
	/*
	 * rwlock may be destroyed while we're waiting in the mutex_lock() call
	 */
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block, 0);
#endif /* TRACE */
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_THREAD) {
	    /*
	     * what should we check to determine if we should block or not?
	     * it's obvious we must block if rw_writer or rw_writerwanted is non-0.
	     * if they are both 0, it's still possible some readers are just
	     * waking up, except we acquired the rw_mutex before they could return
	     * from _thr_cond_wait(). In this case, we don't need to block since
	     * we simply acquire the lock in read mode (i.e., join those readers)
	     */
	    if (rwlock->rw_writer || rwlock->rw_writerwanted) {
		/*
		 * must queue the calling thread at the tail of the waiters.
		 * if last is for a reader, just _thr_cond_wait on that condvar.
		 * otherwise, must allocate a condvar for this first reader.
		 * in either case, thread blocks on the condvar until awakened
		 * by a rw_unlock(). If wakeup is caused by any other reason,
		 * go back to sleep to maintain FIFO order. 
		 */
#ifdef TRACE
		did_i_block = TR_DATA_BLOCK;
#endif /* TRACE */

		ASSERT(rwlock->rw_writerwanted == 0 || 
			(rwlock->rw_cvqhead != NULL && rwlock->rw_cvqtail != NULL));
		ASSERT(rwlock->rw_lwpcond.wanted == 0);
		rwcvp = rwlock->rw_cvqtail;
		if (rwcvp == NULL || rwcvp->rwcv_rw == RW_WRITER) {
			np = (rwcv_t *) malloc(sizeof(rwcv_t));
			if (np == NULL) {
				/* malloc failed */
				return(ENOMEM);
			}
			np->rwcv_rw = RW_READER;
			np->rwcv_wakeup = 0;
			np->rwcv_readerwanted = 1;
			np->rwcv_next = NULL;
			_THR_COND_INIT(&np->rwcv_cond, rwlock->rw_type, NULL);
			if (rwcvp == NULL)
				rwlock->rw_cvqhead = np;
			else
				rwcvp->rwcv_next = np;
			rwlock->rw_cvqtail = np;
			rwcvp = np;
		} else {	/* last waiter is a reader */
			ASSERT(rwcvp->rwcv_rw == RW_READER);
			ASSERT(rwcvp->rwcv_wakeup == 0);
#ifdef DEBUG
			if (rwlock->rw_cvqhead != rwcvp) {
				rwcv_t	*rp = rwlock->rw_cvqhead;
				while (rp->rwcv_next != rwcvp)
					rp = rp->rwcv_next;
				ASSERT(rp->rwcv_rw != RW_READER);
			}
#endif /* DEBUG */
			rwcvp->rwcv_readerwanted++;
		}
		/* wait until we are awakened for the right reason */
		while (rwcvp->rwcv_wakeup == 0) {
			/*
			 * use _thr_cond_wait() to allow signals to be
			 * accepted and handled.
			 */
			rval = _thr_cond_wait(&rwcvp->rwcv_cond, 
				&rwlock->rw_mutex,
				(lwp_mutex_t *) NULL,
				(timestruc_t *) 0, B_TRUE, B_TRUE);
			if (rval == EINTR) {
				/*
				 * cond_wait was interrupted by signal or
				 * suspend. We must recheck the condition
				 * to avoid missing a rw_unlock().
				 * Before we start again, we will dispense
				 * with any pending signals. This will
				 * only happen when we call sigon.
				 */
				(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
				_thr_sigon(tp);
				/*
				 * Now we have to recheck the condition
				 * for wake-up, turn off signals and	
				 * relock the rwlock mutex. If mutex_lock()
				 * fails, just free np; updating queue is not
				 * necessary since nothing can be guaranteed 
				 * without the mutex being locked.
				 */
				_thr_sigoff(tp);
				if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
					if (np) {
						free(np->rwcv_cond.c_syncq);
						free((void *)np); /* ok if np == NULL */
					}
					_thr_sigon(tp);
#ifdef TRACE
					TRACE3(tp, TR_CAT_RWLOCK, 
					   TR_EV_RW_RDLOCK, TR_CALL_SECOND,
					   EINVAL, TR_DATA_BLOCK, 0);
#endif /* TRACE */
					return (EINVAL);
				}
			}
		}
		/*
		 * if last reader waking up, remove self from
		 * list and free the structure rwcv_t
		 */
		if (--rwcvp->rwcv_readerwanted == 0) {
			if ((rwlock->rw_cvqhead = rwcvp->rwcv_next) == NULL) {
				rwlock->rw_cvqtail = NULL;
			}
			free(rwcvp->rwcv_cond.c_syncq);
			free((void *)rwcvp);
		}
	    }
	} else {	/* rw_type == USYNC_PROCESS */
		ASSERT(rwlock->rw_type == USYNC_PROCESS);
uproc_again:
	    if (rwlock->rw_writer || rwlock->rw_writerwanted) {
#ifdef TRACE
		did_i_block = 1;
#endif /* TRACE */
		ASSERT(rwlock->rw_cvqhead == NULL && rwlock->rw_cvqtail == NULL);
		rval = _thr_rwlock_wait(rwlock, PREPBLOCK_READER);
		if (rval == EINTR) {
			/*
			 * _thr_rwlock_wait() was interrupted by 
			 * signal, suspend or forkall. We must recheck 
			 * the condition to avoid missing a rw_unlock.
			 * Before we start again, we will dispense
			 * with any pending signals. This will
			 * only happen when we call sigon.
			 */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			/*
			 * Now we have to recheck the condition
			 * for wake-up, turn off signals and	
			 * relock the rwlock mutex. If mutex_lock()
			 * fails, just return EINVAL. 
			 */
			_thr_sigoff(tp);
			if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
#ifdef TRACE
				TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK,
				   TR_CALL_SECOND, EINVAL, did_i_block, 0);
#endif /* TRACE */
				_thr_sigon(tp);
				return (EINVAL);
			}
		} else if (rval != 0) {	/* EINVAL */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
			TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK,
			   TR_CALL_SECOND, EINVAL, did_i_block, 0);
#endif /* TRACE */
			_thr_sigon(tp);
			return (EINVAL);
		}
		/*
		 * a reader is waking up for the right reason only if
		 * rdwakecnt > 0; in case some waiting readers are interrupted
		 * by a signal, there may be more awakened readers than rdwakecnt.
		 * those left behind have to recheck the condition for blocking
		 */
		if (rwlock->rw_rdwakecnt == 0)
			goto uproc_again;
		rwlock->rw_rdwakecnt--;
	    }
	}
	rwlock->rw_readers++;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
	TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_RDLOCK, TR_CALL_SECOND,
	   0, did_i_block, rwlock->rw_readers);
#endif /* TRACE */
	_thr_sigon(tp);
	return (0);
}

/*
 * int rw_wrlock(rwlock_t *rwlock)
 *	acquire the read-write lock in write mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If the caller has to block, it allocates a condition
 *	variable, and calls _thr_cond_wait() on the condition variable. When it 
 *	is unblocked by a cond_signal()/rw_unlock(), it deallocates the 
 *	condition variable.
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in write mode.
 *	EINVAL: The value of 'rw_type' in rwlock is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or if the lock has been destroyed.
 *
 *	No new locks are held on exit.
 *	
 */

int
rw_wrlock(rwlock_t *rwlock)
{
	rwcv_t	*np, *rwcvp;
	thread_desc_t   *tp = curthread;
	int rval;
#ifdef TRACE
	int	did_i_block = TR_DATA_NOBLOCK;
#endif /* TRACE */

	TRACE1(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK, TR_CALL_FIRST, rwlock);

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
#ifdef TRACE
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block);
#endif /* TRACE */
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
#ifdef TRACE
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block);
#endif /* TRACE */
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK, TR_CALL_SECOND,
		   EINVAL, did_i_block);
#endif /* TRACE */
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_THREAD) {
	    /*
	     * what should we check to determine if we should block or not?
	     * it's obvious we must block if rw_readers or rw_writer is non-0.
	     * if they are both 0, it's still possible some writers have been 
	     * waiting, i.e., rw_writerwanted is non-0, or some readers have been
	     * waiting, i.e., rw_cvqhead is not NULL. Although the top waiting
	     * writer or readers should be waking up, this caller may get here
	     * before them (i.e., grabbed rw_mutex before those waking up could
	     * in _thr_cond_wait()), so it must also check if anyone is waiting.
	     */
	    if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_cvqhead != NULL) {
#ifdef TRACE
		did_i_block = TR_DATA_BLOCK;
#endif /* TRACE */
		rwcvp = rwlock->rw_cvqtail;
		/*
		 * must queue the calling thread at the end of the waiters.
		 * need to allocate a condvar for every writer.
		 * thread then blocks on the condvar until awakened
		 * by a rw_unlock(). If wakeup is caused by any other reason,
		 * go back to sleep since we must maintain FIFO order. 
		 */
		np = (rwcv_t *) malloc(sizeof(rwcv_t));
		if (np == NULL) {
			/* malloc failed */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			return(ENOMEM);
		}
		np->rwcv_rw = RW_WRITER;
		np->rwcv_wakeup = 0;
		np->rwcv_next = NULL;
		_THR_COND_INIT(&np->rwcv_cond, rwlock->rw_type, NULL);
		if (rwcvp == NULL) {
			rwlock->rw_cvqhead = np;	/* the first */
		} else {
			rwcvp->rwcv_next = np;
		}
		rwlock->rw_cvqtail = np;
		rwlock->rw_writerwanted++;
		while (np->rwcv_wakeup == 0) {
			rval = _thr_cond_wait(&np->rwcv_cond, 
				&rwlock->rw_mutex,
				(lwp_mutex_t *) NULL,
				(timestruc_t *) 0, B_TRUE, B_TRUE);
			if (rval == EINTR) {
				(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
				_thr_sigon(tp); /* let signal come in */
				_thr_sigoff(tp);
				if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
					if (np) {
						free(np->rwcv_cond.c_syncq);
						free((void *)np);
					}
#ifdef TRACE
					TRACE2(tp, TR_CAT_RWLOCK, 
					   TR_EV_RW_WRLOCK, TR_CALL_SECOND,
					   EINVAL, did_i_block);
#endif /* TRACE */
					_thr_sigon(tp);
					return (EINVAL);
				}
			}
		}
		/*
		 * on wakeup by a rw_unlock, we remove self from the list
		 * and free the rwcv_t
		 */
		ASSERT(np == rwlock->rw_cvqhead);
		ASSERT(rwlock->rw_writerwanted);
		rwlock->rw_writerwanted--;
		if ((rwlock->rw_cvqhead = np->rwcv_next) == NULL) {
			rwlock->rw_cvqtail = NULL;
		}
		free(np->rwcv_cond.c_syncq);
		free((void *)np);
	    }
	} else {	/* rw_type == USYNC_PROCESS */
	    ASSERT(rwlock->rw_type == USYNC_PROCESS);
uproc_again:
	    if (rwlock->rw_readers || rwlock->rw_writer || 
		  rwlock->rw_writerwanted || rwlock->rw_rdwakecnt) {
#ifdef TRACE
		did_i_block = TR_DATA_BLOCK;
#endif /* TRACE */
		rwlock->rw_writerwanted++;
		rval = _thr_rwlock_wait(rwlock, PREPBLOCK_WRITER);
		if (rval == EINTR) {
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			_thr_sigoff(tp);
			if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
#ifdef TRACE
				TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK,
				   TR_CALL_SECOND, EINVAL, did_i_block);
#endif /* TRACE */
				_thr_sigon(tp);
				return (EINVAL);
			}
		} else if (rval != 0) { /* EINVAL */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
			TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK,
			   TR_CALL_SECOND, EINVAL, did_i_block);
#endif /* TRACE */
			_thr_sigon(tp);
			return (EINVAL);
		}
		/*
		 * a writer is waking up for the right reason only if
		 * wrwakeup == 1; in case some other waiting writers are 
		 * interrupted by a signal, there may be more awakened writers 
		 * than 1. those finding wrwakeup == 0 have to recheck the 
		 * condition for blocking.
		 */
		if (rwlock->rw_wrwakeup == 0) {
			rwlock->rw_writerwanted--;
			goto uproc_again;
		}
		rwlock->rw_wrwakeup = 0;
		ASSERT(rwlock->rw_writerwanted);
		rwlock->rw_writerwanted--;
	   }
	}
	rwlock->rw_writer = 1;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
	TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_WRLOCK, TR_CALL_SECOND,
	   0, did_i_block);
#endif /* TRACE */
	_thr_sigon(tp);
	return (0);
}

/*
 * int rw_unlock(rwlock_t *rwlock)
 *	release a read-write lock.
 *
 * Parameter/Calling State:
 *      No library locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If caller has to unblock a waiter, it calls cond_broadcast()
 *	on the condition variable associated with the waiter(s).
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	If the read-write lock is in write mode, it is released. If
 *		it's in read mode, rw_readers is decremented by 1, and if it
 *		becomes 0 as a result, the lock is released.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	ENOLCK: The read-write lock was not locked.
 *
 *	No new locks are held on exit.
 *	
 */

int
rw_unlock(rwlock_t *rwlock)
{
	rwcv_t	*rwcvp;
	thread_desc_t *tp = curthread;
/* LINTED */
	int	rval;	/* linted - unused */
#ifdef TRACE
	int woke_someone = 0;
#endif /* TRACE */

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_UNLOCK, TR_CALL_ONLY,
		   rwlock, EINVAL, TR_DATA_DONTKNOW);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_UNLOCK, TR_CALL_ONLY,
		   rwlock, EINVAL, TR_DATA_DONTKNOW);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_UNLOCK, TR_CALL_ONLY,
		   rwlock, EINVAL, TR_DATA_DONTKNOW);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (!rwlock->rw_readers && !rwlock->rw_writer) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_UNLOCK, TR_CALL_ONLY,
		   rwlock, ENOLCK, TR_DATA_DONTKNOW);
		_thr_sigon(tp);
		return(ENOLCK);
	}
	/*
	 * we assume rw_unlock is always called by the owner of the rwlock.
	 * i.e., if rw_writer == 1, then the caller is the writer.
	 * we don't enforce since it's too costly to register/check owner.
	 */
	ASSERT((rwlock->rw_writer == 1) || (rwlock->rw_writer == 0));
	if (rwlock->rw_type == USYNC_THREAD) {
	    if (rwlock->rw_writer == 1) {
		ASSERT(rwlock->rw_readers == 0);
		/* 
		 * caller is the writer; reset rw_writer.
		 */
		rwlock->rw_writer = 0;
		rwcvp = rwlock->rw_cvqhead;
		if (rwcvp == NULL) {
			ASSERT(rwlock->rw_writerwanted == 0);
			goto rw_unlock_out;
		}
		/*
		 * we have to wake up the waiter, set wakeup so they know
		 * they wake up for the right reason
		 */
		ASSERT(rwcvp->rwcv_wakeup == 0);
		rwcvp->rwcv_wakeup = 1;
#ifdef TRACE
		woke_someone = 1;
#endif /* TRACE */
		/*
		 * wake up a writer or a group of readers.
		 * (the rwcv_t structure is deallocated by the waking up thread)
		 */
		(void) _THR_COND_BROADCAST(&rwcvp->rwcv_cond);
	    } else {
		/* 
		 * caller is a reader
		 */
		ASSERT(rwlock->rw_writer == 0);
		ASSERT(rwlock->rw_readers);
		if (--rwlock->rw_readers)
			goto rw_unlock_out;
		/* 
		 * the top waiter, if any, should be a writer
		 * however, it may happen that when a group of readers
		 * is waking up, one or more of them could complete
		 * their work and call rw_unlock() before others have
		 * had a chance to even increment rw_readers, thus causing 
		 * the --rwlock->rw_readers to become zero and the other
		 * still-waking-up readers to be awakened again. This
		 * requires the following checks.
		 */
		rwcvp = rwlock->rw_cvqhead;
		if (rwcvp != NULL) {
			if (rwcvp->rwcv_rw == RW_READER) {
				ASSERT(rwcvp->rwcv_wakeup == 1);
				goto rw_unlock_out;
			}
			ASSERT(rwcvp->rwcv_rw == RW_WRITER);
			ASSERT(rwlock->rw_writerwanted);
			rwcvp->rwcv_wakeup = 1;
#ifdef TRACE
			woke_someone = 1;
#endif /* TRACE */
			_THR_COND_SIGNAL(&rwcvp->rwcv_cond);
		}
	    }
	} else {
		ASSERT(rwlock->rw_type == USYNC_PROCESS);
		if (rwlock->rw_writer == 1) {
			ASSERT(rwlock->rw_readers == 0);
			/* 
			 * caller is the writer; reset rw_writer.
			 */
			rwlock->rw_writer = 0;
	   		if (rwlock->rw_lwpcond.wanted == 0) { /* no waiters */
				goto rw_unlock_out;
			}
			/*
			 * we have to wake up a waiter.
			 * first try waking up a reader. if found, unblock until
			 * a writer or end of list. if first not a reader, it
			 * must be a writer, wake it up.
			 */
			if (!unblock((vaddr_t)&rwlock->rw_lwpcond,
			   	    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_READER)) {
				rwlock->rw_rdwakecnt = 1; /* first reader */
				while ((rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_READER)) == 0)
					rwlock->rw_rdwakecnt++;
				ASSERT(rval != 0 && rwlock->rw_rdwakecnt);
			} else {
				if ((rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_WRITER)) == 0) {
					rwlock->rw_wrwakeup = 1;
#ifdef TRACE
					woke_someone = 1;
#endif /* TRACE */
				}
				ASSERT(rwlock->rw_wrwakeup);
			}
		} else {
			/* 
			 * caller is a reader
		 	 */
			ASSERT(rwlock->rw_writer == 0);
			ASSERT(rwlock->rw_readers);
			if (--rwlock->rw_readers || rwlock->rw_rdwakecnt)
				goto rw_unlock_out;
	   		if (rwlock->rw_writerwanted == 0) /* no waiters */
				goto rw_unlock_out;
			rwlock->rw_wrwakeup = 1; /* must be a writer */
			rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				(char *)&rwlock->rw_lwpcond.wanted,
				UNBLOCK_WRITER);
			ASSERT(rval == 0);
#ifdef TRACE
			woke_someone = 1;
#endif /* TRACE */
		}
	}
rw_unlock_out:
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
#ifdef TRACE
	TRACE3(tp, TR_CAT_RWLOCK, TR_EV_RW_UNLOCK, TR_CALL_ONLY,
	   rwlock, 0, woke_someone);
#endif /* TRACE */
	_thr_sigon(tp);
	return (0);
}

/*
 * int rw_tryrdlock(rwlock_t *rwlock)
 *	conditionally acquire the read-write lock in read mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in read mode.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' cannot be acquired because of contention.
 *
 *	No new locks are held on exit.
 *	
 */

int
rw_tryrdlock(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;
	int rval;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYRD, TR_CALL_ONLY,
		   rwlock, EINVAL);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYRD, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYRD, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	/* same test for availability as in rw_rdlock() */
	if (rwlock->rw_writer || rwlock->rw_writerwanted) {
		rval = EBUSY;
	} else {
		rwlock->rw_readers++;
		rval = 0;
	}
        _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYRD, TR_CALL_ONLY, rwlock, rval);
	_thr_sigon(tp);
	return(rval);
}

/*
 * int rw_trywrlock(rwlock_t *rwlock)
 *	conditionally acquire the read-write lock in write mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in write mode.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' cannot be acquired because of contention.
 *
 *	No new locks are held on exit.
 *	
 */

int
rw_trywrlock(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;
	int rval;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYWR, TR_CALL_ONLY,
		   rwlock, EINVAL);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYWR, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYWR, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	/* same test for availability as in rw_wrlock() */
	if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_writerwanted
	    || rwlock->rw_cvqhead != NULL || rwlock->rw_rdwakecnt) {
		rval = EBUSY;
	} else {
		rwlock->rw_writer = 1;
		rval = 0;
	}
        _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RW_TRYWR, TR_CALL_ONLY, rwlock, rval);
	_thr_sigon(tp);
	return(rval);
}

/*
 * int rwlock_destroy(rwlock_t *rwlock)
 *	attempt to destroy a read-write lock.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is destroyed.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' was already in locked state.
 *
 *	No new locks are held on exit.
 *	
 */

int
rwlock_destroy(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RWDESTROY, TR_CALL_ONLY,
		   rwlock, EINVAL);
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RWDESTROY, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RWDESTROY, TR_CALL_ONLY,
		   rwlock, EINVAL);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_writerwanted
	    || rwlock->rw_cvqhead != NULL || rwlock->rw_rdwakecnt) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RWDESTROY, TR_CALL_ONLY,
		   rwlock, EBUSY);
		_thr_sigon(tp);
		return(EBUSY);
	}
	/*
	 * now invalidate the read-write lock to prevent any new attempts
	 * to use it
	 */
	rwlock->rw_type = USYNC_DESTROYED;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	TRACE2(tp, TR_CAT_RWLOCK, TR_EV_RWDESTROY, TR_CALL_ONLY, rwlock, 0);
	_thr_sigon(tp);
	/* 
	 * we don't bother to destroy the mutex_t, rw_mutex, since the
	 * rwlock_t is USYNC_DESTROYED.
	 */
	return (0);
}

/*
 * int
 * _thr_rwlock_wait(rwlock_t *rwlock, int rdwr)
 *
 * Parameter/Calling State:
 *      rwlock->rw_mutex is held on entry
 *      signal is disabled (_thr_sigoff) on entry
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *      argument 'rdwr' is either PREPBLOCK_WRITER or PREPBLOCK_READER.
 *
 *      During processing the rw_mutex lock of the 'rwlock' is released and
 * 	re-acquired. It calls prepblock() and block() to block the calling
 *	thread. 
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in read mode.
 *	EINTR: block was interrupted by signal, suspend, or forkall.
 *	EINVAL: returned by block() or mutex_lock() 
 *
 *      rwlock->rw_mutex is held on exit
 *      signal remains disabled (_thr_sigoff) on exit
 *	
 */

STATIC int
_thr_rwlock_wait(rwlock_t *rwlock, int rdwr)
{
	int rval, rval2;

	/*
	 * Call prepblock to place the calling LWP 
	 * on the sync-queue associated with rw_lwpcond
	 * Prepblock may set rw_lwpcond.wanted.
	 */
	while ((rval = prepblock((vaddr_t)&rwlock->rw_lwpcond,
    	       (char *)&rwlock->rw_lwpcond.wanted, rdwr)) == EINVAL) {
		/*
		 * The calling LWP is placed on another
		 * sync-queue. Remove it from the 
		 * sync-queue and prepblock again.
		 */
		cancelblock();
		continue;
	}
	if (rval != 0)
		return(rval);
	/*
	 * Release the mutex.
	 */
	rval = _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		
	if (rval != 0) {
		/*
		 * mutex_unlock failed.
		 * Remove the calling LWP from the 
		 * sync-queue, and return.
		 */
		cancelblock();
		return(rval);
	}
	/*
	 * Give up the processor and wait for 
	 * being awakened.
	 */
	if ((rval = block(NULL)) != 0) {
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
		case EFAULT:
		case EINVAL:
			/*
			 * The calling LWP may be still
			 * placed on the sync-queue. Remove it.
			 */
			cancelblock();
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
	 * Re-acquire the mutex.
	 */
	rval2 = _THR_MUTEX_LOCK(&rwlock->rw_mutex);

	/*
	 * If break was caused by a pending signal, return.
	 */
	if (rval == EINTR)
		return(rval);
	if (rval2 != 0) {
		/*
		 * mutex_lock failed. Replace a return value 
		 * with rval2 only if block returned 0
		 */
		if (rval == 0)
			rval = rval2;
	}
	return(rval);
}



/*
 * The following routines provide the same functionality as the
 * interfaces above except they do not log trace information.
 * They are provided for internal use by the library and must be
 * kept in sync with the above interfaces, e.g., _thr_rwlock_init
 * must be identical to rwlock_init except for calls to TRACE.
 */

#ifdef TRACE

/*
 * int _thr_notrace_rwlock_init(rwlock_t *rwlock, int type, void *arg)
 *	initialize a read-write lock.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *      argument 'type' is an int: either USYNC_THREAD or USYNC_PROCESS
 *      argument 'arg' is not currently used
 *      The rw_mutex lock in 'rwlock' is initialized by calling mutex_init().
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	0:	The 'rwlock' lock is initialized to the type specified
 *		as the 'type' argument.
 *	EINVAL: The value of 'type' is neither USYNC_THREAD nor USYNC_PROCESS.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rwlock_init(rwlock_t *rwlock, int type, void *arg)
{
	if (type != USYNC_THREAD && type != USYNC_PROCESS) {
		return(EINVAL);
	}
	/*
	 * zero fill structure, since this is equivalent to a
	 * USYNC_THREAD condition variable by definition.
	 */
	memset((void *)rwlock, 0, sizeof(rwlock_t));

	if (type == USYNC_PROCESS) {
		rwlock->rw_type = type;
		if (_THR_MUTEX_INIT(&rwlock->rw_mutex, type, arg) == EINVAL) {
			return(EINVAL);
		}
	}
	return(0);
}

/*
 * int _thr_notrace_rw_rdlock(rwlock_t *rwlock)
 *	acquire the read-write lock in read mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If the caller has to block, it allocates a condition
 *	variable if one is needed, and calls _thr_cond_wait() on the condition 
 *	variable. When it is unblocked by a cond_signal()/rw_unlock(), it 
 *	deallocates the condition variable if it is the last reader awakened.
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in read mode.
 *	EINVAL: The value of 'rw_type' in rwlock is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or if the lock has been destroyed.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rw_rdlock(rwlock_t *rwlock)
{
	rwcv_t	*rwcvp, *np = NULL;
	thread_desc_t	*tp = curthread;
	int rval;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	/*
	 * rwlock may be destroyed while we're waiting in the mutex_lock() call
	 */
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_THREAD) {
	    /*
	     * what should we check to determine if we should block or not?
	     * it's obvious we must block if rw_writer or rw_writerwanted is non-0.
	     * if they are both 0, it's still possible some readers are just
	     * waking up, except we acquired the rw_mutex before they could return
	     * from _thr_cond_wait(). In this case, we don't need to block since
	     * we simply acquire the lock in read mode (i.e., join those readers)
	     */
	    if (rwlock->rw_writer || rwlock->rw_writerwanted) {
		/*
		 * must queue the calling thread at the tail of the waiters.
		 * if last is for a reader, just _thr_cond_wait on that condvar.
		 * otherwise, must allocate a condvar for this first reader.
		 * in either case, thread blocks on the condvar until awakened
		 * by a rw_unlock(). If wakeup is caused by any other reason,
		 * go back to sleep to maintain FIFO order. 
		 */

		ASSERT(rwlock->rw_writerwanted == 0 || 
			(rwlock->rw_cvqhead != NULL && rwlock->rw_cvqtail != NULL));
		ASSERT(rwlock->rw_lwpcond.wanted == 0);
		rwcvp = rwlock->rw_cvqtail;
		if (rwcvp == NULL || rwcvp->rwcv_rw == RW_WRITER) {
			np = (rwcv_t *) malloc(sizeof(rwcv_t));
			if (np == NULL) {
				/* malloc failed */
				return(ENOMEM);
			}
			np->rwcv_rw = RW_READER;
			np->rwcv_wakeup = 0;
			np->rwcv_readerwanted = 1;
			np->rwcv_next = NULL;
			_THR_COND_INIT(&np->rwcv_cond, rwlock->rw_type, NULL);
			if (rwcvp == NULL)
				rwlock->rw_cvqhead = np;
			else
				rwcvp->rwcv_next = np;
			rwlock->rw_cvqtail = np;
			rwcvp = np;
		} else {	/* last waiter is a reader */
			ASSERT(rwcvp->rwcv_rw == RW_READER);
			ASSERT(rwcvp->rwcv_wakeup == 0);
#ifdef DEBUG
			if (rwlock->rw_cvqhead != rwcvp) {
				rwcv_t	*rp = rwlock->rw_cvqhead;
				while (rp->rwcv_next != rwcvp)
					rp = rp->rwcv_next;
				ASSERT(rp->rwcv_rw != RW_READER);
			}
#endif /* DEBUG */
			rwcvp->rwcv_readerwanted++;
		}
		/* wait until we are awakened for the right reason */
		while (rwcvp->rwcv_wakeup == 0) {
			/*
			 * use _thr_cond_wait() to allow signals to be
			 * accepted and handled.
			 */
			rval = _thr_cond_wait(&rwcvp->rwcv_cond, 
				&rwlock->rw_mutex,
				(lwp_mutex_t *) NULL,
				(timestruc_t *) 0, B_TRUE, B_TRUE);
			if (rval == EINTR) {
				/*
				 * cond_wait was interrupted by signal or
				 * suspend. We must recheck the condition
				 * to avoid missing a rw_unlock().
				 * Before we start again, we will dispense
				 * with any pending signals. This will
				 * only happen when we call sigon.
				 */
				(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
				_thr_sigon(tp);
				/*
				 * Now we have to recheck the condition
				 * for wake-up, turn off signals and	
				 * relock the rwlock mutex. If mutex_lock()
				 * fails, just free np; updating queue is not
				 * necessary since nothing can be guaranteed 
				 * without the mutex being locked.
				 */
				_thr_sigoff(tp);
				if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
					if (np) {
						free(np->rwcv_cond.c_syncq);
						free((void *)np); /* ok if np == NULL */
					}
					_thr_sigon(tp);
					return (EINVAL);
				}
			}
		}
		/*
		 * if last reader waking up, remove self from
		 * list and free the structure rwcv_t
		 */
		if (--rwcvp->rwcv_readerwanted == 0) {
			if ((rwlock->rw_cvqhead = rwcvp->rwcv_next) == NULL) {
				rwlock->rw_cvqtail = NULL;
			}
			free(rwcvp->rwcv_cond.c_syncq);
			free((void *)rwcvp);
		}
	    }
	} else {	/* rw_type == USYNC_PROCESS */
		ASSERT(rwlock->rw_type == USYNC_PROCESS);
uproc_again:
	    if (rwlock->rw_writer || rwlock->rw_writerwanted) {
		ASSERT(rwlock->rw_cvqhead == NULL && rwlock->rw_cvqtail == NULL);
		rval = _thr_rwlock_wait(rwlock, PREPBLOCK_READER);
		if (rval == EINTR) {
			/*
			 * _thr_rwlock_wait() was interrupted by 
			 * signal, suspend or forkall. We must recheck 
			 * the condition to avoid missing a rw_unlock.
			 * Before we start again, we will dispense
			 * with any pending signals. This will
			 * only happen when we call sigon.
			 */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			/*
			 * Now we have to recheck the condition
			 * for wake-up, turn off signals and	
			 * relock the rwlock mutex. If mutex_lock()
			 * fails, just return EINVAL. 
			 */
			_thr_sigoff(tp);
			if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
				_thr_sigon(tp);
				return (EINVAL);
			}
		} else if (rval != 0) {	/* EINVAL */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			return (EINVAL);
		}
		/*
		 * a reader is waking up for the right reason only if
		 * rdwakecnt > 0; in case some waiting readers are interrupted
		 * by a signal, there may be more awakened readers than rdwakecnt.
		 * those left behind have to recheck the condition for blocking
		 */
		if (rwlock->rw_rdwakecnt == 0)
			goto uproc_again;
		rwlock->rw_rdwakecnt--;
	    }
	}
	rwlock->rw_readers++;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	return (0);
}

/*
 * int _thr_notrace_rw_wrlock(rwlock_t *rwlock)
 *	acquire the read-write lock in write mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If the caller has to block, it allocates a condition
 *	variable, and calls _thr_cond_wait() on the condition variable. When it 
 *	is unblocked by a cond_signal()/rw_unlock(), it deallocates the 
 *	condition variable.
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in write mode.
 *	EINVAL: The value of 'rw_type' in rwlock is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or if the lock has been destroyed.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rw_wrlock(rwlock_t *rwlock)
{
	rwcv_t	*np, *rwcvp;
	thread_desc_t   *tp = curthread;
	int rval;


	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_THREAD) {
	    /*
	     * what should we check to determine if we should block or not?
	     * it's obvious we must block if rw_readers or rw_writer is non-0.
	     * if they are both 0, it's still possible some writers have been 
	     * waiting, i.e., rw_writerwanted is non-0, or some readers have been
	     * waiting, i.e., rw_cvqhead is not NULL. Although the top waiting
	     * writer or readers should be waking up, this caller may get here
	     * before them (i.e., grabbed rw_mutex before those waking up could
	     * in _thr_cond_wait()), so it must also check if anyone is waiting.
	     */
	    if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_cvqhead != NULL) {
		rwcvp = rwlock->rw_cvqtail;
		/*
		 * must queue the calling thread at the end of the waiters.
		 * need to allocate a condvar for every writer.
		 * thread then blocks on the condvar until awakened
		 * by a rw_unlock(). If wakeup is caused by any other reason,
		 * go back to sleep since we must maintain FIFO order. 
		 */
		np = (rwcv_t *) malloc(sizeof(rwcv_t));
		if (np == NULL) {
			/* malloc failed */
			return(ENOMEM);
		}
		np->rwcv_rw = RW_WRITER;
		np->rwcv_wakeup = 0;
		np->rwcv_next = NULL;
		_THR_COND_INIT(&np->rwcv_cond, rwlock->rw_type, NULL);
		if (rwcvp == NULL) {
			rwlock->rw_cvqhead = np;	/* the first */
		} else {
			rwcvp->rwcv_next = np;
		}
		rwlock->rw_cvqtail = np;
		rwlock->rw_writerwanted++;
		while (np->rwcv_wakeup == 0) {
			rval = _thr_cond_wait(&np->rwcv_cond, 
				&rwlock->rw_mutex,
				(lwp_mutex_t *) NULL,
				(timestruc_t *) 0, B_TRUE, B_TRUE);
			if (rval == EINTR) {
				(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
				_thr_sigon(tp); /* let signal come in */
				_thr_sigoff(tp);
				if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
					if (np) {
						free(np->rwcv_cond.c_syncq);
						free((void *)np);
					}
					_thr_sigon(tp);
					return (EINVAL);
				}
			}
		}
		/*
		 * on wakeup by a rw_unlock, we remove self from the list
		 * and free the rwcv_t
		 */
		ASSERT(np == rwlock->rw_cvqhead);
		ASSERT(rwlock->rw_writerwanted);
		rwlock->rw_writerwanted--;
		if ((rwlock->rw_cvqhead = np->rwcv_next) == NULL) {
			rwlock->rw_cvqtail = NULL;
		}
		free(np->rwcv_cond.c_syncq);
		free((void *)np);
	    }
	} else {	/* rw_type == USYNC_PROCESS */
	    ASSERT(rwlock->rw_type == USYNC_PROCESS);
uproc_again:
	    if (rwlock->rw_readers || rwlock->rw_writer || 
		  rwlock->rw_writerwanted || rwlock->rw_rdwakecnt) {
		rwlock->rw_writerwanted++;
		rval = _thr_rwlock_wait(rwlock, PREPBLOCK_WRITER);
		if (rval == EINTR) {
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			_thr_sigoff(tp);
			if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
				_thr_sigon(tp);
				return (EINVAL);
			}
		} else if (rval != 0) { /* EINVAL */
			(void) _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
			_thr_sigon(tp);
			return (EINVAL);
		}
		/*
		 * a writer is waking up for the right reason only if
		 * wrwakeup == 1; in case some other waiting writers are 
		 * interrupted by a signal, there may be more awakened writers 
		 * than 1. those finding wrwakeup == 0 have to recheck the 
		 * condition for blocking.
		 */
		if (rwlock->rw_wrwakeup == 0) {
			rwlock->rw_writerwanted--;
			goto uproc_again;
		}
		rwlock->rw_wrwakeup = 0;
		ASSERT(rwlock->rw_writerwanted);
		rwlock->rw_writerwanted--;
	   }
	}
	rwlock->rw_writer = 1;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	return (0);
}

/*
 * int _thr_notrace_rw_unlock(rwlock_t *rwlock)
 *	release a read-write lock.
 *
 * Parameter/Calling State:
 *      No library locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released. If caller has to unblock a waiter, it calls cond_broadcast()
 *	on the condition variable associated with the waiter(s).
 *	Thread lock and synclock/sleep queue may be locked/released indirectly.
 *
 * Return Values/Exit State:
 *	0:	If the read-write lock is in write mode, it is released. If
 *		it's in read mode, rw_readers is decremented by 1, and if it
 *		becomes 0 as a result, the lock is released.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	ENOLCK: The read-write lock was not locked.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rw_unlock(rwlock_t *rwlock)
{
	rwcv_t	*rwcvp;
	thread_desc_t *tp = curthread;
	int	rval;	/* linted - unused */

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (!rwlock->rw_readers && !rwlock->rw_writer) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(ENOLCK);
	}
	/*
	 * we assume rw_unlock is always called by the owner of the rwlock.
	 * i.e., if rw_writer == 1, then the caller is the writer.
	 * we don't enforce since it's too costly to register/check owner.
	 */
	ASSERT((rwlock->rw_writer == 1) || (rwlock->rw_writer == 0));
	if (rwlock->rw_type == USYNC_THREAD) {
	    if (rwlock->rw_writer == 1) {
		ASSERT(rwlock->rw_readers == 0);
		/* 
		 * caller is the writer; reset rw_writer.
		 */
		rwlock->rw_writer = 0;
		rwcvp = rwlock->rw_cvqhead;
		if (rwcvp == NULL) {
			ASSERT(rwlock->rw_writerwanted == 0);
			goto rw_unlock_out;
		}
		/*
		 * we have to wake up the waiter, set wakeup so they know
		 * they wake up for the right reason
		 */
		ASSERT(rwcvp->rwcv_wakeup == 0);
		rwcvp->rwcv_wakeup = 1;
		/*
		 * wake up a writer or a group of readers.
		 * (the rwcv_t structure is deallocated by the waking up thread)
		 */
		(void) _THR_COND_BROADCAST(&rwcvp->rwcv_cond);
	    } else {
		/* 
		 * caller is a reader
		 */
		ASSERT(rwlock->rw_writer == 0);
		ASSERT(rwlock->rw_readers);
		if (--rwlock->rw_readers)
			goto rw_unlock_out;
		/* 
		 * the top waiter, if any, should be a writer
		 * however, it may happen that when a group of readers
		 * is waking up, one or more of them could complete
		 * their work and call rw_unlock() before others have
		 * had a chance to even increment rw_readers, thus causing 
		 * the --rwlock->rw_readers to become zero and the other
		 * still-waking-up readers to be awakened again. This
		 * requires the following checks.
		 */
		rwcvp = rwlock->rw_cvqhead;
		if (rwcvp != NULL) {
			if (rwcvp->rwcv_rw == RW_READER) {
				ASSERT(rwcvp->rwcv_wakeup == 1);
				goto rw_unlock_out;
			}
			ASSERT(rwcvp->rwcv_rw == RW_WRITER);
			ASSERT(rwlock->rw_writerwanted);
			rwcvp->rwcv_wakeup = 1;
			_THR_COND_SIGNAL(&rwcvp->rwcv_cond);
		}
	    }
	} else {
		ASSERT(rwlock->rw_type == USYNC_PROCESS);
		if (rwlock->rw_writer == 1) {
			ASSERT(rwlock->rw_readers == 0);
			/* 
			 * caller is the writer; reset rw_writer.
			 */
			rwlock->rw_writer = 0;
	   		if (rwlock->rw_lwpcond.wanted == 0) { /* no waiters */
				goto rw_unlock_out;
			}
			/*
			 * we have to wake up a waiter.
			 * first try waking up a reader. if found, unblock until
			 * a writer or end of list. if first not a reader, it
			 * must be a writer, wake it up.
			 */
			if (!unblock((vaddr_t)&rwlock->rw_lwpcond,
			   	    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_READER)) {
				rwlock->rw_rdwakecnt = 1; /* first reader */
				while ((rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_READER)) == 0)
					rwlock->rw_rdwakecnt++;
				ASSERT(rval != 0 && rwlock->rw_rdwakecnt);
			} else {
				if ((rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				    (char *)&rwlock->rw_lwpcond.wanted,
				    UNBLOCK_WRITER)) == 0) {
					rwlock->rw_wrwakeup = 1;
				}
				ASSERT(rwlock->rw_wrwakeup);
			}
		} else {
			/* 
			 * caller is a reader
		 	 */
			ASSERT(rwlock->rw_writer == 0);
			ASSERT(rwlock->rw_readers);
			if (--rwlock->rw_readers || rwlock->rw_rdwakecnt)
				goto rw_unlock_out;
	   		if (rwlock->rw_writerwanted == 0) /* no waiters */
				goto rw_unlock_out;
			rwlock->rw_wrwakeup = 1; /* must be a writer */
			rval = unblock((vaddr_t)&rwlock->rw_lwpcond,
				(char *)&rwlock->rw_lwpcond.wanted,
				UNBLOCK_WRITER);
			ASSERT(rval == 0);
		}
	}
rw_unlock_out:
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	return (0);
}

/*
 * int _thr_notrace_rw_tryrdlock(rwlock_t *rwlock)
 *	conditionally acquire the read-write lock in read mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in read mode.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' cannot be acquired because of contention.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rw_tryrdlock(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;
	int rval;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	/* same test for availability as in rw_rdlock() */
	if (rwlock->rw_writer || rwlock->rw_writerwanted) {
		rval = EBUSY;
	} else {
		rwlock->rw_readers++;
		rval = 0;
	}
        _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	return(rval);
}

/*
 * int _thr_notrace_rw_trywrlock(rwlock_t *rwlock)
 *	conditionally acquire the read-write lock in write mode.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is acquired in write mode.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' cannot be acquired because of contention.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rw_trywrlock(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;
	int rval;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	/* same test for availability as in rw_wrlock() */
	if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_writerwanted
	    || rwlock->rw_cvqhead != NULL || rwlock->rw_rdwakecnt) {
		rval = EBUSY;
	} else {
		rwlock->rw_writer = 1;
		rval = 0;
	}
        _THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	return(rval);
}

/*
 * int _thr_notrace_rwlock_destroy(rwlock_t *rwlock)
 *	attempt to destroy a read-write lock.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *      argument 'rwlock' is a pointer to a read-write lock
 *
 *      During processing the rw_mutex lock of the 'rwlock' is acquired and
 * 	released.
 *
 * Return Values/Exit State:
 *	0:	The read-write lock is destroyed.
 *	EINVAL: The value of rw_type in 'rwlock' is neither USYNC_THREAD nor 
 *		USYNC_PROCESS, or the lock has been destroyed.
 *	EBUSY:  The 'rwlock' was already in locked state.
 *
 *	No new locks are held on exit.
 *	
 */

int
_thr_notrace_rwlock_destroy(rwlock_t *rwlock)
{
	thread_desc_t   *tp = curthread;

	if (rwlock->rw_type != USYNC_THREAD && 
	    rwlock->rw_type != USYNC_PROCESS) {
		return(EINVAL);
	}
	_thr_sigoff(tp);
	if (_THR_MUTEX_LOCK(&rwlock->rw_mutex) == EINVAL) {
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_type == USYNC_DESTROYED) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EINVAL);
	}
	if (rwlock->rw_readers || rwlock->rw_writer || rwlock->rw_writerwanted
	    || rwlock->rw_cvqhead != NULL || rwlock->rw_rdwakecnt) {
		_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
		_thr_sigon(tp);
		return(EBUSY);
	}
	/*
	 * now invalidate the read-write lock to prevent any new attempts
	 * to use it
	 */
	rwlock->rw_type = USYNC_DESTROYED;
	_THR_MUTEX_UNLOCK(&rwlock->rw_mutex);
	_thr_sigon(tp);
	/* 
	 * we don't bother to destroy the mutex_t, rw_mutex, since the
	 * rwlock_t is USYNC_DESTROYED.
	 */
	return (0);
}

#endif /* TRACE */
