/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ICMP_ICMP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_ICMP_ICMP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/icmp/icmp_hier.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined (_KERNEL)

#define ICMP_HIER_BASE		(1)
#define ICMB_RWLCK_HIER		(ICMP_HIER_BASE)
#define ICMP_INP_RWLCK_HIER	(ICMB_RWLCK_HIER + 2)
#define ICMP_ADDR_RWLCK_HIER	(ICMP_INP_RWLCK_HIER + 2)
#define ICMP_MINFO_LCK_HIER	(32)
#define ICMP_QBOT_LCK_HIER	(32)

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ICMP_ICMP_HIER_H */
