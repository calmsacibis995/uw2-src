/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ip/ip_hier.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

#define	NETMP_LCK_HIER		(32)

#define IP_HIER_BASE		(20)

#define IPQ_LCK_HIER		(IP_HIER_BASE)

#define IPFRAG_LCK_HIER		(IP_HIER_BASE+1)

#define PROV_LCK_HIER		(IP_HIER_BASE)

#define IP_HOP_LCK_HIER		(IP_HIER_BASE+1)

#define ROUTE_LCK_HIER		(IP_HIER_BASE+2)

#define IP_LCK_HIER		(IP_HIER_BASE+3)

#define IFSTATS_LCK_HIER	(30)

#define IP_FAST_LCK_HIER	(32)

#define IPQBOT_LCK_HIER		(32)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_HIER_H */
