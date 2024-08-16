/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/rmutex.c	1.4"

#include <unistd.h>
#include <libthread.h>

/*
 * rmutex_init(rmutex_t *mutex, int type, void *arg)
 *	initializes a recursive mutex to the specified type
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	first argument is a pointer to an rmutex
 *	second argument is an int: either USYNC_THREAD or USYNC_PROCESS
 *	third argument is not currently used
 *
 *	no locks are acquired during processing
 *
 * Return Values/Exit State:
 *	Returns 0 on success or EINVAL if type is invalid.
 *	On success, mutex is initialized to the specified type.
 */
/* ARGSUSED */
int
rmutex_init(rmutex_t *rmutex, int type, void *arg)
{
	int rval;

	rval = _THR_MUTEX_INIT(&(rmutex->rm_mutex), type, NULL);
	if (rval == 0) {
		rmutex->rm_pid = 0;
		rmutex->rm_owner = 0;
		rmutex->rm_depth = 0;
	}
	return(rval);
}



/*
 * rmutex_lock(rmutex_t *rmutex)
 *	obtains the specified rmutex lock, blocking if necessary
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a rmutex.
 *
 *	The lock of the calling thread and the sync_lock of the
 *	desired rmutex are acquired during processing; also the
 *	desired rmutex is locked.
 *
 * Return Values/Exit State:
 *	Returns zero and acquires the specified rmutex on success; 
 *	otherwise, returns an appropriate errno value.
 */

int
rmutex_lock(rmutex_t *rmutex)
{
	int		rval = 0;
	boolean_t	checkpid;
	pid_t		mypid;
	thread_desc_t	*curtp = curthread;

	switch(rmutex->rm_mutex.m_type) {
	case USYNC_THREAD:
		checkpid = B_FALSE;
		break;
	case USYNC_PROCESS:
		checkpid = B_TRUE;
		mypid = getpid();
		break;
	default:
		return(EINVAL);
		/* NOTREACHED */
		break;
	}
	_thr_sigoff(curtp);  /* NEW */
	if (LWP_MUTEX_TRYLOCK(&(rmutex->rm_mutex.m_lmutex)) != 0) {
		/* lock is already locked */
		if ((rmutex->rm_owner == curtp->t_tid) &&
		    ((checkpid == B_FALSE) || (rmutex->rm_pid == mypid))) {
			/*
			 * caller owns the lock so simply increment
			 * the depth count and return success.
			 */
			rmutex->rm_depth++;
		} else {
			/*
			 * someone else owns the lock so wait till the
			 * lock is available, then record owner info in it.
			 */
			/* NEXT LINE IS CHANGED */
			rval = _thr_mutex_lock_sigoff(&(rmutex->rm_mutex), 
			    curtp);
			if (rval == 0) {
				ASSERT(rmutex->rm_depth == 0);
				ASSERT(rmutex->rm_owner == 0);
				rmutex->rm_owner = curtp->t_tid;
				rmutex->rm_depth = 1;
				if (checkpid == B_TRUE)
					rmutex->rm_pid = mypid;
			}
		}
	} else {
		/*
		 * got the mutex from the unlocked state; record owner
		 * info in it.
		 */
		ASSERT(rmutex->rm_depth == 0);
		ASSERT(rmutex->rm_owner == 0);
		rmutex->rm_owner = curtp->t_tid;
		rmutex->rm_depth = 1;
		if (checkpid == B_TRUE)
			rmutex->rm_pid = mypid;
	}
	_thr_sigon(curtp);  /* NEW */
	return(rval);
}



/*
 * rmutex_trylock(rmutex_t *rmutex)
 *	makes a single attempt to obtain a rmutex
 *
 * Parameter/Calling State:
 *	No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a rmutex.
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	Returns zero if the lock was available and acquires the
 *	lock; otherwise returns EBUSY and doesn't acquire the lock.
 */

int
rmutex_trylock(rmutex_t *rmutex)
{
	boolean_t	checkpid;
	thread_desc_t	*curtp = curthread;
	int		rval = EBUSY;

	/*
	 * determine whether the rmutex is valid and whether we need to
	 * check process ID of owner
	 */
	switch(rmutex->rm_mutex.m_type) {
	case USYNC_THREAD:
		checkpid = B_FALSE;
		break;
	case USYNC_PROCESS:
		checkpid = B_TRUE;
		break;
	default:
		return(EINVAL);
		/* NOTREACHED */
		break;
	}

	/* try to get the lock */
	if (LWP_MUTEX_TRYLOCK(&(rmutex->rm_mutex.m_lmutex)) == 0) {
		/*
		 * lock was available
		 */
		rmutex->rm_owner = curtp->t_tid;
		rmutex->rm_depth = 1;
		if (checkpid == B_TRUE)
			rmutex->rm_pid = getpid();
		rval = 0;
	} else {
		/*
		 * lock was held; check to see if caller holds it.
		 */
		if ((rmutex->rm_owner == curtp->t_tid) &&
		    ((checkpid == B_FALSE) || (rmutex->rm_pid == getpid()))) {
			/* caller owns the lock */
			rmutex->rm_depth++;
			rval = 0;
		}
	}
	/* lock was held by someone other than the caller */
	return(rval);
}



/*
 * rmutex_unlock(rmutex_t *rmutex)
 *	unlocks a rmutex and awakens a waiting thread if one exists.
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a rmutex.
 *
 *	During processing, the sync_lock of the rmutex may be acquired and
 *	the thread lock of a thread that is awakened may be acquired.
 *
 * Return Values/Exit State:
 *	Returns 0 on success; otherwise, an errno value.  On success, if 
 *	a thread was waiting for the rmutex, exactly one thread will be 
 *	awakened to try to obtain the rmutex.
 */

int
rmutex_unlock(rmutex_t *rmutex)
{
	int		rval = 0;
	boolean_t	checkpid;
	thread_desc_t	*curtp = curthread;

	switch(rmutex->rm_mutex.m_type) {
	case USYNC_THREAD:
		checkpid = B_FALSE;
		break;
	case USYNC_PROCESS:
		checkpid = B_TRUE;
		break;
	default:
		return(EINVAL);
		/* NOTREACHED */
		break;
	}

	if ((rmutex->rm_owner != curtp->t_tid) ||
	    ((checkpid == B_TRUE) && (rmutex->rm_pid != getpid()))) {
		/* caller doesn't own the rmutex */
		rval = EACCES;
	} else {
		/* caller owns the rmutex */
		ASSERT(rmutex->rm_depth > 0);
		rmutex->rm_depth--;
		if (rmutex->rm_depth == 0) {
			/*
			 * this is the last unlock, so clear all the
			 * owner info and release the underlying mutex
			 */
			rmutex->rm_owner = 0;
			if (checkpid == B_TRUE)
				rmutex->rm_pid = 0;
			rval = _THR_MUTEX_UNLOCK(&(rmutex->rm_mutex));
		}
	}
	return(rval);
}



/*
 * rmutex_destroy(rmutex_t *rmutex)
 *	destroys a rmutex
 *
 * Parameter/Calling State:
 *      No locks are held on entry -- user interface.
 *
 *	The argument is a pointer to a rmutex.
 *
 *	During processing, the rmutex lock may be acquired and the sync_lock
 *	of the rmutex may also be acquired.
 *
 * Return Values/Exit State:
 *	Returns zero and leaves the rmutex in an invalid state on success; 
 *	otherwise an errno value.
 */

int
rmutex_destroy(rmutex_t *rmutex)
{
	int	rval;

	rval = _THR_MUTEX_DESTROY(&(rmutex->rm_mutex));
	return(rval);
}
		
