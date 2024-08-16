/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_RAWIP_RAWIP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_RAWIP_RAWIP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/rawip/rawip_hier.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define RAW_HIER_BASE		(1)

#define RAW_HEAD_LCK_HIER	(RAW_HIER_BASE)
#define RAW_LCK_HIER		(RAW_HIER_BASE + 2)
#define RAW_QBOT_LCK_HIER	(32)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_RAWIP_RAWIP_HIER_H */
