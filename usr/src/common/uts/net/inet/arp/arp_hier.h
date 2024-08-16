/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ARP_ARP_HIER_H	/* wrapper symbol for kernel use */
#define _NET_INET_ARP_ARP_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/arp/arp_hier.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define ARP_HIER_BASE		(1)

#define	ARP_LCK_HIER		(ARP_HIER_BASE)
#define	ARPTAB_LCK_HIER		(ARP_LCK_HIER+1)
#define	ARPQBOT_INUSE_LCK_HIER	(32)
#define	ARPQTOP_INUSE_LCK_HIER	(32)
#define	APP_Q_INUSE_LCK_HIER	(32)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ARP_ARP_HIER_H */
