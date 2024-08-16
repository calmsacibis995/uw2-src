/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcvdis.c	1.10.5.8"
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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_rcvdis
#undef t_rcvdis
#endif

#pragma weak _xti_rcvdis = t_rcvdis

t_rcvdis(fd, discon)
int fd;
struct t_discon *discon;
{
	struct strbuf ctlbuf;
	struct strbuf databuf;
	int retval;
	int flg = 0;
	union T_primitives *pptr;
	register struct _ti_user *tiptr;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype == T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if ((tiptr->ti_state != T_DATAXFER) &&
                        (tiptr->ti_state != T_OUTCON) &&
                        (tiptr->ti_state != T_OUTREL) &&
                        (tiptr->ti_state != T_INREL) &&
                        ((tiptr->ti_state != T_INCON) && (tiptr->ti_ocnt == 0))) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	/*
         * is there a discon in look buffer
	 */
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (tiptr->ti_lookflg
	 && (*((long *)tiptr->ti_lookcbuf) == T_DISCON_IND)) {
		ctlbuf.maxlen = tiptr->ti_lookcsize;
		ctlbuf.len = tiptr->ti_lookcsize;
		ctlbuf.buf = tiptr->ti_lookcbuf;
		databuf.maxlen = tiptr->ti_lookdsize;
		databuf.len = tiptr->ti_lookdsize;
		databuf.buf = tiptr->ti_lookdbuf;
	} else {

		/*
		 * __xti_look() must be called instead of _xti_look(),
		 * since _xti_look()
		 * calls _t_checkfd(), which acquires both _nsl_lock and
		 * tiptr->lock.
		 * We have already called _t_checkfd(), so we need not do it
		 * again.  
		 * By using this approach, we can hold our lock continuously.
		 */

		if ((retval = __xti_look(fd, tiptr)) < 0) {
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		

		if (retval != T_DISCONNECT) {
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TNODIS);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}

		/*
		 * get disconnect off read queue.
		 * use ctl and rcv buffers
		 */
		ctlbuf.maxlen = tiptr->ti_ctlsize;
		ctlbuf.len = 0;
		ctlbuf.buf = tiptr->ti_ctlbuf;
		databuf.maxlen = tiptr->ti_rcvsize;
		databuf.len = 0;
		databuf.buf = tiptr->ti_rcvbuf;
	
		if ((retval = getmsg(fd, &ctlbuf, &databuf, &flg)) < 0) {
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TSYSERR);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		if (databuf.len == -1) databuf.len = 0;

		/*
		 * did I get entire message?
		 */
		if (retval) {
			(void) sigprocmask(SIG_SETMASK, &oldmask,
					   (sigset_t *)NULL);
			set_t_errno(TSYSERR);
			errno = EIO;
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	}

	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	tiptr->ti_lookflg = 0;
	
	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_discon_ind)) ||
	    (pptr->type != T_DISCON_IND)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 * clear more and expedited flags
	 */
	tiptr->ti_flags &= ~(MORE | EXPEDITED);

	if (tiptr->ti_ocnt <= 0)
		tiptr->ti_state = TLI_NEXTSTATE(T_RCVDIS1, tiptr->ti_state);
	else {
		if (tiptr->ti_ocnt == 1)
			tiptr->ti_state
			   = TLI_NEXTSTATE(T_RCVDIS2, tiptr->ti_state);
		else
			tiptr->ti_state
			   = TLI_NEXTSTATE(T_RCVDIS3, tiptr->ti_state);
		tiptr->ti_ocnt--;
	}

	if (discon != NULL) {
		discon->reason = pptr->discon_ind.DISCON_reason;
		discon->sequence = (long) pptr->discon_ind.SEQ_number;
		discon->udata.len = 0;
		if (discon->udata.maxlen != 0) {
			if (databuf.len > discon->udata.maxlen) {
				set_t_errno(TBUFOVFLW);
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
			}
			memcpy(discon->udata.buf, databuf.buf,
			 (int)databuf.len);
			discon->udata.len = databuf.len;
		}
	}

	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
