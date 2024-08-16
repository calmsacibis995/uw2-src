/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_raw.c	1.3.8.3"
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
 * clnt_raw.c
 *
 * Memory based rpc for simple testing and timing.
 * Interface to create an rpc client and server in the same process.
 * This lets us similate rpc and get round trip overhead, without
 * any interference from the kernel.
 */

#include <unistd.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/raw.h>
#include <sys/syslog.h>

#define	MCALL_MSG_SIZE 24
#ifndef UDPMSGSIZE
#define	UDPMSGSIZE 8800
#endif

/*
 * This is the "network" we will be moving stuff over.
 */
static struct clnt_raw_private {
	CLIENT	client_object;
	XDR	xdr_stream;
	char	*raw_buf;	/* should be shared with server handle */
	char	mashl_callmsg[MCALL_MSG_SIZE];
	u_int	mcnt;
} *clnt_raw_private;

static struct clnt_ops *clnt_raw_ops();

extern char	*calloc();
void svc_getreqcommon(int);

/*
 * Create a client handle for memory based rpc.
 */
CLIENT *
clnt_raw_create(prog, vers)
	u_long prog;
	u_long vers;
{
	register struct clnt_raw_private *clp = clnt_raw_private;
	struct rpc_msg call_msg;
	XDR *xdrs;
	CLIENT *client;

	trace3(TR_clnt_raw_create, 0, prog, vers);
	if (clp == NULL) {
		clp = (struct clnt_raw_private *)calloc(1, sizeof (*clp));
		if (clp == NULL) {
			trace3(TR_clnt_raw_create, 1, prog, vers);
			return ((CLIENT *)NULL);
		}
		if (_rawcombuf == NULL)
			_rawcombuf = (char *)calloc(UDPMSGSIZE, sizeof (char));
		clp->raw_buf = _rawcombuf; /* Share it with the server */
		clnt_raw_private = clp;
	}
	xdrs = &clp->xdr_stream;
	client = &clp->client_object;

	/*
	 * pre-serialize the static part of the call msg and stash it away
	 */
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;
	xdrmem_create(xdrs, clp->mashl_callmsg, MCALL_MSG_SIZE, XDR_ENCODE);
	if (! xdr_callhdr(xdrs, &call_msg))
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:82",
			"clnt_raw_create: Fatal header serialization error."));

	clp->mcnt = XDR_GETPOS(xdrs);
	XDR_DESTROY(xdrs);

	/*
	 * Set xdrmem for client/server shared buffer
	 */
	xdrmem_create(xdrs, clp->raw_buf, UDPMSGSIZE, XDR_FREE);

	/*
	 * create client handle
	 */
	client->cl_ops = clnt_raw_ops();
	client->cl_auth = authnone_create();
	trace3(TR_clnt_raw_create, 1, prog, vers);
	return (client);
}

static enum clnt_stat
clnt_raw_call(h, proc, xargs, argsp, xresults, resultsp, timeout)
	CLIENT *h;
	u_long proc;
	xdrproc_t xargs;
	caddr_t argsp;
	xdrproc_t xresults;
	caddr_t resultsp;
	struct timeval timeout;
{
	register struct clnt_raw_private *clp = clnt_raw_private;
	register XDR *xdrs = &clp->xdr_stream;
	struct rpc_msg msg;
	enum clnt_stat status;
	struct rpc_err error;

	trace3(TR_clnt_raw_call, 0, h, proc);
	if (clp == NULL) {
		trace3(TR_clnt_raw_call, 1, h, proc);
		return (RPC_FAILED);
	}
call_again:
	/*
	 * send request
	 */
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	((struct rpc_msg *)clp->mashl_callmsg)->rm_xid++;
	if ((! XDR_PUTBYTES(xdrs, clp->mashl_callmsg, clp->mcnt)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp))) {
		trace3(TR_clnt_raw_call, 1, h, proc);
		return (RPC_CANTENCODEARGS);
	}
	(void) XDR_GETPOS(xdrs);  /* called just to cause overhead */

	/*
	 * We have to call server input routine here because this is
	 * all going on in one process.
	 */
	svc_getreq_common(FD_SETSIZE);

	/*
	 * get results
	 */
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = resultsp;
	msg.acpted_rply.ar_results.proc = xresults;
	if (! xdr_replymsg(xdrs, &msg)) {
		trace3(TR_clnt_raw_call, 1, h, proc);
		return (RPC_CANTDECODERES);
	}
	if ((msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
		    (msg.acpted_rply.ar_stat == SUCCESS))
		status = RPC_SUCCESS;
	else {
		_seterr_reply(&msg, &error);
		status = error.re_status;
	}

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
		/* end successful completion */
	} else {
		if (AUTH_REFRESH(h->cl_auth))
			goto call_again;
		/* end of unsuccessful completion */
	}

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
		if (msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void) xdr_opaque_auth(xdrs,
					&(msg.acpted_rply.ar_verf));
		}
	}
	trace3(TR_clnt_raw_call, 1, h, proc);
	return (status);
}

static void
clnt_raw_geterr(cl, errp)
CLIENT *cl;
struct rpc_err *errp;
{
	trace1(TR_clnt_raw_geterr, 0);
	trace1(TR_clnt_raw_geterr, 1);
}

static bool_t
clnt_raw_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct clnt_raw_private *clp = clnt_raw_private;
	register XDR *xdrs = &clp->xdr_stream;
	bool_t dummy;

	trace2(TR_clnt_raw_freeres, 0, cl);
	if (clp == NULL) {
		trace2(TR_clnt_raw_freeres, 1, cl);
		return (FALSE);
	}
	xdrs->x_op = XDR_FREE;
	dummy  = (*xdr_res)(xdrs, res_ptr);
	trace2(TR_clnt_raw_freeres, 1, cl);
	return (dummy);
}

static void
clnt_raw_abort(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	trace1(TR_clnt_raw_abort, 0);
	trace1(TR_clnt_raw_abort, 1);
}

static bool_t
clnt_raw_control(cl, request, info)
	CLIENT *cl;
	int request;
	char *info;
{
	trace1(TR_clnt_raw_control, 0);
	trace1(TR_clnt_raw_control, 1);
	return (FALSE);
}

static void
clnt_raw_destroy(cl)
	CLIENT *cl;
{
	trace1(TR_clnt_raw_destroy, 0);
	trace1(TR_clnt_raw_destroy, 1);
}

static struct clnt_ops *
clnt_raw_ops()
{
	static struct clnt_ops ops;

	trace1(TR_clnt_raw_ops, 0);
	if (ops.cl_control == NULL) {
		ops.cl_call = clnt_raw_call;
		ops.cl_abort = clnt_raw_abort;
		ops.cl_geterr = clnt_raw_geterr;
		ops.cl_freeres = clnt_raw_freeres;
		ops.cl_destroy = clnt_raw_destroy;
		ops.cl_control = clnt_raw_control;
	}
	trace1(TR_clnt_raw_ops, 1);
	return (&ops);
}
