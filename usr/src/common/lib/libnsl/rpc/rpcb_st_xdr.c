/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpcb_st_xdr.c	1.2.2.1"
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
 * rpcb_st_xdr.c
 *
 * This file was generated from rpcb_prot.x, but includes only those
 * routines used with the rpcbind stats facility.
 */

#include <rpc/rpc.h>
#include "trace.h"

/* Link list of all the stats about getport and getaddr */

bool_t
xdr_rpcbs_addrlist(xdrs, objp)
	XDR *xdrs;
	rpcbs_addrlist *objp;
{
	trace1(TR_xdr_rpcbs_addrlist, 0);

	    if (!xdr_u_long(xdrs, &objp->prog)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }
	    if (!xdr_u_long(xdrs, &objp->vers)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }
	    if (!xdr_int(xdrs, &objp->success)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }
	    if (!xdr_int(xdrs, &objp->failure)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }
	    if (!xdr_string(xdrs, &objp->netid, ~0)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }

	    if (!xdr_pointer(xdrs, (char **)&objp->next,
			sizeof (rpcbs_addrlist),
			(xdrproc_t)xdr_rpcbs_addrlist)) {
		trace1(TR_xdr_rpcbs_addrlist, 1);
		return (FALSE);
	    }

	return (TRUE);
}

/* Link list of all the stats about rmtcall */

bool_t
xdr_rpcbs_rmtcalllist(xdrs, objp)
	XDR *xdrs;
	rpcbs_rmtcalllist *objp;
{
	register long *buf;

	trace1(TR_xdr_rpcbs_rmtcalllist, 0);
	if (xdrs->x_op == XDR_ENCODE) {
	buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT);
	if (buf == NULL) {
		if (!xdr_u_long(xdrs, &objp->prog)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->vers)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->proc)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->success)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->failure)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->indirect)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
	} else {
		IXDR_PUT_U_LONG(buf, objp->prog);
		IXDR_PUT_U_LONG(buf, objp->vers);
		IXDR_PUT_U_LONG(buf, objp->proc);
		IXDR_PUT_LONG(buf, objp->success);
		IXDR_PUT_LONG(buf, objp->failure);
		IXDR_PUT_LONG(buf, objp->indirect);
	}
	if (!xdr_string(xdrs, &objp->netid, ~0)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next,
			sizeof (rpcbs_rmtcalllist),
			(xdrproc_t)xdr_rpcbs_rmtcalllist)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_rmtcalllist, 1);
	return (TRUE);
	} else if (xdrs->x_op == XDR_DECODE) {
	buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT);
	if (buf == NULL) {
		if (!xdr_u_long(xdrs, &objp->prog)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->vers)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->proc)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->success)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->failure)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
		if (!xdr_int(xdrs, &objp->indirect)) {
			trace1(TR_xdr_rpcbs_rmtcalllist, 1);
			return (FALSE);
		}
	} else {
		objp->prog = IXDR_GET_U_LONG(buf);
		objp->vers = IXDR_GET_U_LONG(buf);
		objp->proc = IXDR_GET_U_LONG(buf);
		objp->success = IXDR_GET_LONG(buf);
		objp->failure = IXDR_GET_LONG(buf);
		objp->indirect = IXDR_GET_LONG(buf);
	}
	if (!xdr_string(xdrs, &objp->netid, ~0)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next,
			sizeof (rpcbs_rmtcalllist),
			(xdrproc_t)xdr_rpcbs_rmtcalllist)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_rmtcalllist, 1);
	return (TRUE);
	}
	if (!xdr_u_long(xdrs, &objp->prog)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->vers)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->proc)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->success)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->failure)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->indirect)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->netid, ~0)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next,
			sizeof (rpcbs_rmtcalllist),
			(xdrproc_t)xdr_rpcbs_rmtcalllist)) {
		trace1(TR_xdr_rpcbs_rmtcalllist, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_rmtcalllist, 1);
	return (TRUE);
}

bool_t
xdr_rpcbs_proc(xdrs, objp)
	XDR *xdrs;
	rpcbs_proc objp;
{
	trace1(TR_xdr_rpcbs_proc, 0);
	if (!xdr_vector(xdrs, (char *)objp, RPCBSTAT_HIGHPROC, sizeof (int),
			(xdrproc_t)xdr_int)) {
		trace1(TR_xdr_rpcbs_proc, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_proc, 1);
	return (TRUE);
}

bool_t
xdr_rpcbs_addrlist_ptr(xdrs, objp)
	XDR *xdrs;
	rpcbs_addrlist_ptr *objp;
{
	trace1(TR_xdr_rpcbs_addrlist_ptr, 0);
	if (!xdr_pointer(xdrs, (char **)objp, sizeof (rpcbs_addrlist),
			(xdrproc_t)xdr_rpcbs_addrlist)) {
		trace1(TR_xdr_rpcbs_addrlist_ptr, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_addrlist_ptr, 1);
	return (TRUE);
}

bool_t
xdr_rpcbs_rmtcalllist_ptr(xdrs, objp)
	XDR *xdrs;
	rpcbs_rmtcalllist_ptr *objp;
{
	trace1(TR_xdr_rpcbs_rmtcalllist_ptr, 0);
	if (!xdr_pointer(xdrs, (char **)objp, sizeof (rpcbs_rmtcalllist),
			(xdrproc_t)xdr_rpcbs_rmtcalllist)) {
		trace1(TR_xdr_rpcbs_rmtcalllist_ptr, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcbs_rmtcalllist_ptr, 1);
	return (TRUE);
}

bool_t
xdr_rpcb_stat(xdrs, objp)
	XDR *xdrs;
	rpcb_stat *objp;
{

	trace1(TR_xdr_rpcb_stat, 0);
	if (!xdr_rpcbs_proc(xdrs, objp->info)) {
		trace1(TR_xdr_rpcb_stat, 1);
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->setinfo)) {
		trace1(TR_xdr_rpcb_stat, 1);
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->unsetinfo)) {
		trace1(TR_xdr_rpcb_stat, 1);
		return (FALSE);
	}
	if (!xdr_rpcbs_addrlist_ptr(xdrs, &objp->addrinfo)) {
		trace1(TR_xdr_rpcb_stat, 1);
		return (FALSE);
	}
	if (!xdr_rpcbs_rmtcalllist_ptr(xdrs, &objp->rmtinfo)) {
		trace1(TR_xdr_rpcb_stat, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcb_stat, 1);
	return (TRUE);
}

/*
 * One rpcb_stat structure is returned for each version of rpcbind
 * being monitored.
 */
bool_t
xdr_rpcb_stat_byvers(xdrs, objp)
    XDR *xdrs;
    rpcb_stat_byvers objp;
{
	trace1(TR_xdr_rpcb_stat_byvers, 0);
	if (!xdr_vector(xdrs, (char *)objp, RPCBVERS_STAT, sizeof (rpcb_stat),
	    (xdrproc_t)xdr_rpcb_stat)) {
		trace1(TR_xdr_rpcb_stat_byvers, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcb_stat_byvers, 1);
	return (TRUE);
}
