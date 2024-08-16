/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_raw.c	1.4.9.1"
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
 * svc_raw.c,   This a toy for simple testing and timing.
 * Interface to create an rpc client and server in the same UNIX process.
 * This lets us similate rpc and get rpc (round trip) overhead, without
 * any interference from the kernal.
 *
 */

#include <rpc/rpc.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/raw.h>
#ifndef UDPMSGSIZE
#define	UDPMSGSIZE 8800
#endif

char	*_rawcombuf;

/*
 * This is the "network" that we will be moving data over
 */
static struct svc_raw_private {
	char	*raw_buf;	/* should be shared with the cl handle */
	SVCXPRT	server;
	XDR	xdr_stream;
	char	verf_body[MAX_AUTH_BYTES];
} *svc_raw_private;

static struct xp_ops *svc_raw_ops();
extern char *calloc();

SVCXPRT *
svc_raw_create()
{
	register struct svc_raw_private *srp = svc_raw_private;

	trace1(TR_svc_raw_create, 0);
	if (srp == NULL) {
		srp = (struct svc_raw_private *)calloc(1, sizeof (*srp));
		if (srp == NULL) {
			trace1(TR_svc_raw_create, 1);
			return ((SVCXPRT *)NULL);
		}
		if (_rawcombuf == NULL)
			_rawcombuf = (char *)calloc(UDPMSGSIZE, sizeof (char));
		srp->raw_buf = _rawcombuf; /* Share it with the client */
		svc_raw_private = srp;
	}
	srp->server.xp_fd = FD_SETSIZE;
	srp->server.xp_port = 0;
	srp->server.xp_p3 = NULL;
	srp->server.xp_ops = svc_raw_ops();
	srp->server.xp_verf.oa_base = srp->verf_body;
	xdrmem_create(&srp->xdr_stream, srp->raw_buf, UDPMSGSIZE, XDR_DECODE);
	xprt_register(&srp->server);
	trace1(TR_svc_raw_create, 1);
	return (&srp->server);
}

static enum xprt_stat
svc_raw_stat(xprt)
SVCXPRT *xprt; /* args needed to satisfy ANSI-C typechecking */
{
	trace1(TR_svc_raw_stat, 0);
	trace1(TR_svc_raw_stat, 1);
	return (XPRT_IDLE);
}

static bool_t
svc_raw_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svc_raw_private *srp = svc_raw_private;
	register XDR *xdrs;

	trace1(TR_svc_raw_recv, 0);
	if (srp == NULL) {
		trace1(TR_svc_raw_recv, 1);
		return (FALSE);
	}
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_DECODE;
	(void) XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg)) {
		trace1(TR_svc_raw_recv, 1);
		return (FALSE);
	}
	trace1(TR_svc_raw_recv, 1);
	return (TRUE);
}

static bool_t
svc_raw_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svc_raw_private *srp = svc_raw_private;
	register XDR *xdrs;

	trace1(TR_svc_raw_reply, 0);
	if (srp == NULL) {
		trace1(TR_svc_raw_reply, 1);
		return (FALSE);
	}
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_ENCODE;
	(void) XDR_SETPOS(xdrs, 0);
	if (! xdr_replymsg(xdrs, msg)) {
		trace1(TR_svc_raw_reply, 1);
		return (FALSE);
	}
	(void) XDR_GETPOS(xdrs);  /* called just for overhead */
	trace1(TR_svc_raw_reply, 1);
	return (TRUE);
}

static bool_t
svc_raw_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register struct svc_raw_private *srp = svc_raw_private;
	bool_t dummy1;

	trace1(TR_svc_raw_getargs, 0);
	if (srp == NULL) {
		trace1(TR_svc_raw_getargs, 1);
		return (FALSE);
	}
	dummy1 = (*xdr_args)(&srp->xdr_stream, args_ptr);
	trace1(TR_svc_raw_getargs, 1);
	return (dummy1);
}

static bool_t
svc_raw_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register struct svc_raw_private *srp = svc_raw_private;
	register XDR *xdrs;
	bool_t dummy2;

	trace1(TR_svc_raw_freeargs, 0);
	if (srp == NULL) {
		trace1(TR_svc_raw_freeargs, 1);
		return (FALSE);
	}
	xdrs = &srp->xdr_stream;
	xdrs->x_op = XDR_FREE;
	dummy2 = (*xdr_args)(xdrs, args_ptr);
	trace1(TR_svc_raw_freeargs, 1);
	return (dummy2);
}

static void
svc_raw_destroy(xprt)
SVCXPRT *xprt;
{
	trace1(TR_svc_raw_destroy, 0);
	trace1(TR_svc_raw_destroy, 1);
}

static struct xp_ops *
svc_raw_ops()
{
	static struct xp_ops ops;

	trace1(TR_svc_raw_ops, 0);
	if (ops.xp_destroy == NULL) {
		ops.xp_recv = svc_raw_recv;
		ops.xp_stat = svc_raw_stat;
		ops.xp_getargs = svc_raw_getargs;
		ops.xp_reply = svc_raw_reply;
		ops.xp_freeargs = svc_raw_freeargs;
		ops.xp_destroy = svc_raw_destroy;
	}
	trace1(TR_svc_raw_ops, 1);
	return (&ops);
}
