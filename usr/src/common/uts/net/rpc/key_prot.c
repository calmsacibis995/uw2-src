/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/key_prot.c	1.8"
#ident 	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* 
 *	key_prot.c, xdr routines for protocol with keyserver.
 */

#include <util/types.h>
#include <net/rpc/rpc.h>
#include <net/rpc/key_prot.h>

/*
 * xdr_keystatus(XDR *xdrs, keystatus *objp)
 *	XDR keystatus.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs the keystatus.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# keystatus
 */
bool_t
xdr_keystatus(XDR *xdrs, keystatus *objp)
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_netnamestr(XDR *xdrs, netnamestr *objp)
 *	XDR a netname string.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a netname string.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# netnamestr
 */
bool_t
xdr_netnamestr(XDR *xdrs, netnamestr *objp)
{
	if (!xdr_string(xdrs, objp, MAXNETNAMELEN)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_cryptkeyarg(XDR *xdrs, cryptkeyarg *objp)
 *	XDR a cryptkeyarg.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a cryptkeyarg.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# cryptkeyarg
 */
bool_t
xdr_cryptkeyarg(XDR *xdrs, cryptkeyarg *objp)
{
	if (!xdr_netnamestr(xdrs, &objp->remotename)) {
		return (FALSE);
	}
	if (!xdr_des_block(xdrs, &objp->deskey)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_cryptkeyres(XDR *xdrs, cryptkeyres *objp)
 *	XDR a cryptkeyres.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a cryptkeyres.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# cryptkeyres
 */
bool_t
xdr_cryptkeyres(XDR *xdrs, cryptkeyres *objp)
{
	if (!xdr_keystatus(xdrs, &objp->status)) {
		return (FALSE);
	}
	switch (objp->status) {
	case KEY_SUCCESS:
		if (!xdr_des_block(xdrs, &objp->cryptkeyres_u.deskey)) {
			return (FALSE);
		}
		break;
	}
	return (TRUE);
}

/*
 * xdr_unixcred(XDR *xdrs, unixcred *objp)
 *	XDR a unixcred.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a unixcred.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# unixcred
 */
bool_t
xdr_unixcred(XDR *xdrs, unixcred *objp)
{
	if (!xdr_int(xdrs, (int *)&objp->uid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, (int *)&objp->gid)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->gids.gids_val,
		(u_int *)&objp->gids.gids_len, NGRPS, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * xdr_getcredres(XDR *xdrs, getcredres *objp)
 *	XDR a getcredres.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a getcredres.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to encode/decode
 *	objp			# getcredres
 */
bool_t
xdr_getcredres(XDR *xdrs, getcredres *objp)
{
	if (!xdr_keystatus(xdrs, &objp->status)) {
		return (FALSE);
	}
	switch (objp->status) {
	case KEY_SUCCESS:
		if (!xdr_unixcred(xdrs, &objp->getcredres_u.cred)) {
			return (FALSE);
		}
		break;
	}
	return (TRUE);
}
