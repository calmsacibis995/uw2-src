/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcvrel.c	1.7.8.7"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <signal.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_rcvrel
#undef t_rcvrel
#endif

#pragma weak _xti_rcvrel = t_rcvrel

t_rcvrel(fd)
int fd;
{
	struct strbuf ctlbuf;
	struct strbuf databuf;
	int retval;
	int flg = 0;
	union T_primitives *pptr;
	struct _ti_user *tiptr;
	sigset_t		oldmask;
	sigset_t		newmask;

	/* _t_checkfd() must be called without holding any locks. */
	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype != T_COTS_ORD) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if ((tiptr->ti_state != T_DATAXFER) &&
			(tiptr->ti_state != T_OUTREL)) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	/*
	 * __xti_look() must be called instead of _xti_look(),
	 * since _xti_look() calls
	 * _t_checkfd(), which acquires both _nsl_lock and tiptr->lock.
	 * We have already called _t_checkfd(), so we need not do it again.
	 * By using this approach, we can hold our lock continuously.
	 */

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if ((retval = __xti_look(fd, tiptr)) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (retval == T_DISCONNECT) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (tiptr->ti_lookflg
	 && (*((long *)tiptr->ti_lookcbuf) == T_ORDREL_IND)) {
		tiptr->ti_lookflg = 0;
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

		tiptr->ti_state = TLI_NEXTSTATE(T_RCVREL, tiptr->ti_state);
		MUTEX_UNLOCK(&tiptr->lock);
		return(0);
	} else {
		if (retval != T_ORDREL) {
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TNOREL);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	}

	/*
	 * get ordrel off read queue.
	 * use ctl and rcv buffers
	 */
	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	databuf.maxlen = tiptr->ti_rcvsize;
	databuf.len = 0;
	databuf.buf = tiptr->ti_rcvbuf;

	if ((retval = getmsg(fd, &ctlbuf, &databuf, &flg)) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	/*
	 * did I get entire message?
	 */
	if (retval) {
		set_t_errno(TSYSERR);
		errno = EIO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_ordrel_ind)) ||
	    (pptr->type != T_ORDREL_IND)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_RCVREL, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
