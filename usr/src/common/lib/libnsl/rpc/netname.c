/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/netname.c	1.3.9.2"
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
 * netname.c
 * netname utility routines
 *
 * convert from unix names (uid, gid) to network wide names and vice-versa
 * This module is operating system dependent!
 * What we define here will work with any unix system that has adopted
 * the Sun NIS domain architecture.
 */

#include <string.h>
#include <sys/param.h>
#include <ctype.h>
#include "trace.h"
#include <rpc/rpc.h>

#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN 256
#endif
#ifndef NGROUPS
#define	NGROUPS 16
#endif


static char *OPSYS = "unix";

/*
 * Figure out my fully qualified network name
 */
getnetname(name)
	char name[MAXNETNAMELEN + 1];
{
	uid_t uid;
	int dummy;

	trace1(TR_getnetname, 0);
	uid = geteuid();
	if (uid == 0) {
		dummy = host2netname(name, (char *) NULL, (char *) NULL);
		trace1(TR_getnetname, 1);
		return (dummy);
	} else {
		dummy = user2netname(name, uid, (char *) NULL);
		trace1(TR_getnetname, 1);
		return (dummy);
	}
}


/*
 * Convert unix cred to network-name
 */
user2netname(netname, uid, domain)
	char netname[MAXNETNAMELEN + 1];
	uid_t uid;
	char *domain;
{
	char *dfltdom;
	register int i;

#define	MAXIPRINT	(11)	/* max length of printed integer */

	trace1(TR_user2netname, 0);

	if (domain == NULL) {
		if (_rpc_get_default_domain(&dfltdom) != 0) {
			trace1(TR_user2netname, 1);
			return (0);
		}
		domain = dfltdom;
	}

	if ((int)strlen(domain) + 1 + MAXIPRINT > MAXNETNAMELEN) {
		trace1(TR_user2netname, 1);
		return (0);
	}
	(void) sprintf(netname, "%s.%d@%s", OPSYS, uid, domain);
	i = strlen(netname);
	if (netname[i-1] == '.')
		netname[i-1] = '\0';
	trace1(TR_user2netname, 1);
	return (1);
}


/*
 * Convert host to network-name
 */
host2netname(netname, host, domain)
	char netname[MAXNETNAMELEN + 1];
	char *host;
	char *domain;
{
	char *dfltdom;
	char hostname[MAXHOSTNAMELEN+1];

	trace1(TR_host2netname, 0);
	if (domain == NULL) {
		if (_rpc_get_default_domain(&dfltdom) != 0) {
			trace1(TR_host2netname, 1);
			return (0);
		}
		domain = dfltdom;
	}
	if (host == NULL) {
		(void) gethostname(hostname, sizeof (hostname));
		host = hostname;
	}
	if ((int)strlen(domain) + 1 + (int)strlen(host) > MAXNETNAMELEN) {
		trace1(TR_host2netname, 1);
		return (0);
	}
	(void) sprintf(netname, "%s.%s@%s", OPSYS, host, domain);
	trace1(TR_host2netname, 1);
	return (1);
}
