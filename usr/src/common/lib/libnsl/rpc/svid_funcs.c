/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svid_funcs.c	1.2.2.1"
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
 * svid_funcs.c
 *
 *	These functions are documented in the SVID as being part of libnsl.
 *	They are also defined as macros in various RPC header files.  To
 *	ensure that these interfaces exist as functions, we've created this
 *	(we hope unused) file.
 */

#include <rpc/rpc.h>
#include <sys/types.h>
#include "trace.h"

/* LINTLIBRARY */

#undef	auth_destroy
#undef	clnt_call
#undef	clnt_control
#undef	clnt_destroy
#undef	clnt_freeres
#undef	clnt_geterr
#undef	svc_destroy
#undef	svc_freeargs
#undef	svc_getargs
#undef	svc_getrpccaller
#undef	xdr_destroy
#undef	xdr_getpos
#undef	xdr_inline
#undef	xdr_setpos

void
auth_destroy(auth)
	AUTH	*auth;
{
	trace1(TR_auth_destroy, 0);
	((*((auth)->ah_ops->ah_destroy))(auth));
	trace1(TR_auth_destroy, 1);
}

enum clnt_stat
clnt_call(cl, proc, xargs, argsp, xres, resp, timeout)
	CLIENT		*cl;
	u_long		proc;
	xdrproc_t	xargs;
	caddr_t		argsp;
	xdrproc_t	xres;
	caddr_t		resp;
	struct timeval	timeout;
{
	enum clnt_stat dummy;

	trace2(TR_clnt_call, 0, proc);
	dummy = (*(cl)->cl_ops->cl_call)(cl, proc, xargs, argsp, xres, resp,
		timeout);
	trace2(TR_clnt_call, 1, proc);
	return (dummy);
}

bool_t
clnt_control(cl, rq, in)
	CLIENT	*cl;
	u_int	rq;
	char	*in;
{
	bool_t dummy;

	trace2(TR_clnt_control, 0, rq);
	dummy = (*(cl)->cl_ops->cl_control)(cl, rq, in);
	trace2(TR_clnt_control, 1, rq);
	return (dummy);
}


void
clnt_destroy(cl)
	CLIENT	*cl;
{
	trace2(TR_clnt_destroy, 0, cl);
	((*(cl)->cl_ops->cl_destroy)(cl));
	trace2(TR_clnt_destroy, 1, cl);
}

bool_t
clnt_freeres(cl, xres, resp)
	CLIENT		*cl;
	xdrproc_t	xres;
	caddr_t		resp;
{
	bool_t dummy;

	trace2(TR_clnt_freeres, 0, cl);
	dummy = (*(cl)->cl_ops->cl_freeres)(cl, xres, resp);
	trace2(TR_clnt_freeres, 1, cl);
	return (dummy);
}

void
clnt_geterr(cl, errp)
	CLIENT		*cl;
	struct rpc_err	*errp;
{
	trace2(TR_clnt_geterr, 0, cl);
	(*(cl)->cl_ops->cl_geterr)(cl, errp);
	trace2(TR_clnt_geterr, 1, cl);
}

void
svc_destroy(xprt)
	SVCXPRT	*xprt;
{
	trace1(TR_svc_destroy, 0);
	(*(xprt)->xp_ops->xp_destroy)(xprt);
	trace1(TR_svc_destroy, 1);
}

bool_t
svc_freeargs(xprt, xargs, argsp)
	SVCXPRT		*xprt;
	xdrproc_t	xargs;
	char		*argsp;
{
	bool_t dummy;

	trace1(TR_svc_freeargs, 0);
	dummy = (*(xprt)->xp_ops->xp_freeargs)(xprt, xargs, argsp);
	trace1(TR_svc_freeargs, 1);
	return (dummy);
}

bool_t
svc_getargs(xprt, xargs, argsp)
	SVCXPRT		*xprt;
	xdrproc_t	xargs;
	char		*argsp;
{
	bool_t dummy;

	trace1(TR_svc_getargs, 0);
	dummy = (*(xprt)->xp_ops->xp_getargs)(xprt, xargs, argsp);
	trace1(TR_svc_getargs, 1);
	return (dummy);
}

struct netbuf *
svc_getrpccaller(xprt)
	SVCXPRT	*xprt;
{
	struct netbuf *dummy;

	trace1(TR_svc_getrpccaller, 0);
	dummy = &(xprt)->xp_rtaddr;
	trace1(TR_svc_getrpccaller, 1);
	return (dummy);
}

void
xdr_destroy(xdrs)
	XDR	*xdrs;
{
	trace1(TR_xdr_destroy, 0);
	(*(xdrs)->x_ops->x_destroy)(xdrs);
	trace1(TR_xdr_destroy, 1);
}

u_int
xdr_getpos(xdrs)
	XDR	*xdrs;
{
	u_int dummy;

	trace1(TR_xdr_getpos, 0);
	dummy = (*(xdrs)->x_ops->x_getpostn)(xdrs);
	trace1(TR_xdr_getpos, 1);
	return (dummy);
}

long *
xdr_inline(xdrs, len)
	XDR	*xdrs;
	int	len;
{
	long *dummy;

	trace2(TR_xdr_inline, 0, len);
	dummy = (*(xdrs)->x_ops->x_inline)(xdrs, len);
	trace2(TR_xdr_inline, 1, len);
	return (dummy);
}

bool_t
xdr_setpos(xdrs, pos)
	XDR	*xdrs;
	u_int	pos;
{
	bool_t dummy;

	trace2(TR_xdr_setpos, 0, pos);
	dummy = (*(xdrs)->x_ops->x_setpostn)(xdrs, pos);
	trace2(TR_xdr_setpos, 1, pos);
	return (dummy);
}
