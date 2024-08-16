/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_accept.c	1.5.7.12"
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
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/sysmacros_f.h>
#include "_import.h"

extern struct _ti_user *_t_checkfd();
extern int _t_is_event();

#ifdef t_accept
#undef t_accept
#endif
#pragma weak _xti_accept = t_accept

t_accept(fd, resfd, call)
int fd;
int resfd;
struct t_call *call;
{
	char *buf;
	register struct T_conn_res *cres;
	struct strfdinsert strfdinsert;
	int size;
	int retval;
	register struct _ti_user *tiptr;
	register struct _ti_user *restiptr;
	sigset_t		oldmask;
	sigset_t		newmask;
	struct stat fd_stat_buf, resfd_stat_buf;
	struct T_info_ack inforeq;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	/*
	 * verify that provider supports t_accept operation
	 * MR=ul94-13933
	 */
	if (tiptr->ti_servtype == T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 *  Verify current state for valid t_accept() operation,
	 *  only allowed in T_INCON state.
	 */
	if (tiptr->ti_state != T_INCON){
		MUTEX_UNLOCK(&tiptr->lock);
		set_t_errno(TOUTSTATE);
		return(-1);
	}

	/*
	 *  Perform an information request for validation of user data,
	 *  if user data present in call structure.
	 */
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (call->udata.len > 0) {
		inforeq.PRIM_type = T_INFO_REQ;

		/* Get provider information. */ 
		if (!_t_do_ioctl(fd, (caddr_t)&inforeq,
				 sizeof(struct T_info_req),
				 TI_GETINFO, &retval)) {
			MUTEX_UNLOCK(&tiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			return(-1);
		}
		
		if (retval != sizeof(struct T_info_ack)) {
			set_t_errno(TSYSERR);
			errno = EIO;
			MUTEX_UNLOCK(&tiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			return(-1);
		}

		/*
		 *  Validate udata sizes.
		 */
		if (inforeq.CDATA_size < 0) {
			/* If -2, means NO data allowed. */
			if (inforeq.CDATA_size == -2) {
				set_t_errno(TBADDATA);
				MUTEX_UNLOCK(&tiptr->lock);
				(void) sigprocmask(SIG_SETMASK, &oldmask,
						   (sigset_t *)NULL);
				return(-1);
			}
			/* -1, means unlimit data. */
		} else {
			/* Check data size upper boundary. */
			if (call->udata.len > inforeq.CDATA_size) {
				set_t_errno(TBADDATA);
				MUTEX_UNLOCK(&tiptr->lock);
				(void) sigprocmask(SIG_SETMASK, &oldmask,
						   (sigset_t *)NULL);
				return(-1);
			}
		}
		/* User data okay to send. */
	}

	 /*
         * If fd!=resfd and they do not refer to the same provider,
         * return error. The same device is determined by the 
	 * device number and file system identifier returned from fstat().
         */
        if (fd != resfd) {
                if ((restiptr = _t_checkfd(resfd)) == NULL) {
			MUTEX_UNLOCK(&tiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
                        return(-1);
                }

                fstat(fd, &fd_stat_buf);
                fstat(resfd, &resfd_stat_buf);
                if (
		      (fd_stat_buf.st_dev != resfd_stat_buf.st_dev) ||
        	      (getmajor(fd_stat_buf.st_rdev) != 
				getmajor(resfd_stat_buf.st_rdev))) {
                        MUTEX_UNLOCK(&tiptr->lock);
			MUTEX_UNLOCK(&restiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
                        set_t_errno(TPROVMISMATCH);
                        return(-1);
                }

		/*
		 *  Verify current state for valid t_accept() operation,
		 *  on resfd, only allowed in T_IDLE state.
		 */
		if (restiptr->ti_state != T_IDLE &&
		    restiptr->ti_state != T_UNBND) {
			MUTEX_UNLOCK(&tiptr->lock);
			MUTEX_UNLOCK(&restiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TOUTSTATE);
			return(-1);
		}

		/*
		 *  If the user chooses to bind the endpoint, it must
		 *  have qlen==0 and be in state T_IDLE
		 */
                if ((restiptr->ti_qlen != 0) && (restiptr->ti_state == T_IDLE)){
                        MUTEX_UNLOCK(&tiptr->lock);
			MUTEX_UNLOCK(&restiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
                        set_t_errno(TRESQLEN);
                        return(-1);
                }

        } else {
		/*
		 * if fd == resfd and there is anything at the stream head,
		 * return TINDOUT for T_INCON or TLOOK for T_DISCONNECT.
		 * Retval contains the number of messages on the stream head.
		 */
		if ((retval = ioctl(fd, I_NREAD, &size)) < 0) {
			MUTEX_UNLOCK(&tiptr->lock);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TSYSERR);
			return(-1);
		}
		/*
		 *  If there are messages on the stream head, determine the
		 *  message type and return the appropriate error.
		 */
		if (retval > 0) {
			int evt;

			/*
			 *  __xti_look() must be called instead of _xti_look(),
			 *  since _xti_look() calls _t_checkfd(), which acquires
			 *  both _nsl_lock and tiptr->lock.  We have already
			 *  called _t_checkfd(), so we need not do it again.  
			 *  By using this approach, we can hold our lock
			 *  continuously.
			 */
			if ((evt = (int)__xti_look(fd, tiptr)) == -1) {
				MUTEX_UNLOCK(&tiptr->lock);
				(void) sigprocmask(SIG_SETMASK, &oldmask,
						   (sigset_t *)NULL);
				return(-1);
			}
			MUTEX_UNLOCK(&tiptr->lock);

			/*
			 *  If event, is T_DISCONNECT fail with TLOOK, else
			 *  assume T_INCON and fail with TINDOUT.
			 */
			if (evt == T_DISCONNECT) {
				set_t_errno(TLOOK);
			} else {
				set_t_errno(TINDOUT);
			}
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
                        return(-1);
		}
	}

	if (fd != resfd)
	{
		if ((retval = ioctl(resfd,I_NREAD,&size)) < 0)
		{
			MUTEX_UNLOCK(&tiptr->lock);
			MUTEX_UNLOCK(&restiptr->lock);
			set_t_errno(TSYSERR);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			return(-1);
		}
		if (retval)
		{
			MUTEX_UNLOCK(&tiptr->lock);
			MUTEX_UNLOCK(&restiptr->lock);
			set_t_errno(TBADF);
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			return(-1);
		}
	}

        if (_t_is_event(fd, tiptr)) {
                MUTEX_UNLOCK(&tiptr->lock);
                if (fd != resfd) MUTEX_UNLOCK(&restiptr->lock);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
                return(-1);
        }

	buf = tiptr->ti_ctlbuf;
	cres = (struct T_conn_res *)buf;
	cres->PRIM_type = T_CONN_RES;
	cres->OPT_length = call->opt.len;
	cres->OPT_offset = 0;
	cres->SEQ_number = call->sequence;
	size = sizeof(struct T_conn_res);

	if (call->opt.len) {
		_t_aligned_copy(buf, call->opt.len, size,
				call->opt.buf, &cres->OPT_offset);
		size = cres->OPT_offset + cres->OPT_length;
	}

	/*
	 * turn off flow control flag. If the stream is flow controlled, 
	 * turn the flag back on after attempting the FDINSERT.
	*/
	tiptr->ti_flags &= ~FLOWCNTL;

	strfdinsert.ctlbuf.maxlen = tiptr->ti_ctlsize;
	strfdinsert.ctlbuf.len = size;
	strfdinsert.ctlbuf.buf = buf;
	strfdinsert.databuf.maxlen = call->udata.maxlen;
	strfdinsert.databuf.len = (call->udata.len? call->udata.len: -1);
	strfdinsert.databuf.buf = call->udata.buf;
	strfdinsert.fildes = resfd;
	strfdinsert.offset = sizeof(long);
	strfdinsert.flags = 0;      /* could be EXPEDITED also */

	if (ioctl(fd, I_FDINSERT, &strfdinsert) < 0) {
		 if (errno == EAGAIN) {
                        set_t_errno(TFLOW);
                        tiptr->ti_flags |= FLOWCNTL;
                }
                else
			set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		if (fd != resfd) MUTEX_UNLOCK(&restiptr->lock);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}

	if (!_t_is_ok(fd, tiptr, T_CONN_RES)) {
		MUTEX_UNLOCK(&tiptr->lock);
		if (fd != resfd) MUTEX_UNLOCK(&restiptr->lock);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}

	if (tiptr->ti_ocnt == 1) {
		if (fd == resfd)
			tiptr->ti_state
			   = TLI_NEXTSTATE(T_ACCEPT1, tiptr->ti_state);
		else {
			tiptr->ti_state
			   = TLI_NEXTSTATE(T_ACCEPT2, tiptr->ti_state);
			restiptr->ti_state
			   = TLI_NEXTSTATE(T_PASSCON, restiptr->ti_state);
		}
	}
	else {
		tiptr->ti_state = TLI_NEXTSTATE(T_ACCEPT3, tiptr->ti_state);
		restiptr->ti_state
		   = TLI_NEXTSTATE(T_PASSCON, restiptr->ti_state);
	}

	tiptr->ti_ocnt--;
	MUTEX_UNLOCK(&tiptr->lock);
	if (fd != resfd) MUTEX_UNLOCK(&restiptr->lock);
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	return(0);
}
