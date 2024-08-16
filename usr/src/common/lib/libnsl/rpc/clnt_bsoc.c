/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_bsoc.c	1.2.8.1"
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
 * clnt_bsoc.c, Help interface to broadcast service.
 */

#include <rpc/rpc.h>
#include "trace.h"
#include <netconfig.h>

/*
 * Finds a list of broadcast addresses for the given transport tokenid
 * returns the number of such addresses found. Uses rpcbind and the
 * netdir daemon
 *
 * Returns the number of broadcast addresses found
 */
int
getbroadcastnets(fd, addrs, nconf)
	int fd;		/* Not being used currently */
	struct netbuf* addrs;
	struct netconfig *nconf;
{
	trace1(TR_getbroadcastnets, 0);
	trace1(TR_getbroadcastnets, 1);
	return (1);
}

/*
 * Do all the options managements stuff here. This is very protocol specific
 * and should be provided by the vendor. The following case is just for
 * socket/IP.
 * Returns 0 if succeeds else returns 1.
 */
int
negotiate_broadcast(fd, nconf)
	int fd;
	struct netconfig *nconf;
{
	trace1(TR_negotiate_broadcast, 0);
	trace1(TR_negotiate_broadcast, 1);
	return (0);
}
