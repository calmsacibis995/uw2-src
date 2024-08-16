/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_open.c	1.5.8.9"
#ident	"$Header: $"

#include <sys/param.h>
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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ulimit.h>
#include "_import.h"


extern struct _ti_user **fdp;
extern long openfiles;

#ifdef t_open
#undef t_open
#endif

t_open(path, flags, info)
char *path;
int flags;
register struct t_info *info;
{
	int retval, fd, save_errno;
	struct T_info_ack inforeq;
	register struct _ti_user *tiptr;
	int retlen;
	int wr_lock = 0;
	sigset_t		oldmask;
	sigset_t		newmask;

	if (!(flags & O_RDWR)) {
		errno = 0;
		set_t_errno(TBADFLAG);
		return (-1);
	}

	if ((fd = open(path, flags)) < 0) {
		if (errno == ENOENT)
                        set_t_errno(TBADNAME);
                else
                        set_t_errno(TSYSERR);
                return(-1);
        }


	/*
	 * is module already pushed
	 */
	if ((retval = ioctl(fd, I_FIND, "timod")) < 0) {
		save_errno = errno;
		if ((errno == ENOSTR) || (errno == ENOENT) || (errno == ENODEV))
                        set_t_errno(TBADNAME);
                else
                        set_t_errno(TSYSERR);
                close(fd);
		errno = save_errno;
                return(-1);
        }

	if (!retval)
		if (ioctl(fd, I_PUSH, "timod") < 0) {
			save_errno = errno;
                        if (errno == ENOENT)
                                set_t_errno(TBADNAME);
                        else
                                set_t_errno(TSYSERR);
                        close(fd);
			errno = save_errno;
                        return(-1);
                }
	
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	inforeq.PRIM_type = T_INFO_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)&inforeq, sizeof(struct T_info_req),
			 TI_GETINFO, &retlen)) {
		save_errno = errno;
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		close(fd);
		errno = save_errno;
		return(-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		
	if (retlen != sizeof(struct T_info_ack)) {
		save_errno = errno;
		set_t_errno(TSYSERR);
		errno = EIO;
		close(fd);
		errno = save_errno;
		return(-1);
	}

	if (info != NULL) {
		info->addr = inforeq.ADDR_size;
		info->options = inforeq.OPT_size;
		info->tsdu = inforeq.TSDU_size;
		info->etsdu = inforeq.ETSDU_size;
		info->connect = inforeq.CDATA_size;
		info->discon = inforeq.DDATA_size;
		info->servtype = inforeq.SERV_type;
	/* ***************************************************
	 *
	 * Do NOT assign anything to info->flags in this function,
	 * or the calling program will dump core!
	 *
	 * ***************************************************/
	}

	/*
	 * Multiple threads may read the pointer array without conflict.
	 * However, if we must allocate an array to hold a pointer to 
	 * the _ti_user structure for each file descriptor,
	 * then we must have exclusive access to the pointer to the array
	 * of pointers.  Thus we acquire _nsl_lock for reading to check
	 * whether the array has been allocated, and we acquire _nsl_lock
	 * for writing if we may have to allocate the array.  (Another
	 * thread could have done the allocation after our first check,
	 * so we must check a second time.)
	 * Furthermore, we must hold _nsl_lock for writing until we are sure
	 * we will not have to free the array, since another thread may try to
	 * use the _ti_user structure that we create, including its lock.
	 */
	RW_RDLOCK(&_nsl_lock);
	if (!fdp) { /* The array of pointers has not been allocated */
		RW_UNLOCK(&_nsl_lock);
		RW_WRLOCK(&_nsl_lock);
		wr_lock = 1;
		openfiles = OPENFILES;
		if (!fdp) { /* The array of pointers is still not allocated */
			int i;
			/* Allocate the array of pointers */
			if ((fdp = (struct _ti_user **)
	 			   calloc(openfiles, sizeof(struct _ti_user *)))
			    == NULL) {
				save_errno = errno;
				RW_UNLOCK(&_nsl_lock);
				set_t_errno(TSYSERR);
				close(fd);
				errno = save_errno;
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
		/* The _ti_user structure has not been allocated */
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
					save_errno = errno;
					RW_UNLOCK(&_nsl_lock);
					set_t_errno(TSYSERR);
					close(fd);
					errno = save_errno;
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
				save_errno = errno;
				RW_UNLOCK(&_nsl_lock);
				set_t_errno(TSYSERR);
				close(fd);
				errno = save_errno;
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

	/*
	 * If first time done, then initialize data structure
	 * and allocate buffers.
	 */
	if (tiptr->ti_flags & USED) {
		/*
		 * This is the case when fd was closed (not!! t_close)
		 * before a fork(). We need to clean up tiptr and allocate
		 * bufs with the current inforeq (it is conceivable that
		 * the provider attributes are different from those
		 * that are in tiptr from an earlier open).
		 *
		 * After a fork() & exec() this would not be true.
		 */
		if (tiptr->ti_rcvbuf != NULL)
			(void)free(tiptr->ti_rcvbuf);
		if (tiptr->ti_lookdbuf != NULL)
			(void)free(tiptr->ti_lookdbuf);
		(void)free(tiptr->ti_ctlbuf);
		(void)free(tiptr->ti_lookcbuf);
		_null_tiptr(tiptr);
	}
	if ((_t_alloc_bufs(fd, tiptr, inforeq) < 0) ||
	    (ioctl(fd, I_FLUSH, FLUSHRW) < 0)) {
		MUTEX_UNLOCK(&tiptr->lock);
		t_close(fd);
		set_t_errno(TSYSERR);
		return(-1);
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_OPEN, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(fd);
}
