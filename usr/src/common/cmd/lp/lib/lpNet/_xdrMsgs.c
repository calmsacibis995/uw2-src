/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lpNet/_xdrMsgs.c	1.2.5.3"
#ident	"$Header: $"
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include "_xdrMsgs.h"

bool_t
xdr_physicalMsgTag(xdrs, objp)
	XDR *xdrs;
	physicalMsgTag *objp;
{
	if (!xdr_long(xdrs, &objp->physicalMsgSize)) {
		return (FALSE);
	}
	return (TRUE);
}