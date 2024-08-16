/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpcb_prot.c	1.4.7.2"
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
 * rpcb_prot.c
 *
 * XDR routines for the rpcbinder version 3.
 */

#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/rpcb_prot.h>


bool_t
xdr_rpcb(xdrs, objp)
	XDR *xdrs;
	RPCB *objp;
{
	trace1(TR_xdr_rpcb, 0);
	if (!xdr_u_long(xdrs, &objp->r_prog)) {
		trace1(TR_xdr_rpcb, 1);
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->r_vers)) {
		trace1(TR_xdr_rpcb, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_netid, ~0)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_addr, ~0)) {
		trace1(TR_xdr_rpcb, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_owner, ~0)) {
		trace1(TR_xdr_rpcb, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcb, 1);
	return (TRUE);
}

/*
 * rpcblist_ptr implements a linked list.  The RPCL definition from
 * rpcb_prot.x is:
 *
 * struct rpcblist {
 * 	rpcb		rpcb_map;
 *	struct rpcblist *rpcb_next;
 * };
 * typedef rpcblist *rpcblist_ptr;
 *
 * Recall that "pointers" in XDR are encoded as a boolean, indicating whether
 * there's any data behind the pointer, followed by the data (if any exists).
 * The boolean can be interpreted as ``more data follows me''; if FALSE then
 * nothing follows the boolean; if TRUE then the boolean is followed by an
 * actual struct rpcb, and another rpcblist_ptr (declared in RPCL as "struct
 * rpcblist *").
 *
 * This could be implemented via the xdr_pointer type, though this would
 * result in one recursive call per element in the list.  Rather than do that
 * we can ``unwind'' the recursion into a while loop and use xdr_reference to
 * serialize the rpcb elements.
 */

bool_t
xdr_rpcblist_ptr(xdrs, rp)
	XDR *xdrs;
	rpcblist_ptr *rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	rpcblist_ptr next;

	trace1(TR_xdr_rpcblist_ptr, 0);
	while (TRUE) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements)) {
			trace1(TR_xdr_rpcblist_ptr, 1);
			return (FALSE);
		}
		if (! more_elements) {
			trace1(TR_xdr_rpcblist_ptr, 1);
			return (TRUE);  /* we are done */
		}
		if (freeing) {
			next = (*rp)->rpcb_next;
		}
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof (rpcblist), (xdrproc_t)xdr_rpcb)) {
			trace1(TR_xdr_rpcblist_ptr, 1);
			return (FALSE);
		}
		rp = (freeing) ? &next : &((*rp)->rpcb_next);
	}
}

/*
 * xdr_rpcblist() is specified to take a RPCBLIST **, but is identical in
 * functionality to xdr_rpcblist_ptr().
 */
bool_t
xdr_rpcblist(xdrs, rp)
	XDR *xdrs;
	RPCBLIST **rp;
{
	bool_t	dummy;

	dummy = xdr_rpcblist_ptr(xdrs, (rpcblist_ptr *)rp);
	return (dummy);
}


bool_t
xdr_rpcb_entry(xdrs, objp)
	XDR *xdrs;
	rpcb_entry *objp;
{
	trace1(TR_xdr_rpcb_entry, 0);
	if (!xdr_string(xdrs, &objp->r_maddr, ~0)) {
		trace1(TR_xdr_rpcb_entry, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_netid, ~0)) {
		trace1(TR_xdr_rpcb_entry, 1);
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->r_nc_semantics)) {
		trace1(TR_xdr_rpcb_entry, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_protofmly, ~0)) {
		trace1(TR_xdr_rpcb_entry, 1);
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_nc_proto, ~0)) {
		trace1(TR_xdr_rpcb_entry, 1);
		return (FALSE);
	}
	trace1(TR_xdr_rpcb_entry, 1);
	return (TRUE);
}

bool_t
xdr_rpcb_entry_list_ptr(xdrs, rp)
	XDR *xdrs;
	rpcb_entry_list_ptr *rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	rpcb_entry_list_ptr next;

	trace1(TR_xdr_rpcb_entry_list_ptr, 0);
	while (TRUE) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements)) {
			trace1(TR_xdr_rpcb_entry_list, 1);
			return (FALSE);
		}
		if (! more_elements) {
			trace1(TR_xdr_rpcb_entry_list, 1);
			return (TRUE);  /* we are done */
		}
		if (freeing)
			next = (*rp)->rpcb_entry_next;
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof (rpcb_entry_list),
				    (xdrproc_t)xdr_rpcb_entry)) {
			trace1(TR_xdr_rpcb_entry_list, 1);
			return (FALSE);
		}
		rp = (freeing) ? &next : &((*rp)->rpcb_entry_next);
	}
}

/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rpcb_rmtcallargs(xdrs, objp)
	XDR *xdrs;
	struct r_rpcb_rmtcallargs *objp;
{
	u_int lenposition, argposition, position;
	long *buf;

	trace1(TR_xdr_rpcb_rmtcallargs, 0);
	buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT);
	if (buf == NULL) {
		if (!xdr_u_long(xdrs, &objp->prog)) {
			trace1(TR_xdr_rpcb_rmtcallargs, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->vers)) {
			trace1(TR_xdr_rpcb_rmtcallargs, 1);
			return (FALSE);
		}
		if (!xdr_u_long(xdrs, &objp->proc)) {
			trace1(TR_xdr_rpcb_rmtcallargs, 1);
			return (FALSE);
		}
	} else {
		IXDR_PUT_U_LONG(buf, objp->prog);
		IXDR_PUT_U_LONG(buf, objp->vers);
		IXDR_PUT_U_LONG(buf, objp->proc);
	}

	/*
	 * All the jugglery for just getting the size of the arguments
	 */
	lenposition = XDR_GETPOS(xdrs);
	if (! xdr_u_int(xdrs, &(objp->args.args_len))) {
		trace1(TR_xdr_rpcb_rmtcallargs, 1);
		return (FALSE);
	}
	argposition = XDR_GETPOS(xdrs);
	if (! (*objp->xdr_args)(xdrs, objp->args.args_val)) {
		trace1(TR_xdr_rpcb_rmtcallargs, 1);
		return (FALSE);
	}
	position = XDR_GETPOS(xdrs);
	objp->args.args_len = (u_long)position - (u_long)argposition;
	XDR_SETPOS(xdrs, lenposition);
	if (! xdr_u_int(xdrs, &(objp->args.args_len))) {
		trace1(TR_xdr_rpcb_rmtcallargs, 1);
		return (FALSE);
	}
	XDR_SETPOS(xdrs, position);
	trace1(TR_xdr_rpcb_rmtcallargs, 1);
	return (TRUE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rpcb_rmtcallres(xdrs, objp)
	XDR *xdrs;
	struct r_rpcb_rmtcallres *objp;
{
	bool_t dummy;

	trace1(TR_xdr_rpcb_rmtcallres, 0);
	if (!xdr_string(xdrs, &objp->addr, ~0)) {
		trace1(TR_xdr_rpcb_rmtcallres, 1);
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->results.results_len)) {
		trace1(TR_xdr_rpcb_rmtcallres, 1);
		return (FALSE);
	}
	dummy = (*(objp->xdr_res))(xdrs, objp->results.results_val);
	trace1(TR_xdr_rpcb_rmtcallres, 1);
	return (dummy);
}

bool_t
xdr_netbuf(xdrs, objp)
	XDR *xdrs;
	struct netbuf *objp;
{
	bool_t dummy;

	trace1(TR_xdr_netbuf, 0);
	if (!xdr_u_long(xdrs, (u_long *) &objp->maxlen)) {
		trace1(TR_xdr_netbuf, 1);
		return (FALSE);
	}
	dummy = xdr_bytes(xdrs, (char **)&(objp->buf),
			(u_int *)&(objp->len), objp->maxlen);
	trace1(TR_xdr_netbuf, 1);
	return (dummy);
}
