/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/pmap_prot.c	1.2.9.2"
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
 * pmap_prot.c
 *
 * Protocol for the local binder service, or pmap.
 * All the pmap xdr routines here.
 *
 */

#include <rpc/types.h>
#include "trace.h"
#include <rpc/xdr.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_rmt.h>

bool_t
xdr_pmap(xdrs, objp)
	XDR *xdrs;
	struct pmap *objp;
{

	long *buf;
	bool_t dummy;

	trace1(TR_xdr_pmap, 0);
	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE(xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_u_long(xdrs, &objp->pm_prog)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_vers)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_prot)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_port)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
		} else {
			IXDR_PUT_U_LONG(buf, objp->pm_prog);
			IXDR_PUT_U_LONG(buf, objp->pm_vers);
			IXDR_PUT_U_LONG(buf, objp->pm_prot);
			IXDR_PUT_U_LONG(buf, objp->pm_port);
		}

		trace1(TR_xdr_pmap, 1);
		return (TRUE);
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE(xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_u_long(xdrs, &objp->pm_prog)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_vers)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_prot)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &objp->pm_port)) {
				trace1(TR_xdr_pmap, 1);
				return (FALSE);
			}

		} else {
			objp->pm_prog = IXDR_GET_U_LONG(buf);
			objp->pm_vers = IXDR_GET_U_LONG(buf);
			objp->pm_prot = IXDR_GET_U_LONG(buf);
			objp->pm_port = IXDR_GET_U_LONG(buf);
		}
		trace1(TR_xdr_pmap, 1);
		return (TRUE);
	}

	if (xdr_u_long(xdrs, &objp->pm_prog) &&
	    xdr_u_long(xdrs, &objp->pm_vers) &&
	    xdr_u_long(xdrs, &objp->pm_prot)) {
		dummy = xdr_u_long(xdrs, &objp->pm_port);
		trace1(TR_xdr_pmap, 1);
		return (dummy);
	}
	trace1(TR_xdr_pmap, 1);
	return (FALSE);
}

/*
 * pmaplist_ptr implements a linked list.  The RPCL definition from
 * pmap_prot.x is:
 *
 * struct pm__list {
 * 	pmap		pml_map;
 *	struct pm__list *pml_next;
 * };
 * typedef pm__list *pmaplist_ptr;
 *
 * Recall that "pointers" in XDR are encoded as a boolean, indicating whether
 * there's any data behind the pointer, followed by the data (if any exists).
 * The boolean can be interpreted as ``more data follows me''; if FALSE then
 * nothing follows the boolean; if TRUE then the boolean is followed by an
 * actual struct pmap, and another pmaplist_ptr (declared in RPCL as "struct
 * pmaplist *").
 *
 * This could be implemented via the xdr_pointer type, though this would
 * result in one recursive call per element in the list.  Rather than do that
 * we can ``unwind'' the recursion into a while loop and use xdr_reference to
 * serialize the pmap elements.
 */
bool_t
xdr_pmaplist_ptr(xdrs, rp)
	XDR *xdrs;
	pmaplist_ptr *rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	int freeing = (xdrs->x_op == XDR_FREE);
	pmaplist_ptr next;

	trace1(TR_xdr_pmaplist_ptr, 0);
	while (TRUE) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements)) {
			trace1(TR_xdr_pmaplist_ptr, 1);
			return (FALSE);
		}
		if (! more_elements) {
			trace1(TR_xdr_pmaplist_ptr, 1);
			return (TRUE);  /* we are done */
		}
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the next object
		 * before we free the current object ...
		 */
		if (freeing)
			next = (*rp)->pml_next;
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof (struct pmaplist), (xdrproc_t) xdr_pmap)) {
			trace1(TR_xdr_pmaplist_ptr, 1);
			return (FALSE);
		}
		rp = (freeing) ? &next : &((*rp)->pml_next);
	}
}

/*
 * xdr_pmaplist() is specified to take a PMAPLIST **, but is identical in
 * functionality to xdr_pmaplist_ptr().
 */
bool_t
xdr_pmaplist(xdrs, rp)
	XDR *xdrs;
	PMAPLIST **rp;
{
	bool_t	dummy;

	dummy = xdr_pmaplist_ptr(xdrs, (pmaplist_ptr *)rp);
	return (dummy);
}


/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rmtcallargs(xdrs, cap)
	XDR *xdrs;
	struct p_rmtcallargs *cap;
{
	u_int lenposition, argposition, position;
	long *buf;


	trace1(TR_xdr_rmtcallargs, 0);
	buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT);
	if (buf == NULL) {
		if (!xdr_u_long(xdrs, &(cap->prog)) ||
		    !xdr_u_long(xdrs, &(cap->vers)) ||
		    !xdr_u_long(xdrs, &(cap->proc))) {
			trace1(TR_xdr_rmtcallargs, 1);
			return (FALSE);
		}
	} else {
		IXDR_PUT_U_LONG(buf, cap->prog);
		IXDR_PUT_U_LONG(buf, cap->vers);
		IXDR_PUT_U_LONG(buf, cap->proc);
	}

	/*
	 * All the jugglery for just getting the size of the arguments
	 */
	lenposition = XDR_GETPOS(xdrs);
	if (! xdr_u_int(xdrs, &(cap->args.args_len)))  {
		trace1(TR_xdr_rmtcallargs, 1);
		return (FALSE);
	}
	argposition = XDR_GETPOS(xdrs);
	if (! (*cap->xdr_args)(xdrs, cap->args.args_val)) {
		trace1(TR_xdr_rmtcallargs, 1);
		return (FALSE);
	}
	position = XDR_GETPOS(xdrs);
	cap->args.args_len = position - argposition;
	XDR_SETPOS(xdrs, lenposition);
	if (! xdr_u_int(xdrs, &(cap->args.args_len))) {
		trace1(TR_xdr_rmtcallargs, 1);
		return (FALSE);
	}
	XDR_SETPOS(xdrs, position);
	trace1(TR_xdr_rmtcallargs, 1);
	return (TRUE);


}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rmtcallres(xdrs, crp)
	XDR *xdrs;
	struct p_rmtcallres *crp;
{
	bool_t	dummy;

	trace1(TR_xdr_rmtcallres, 0);
	if (xdr_u_long(xdrs, &crp->port) &&
			xdr_u_int(xdrs, &crp->res.res_len)) {

		dummy = (*(crp->xdr_res))(xdrs, crp->res.res_val);
		trace1(TR_xdr_rmtcallres, 1);
		return (dummy);
	}
	trace1(TR_xdr_rmtcallres, 1);
	return (FALSE);
}
