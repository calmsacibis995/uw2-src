/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/secretkey.c	1.1.7.1"
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
 * secretkey.c
 *
 * Secret key lookup routines
 */

#include <stdio.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <string.h>
#include "trace.h"

/*
 * Get somebody's encrypted secret key from the database, using the given
 * passwd to decrypt it.
 */
getsecretkey(netname, secretkey, passwd)
	char *netname;
	char *secretkey;
	char *passwd;
{
	char lookup[3 * HEXKEYBYTES];
	char *p;

        trace1(TR_getsecretkey, 0);
	if (!getpublicandprivatekey(netname, lookup)) {
		trace1(TR_getsecretkey, 1);
		return (0);
	}
	p = strchr(lookup, ':');
	if (p == NULL) {
		trace1(TR_getsecretkey, 1);
		return (0);
	}
	p++;
	if (!xdecrypt(p, passwd)) {
		trace1(TR_getsecretkey, 1);
		return (0);
	}
	if (memcmp(p, p + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0) {
		secretkey[0] = 0;
		trace1(TR_getsecretkey, 1);
		return (1);
	}
	p[HEXKEYBYTES] = 0;
	(void) strcpy(secretkey, p);
	trace1(TR_getsecretkey, 1);
	return (1);
}
