/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_optmgmt.c	1.3.9.6"
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
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();


t_optmgmt(fd, req, ret)
int fd;
struct t_optmgmt *req;
struct t_optmgmt *ret;
{
	int size;
	register char *buf;
	register struct T_optmgmt_req *optreq;
	register struct _ti_user *tiptr;
	sigset_t		oldmask;
	sigset_t		newmask;


	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	buf = tiptr->ti_ctlbuf;
	optreq = (struct T_optmgmt_req *)buf;
	optreq->PRIM_type = T_OPTMGMT_REQ;
	optreq->OPT_length = req->opt.len;
	optreq->OPT_offset = 0;
	optreq->MGMT_flags = req->flags;
	size = sizeof(struct T_optmgmt_req);

	if (req->opt.len) {
		_t_aligned_copy(buf, req->opt.len, size,
			     req->opt.buf, &optreq->OPT_offset);
		size = optreq->OPT_offset + optreq->OPT_length;
	}

	if (!_t_do_ioctl(fd, buf, size, TI_OPTMGMT, NULL)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);


	ret->opt.len = 0;
	if (ret->opt.maxlen != 0) {
		if (optreq->OPT_length > ret->opt.maxlen) {
			set_t_errno(TBUFOVFLW);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		memcpy(ret->opt.buf, (char *) (buf + optreq->OPT_offset),
		 (int)optreq->OPT_length);
		ret->opt.len = optreq->OPT_length;
	}
	ret->flags = optreq->MGMT_flags;

	tiptr->ti_state = TLI_NEXTSTATE(T_OPTMGMT, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

