/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/ypupd.c	1.2.6.1"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include <rpcsvc/ypupd.h>

/*
 * Compiled from ypupdate_prot.x using rpcgen
 * This is NOT source code!
 * DO NOT EDIT THIS FILE!
 */

bool_t
xdr_yp_buf(xdrs, objp)
	XDR *xdrs;
	yp_buf *objp;
{
	if (!xdr_bytes(xdrs, (char **)&objp->yp_buf_val, (u_int *)&objp->yp_buf_len, MAXYPDATALEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_ypupdate_args(xdrs, objp)
	XDR *xdrs;
	ypupdate_args *objp;
{
	if (!xdr_string(xdrs, &objp->mapname, MAXMAPNAMELEN)) {
		return (FALSE);
	}
	if (!xdr_yp_buf(xdrs, &objp->key)) {
		return (FALSE);
	}
	if (!xdr_yp_buf(xdrs, &objp->datum)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_ypdelete_args(xdrs, objp)
	XDR *xdrs;
	ypdelete_args *objp;
{
	if (!xdr_string(xdrs, &objp->mapname, MAXMAPNAMELEN)) {
		return (FALSE);
	}
	if (!xdr_yp_buf(xdrs, &objp->key)) {
		return (FALSE);
	}
	return (TRUE);
}
