/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_hier.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define TCP_HIER_BASE		(1)

#define TCP_HEAD_HIER		(TCP_HIER_BASE)
#define TCP_INP_HIER		(TCP_HIER_BASE + 2)
#define TCP_ADDR_HIER		(TCP_INP_HIER + 2)

#define TCP_CONN_HIER		(32)
#define TCP_DEBUG_HIER		(32)
#define TCP_ISS_HIER		(32)
#define TCP_QBOT_HIER		(32)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_HIER_H */
