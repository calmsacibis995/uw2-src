/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_conn_util.c	1.6.8.5"
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
#include <string.h>
#include "_import.h"

extern struct _ti_user *_t_checkfd();
extern struct _ti_user **fdp;

/*
 * Snd_conn_req - send connect request message to 
 * transport provider.
 * _snd_conn_req() is called only in t_connect().
 * Before _snd_conn_req() is called (in t_connect()),
 * _t_checkfd() is called to verify that fdp[fd] is not NULL and
 * points to the proper _ti_user structure.  Since fdp[fd] is NULL,
 * no thread will attempt to update it, so _nsl_lock need not be acquired.
 * However, we must acquire fdp[fd]->lock for writing in order to 
 * update the _ti_user structure and its contents.
 */
_snd_conn_req(fd, tiptr, call)
int fd;
struct _ti_user *tiptr;
struct t_call *call;
{
	struct T_conn_req *creq;
	struct strbuf ctlbuf;
	char *buf;
	int size;
	sigset_t		oldmask;
	sigset_t		newmask;
	
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (_t_is_event(fd, tiptr)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}


	buf = tiptr->ti_ctlbuf;
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof(struct T_conn_req);

	if (call->addr.len) {
		_t_aligned_copy(buf, call->addr.len, size,
			     call->addr.buf, &creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if (call->opt.len) {
		_t_aligned_copy(buf, call->opt.len, size,
			     call->opt.buf, &creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	if (putmsg(fd, &ctlbuf,
		   (call->udata.len? (struct strbuf *)&call->udata: NULL), 0)
	    < 0) {
		set_t_errno(TSYSERR);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}

	if (!_t_is_ok(fd, tiptr, T_CONN_REQ)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}

	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	return(0);
}



/*
 * Rcv_conn_con - get connection confirmation off
 * of read queue
 * Called only by t_rcvconnect() and t_connect() after _t_checkfd() is called.
 * Therefore, fdp[fd] is not NULL and will not be changed, so
 * _nsl_lock need not be acquired.
 */
_rcv_conn_con(fd, tiptr, call)
int fd;
struct _ti_user *tiptr;
struct t_call *call;
{
	struct strbuf ctlbuf;
	struct strbuf rcvbuf;
	int flg = 0;
	union T_primitives *pptr;
	int retval;


	/*
         * see if there is something in look buffer
	 */
	if (tiptr->ti_lookflg) {
		set_t_errno(TLOOK);
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
		return(-1);
	}
	if (rcvbuf.len == -1) rcvbuf.len = 0;


	/*
	 * did we get entire message 
	 */
	if (retval) {
		set_t_errno(TSYSERR);
		errno = EIO;
		return(-1);
	}

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		return(-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;

	switch(pptr->type) {

		case T_CONN_CON:

			if ((ctlbuf.len < sizeof(struct T_conn_con)) ||
			    (pptr->conn_con.OPT_length != 0 &&
			    (ctlbuf.len < (pptr->conn_con.OPT_length +
			     pptr->conn_con.OPT_offset)))) {
				set_t_errno(TSYSERR);
				errno = EPROTO;
				return(-1);
			}

			if (call != NULL) {
				if (((call->udata.maxlen != 0) &&
				 (rcvbuf.len > call->udata.maxlen))
				 || ((call->addr.maxlen != 0) &&
				     (pptr->conn_con.RES_length >
				      call->addr.maxlen))
				 || ((call->opt.maxlen != 0) &&
				     (pptr->conn_con.OPT_length >
				      call->opt.maxlen))) {
					set_t_errno(TBUFOVFLW);
					return(-1);
				}
				call->addr.len = 0;
				call->opt.len = 0;
				call->udata.len = 0;
				if (call->addr.maxlen != 0) {
					memcpy(call->addr.buf, ctlbuf.buf +
					 pptr->conn_con.RES_offset,
					 (int)pptr->conn_con.RES_length);
					call->addr.len =
					 pptr->conn_con.RES_length;
				}
				if (call->opt.maxlen != 0) {
					memcpy(call->opt.buf, ctlbuf.buf +
					 pptr->conn_con.OPT_offset,
					 (int)pptr->conn_con.OPT_length);
					call->opt.len =
					 pptr->conn_con.OPT_length;
				}
				if (call->udata.maxlen != 0) {
					memcpy(call->udata.buf, rcvbuf.buf,
					 (int)rcvbuf.len);
					call->udata.len = rcvbuf.len;
				}
				/*
				 * since a confirmation seq number
				 * is -1 by default
				 */
				call->sequence = (long) -1;
			}
			return(0);

		case T_DISCON_IND:

			/*
			 * if disconnect indication then put it in look buf
			 */
			_t_putback(tiptr, rcvbuf.buf, rcvbuf.len, ctlbuf.buf,
				   ctlbuf.len);
			set_t_errno(TLOOK);
			return(-1);

		default:
			break;
	}

	set_t_errno(TSYSERR);
	errno = EPROTO;
	return(-1);
}

int
_t_force_cots(int fd)
{
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}
	tiptr->ti_servtype = T_COTS;
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}
