/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_listen.c	1.5.5.7"
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

#ifdef t_listen
#undef t_listen
#endif

#pragma weak _xti_listen = t_listen

t_listen(fd, call)
int fd;
struct t_call *call;
{
	struct strbuf ctlbuf;
	struct strbuf rcvbuf;
	int flg = 0;
	int retval;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

        if ((tiptr->ti_state != T_IDLE) && (tiptr->ti_state != T_INCON)) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	/*
         * Verify there is nothing in the look buffer, the provider is
         * connection oriented, the qlen is greater than 0, the current
         * state is T_IDLE or T_INCON and the max number of connect
	 * indicationshave not been reached.
         */

	if (tiptr->ti_lookflg) {
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (tiptr->ti_servtype == T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (tiptr->ti_qlen == 0) {
                set_t_errno(TBADQLEN);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

        if (tiptr->ti_ocnt >= tiptr->ti_qlen) {
                set_t_errno(TQFULL);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	rcvbuf.maxlen = tiptr->ti_rcvsize;
	rcvbuf.len = 0;
	rcvbuf.buf = tiptr->ti_rcvbuf;

	if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
		if (errno == EAGAIN)
			set_t_errno(TNODATA);
		else
			set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if (rcvbuf.len == -1) rcvbuf.len = 0;

	/*
	 * did I get entire message?
	 */
	if (retval) {
		set_t_errno(TSYSERR);
		errno = EIO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 * is ctl part large enough to determine type
	 */
	if (ctlbuf.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;

	switch(pptr->type) {

		case T_CONN_IND:
			if ((ctlbuf.len < sizeof(struct T_conn_ind)) ||
			    (ctlbuf.len < (pptr->conn_ind.OPT_length
			    + pptr->conn_ind.OPT_offset))) {
				set_t_errno(TSYSERR);
				errno = EPROTO;
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
			}
			if (((call->udata.maxlen != 0) &&
			 (rcvbuf.len > call->udata.maxlen)) ||
			 ((call->addr.maxlen != 0) &&
			  (pptr->conn_ind.SRC_length > call->addr.maxlen)) ||
			 ((call->opt.maxlen != 0) &&
			  (pptr->conn_ind.OPT_length > call->opt.maxlen))) {
				set_t_errno(TBUFOVFLW);
				tiptr->ti_ocnt++;
				tiptr->ti_state
				   = TLI_NEXTSTATE(T_LISTN, tiptr->ti_state);
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
			}

			call->addr.len = 0;
			call->opt.len = 0;
			call->udata.len = 0;
			if (call->addr.maxlen != 0) {
				memcpy(call->addr.buf,
				 ctlbuf.buf + pptr->conn_ind.SRC_offset,
				 (int)pptr->conn_ind.SRC_length);
				call->addr.len = pptr->conn_ind.SRC_length;
			}
			if (call->opt.maxlen != 0) {
				memcpy(call->opt.buf,
				 ctlbuf.buf + pptr->conn_ind.OPT_offset,
				 (int)pptr->conn_ind.OPT_length);
				call->opt.len = pptr->conn_ind.OPT_length;
			}
			if (call->udata.maxlen != 0) {
				memcpy(call->udata.buf, rcvbuf.buf,
				 (int)rcvbuf.len);
				call->udata.len = rcvbuf.len;
			}
			call->sequence = (long) pptr->conn_ind.SEQ_number;

			tiptr->ti_ocnt++;
			tiptr->ti_state
			   = TLI_NEXTSTATE(T_LISTN, tiptr->ti_state);
			MUTEX_UNLOCK(&tiptr->lock);
			return(0);

		case T_DISCON_IND:
			_t_putback(tiptr, rcvbuf.buf, rcvbuf.len,
				   ctlbuf.buf, ctlbuf.len);
			set_t_errno(TLOOK);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);

		default:
			break;
	}

	set_t_errno(TSYSERR);
	errno = EPROTO;
	MUTEX_UNLOCK(&tiptr->lock);
	return(-1);
}
