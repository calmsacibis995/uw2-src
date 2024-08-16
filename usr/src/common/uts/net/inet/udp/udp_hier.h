/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_UDP_UDP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_UDP_UDP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/udp/udp_hier.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

#define UDP_HIER_BASE		(1)

#define UDB_RWLCK_HIER		(UDP_HIER_BASE)
#define UDP_INP_RWLCK_HIER	(UDB_RWLCK_HIER + 2)
#define UDP_ADDR_LCK_HIER	(UDP_INP_RWLCK_HIER + 2)
#define UDP_MINFO_LCK_HIER	(32)
#define UDP_QBOT_LCK_HIER	(32)

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_UDP_UDP_HIER_H */
