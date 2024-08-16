/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)libsocket:common/lib/libsocket/socket/_conn_util.c	1.6.10.5"
#ident	"$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <sys/sockmod.h>
#include <sys/signal.h>
#include <fcntl.h>

static const int eagain_limit = 3;

/*
 * Snd_conn_req - send connect request message to
 * transport provider
 */
int
_s_snd_conn_req(siptr, call)
	register struct _si_user	*siptr;
	register struct t_call		*call;
{
	register struct T_conn_req	*creq;
	register char			*buf;
	register int			size;
	struct strbuf			ctlbuf;

	buf = siptr->ctlbuf;
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof (struct T_conn_req);

	if ((int)call->addr.len > 0 && buf != (char *)NULL) {
		_s_aligned_copy(buf, call->addr.len, size,
			call->addr.buf, &creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if ((int)call->opt.len > 0 && buf != (char *)NULL) {
		_s_aligned_copy(buf, call->opt.len, size,
			call->opt.buf, &creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	if (putmsg(siptr->fd, &ctlbuf, (call->udata.len? &call->udata: NULL),
					0) < 0)
		return (-1);

	if (!_s_is_ok(siptr, T_CONN_REQ))
		return (-1);

	return (0);
}

/*
 * Rcv_conn_con - get connection confirmation off
 * of read queue
 */
int
_s_rcv_conn_con(siptr)
	register struct _si_user	*siptr;
{
	struct strbuf			ctlbuf;
	struct strbuf			databuf;
	register union T_primitives	*pptr;
	register int			retval;
	int				flg = 0;
	char				dbuf[128];
	int				fctlflg;
	int				i;

	if (siptr->udata.servtype == T_CLTS) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	if ((fctlflg = _fcntl(siptr->fd, F_GETFL, 0)) < 0) {
		return (-1);
	}
again:
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	databuf.maxlen = sizeof (dbuf);
	databuf.len = 0;
	databuf.buf = dbuf;

	/*
	 * Allow for a few EAGAINs, then fail, to deal with STREAMS
	 * asynchrony.
	 * No data are expected, but play it safe and provide a data buffer.
	 */
	for (i=1; ; i++) { 
		/* 
		 * The loop counter is checked in the EAGAIN case, since
		 * only there can we continue to iterate.
		 * In case the getmsg() succeeds, we break out of the loop.
		 * In all other situations, we return from inside the loop.
		 */
		if ((retval = getmsg(siptr->fd, &ctlbuf, &databuf, &flg)) >= 0){
			break;
		}
		switch (errno) {
		case EAGAIN:
			if ((fctlflg & (O_NDELAY | O_NONBLOCK)) 
			 && (i < eagain_limit)) {
				sleep(1);
				continue;
			}
			if (fctlflg & (O_NDELAY | O_NONBLOCK)) {
				errno = EINPROGRESS;
			}
			return (-1);
		case ENXIO:
			errno = ECONNREFUSED;
			return (-1);
		default:
			return (-1);
		}
	}


	/*
	 * did we get entire message
	 */
	if (retval) {
		errno = EIO;
		return (-1);
	}

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof (long)) {
		errno = EPROTO;
		return (-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_CON:
			return (0);

		case T_DISCON_IND:
			if (ctlbuf.len < sizeof (struct T_discon_ind))
				errno = ECONNREFUSED;
			else	errno = pptr->discon_ind.DISCON_reason;
			return (-1);

		default:
			break;
	}

	errno = EPROTO;
	return (-1);
}
