/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/authesvprt.c	1.9"
#ident 	"$Header: $"

/*
 *	authesv_prot.c, XDR routines for ESV style authentication
 *	parameters for kernel rpc.
 */

#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/auth_esv.h>

#ifdef RPCESV

/*
 * xdr_authesv_params(xdrs, p)
 *	XDR routine for secure authentication parameters.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Serializes esv style credentials.
 *
 * Parameters:
 *	
 *	xdrs			# stream to serialize into
 *	p			# pointer to esv parameters
 *
 */
bool_t
xdr_authesv_parms(XDR *xdrs, struct authesv_parms *p)
{
	if (xdr_u_long(xdrs, &(p->auc_stamp))
		&& xdr_string(xdrs, &(p->auc_machname), MAX_ESVMACH_NAME)
		&& xdr_int(xdrs, (int *)&(p->auc_uid))
		&& xdr_int(xdrs, (int *)&(p->auc_gid))
		&& xdr_array(xdrs, (caddr_t *)&(p->auc_gids),
			&(p->auc_len), ESV_NGRPS, sizeof(int), xdr_int)
		&& xdr_u_long(xdrs, &(p->auc_aid))
		&& xdr_u_long(xdrs, (u_long *)&(p->auc_privs))
		&& xdr_u_long(xdrs, (u_long *)&(p->auc_sens))
		&& xdr_u_long(xdrs, (u_long *)&(p->auc_info))
		&& xdr_u_long(xdrs, (u_long *)&(p->auc_integ))
		&& xdr_u_long(xdrs, (u_long *)&(p->auc_ncs)) ) {

		return (TRUE);
	}

	return (FALSE);
}

#endif
