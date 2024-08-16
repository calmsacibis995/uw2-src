/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/authsys_prot.c	1.3.9.2"
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
 * authsys_prot.c
 * XDR for UNIX style authentication parameters for RPC
 *
 */

#include <rpc/types.h>
#include "trace.h"
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_sys.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  256
#endif

/*
 * XDR for unix authentication parameters.
 */
bool_t
xdr_authsys_parms(xdrs, p)
	register XDR *xdrs;
	register struct authsys_parms *p;
{
	trace1(TR_xdr_authsys_parms, 0);
	if (xdr_u_long(xdrs, &(p->aup_time)) &&
	    xdr_string(xdrs, &(p->aup_machname), MAXHOSTNAMELEN) &&
	    xdr_uid_t(xdrs, (uid_t *)&(p->aup_uid)) &&
	    xdr_gid_t(xdrs, (gid_t *)&(p->aup_gid)) &&
	    xdr_array(xdrs, (caddr_t *)&(p->aup_gids),
			&(p->aup_len), NGRPS, sizeof (gid_t),
			(xdrproc_t) xdr_gid_t)) {
		trace1(TR_xdr_authsys_parms, 1);
		return (TRUE);
	}
	trace1(TR_xdr_authsys_parms, 1);
	return (FALSE);
}

/*
 * XDR user id types (uid_t)
 */
bool_t
xdr_uid_t(xdrs, ip)
	XDR *xdrs;
	uid_t *ip;
{
	bool_t dummy;

	trace1(TR_xdr_uid_t, 0);
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	dummy = xdr_long(xdrs, (long *)ip);
	trace1(TR_xdr_uid_t, 1);
	return (dummy);
#else
	if (sizeof (uid_t) == sizeof (long)) {
		dummy = xdr_long(xdrs, (long *)ip);
		trace1(TR_xdr_uid_t, 1);
		return (dummy);
	} else {
		dummy = xdr_short(xdrs, (short *)ip);
		trace1(TR_xdr_uid_t, 1);
		return (dummy);
	}
#endif
}

/*
 * XDR group id types (gid_t)
 */
bool_t
xdr_gid_t(xdrs, ip)
	XDR *xdrs;
	gid_t *ip;
{
	bool_t dummy;

	trace1(TR_xdr_gid_t, 0);
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	dummy = xdr_long(xdrs, (long *)ip);
	trace1(TR_xdr_gid_t, 1);
	return (dummy);
#else
	if (sizeof (gid_t) == sizeof (long)) {
		dummy = xdr_long(xdrs, (long *)ip);
		trace1(TR_xdr_gid_t, 1);
		return (dummy);
	} else {
		dummy = xdr_short(xdrs, (short *)ip);
		trace1(TR_xdr_gid_t, 1);
		return (dummy);
	}
#endif
}
