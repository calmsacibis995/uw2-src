/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_dg.c	1.3.13.6"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * clnt_dg.c
 * Implements a connectionless client side RPC.
 */

#include <stdlib.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <errno.h>
#include <sys/poll.h>
#include <sys/syslog.h>
#include <sys/types.h>
/*
#include <sys/kstat.h>
*/
#include <sys/time.h>
#include "rpc_mt.h"

#define	RPC_MAX_BACKOFF		30 /* seconds */
#undef rpc_createerr		/* need to create automatics with same name */

static const struct clnt_ops *clnt_dg_ops();
static bool_t time_not_ok();
static enum clnt_stat clnt_dg_call();
static void clnt_dg_abort();
static void clnt_dg_geterr();
static bool_t clnt_dg_freeres();
static void clnt_dg_destroy();
static bool_t clnt_dg_control();

static const struct clnt_ops ops = {
	clnt_dg_call,
	clnt_dg_abort,
	clnt_dg_geterr,
	clnt_dg_freeres,
	clnt_dg_destroy,
	clnt_dg_control
};

/*
 * Private data kept per client handle
 */
struct cu_data {
	int			cu_fd;		/* connections fd */
	bool_t			cu_closeit;	/* opened by library */
	struct netbuf		cu_raddr;	/* remote address */
	struct timeval		cu_wait;	/* retransmit interval */
	struct timeval		cu_total;	/* total time for the call */
	struct rpc_err		cu_error;
	struct t_unitdata	*cu_tr_data;
	XDR			cu_outxdrs;
	u_int			cu_xdrpos;
	u_int			cu_sendsz;	/* send size */
	char			*cu_outbuf;
	u_int			cu_recvsz;	/* recv size */
	char			cu_inbuf[1];
};

/*
 * Connection less client creation returns with client handle parameters.
 * Default options are set, which the user can change using clnt_control().
 * fd should be open and bound.
 * NB: The rpch->cl_auth is initialized to null authentication.
 * 	Caller may wish to set this something more useful.
 *
 * sendsz and recvsz are the maximum allowable packet sizes that can be
 * sent and received. Normally they are the same, but they can be
 * changed to improve the program efficiency and buffer allocation.
 * If they are 0, use the transport default.
 *
 * If svcaddr is NULL, returns NULL.
 */
CLIENT *
clnt_dg_create(fd, svcaddr, program, version, sendsz, recvsz)
	int fd;				/* open file descriptor */
	struct netbuf *svcaddr;		/* servers address */
	u_long program;			/* program number */
	u_long version;			/* version number */
	u_int sendsz;			/* buffer recv size */
	u_int recvsz;			/* buffer send size */
{
	CLIENT *cl = NULL;			/* client handle */
	register struct cu_data *cu = NULL;	/* private data */
	struct t_unitdata *tr_data;
	struct t_info tinfo;
	struct timeval now;
	struct rpc_msg call_msg;
	rpc_createerr_t rpc_createerr = { 0 };

	trace5(TR_clnt_dg_create, 0, program, version, sendsz, recvsz);
	if (svcaddr == (struct netbuf *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNADDR;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_dg_create, 1, program, version);
		return ((CLIENT *)NULL);
	}
	if (t_getinfo(fd, &tinfo) == -1) {
		rpc_createerr.cf_stat = RPC_TLIERROR;
		rpc_createerr.cf_error.re_errno = 0;
		rpc_createerr.cf_error.re_terrno = t_errno;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_dg_create, 1, program, version);
		return ((CLIENT *)NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	sendsz = _rpc_get_t_size((int)sendsz, tinfo.tsdu);
	recvsz = _rpc_get_t_size((int)recvsz, tinfo.tsdu);
	if ((sendsz == 0) || (recvsz == 0)) {
		rpc_createerr.cf_stat = RPC_TLIERROR; /* XXX */
		rpc_createerr.cf_error.re_errno = 0;
		rpc_createerr.cf_error.re_terrno = 0;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_dg_create, 1, program, version);
		return ((CLIENT *)NULL);
	}

	if ((cl = (CLIENT *)mem_alloc(sizeof (CLIENT))) == (CLIENT *)NULL)
		goto err1;
	/*
	 * Should be multiple of 4 for XDR.
	 */
	sendsz = ((sendsz + 3) / 4) * 4;
	recvsz = ((recvsz + 3) / 4) * 4;
	cu = (struct cu_data *)mem_alloc(sizeof (*cu) + sendsz + recvsz);
	if (cu == (struct cu_data *)NULL)
		goto err1;
	cu->cu_raddr = *svcaddr;
	if ((cu->cu_raddr.buf = mem_alloc(svcaddr->len)) == NULL)
		goto err1;
	(void) memcpy(cu->cu_raddr.buf, svcaddr->buf, (int)svcaddr->len);
	cu->cu_outbuf = &cu->cu_inbuf[recvsz];
	/* Other values can also be set through clnt_control() */
	cu->cu_wait.tv_sec = 15;	/* heuristically chosen */
	cu->cu_wait.tv_usec = 0;
	cu->cu_total.tv_sec = -1;
	cu->cu_total.tv_usec = -1;
	cu->cu_sendsz = sendsz;
	cu->cu_recvsz = recvsz;
	(void) gettimeofday(&now, (struct timezone *) 0);
	call_msg.rm_xid = getpid() ^ now.tv_sec ^ now.tv_usec;
	call_msg.rm_call.cb_prog = program;
	call_msg.rm_call.cb_vers = version;
	xdrmem_create(&(cu->cu_outxdrs), cu->cu_outbuf, sendsz, XDR_ENCODE);
	if (! xdr_callhdr(&(cu->cu_outxdrs), &call_msg)) {
		rpc_createerr.cf_stat = RPC_CANTENCODEARGS;  /* XXX */
		rpc_createerr.cf_error.re_errno = 0;
		rpc_createerr.cf_error.re_terrno = 0;
		set_rpc_createerr(rpc_createerr);
		goto err2;
	}
	cu->cu_xdrpos = XDR_GETPOS(&(cu->cu_outxdrs));

	tr_data = (struct t_unitdata *)t_alloc(fd,
				T_UNITDATA, T_ADDR | T_OPT);
	if (tr_data == (struct t_unitdata *)NULL) {
		goto err1;
	}
	tr_data->udata.maxlen = cu->cu_recvsz;
	tr_data->udata.buf = cu->cu_inbuf;
	cu->cu_tr_data = tr_data;

	/*
	 * By default, closeit is always FALSE. It is users responsibility
	 * to do a t_close on it, else the user may use clnt_control
	 * to let clnt_destroy do it for him/her.
	 */
	cu->cu_closeit = FALSE;
	cu->cu_fd = fd;
	cl->cl_ops = (struct clnt_ops *)clnt_dg_ops();
	cl->cl_private = (caddr_t)cu;
	cl->cl_auth = authnone_create();
	cl->cl_tp = (char *) NULL;
	cl->cl_netid = (char *) NULL;
	trace3(TR_clnt_dg_create, 1, program, version);
	return (cl);
err1:
	(void) syslog(LOG_ERR, 
	    gettxt("uxnsl:32", "%s: out of memory"),
	    "clnt_dg_create");
	rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = errno;
	rpc_createerr.cf_error.re_terrno = 0;
	set_rpc_createerr(rpc_createerr);
err2:
	if (cl) {
		mem_free((caddr_t)cl, sizeof (CLIENT));
		if (cu) {
			mem_free(cu->cu_raddr.buf, cu->cu_raddr.len);
			mem_free((caddr_t)cu, sizeof (*cu) + sendsz + recvsz);
		}
	}
	trace3(TR_clnt_dg_create, 1, program, version);
	return ((CLIENT *)NULL);
}

static enum clnt_stat
clnt_dg_call(cl, proc, xargs, argsp, xresults, resultsp, utimeout)
	register CLIENT	*cl;		/* client handle */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	struct timeval	utimeout;	/* seconds to wait before giving up */
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs;
	register int outlen;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	struct timeval time_waited;
	bool_t ok;
	int nrefreshes = 2;		/* number of times to refresh cred */
	struct timeval timeout;
	struct timeval retransmit_time;
	struct timeval startime, curtime;
	int firsttimeout = 1;
	struct t_unitdata tu_data;
	int res;			/* result of operations */
#ifdef _REENTRANT
	struct pollfd *pfdp;
	struct pollfd pfd;
#else
	static struct pollfd *pfdp;
#endif /* _REENTRANT */
	int dtbsize = _rpc_dtbsize();

	trace3(TR_clnt_dg_call, 0, cl, proc);
	if (cu->cu_total.tv_usec == -1) {
		timeout = utimeout;	/* use supplied timeout */
	} else {
		timeout = cu->cu_total;	/* use default timeout */
	}

	time_waited.tv_sec = 0;
	time_waited.tv_usec = 0;
	retransmit_time = cu->cu_wait;

	tu_data.addr = cu->cu_raddr;

call_again:
	xdrs = &(cu->cu_outxdrs);
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, cu->cu_xdrpos);
	/*
	 * the transaction is the first thing in the out buffer
	 */
	(*(u_long *)(cu->cu_outbuf))++;
	if ((! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp))) {
		trace2(TR_clnt_dg_call, 1, cl);
		return (cu->cu_error.re_status = RPC_CANTENCODEARGS);
	}
	outlen = (int)XDR_GETPOS(xdrs);

send_again:
	tu_data.udata.buf = cu->cu_outbuf;
	tu_data.udata.len = outlen;
	tu_data.opt.len = 0;
	if (t_sndudata(cu->cu_fd, &tu_data) == -1) {
		cu->cu_error.re_terrno = t_errno;
		cu->cu_error.re_errno = errno;
		trace2(TR_clnt_dg_call, 1, cl);
		return (cu->cu_error.re_status = RPC_CANTSEND);
	}

	/*
	 * Hack to provide rpc-based message passing
	 */
	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		trace2(TR_clnt_dg_call, 1, cl);
		return (cu->cu_error.re_status = RPC_TIMEDOUT);
	}
	/*
	 * sub-optimal code appears here because we have
	 * some clock time to spare while the packets are in flight.
	 * (We assume that this is actually only executed once.)
	 */
	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xresults;
	
	/*
	 * Under _REENTRANT, we don't support call back feature.
	 * _svc_getreqset_proc should be NULL.
	 */
#ifdef _REENTRANT
	pfdp = &pfd;
#else
	if (pfdp == (struct pollfd *) NULL) {
		pfdp = (struct pollfd *)
			malloc(sizeof (struct pollfd) * dtbsize);
		if (pfdp == (struct pollfd *) NULL) {
			trace2(TR_clnt_dg_call, 1, cl);
			return (-1);
		}
	}
#endif /* _REENTRANT */
	/*
	 *	N.B.:  slot 0 in the pollfd array is reserved for the file
	 *	descriptor we're really interested in (as opposed to the
	 *	callback descriptors).
	 */
	pfdp->fd = cu->cu_fd;
	pfdp->events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND;

	for (;;) {
		extern void (*_svc_getreqset_proc)();
		extern fd_set svc_fdset;
		int fds;
		int nfds;

		/*
		 * This provides for callback support.  When a client
		 * recv's a call from another client on the server fd's,
		 * it calls _svc_getreqset(&pfdp[1], fds) which would return
		 * after serving all the server requests.  Also look under
		 * svc.c
		 */
		if (_svc_getreqset_proc)
			/* ``+ 1'' because of pfdp[0] */
			nfds = _rpc_select_to_poll(dtbsize,
						&svc_fdset, &pfdp[1]) + 1;
		else
			nfds = 1;	/* don't forget about pfdp[0] */


		switch (fds = poll(pfdp, nfds,
				_rpc_timeval_to_msec(&retransmit_time))) {
		case 0:
			time_waited.tv_sec += retransmit_time.tv_sec;
			time_waited.tv_usec += retransmit_time.tv_usec;
			while (time_waited.tv_usec >= 1000000) {
				time_waited.tv_sec++;
				time_waited.tv_usec -= 1000000;
			}
			/* update retransmit_time */
			if (retransmit_time.tv_sec < RPC_MAX_BACKOFF) {
				retransmit_time.tv_usec *= 2;
				retransmit_time.tv_sec *= 2;
				while (retransmit_time.tv_usec >= 1000000) {
					retransmit_time.tv_sec++;
					retransmit_time.tv_usec -= 1000000;
				}
			}

			if ((time_waited.tv_sec < timeout.tv_sec) ||
			    ((time_waited.tv_sec == timeout.tv_sec) &&
				(time_waited.tv_usec < timeout.tv_usec)))
				goto send_again;
			trace2(TR_clnt_dg_call, 1, cl);
			return (cu->cu_error.re_status = RPC_TIMEDOUT);

		case -1:
			if (errno == EBADF) {
				cu->cu_error.re_errno = EBADF;
				cu->cu_error.re_terrno = 0;
				trace2(TR_clnt_dg_call, 1, cl);
				return (cu->cu_error.re_status = RPC_CANTRECV);
			}
			if (errno != EINTR) {
				errno = 0; /* reset it */
				continue;
			}
			/* interrupted by another signal, update time_waited */
			if (firsttimeout) {
				/*
				 * Could have done gettimeofday before clnt_call
				 * but that means 1 more system call per each
				 * clnt_call, so do it after first time out
				 */
				if (gettimeofday(&startime, (struct timezone *) 0) == -1) {
					errno = 0;
					continue;
				}
				firsttimeout = 0;
				errno = 0;
				continue;
			};
			if (gettimeofday(&curtime, (struct timezone *) 0) == -1) {
				errno = 0;
				continue;
			};
			time_waited.tv_sec += curtime.tv_sec - startime.tv_sec;
			time_waited.tv_usec += curtime.tv_usec -
							startime.tv_usec;
			while (time_waited.tv_usec < 0) {
				time_waited.tv_sec--;
				time_waited.tv_usec += 1000000;
			};
			while (time_waited.tv_usec >= 1000000) {
				time_waited.tv_sec++;
				time_waited.tv_usec -= 1000000;
			}
			startime.tv_sec = curtime.tv_sec;
			startime.tv_usec = curtime.tv_usec;
			if ((time_waited.tv_sec > timeout.tv_sec) ||
				((time_waited.tv_sec == timeout.tv_sec) &&
				(time_waited.tv_usec > timeout.tv_usec))) {
				trace2(TR_clnt_dg_call, 1, cl);
				return (cu->cu_error.re_status = RPC_TIMEDOUT);
			}
			errno = 0; /* reset it */
			continue;
		};

		if (pfdp->revents == 0) {
			/* must be for server side of the house */
			(*_svc_getreqset_proc)(&pfdp[1], fds);
			continue;			/* do poll again */
		} else if (pfdp->revents & POLLNVAL) {
			cu->cu_error.re_status = RPC_CANTRECV;
			/*
			 *	Note:  we're faking errno here because we
			 *	previously would have expected select() to
			 *	return -1 with errno EBADF.  Poll(BA_OS)
			 *	returns 0 and sets the POLLNVAL revents flag
			 *	instead.
			 */
			cu->cu_error.re_errno = errno = EBADF;
			trace2(TR_clnt_dg_call, 1, cl);
			return (-1);
		}

		/* We have some data now */
		do {
			int moreflag;		/* flag indicating more data */

			moreflag = 0;
			if (errno == EINTR) {
				/*
				 * Must make sure errno was not already
				 * EINTR in case t_rcvudata() returns -1.
				 * This way will only stay in the loop
				 * if getmsg() sets errno to EINTR.
				 */
				errno = 0;
			}
			res = t_rcvudata(cu->cu_fd, cu->cu_tr_data, &moreflag);
			if (moreflag & T_MORE) {
				/*
				 * Drop this packet. I aint got any
				 * more space.
				 */
				res = -1;
				/* I should not really be doing this */
				errno = 0;
				/*
				 * XXX: Not really Buffer overflow in the
				 * sense of TLI.
				 */
				set_t_errno(TBUFOVFLW);
			}
		} while (res < 0 && errno == EINTR);
		if (res < 0) {
			if (errno == EAGAIN)
				continue;
			if (t_errno == TLOOK) {
				int old;

				old = t_errno;
				if (t_rcvuderr(cu->cu_fd, NULL) == 0)
					continue;
				else
					cu->cu_error.re_terrno = old;
			} else {
				cu->cu_error.re_terrno = t_errno;
			}
			cu->cu_error.re_errno = errno;
			trace2(TR_clnt_dg_call, 1, cl);
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}
		if (cu->cu_tr_data->udata.len < sizeof (u_long))
			continue;
		/* see if reply transaction id matches sent id */
		if (*((u_long *)(cu->cu_inbuf)) != *((u_long *)(cu->cu_outbuf)))
			continue;
		/* we now assume we have the proper reply */
		break;
	}

	/*
	 * now decode and validate the response
	 */

	xdrmem_create(&reply_xdrs, cu->cu_inbuf,
		(u_int)cu->cu_tr_data->udata.len, XDR_DECODE);
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	/* XDR_DESTROY(&reply_xdrs);	save a few cycles on noop destroy */
	if (ok) {
		if ((reply_msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
			(reply_msg.acpted_rply.ar_stat == SUCCESS))
			cu->cu_error.re_status = RPC_SUCCESS;
		else
			_seterr_reply(&reply_msg, &(cu->cu_error));

		if (cu->cu_error.re_status == RPC_SUCCESS) {
			if (! AUTH_VALIDATE(cl->cl_auth,
					    &reply_msg.acpted_rply.ar_verf)) {
				cu->cu_error.re_status = RPC_AUTHERROR;
				cu->cu_error.re_why = AUTH_INVALIDRESP;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				xdrs->x_op = XDR_FREE;
				(void) xdr_opaque_auth(xdrs,
					&(reply_msg.acpted_rply.ar_verf));
			}
		}		/* end successful completion */
		/*
		 * If unsuccesful AND error is an authentication error
		 * then refresh credentials and try again, else break
		 */
		else if (cu->cu_error.re_status == RPC_AUTHERROR)
			/* maybe our credentials need to be refreshed ... */
			if (nrefreshes > 0 && AUTH_REFRESH(cl->cl_auth)) {
				nrefreshes--;
				goto call_again;
			}
		/* end of unsuccessful completion */
	}	/* end of valid reply message */
	else {
		cu->cu_error.re_status = RPC_CANTDECODERES;

	}
	trace2(TR_clnt_dg_call, 1, cl);
	return (cu->cu_error.re_status);
}

static void
clnt_dg_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	trace2(TR_clnt_dg_geterr, 0, cl);
	*errp = cu->cu_error;
	trace2(TR_clnt_dg_geterr, 1, cl);
}

static bool_t
clnt_dg_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs = &(cu->cu_outxdrs);
	bool_t dummy;

	trace2(TR_clnt_dg_freeres, 0, cl);
	xdrs->x_op = XDR_FREE;
	dummy = (*xdr_res)(xdrs, res_ptr);
	trace2(TR_clnt_dg_freeres, 1, cl);
	return (dummy);
}

static void
clnt_dg_abort(/* h */)
	/* CLIENT *h; */
{
	trace1(TR_clnt_dg_abort, 0);
	trace1(TR_clnt_dg_abort, 1);
}

static bool_t
clnt_dg_control(cl, request, info)
	CLIENT *cl;
	int request;
	char *info;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	trace3(TR_clnt_dg_control, 0, cl, request);
	switch (request) {
	case CLSET_FD_CLOSE:
		cu->cu_closeit = TRUE;
		trace2(TR_clnt_dg_control, 1, cl);
		return (TRUE);
	case CLSET_FD_NCLOSE:
		cu->cu_closeit = FALSE;
		trace2(TR_clnt_dg_control, 1, cl);
		return (TRUE);
	}

	/* for other requests which use info */
	if (info == NULL) {
		trace2(TR_clnt_dg_control, 1, cl);
		return (FALSE);
	}
	switch (request) {
	case CLSET_TIMEOUT:
		if (time_not_ok((struct timeval *)info)) {
			trace2(TR_clnt_dg_control, 1, cl);
			return (FALSE);
		}
		cu->cu_total = *(struct timeval *)info;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *)info = cu->cu_total;
		break;
	case CLGET_SERVER_ADDR:		/* Give him the fd address */
		/* Now obsolete. Only for backword compatibility */
		(void) memcpy(info, cu->cu_raddr.buf, (int)cu->cu_raddr.len);
		break;
	case CLSET_RETRY_TIMEOUT:
		if (time_not_ok((struct timeval *)info)) {
			trace2(TR_clnt_dg_control, 1, cl);
			return (FALSE);
		}
		cu->cu_wait = *(struct timeval *)info;
		break;
	case CLGET_RETRY_TIMEOUT:
		*(struct timeval *)info = cu->cu_wait;
		break;
	case CLGET_FD:
		*(int *)info = cu->cu_fd;
		break;
	case CLGET_SVC_ADDR:
		*(struct netbuf *)info = cu->cu_raddr;
		break;
	case CLGET_XID:
		/*
		 * use the knowledge that xid is the
		 * first element in the call structure *.
		 * This will get the xid of the PREVIOUS call
		 */
		*(u_long *)info = ntohl(*(u_long *)cu->cu_outbuf);
		break;
	case CLSET_XID:
		/* This will set the xid of the NEXT call */
		*(u_long *)cu->cu_outbuf =  htonl(*(u_long *)info - 1);
		/* decrement by 1 as clnt_dg_call() increments once */
		break;
	case CLGET_VERS:
		/*
		 * This RELIES on the information that, in the call body,
		 * the version number field is the fifth field from the
		 * begining of the RPC header. MUST be changed if the
		 * call_struct is changed
		 */
		*(u_long *)info = ntohl(*(u_long *)(cu->cu_outbuf +
						    4 * BYTES_PER_XDR_UNIT));
		break;

	case CLSET_VERS:
		*(u_long *)(cu->cu_outbuf + 4 * BYTES_PER_XDR_UNIT)
			= htonl(*(u_long *)info);
		break;

	default:
		trace2(TR_clnt_dg_control, 1, cl);
		return (FALSE);
	}
	trace2(TR_clnt_dg_control, 1, cl);
	return (TRUE);
}

static void
clnt_dg_destroy(cl)
	CLIENT *cl;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	trace2(TR_clnt_dg_destroy, 0, cl);
	if (cu->cu_closeit)
		(void) t_close(cu->cu_fd);
	XDR_DESTROY(&(cu->cu_outxdrs));
	cu->cu_tr_data->udata.buf = NULL;
	(void) t_free((caddr_t)cu->cu_tr_data, T_UNITDATA);
	(void) mem_free(cu->cu_raddr.buf, cu->cu_raddr.len);
	(void) mem_free((caddr_t)cu,
		(sizeof (*cu) + cu->cu_sendsz + cu->cu_recvsz));
	if (cl->cl_netid && cl->cl_netid[0])
		(void) mem_free(cl->cl_netid, strlen(cl->cl_netid) +1);
	if (cl->cl_tp && cl->cl_tp[0])
		(void) mem_free(cl->cl_tp, strlen(cl->cl_tp) +1);
	(void) mem_free((caddr_t)cl, sizeof (CLIENT));
	trace2(TR_clnt_dg_destroy, 1, cl);
}

static const struct clnt_ops *
clnt_dg_ops()
{
	trace1(TR_clnt_dg_ops, 0);
	trace1(TR_clnt_dg_ops, 1);
	return (&ops);
}

/*
 * Make sure that the time is not garbage.  -1 value is allowed.
 */
static bool_t
time_not_ok(t)
	struct timeval *t;
{
	trace1(TR_time_not_ok, 0);
	trace1(TR_time_not_ok, 1);
	return (t->tv_sec < -1 || t->tv_sec > 100000000 ||
		t->tv_usec < -1 || t->tv_usec > 1000000);
}


