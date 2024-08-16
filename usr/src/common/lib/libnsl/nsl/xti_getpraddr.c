/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/xti_getpraddr.c	1.2.3.7"
#ident	"$Header: $"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/xti.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

/*
 * The following symbol is here just-in-case.  In principle,
 * it is not needed since t_strerror() was introduced only with
 * XPG4's XTI.
 */
#pragma weak t_getprotaddr = _xti_getprotaddr

_xti_getprotaddr(int fd, struct t_bind *boundaddr, struct t_bind *peeraddr)
{
	struct _ti_user *tiptr;
	struct T_addr_req areq;
	struct T_addr_ack *aackp;
	struct strbuf ctlbuf;
	sigset_t		oldmask;
	sigset_t		newmask;
	int band = 0;
	int flags = MSG_HIPRI;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	areq.PRIM_type = T_ADDR_REQ;

	ctlbuf.len = sizeof(struct T_addr_req);
	ctlbuf.buf = (caddr_t)&areq;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	if (putmsg(fd, &ctlbuf, NULL, 0) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf; /* Now watch ctlbuf.buf also */

	if (getpmsg(fd, &ctlbuf, NULL, &band, &flags) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	if (ctlbuf.len < sizeof(struct T_addr_ack)) {
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	aackp = (struct T_addr_ack *) ctlbuf.buf; /* Now watch aackp also */
	if (aackp->PRIM_type != T_ADDR_ACK) {
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (boundaddr) {
		/* Will buffer hold returned address? */
		if (boundaddr->addr.maxlen < aackp->LOCADDR_length) {
			set_t_errno(TBUFOVFLW);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		memcpy(boundaddr->addr.buf,
		       (char *) aackp + aackp->LOCADDR_offset,
			aackp->LOCADDR_length);
		boundaddr->addr.len = aackp->LOCADDR_length;
	}

	if (peeraddr && tiptr->ti_state == T_DATAXFER) {
		/* Will buffer hold returned address? */
		if (peeraddr->addr.maxlen < aackp->REMADDR_length) {
			set_t_errno(TBUFOVFLW);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		memcpy(peeraddr->addr.buf,
		       (char *) aackp + aackp->REMADDR_offset,
			aackp->REMADDR_length);
		peeraddr->addr.len = aackp->REMADDR_length;
	}
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
