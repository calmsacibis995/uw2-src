/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.routed/if.c	1.1.8.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)if.c	5.5 (Berkeley) 6/18/88
 */


/*
 * Routing Table Management Daemon
 */
#include "defs.h"

extern	struct interface *ifnet;

/*
 * Find the interface with address addr.
 */
struct interface *
if_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct interface *ifp;

#define	same(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_REMOTE)
			continue;
		if (ifp->int_addr.sa_family != addr->sa_family)
			continue;
		if (same(&ifp->int_addr, addr))
			break;
		if ((ifp->int_flags & IFF_BROADCAST) &&
		    same(&ifp->int_broadaddr, addr))
			break;
	}
	return (ifp);
}

/*
 * Find the point-to-point interface with destination address addr.
 */
struct interface *
if_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct interface *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		if ((ifp->int_flags & IFF_POINTOPOINT) == 0)
			continue;
		if (same(&ifp->int_dstaddr, addr))
			break;
	}
	return (ifp);
}

/*
 * Find the interface on the network 
 * of the specified address.
 */
struct interface *
if_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct interface *ifp;
	register int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= af_max)
		return (0);
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_REMOTE)
			continue;
		if (af != ifp->int_addr.sa_family)
			continue;
		if ((*netmatch)(addr, &ifp->int_addr))
			break;
	}
	return (ifp);
}

/*
 * Find an interface from which the specified address
 * should have come from.  Used for figuring out which
 * interface a packet came in on -- for tracing.
 */
struct interface *
if_iflookup(addr)
	struct sockaddr *addr;
{
	register struct interface *ifp, *maybe;
	register int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= af_max)
		return (0);
	maybe = 0;
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		if (ifp->int_addr.sa_family != af)
			continue;
		if (same(&ifp->int_addr, addr))
			break;
		if ((ifp->int_flags & IFF_BROADCAST) &&
		    same(&ifp->int_broadaddr, addr))
			break;
		if ((ifp->int_flags & IFF_POINTOPOINT) &&
		    same(&ifp->int_dstaddr, addr))
			break;
		if (maybe == 0 && (*netmatch)(addr, &ifp->int_addr))
			maybe = ifp;
	}
	if (ifp == 0)
		ifp = maybe;
	return (ifp);
}
