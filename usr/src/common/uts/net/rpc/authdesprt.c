/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/authdesprt.c	1.10"
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
 *	authdesprt.c, XDR routines for DES style authentication
 *	parameters for kernel rpc.
 */

#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/auth_des.h>

#define ATTEMPT(xdr_op) if (!(xdr_op)) return (FALSE)

/*
 * xdr_authdes_cred()
 *	XDR routine for des style crenedtials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Serializes the des credentials.
 *
 * Parameters:
 *
 *	xdrs			# stream to serialize into
 *	cred			# pointer to des credentials
 *
 */
bool_t
xdr_authdes_cred(XDR *xdrs, struct authdes_cred *cred)
{
	ATTEMPT(xdr_enum(xdrs, (enum_t *)&cred->adc_namekind));
	switch (cred->adc_namekind) {

		case ADN_FULLNAME:

			ATTEMPT(xdr_string(xdrs, &cred->adc_fullname.name,
							MAXNETNAMELEN));
			ATTEMPT(xdr_opaque(xdrs,
					(caddr_t)&cred->adc_fullname.key,
					sizeof(des_block)));
			ATTEMPT(xdr_opaque(xdrs,
					(caddr_t)&cred->adc_fullname.window,
					sizeof(cred->adc_fullname.window)));
			return (TRUE);

		case ADN_NICKNAME:

			ATTEMPT(xdr_long(xdrs, (long *)&cred->adc_nickname));
			return (TRUE);

	default:

		return (FALSE);
	}
}


/*
 * xdr_authdes_verf()
 *	XDR routine for des style credential verifier.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	ReturnsTRUE on success, FALSE on failure.
 *
 * Description:
 *	Serializes the des credential verifier.
 *
 * Parameters:
 *
 *	xdrs			# stream to serialize into
 *	verf			# pointer to des credential verifier
 *
 */
bool_t
xdr_authdes_verf(XDR *xdrs, struct authdes_verf *verf)
{
	ATTEMPT(xdr_opaque(xdrs,
		(caddr_t)&verf->adv_xtimestamp, sizeof(des_block)));
	ATTEMPT(xdr_opaque(xdrs,
		(caddr_t)&verf->adv_int_u, sizeof(verf->adv_int_u)));

	return (TRUE);
}
