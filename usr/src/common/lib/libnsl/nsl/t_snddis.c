/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_snddis.c	1.4.8.7"
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
#include <unistd.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_snddis
#undef t_snddis
#endif

#pragma weak _xti_snddis = t_snddis

t_snddis(fd, call)
int fd;
struct t_call *call;
{
	struct T_discon_req dreq;
	struct strbuf ctlbuf;
	struct strbuf databuf;
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
	
	/*
         * look at look buffer to see if there is a discon there
	 */

	if (__xti_look(fd, tiptr) == T_DISCONNECT) {
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	tiptr->ti_lookflg = 0;

	if (ioctl(fd, I_FLUSH, FLUSHRW) < 0) {
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	dreq.PRIM_type = T_DISCON_REQ;
	dreq.SEQ_number = (call? call->sequence: -1);


	ctlbuf.maxlen = sizeof(struct T_discon_req);
	ctlbuf.len = sizeof(struct T_discon_req);
	ctlbuf.buf = (caddr_t)&dreq;

	databuf.maxlen = (call? call->udata.len: 0);
	databuf.len = (call? call->udata.len: 0);
	databuf.buf = (call? call->udata.buf: NULL);

	if (putmsg(fd, &ctlbuf, (databuf.len? &databuf: NULL), 0) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (!_t_is_ok(fd, tiptr, T_DISCON_REQ)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	tiptr->ti_flags &= ~MORE;
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	if (tiptr->ti_ocnt <= 1) {
		if (tiptr->ti_state == T_INCON)
			tiptr->ti_ocnt--;
		tiptr->ti_state = TLI_NEXTSTATE(T_SNDDIS1, tiptr->ti_state);
	}
	else {
		if (tiptr->ti_state == T_INCON)
			tiptr->ti_ocnt--;
		tiptr->ti_state = TLI_NEXTSTATE(T_SNDDIS2, tiptr->ti_state);
	}	
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
