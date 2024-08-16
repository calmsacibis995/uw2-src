/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpcdname.c	1.1.9.2"
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
 * rpcdname.c
 *
 * Gets the default domain name
 */
#include <string.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include "trace.h"
#include "rpc_mt.h"

/*
 * default_domain:
 * __rpc_lock is held during this variable's initialization.
 */
static char *default_domain = 0;

static char *
get_default_domain()
{
	char temp[256];

	trace1(TR_get_default_domain, 0);
	if (default_domain) {
		trace1(TR_get_default_domain, 1);
		return (default_domain);
	}
	if (getdomainname(temp, sizeof (temp)) < 0) {
		trace1(TR_get_default_domain, 1);
		return (0);
	}
	if ((int) strlen(temp) > 0) {
		MUTEX_LOCK(&__rpc_lock);
		if (default_domain == NULL)
			default_domain = strdup(temp);
		MUTEX_UNLOCK(&__rpc_lock);
	}
	trace1(TR_get_default_domain, 1);
	return (default_domain);
}

/*
 * This is a wrapper for the system call getdomainname which returns a
 * ypclnt.h error code in the failure case.  It also checks to see that
 * the domain name is non-null, knowing that the null string is going to
 * get rejected elsewhere in the yp client package.
 */
int
_rpc_get_default_domain(domain)
	char **domain;
{
	trace1(TR___rpc_get_default_domain, 0);
	if ((*domain = get_default_domain()) != 0) {
		trace1(TR___rpc_get_default_domain, 1);
		return (0);
	}
	trace1(TR___rpc_get_default_domain, 1);
	return (-1);
}
