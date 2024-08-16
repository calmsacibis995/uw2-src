/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/lockmgr/klm_kprot.c	1.4"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	klm_prot.c, xdr routines for kernel lock manager
 */

#include <net/rpc/rpc.h>
#include <net/lockmgr/klm_prot.h>

/*
 * xdr_klm_stats(XDR *xdrs, klm_stats *objp)
 *	Xdr klm_stats struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_stats struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_stats(XDR *xdrs, klm_stats *objp)
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_lock(XDR *xdrs, klm_lock *objp)
 *	Xdr klm_lock struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_lock struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_lock(XDR *xdrs, klm_lock *objp)
{
	if (!xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->fh)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->base)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->type)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->granted)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->color)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->LockID)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->pid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->class)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rsys)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rpid)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_holder(XDR *xdrs, klm_holder *objp)
 *	Xdr klm_holder struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_holder struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_holder(XDR *xdrs, klm_holder *objp)
{
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->base)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->type)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->granted)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->color)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->LockID)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->pid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->class)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rsys)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rpid)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_stat(XDR *xdrs, klm_stat *objp)
 *	Xdr klm_stat struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_stat struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_stat(XDR *xdrs, klm_stat *objp)
{
	if (!xdr_klm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_testrply(XDR *xdrs, klm_testrply *objp)
 *	Xdr klm_testrply struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_testrply struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_testrply(XDR *xdrs, klm_testrply *objp)
{
	if (!xdr_klm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	switch (objp->stat) {
	case klm_denied:
		if (!xdr_klm_holder(xdrs, &objp->klm_testrply_u.holder)) {
			return (FALSE);
		}
		break;
	}
	return (TRUE);
}

/*
 * xdr_klm_lockargs(XDR *xdrs, klm_lockargs *objp)
 *	Xdr klm_lockargs struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_lockargs struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_lockargs(XDR *xdrs, klm_lockargs *objp)
{
	if (!xdr_bool(xdrs, &objp->block)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_testargs(XDR *xdrs, klm_testargs *objp)
 *	Xdr klm_testargs struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *	Xdr klm_testargs struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_testargs(XDR *xdrs, klm_testargs *objp)
{
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_klm_unlockargs(XDR *xdrs, klm_unlockargs *objp)
 *	Xdr klm_unlockargs struct.
 *
 * Calling/Exit State:
 *	No lock held on entry or exit.
 *
 * Description:
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize to/from
 *	objp		# struct to serialize/deserialize
 *
 */
bool_t
xdr_klm_unlockargs(XDR *xdrs, klm_unlockargs *objp)
{
	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}
