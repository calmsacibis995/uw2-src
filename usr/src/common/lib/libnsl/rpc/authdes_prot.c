/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/authdes_prot.c	1.1.9.1"
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
 * authdes_prot.c, XDR routines for DES authentication
 *
 */

#include <sys/types.h>
#include <rpc/types.h>
#include "trace.h"
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_des.h>

#define	ATTEMPT(xdr_op) if (!(xdr_op)) return (FALSE)

bool_t
xdr_authdes_cred(xdrs, cred)
	XDR *xdrs;
	struct authdes_cred *cred;
{
	/*
	 * Unrolled xdr
	 */
	trace1(TR_xdr_authdes_cred, 0);
	ATTEMPT(xdr_enum(xdrs, (enum_t *)&cred->adc_namekind));
	switch (cred->adc_namekind) {
	case ADN_FULLNAME:
		ATTEMPT(xdr_string(xdrs, &cred->adc_fullname.name,
				MAXNETNAMELEN));
		ATTEMPT(xdr_opaque(xdrs, (caddr_t)&cred->adc_fullname.key,
				sizeof (des_block)));
		ATTEMPT(xdr_opaque(xdrs, (caddr_t)&cred->adc_fullname.window,
				sizeof (cred->adc_fullname.window)));
		trace1(TR_xdr_authdes_cred, 1);
		return (TRUE);
	case ADN_NICKNAME:
		ATTEMPT(xdr_opaque(xdrs, (caddr_t)&cred->adc_nickname,
				sizeof (cred->adc_nickname)));
		trace1(TR_xdr_authdes_cred, 1);
		return (TRUE);
	default:
		trace1(TR_xdr_authdes_cred, 1);
		return (FALSE);
	}
}


bool_t
xdr_authdes_verf(xdrs, verf)
	register XDR *xdrs;
	register struct authdes_verf *verf;
{
	/*
	 * Unrolled xdr
	 */
	trace1(TR_xdr_authdes_verf, 0);
	ATTEMPT(xdr_opaque(xdrs, (caddr_t)&verf->adv_xtimestamp,
				sizeof (des_block)));
	ATTEMPT(xdr_opaque(xdrs, (caddr_t)&verf->adv_int_u,
				sizeof (verf->adv_int_u)));
	trace1(TR_xdr_authdes_verf, 1);
	return (TRUE);
}
