/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/auth_none.c	1.2.9.1"
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
 * auth_none.c
 * Creates a client authentication handle for passing "null"
 * credentials and verifiers to remote systems.
 */

#include <stdlib.h>
#include <rpc/types.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include "rpc_mt.h"
#define	MAX_MARSHEL_SIZE 20

static struct auth_ops *authnone_ops();

/*
 * authnone_private:
 * __rpc_lock is held during the initialization of authnone_private
 * in authnone_create().
 */

static struct authnone_private {
	AUTH	no_client;
	char	marshalled_client[MAX_MARSHEL_SIZE];
	u_int	mcnt;
} *authnone_private;

AUTH *
authnone_create()
{
	register struct authnone_private *ap;
	XDR xdr_stream;
	register XDR *xdrs;

	trace1(TR_authnone_create, 0);
	if (authnone_private == NULL) {
		if ((ap = (struct authnone_private *)
			calloc(1, sizeof (*ap))) == NULL) {
			trace1(TR_authnone_create, 1);
			return ((AUTH *)NULL);
		}
		ap->no_client.ah_cred = ap->no_client.ah_verf = _null_auth;
		ap->no_client.ah_ops = authnone_ops();
		xdrs = &xdr_stream;
		xdrmem_create(xdrs, ap->marshalled_client,
			(u_int)MAX_MARSHEL_SIZE, XDR_ENCODE);
		(void) xdr_opaque_auth(xdrs, &ap->no_client.ah_cred);
		(void) xdr_opaque_auth(xdrs, &ap->no_client.ah_verf);
		ap->mcnt = XDR_GETPOS(xdrs);
		XDR_DESTROY(xdrs);

		MUTEX_LOCK(&__rpc_lock);
		if (authnone_private == NULL)
			authnone_private = ap;
		else
			free(ap);
		MUTEX_UNLOCK(&__rpc_lock);
	}
	trace1(TR_authnone_create, 1);
	return (&authnone_private->no_client);
}

/*ARGSUSED*/
static bool_t
authnone_marshal(client, xdrs)
	AUTH *client;
	XDR *xdrs;
{
	register struct authnone_private *ap;
	bool_t dummy;

	trace1(TR_authnone_marshal, 0);
	ap = authnone_private;
	if (ap == NULL) {
		trace1(TR_authnone_marshal, 1);
		return (FALSE);
	}
	dummy = (*xdrs->x_ops->x_putbytes)(xdrs,
			ap->marshalled_client, ap->mcnt);
	trace1(TR_authnone_marshal, 1);
	return (dummy);
}

/* All these unused parameters are required to keep ANSI-C from grumbling */
static void
authnone_verf(client)
AUTH *client;
{
	trace1(TR_authnone_verf, 0);
	trace1(TR_authnone_verf, 1);
}

static bool_t
authnone_validate(client, opaque)
AUTH *client;
struct opaque_auth *opaque;
{
	trace1(TR_authnone_validate, 0);
	trace1(TR_authnone_validate, 1);
	return (TRUE);
}

static bool_t
authnone_refresh(client)
AUTH *client;
{
	trace1(TR_authnone_refresh, 0);
	trace1(TR_authnone_refresh, 1);
	return (FALSE);
}

static void
authnone_destroy(client)
AUTH *client;
{
	trace1(TR_authnone_destroy, 0);
	trace1(TR_authnone_destroy, 1);
}

static struct auth_ops *
authnone_ops()
{
	static struct auth_ops ops;

	trace1(TR_authnone_ops, 0);
	if (ops.ah_destroy == NULL) {
		ops.ah_nextverf = authnone_verf;
		ops.ah_marshal = authnone_marshal;
		ops.ah_validate = authnone_validate;
		ops.ah_refresh = authnone_refresh;
		ops.ah_destroy = authnone_destroy;
	}
	trace1(TR_authnone_ops, 1);
	return (&ops);
}
