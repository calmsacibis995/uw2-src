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

#ident	"@(#)nfs.cmds:lockd/xdr_sm.c	1.3"
#ident  "$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "sm_inter.h"

bool_t
xdr_sm_name(xdrs, objp)
	XDR *xdrs;
	sm_name *objp;
{

	register long *buf;

	if (!xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_my_id(xdrs, objp)
	XDR *xdrs;
	my_id *objp;
{

	register long *buf;

	if (!xdr_string(xdrs, &objp->my_name, SM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->my_prog)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->my_vers)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->my_proc)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mon_id(xdrs, objp)
	XDR *xdrs;
	mon_id *objp;
{

	register long *buf;

	if (!xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_my_id(xdrs, &objp->my_idno)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mon(xdrs, objp)
	XDR *xdrs;
	mon *objp;
{

	register long *buf;

	int i;
	if (!xdr_mon_id(xdrs, &objp->mon_idno)) {
		return (FALSE);
	}
	if (!xdr_opaque(xdrs, objp->priv, 16)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_sm_stat(xdrs, objp)
	XDR *xdrs;
	sm_stat *objp;
{

	register long *buf;

	if (!xdr_int(xdrs, &objp->state)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_res(xdrs, objp)
	XDR *xdrs;
	res *objp;
{

	register long *buf;

	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_sm_stat_res(xdrs, objp)
	XDR *xdrs;
	sm_stat_res *objp;
{

	register long *buf;

	if (!xdr_res(xdrs, &objp->res_stat)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->state)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_status(xdrs, objp)
	XDR *xdrs;
	status *objp;
{

	register long *buf;

	int i;
	if (!xdr_string(xdrs, &objp->mon_name, SM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->state)) {
		return (FALSE);
	}
	if (!xdr_opaque(xdrs, objp->priv, 16)) {
		return (FALSE);
	}
	return (TRUE);
}
