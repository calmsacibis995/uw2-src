/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcvuderr.c	1.5.5.6"
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
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_rcvuderr
#undef t_rcvuderr
#endif

#pragma weak _xti_rcvuderr = t_rcvuderr

t_rcvuderr(fd, uderr)
int fd;
struct t_uderr *uderr;
{
	struct strbuf ctlbuf, rcvbuf;
	int flg;
	int retval;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype != T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	/* 
         * is there an error indication in look buffer
	 */
	if (tiptr->ti_lookflg) {
		ctlbuf.maxlen = tiptr->ti_lookcsize;
		ctlbuf.len = tiptr->ti_lookcsize;
		ctlbuf.buf = tiptr->ti_lookcbuf;
		rcvbuf.maxlen = 0;
		rcvbuf.len = 0;
		rcvbuf.buf = NULL;
	} else {
		if ((retval = __xti_look(fd, tiptr)) < 0) {
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		if (retval != T_UDERR) {
			set_t_errno(TNOUDERR);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	
		ctlbuf.maxlen = tiptr->ti_ctlsize;
		ctlbuf.len = 0;
		ctlbuf.buf = tiptr->ti_ctlbuf;
		rcvbuf.maxlen = 0;
		rcvbuf.len = 0;
		rcvbuf.buf = NULL;

		flg = 0;

		if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
			set_t_errno(TSYSERR);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		/*
		 * did I get entire message?
		 */
		if (retval) {
			set_t_errno(TSYSERR);
			errno = EIO;
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}

	}

	tiptr->ti_lookflg = 0;

	pptr = (union T_primitives *)ctlbuf.buf;

	if ((ctlbuf.len < sizeof(struct T_uderror_ind)) ||
	    (pptr->type != T_UDERROR_IND)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (uderr != (struct t_uderr *)NULL) {
		if (((uderr->addr.maxlen != 0) &&
		 (uderr->addr.maxlen < pptr->uderror_ind.DEST_length)) ||
		 ((uderr->opt.maxlen != 0) &&
		 (uderr->opt.maxlen < pptr->uderror_ind.OPT_length))) {
			set_t_errno(TBUFOVFLW);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	
		uderr->error = pptr->uderror_ind.ERROR_type;
		uderr->addr.len = 0;
		uderr->opt.len = 0;
		if (uderr->addr.maxlen != 0) {
			memcpy(uderr->addr.buf, ctlbuf.buf +
			 pptr->uderror_ind.DEST_offset,
			 (int)pptr->uderror_ind.DEST_length);
			uderr->addr.len = 
			 (unsigned int)pptr->uderror_ind.DEST_length;
		}
		if (uderr->opt.maxlen != 0) {
			memcpy(uderr->opt.buf, ctlbuf.buf +
			 pptr->uderror_ind.OPT_offset,
			 (int)pptr->uderror_ind.OPT_length);
			 uderr->opt.len = (unsigned int)pptr->uderror_ind.OPT_length;
		}
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_RCVUDERR, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
