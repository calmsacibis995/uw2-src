/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/getdname.c	1.7.8.4"
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
 * getdname.c
 *
 * Gets and sets the domain name of the system
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include "rpc_mt.h"

#define sysinfo _abi_sysinfo

#ifndef SI_SRPC_DOMAIN
#define	use_file
#endif

#ifdef use_file
char DOMAIN[] = "/etc/domain";
#endif

#ifdef use_file
/*
 * domainname:
 * Since getdomainname() and setdomainname() share this variable,
 * __rpc_lock serializes their calls.
 */
static char *domainname;
#endif

int
getdomainname(name, namelen)
	char *name;
	int namelen;
{
#ifdef use_file
	char *line;

	trace2(TR_getdomainname, 0, namelen);

	MUTEX_LOCK(&__rpc_lock);
	if (domainname){
		strncpy(name, domainname, namelen);
		MUTEX_UNLOCK(&__rpc_lock);
		trace1(TR_getdomainname, 1);
		return (0);
	}
	MUTEX_UNLOCK(&__rpc_lock);

	line = (char *)calloc(1, 256);

	if ((domain_fd = _fopen(DOMAIN, "r")) == NULL) {
		trace1(TR_getdomainname, 1);
		return (-1);
	}

	if (fscanf(domain_fd, "%s", line) == NULL) {
		fclose(domain_fd);
		trace1(TR_getdomainname, 1);
		return (-1);
	}
	fclose(domain_fd);
	MUTEX_LOCK(&__rpc_lock);
	if (domainname == NULL)
		domainname = line;
	else
		free(line);
	(void) strncpy(name, domainname, namelen);
	MUTEX_UNLOCK(&__rpc_lock);
	trace1(TR_getdomainname, 1);
	return (0);
#else
	int sysinfostatus;

	trace2(TR_getdomainname, 0, namelen);
	sysinfostatus = sysinfo(SI_SRPC_DOMAIN, name, namelen);

	trace1(TR_getdomainname, 1);
	return ((sysinfostatus < 0) ? -1 : 0);
#endif
}

setdomainname(domain, len)
	char *domain;
	int len;
{
#ifdef use_file

	FILE *domain_fd;

	trace2(TR_setdomainname, 0, len);

	MUTEX_LOCK(&__rpc_lock);
	if (domainname)
		free(domainname);

	if ((domain_fd = _fopen(DOMAIN, "rw")) == NULL) {
		MUTEX_UNLOCK(&__rpc_lock);
		trace1(TR_setdomainname, 1);
		return (-1);
	}
	if (fputs(domain, domain_fd) == NULL) {
		MUTEX_UNLOCK(&__rpc_lock);
		trace1(TR_setdomainname, 1);
		return (-1);
	}
	fclose(domain_fd);
	domainname = (char *)calloc(1, 256);
	(void) strncpy(domainname, domain, len);
	MUTEX_UNLOCK(&__rpc_lock);
	trace1(TR_setdomainname, 1);
	return (0);
#else
	int sysinfostatus;

	trace2(TR_setdomainname, 0, len);
	sysinfostatus = sysinfo(SI_SET_SRPC_DOMAIN,
				domain, len + 1); /* add null */
	trace1(TR_setdomainname, 1);
	return ((sysinfostatus < 0) ? -1 : 0);
#endif
}
