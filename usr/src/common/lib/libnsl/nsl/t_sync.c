/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_sync.c	1.4.11.5"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <signal.h>
#include <ulimit.h>
#include "_import.h"

extern struct _ti_user **fdp;
extern long openfiles;
extern _null_tiptr();

#ifdef t_sync
#undef t_sync
#endif

#pragma weak _xti_sync = t_sync

/* 
 * After an exec, the user data structures for the connection are gone.
 * The purpose of t_sync() is to reestablish them here.
 */

t_sync(fd)
int fd;
{
	struct _ti_user *tiptr;
	sigset_t		oldmask;
	sigset_t		newmask;
	struct T_info_ack info;
	int retval, arg, retlen;
	int wr_lock;

	/*
	 * Initialize "openfiles" - global variable
  	 * containing number of open files allowed
	 * per process.
	 */
	
	openfiles = OPENFILES;
	if (fd < 0 || fd >= openfiles) {
		set_t_errno(TBADF);
		return(-1);
	}
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	info.PRIM_type = T_INFO_REQ;
	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TBADF);
		return(-1);
	}
	if (!retval) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TBADF);
		return(-1);
	}
	if (!_t_do_ioctl(fd, (caddr_t)&info, sizeof(struct T_info_req),
			 TI_GETINFO, &retlen) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	if (retlen != sizeof(struct T_info_ack)) {
		errno = EIO;
		set_t_errno(TSYSERR);
		return(-1);
	}

	/*
	 * Multiple threads may read the pointer array without conflict.
	 * However, if we must allocate an array of pointers to 
	 * _ti_user structures (one pointer for each file descriptor),
	 * then we must have exclusive access to the pointer to the array.
	 * Thus we acquire _nsl_lock for reading to check
	 * whether the array has been allocated, and we acquire _nsl_lock
	 * for writing if we may have to allocate the array.
	 * (Another thread could have done the allocation
	 * after our first check, so we must check a second time.)
	 * Furthermore, we must hold _nsl_lock for writing until we are sure
	 * we will not have to free the array, since another thread may try to
	 * use the _ti_user structure that we create, including its lock.
	 */

	RW_RDLOCK(&_nsl_lock);
	if (!fdp) { /* No array of pointers exists */
		RW_UNLOCK(&_nsl_lock);
		RW_WRLOCK(&_nsl_lock);
	 	if (!fdp) {  /* The array of pointers is still not allocated */
			int i;
			/* Allocate the array of pointers */
			if ((fdp = (struct _ti_user **)
	 	     		   calloc(openfiles, sizeof(struct _ti_user *)))
			    == NULL) {
				RW_UNLOCK(&_nsl_lock);
				set_t_errno(TSYSERR);
				return(-1);
			}
			/* Initialize the array of pointers */
			for (i=0; i<openfiles; i++) fdp[i] = NULL;
		}
	}
	/*
	 * Now the array of pointers has been allocated
	 * and either a read lock or a write lock is being held. 
	 * Next, we want to ensure that we have allocated a _ti_user
	 * structure.
	 */

	if (fdp[fd] == NULL) {
		/* The _ti_user structure may not be allocated */
		if (!wr_lock) {
			/* Acquire a write lock and check again */
			RW_UNLOCK(&_nsl_lock);
			RW_WRLOCK(&_nsl_lock);
			if (fdp[fd] == NULL) {
				/* Allocate a _ti_user structure */
				if ((fdp[fd]
				   = (struct _ti_user *)
				     calloc(1, sizeof(struct _ti_user)))
				    == NULL) {
					RW_UNLOCK(&_nsl_lock);
					set_t_errno(TSYSERR);
					close(fd);
					return(-1);
				}
				/* Initialize the lock in the structure */
				MUTEX_INIT(&fdp[fd]->lock, USYNC_THREAD,
					    NULL);
			}
			/* The _ti_user structure has now been allocated */
		}
		else {
			/*
			 * Holding a write lock, we can be sure that
			 * no _ti_user structure has been allocated,
			 * so we try to do so.
			 */
			if ((fdp[fd]
			    = (struct _ti_user *)
			      calloc(1, sizeof(struct _ti_user)))
		    	== NULL) {
				RW_UNLOCK(&_nsl_lock);
				set_t_errno(TSYSERR);
				close(fd);
				return(-1);
			}
			/* Initialize the lock for the _ti_user structure */
			MUTEX_INIT(&fdp[fd]->lock, USYNC_THREAD, NULL);
		}
	}
	/*
	 * At this point, fdp[fd] is not NULL and will not be changed
	 * by this or any other thread, so we do not have to lock it
	 * for further access.  However, we do have to acquire fdp[fd]->lock
	 * for writing in order to initialize the _ti_user structure for fd.
	 */
	tiptr = fdp[fd];
	MUTEX_LOCK(&tiptr->lock);
	RW_UNLOCK(&_nsl_lock);

	if (!(tiptr->ti_flags & USED)) {
		if (_t_alloc_bufs(fd, tiptr, info) < 0) {
			_null_tiptr(tiptr);
			MUTEX_UNLOCK(&fdp[fd]->lock);
			set_t_errno(TSYSERR);
			return(-1);
		}
	}
	switch (info.CURRENT_state) {

	case TS_UNBND:
		retval = tiptr->ti_state = T_UNBND;
		break;
	case TS_IDLE:
		/*
		 *  Check to see if an event (T_ORDREL or T_DISCONNECT)
		 *  is pending on the stream head.  If so, the library
		 *  state must be set to the appropriate value so that
		 *  the event can be received off of the stream head.
		 */
		if ((retval = __xti_look(fd, tiptr)) == -1) {
			/*
			 * We must give some state and T_FAKE
			 * seems appropriate. Don't bother to free
			 * _ti_user, since it will come in handy for
			 * subsequent operations.
			 */
			tiptr->ti_state = T_FAKE;
			set_t_errno(TSYSERR);
			break;
		}
		if (retval == 0) {
			retval = tiptr->ti_state = T_IDLE;
		} else {
			if (tiptr->ti_servtype == T_COTS
				|| tiptr->ti_servtype == T_COTS_ORD)
			{
				/*
				 * To handle data, DISCONNECT, or ORDREL
				 * indications that might be at the stream
				 * head waiting to be read if the transport
				 * provider is connection-oriented .
				 */
				if (retval == T_ORDREL) {
					retval = tiptr->ti_state = T_OUTREL;
				} else {
					retval = tiptr->ti_state = T_DATAXFER;
				}
			} else {
				retval = tiptr->ti_state = T_IDLE;
			}
		}
		break;
	case TS_WRES_CIND:
		retval = tiptr->ti_state = T_INCON;
		break;
	case TS_WCON_CREQ:
		retval = tiptr->ti_state = T_OUTCON;
		break;
	case TS_DATA_XFER:
		/*
		 *  Check to see if a T_CONNECT event is pending
		 *  on the stream head.  If so, the library state
		 *  must be T_OUTCON state not T_DATAXFER so that
		 *  the T_CONNECT event can be received.
		 */
		if ((retval = __xti_look(fd, tiptr)) == -1) {
			/*
			 * See comments in TS_IDLE case.
			 */
			tiptr->ti_state = T_FAKE;
			set_t_errno(TSYSERR);
			break;
		}
		if (retval == T_CONNECT) {
			retval = tiptr->ti_state = T_OUTCON;
		} else {
			retval = tiptr->ti_state = T_DATAXFER;
		}
		break;
	case TS_WIND_ORDREL:
		retval = tiptr->ti_state = T_OUTREL;
		break;
	case TS_WREQ_ORDREL:
		if((retval = ioctl(fd,I_NREAD,&arg)) < 0) {
			/*
			 * See comments in TS_IDLE case.
			 */
			tiptr->ti_state = T_FAKE;
			set_t_errno(TSYSERR);
			retval = -1;
			break;
		}
		if(retval == 0 )
			retval = tiptr->ti_state = T_INREL;
		else {
			/*
			 * To handle T_ORDREL_IND indication that
			 * might be at the stream head waiting to be read.
			 */
			retval = tiptr->ti_state = T_DATAXFER;
		}
		break;
	default:
		tiptr->ti_state = T_FAKE;
		set_t_errno(TSTATECHNG);
		retval = -1;
		break;
	}
	MUTEX_UNLOCK(&tiptr->lock);
	return (retval);
}
