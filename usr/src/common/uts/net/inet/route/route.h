/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ROUTE_ROUTE_H	/* wrapper symbol for kernel use */
#define _NET_INET_ROUTE_ROUTE_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/route/route.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

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
 *	STREAMware TCP/IP Release
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T\'s UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989	 Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/in.h>		/* REQUIRED */
#include <net/socket.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if.h>			/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */

#elif defined(_KMEMUSER)

#include <net/if.h>			/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Kernel resident routing tables.
 *
 * The routing tables are initialized when interface addresses
 * are set by making entries for all directly connected interfaces.
 */

/*
 * A route consists of a destination address and a reference
 * to a routing entry.	These are often held by protocols
 * in their control blocks, e.g. inpcb.
 */
struct route {
	mblk_t		*ro_rt;
	struct sockaddr	 ro_dst;
};

/* MP should use rtfree() */

#ifdef GATEWAY

#define RTHASHSIZ	64

#else /* GATEWAY */

#define RTHASHSIZ	8

#endif /* GATEWAY */

#if (RTHASHSIZ & (RTHASHSIZ - 1)) == 0

#define RTHASHMOD(h)	((h) & (RTHASHSIZ - 1))

#else

#define RTHASHMOD(h)	((h) % RTHASHSIZ)

#endif /* (RTHASHSIZ & (RTHASHSIZ - 1)) == 0 */
#endif /* _KERNEL || _KMEMUSER */

/*
 * We distinguish between routes to hosts and routes to networks,
 * preferring the former if available.	For each route we infer the
 * interface to use from the gateway address supplied when the route
 * was entered.	 Routes that forward packets through gateways are
 * marked so that the output routines know to address the gateway
 * rather than the ultimate destination.
 *
 * Note: Two socket ioctls, SIOCADDRT and SIOCDELRT are defined
 *	 in terms of size of struct rtentry.
 *	 If size of struct rtentry is changed, commands that use
 *	 these ioctls must be recompiled.
 */

struct rtentry {
	unsigned long	    rt_hash;	/* to speed lookups */
	struct sockaddr	    rt_dst;	/* key */
	struct sockaddr	    rt_gateway;	/* value */
	short		    rt_flags;	/* up/down?, host/net */
	short		    rt_refcnt;	/* # held references */
	unsigned long	    rt_use;	/* raw # packets forwarded */
	struct ip_provider *rt_prov;	/* the answer: provider to use */
	int		    rt_metric;	/* metric for route provider */
	int		    rt_proto;	/* protocol route was learned */
	time_t		    rt_age;	/* time of last update */
};

#if defined(_KERNEL)

/*
 * Note: rt_entry must be the first field for BPTORTENTRY cast to work.
 */
struct rte {
	struct rtentry	rt_entry;
	rwlock_t	*rt_rwlck;	/* ptr to rthost_rwlck or rtnet_rwlck */
};

#endif /* _KERNEL */

/*
 * This structure is for passing rtentry information back to user
 * via ioctl.  The pointer to ip_provider is replaced by provider's
 * name.
 */
struct rtrecord {
	struct sockaddr	    rt_dst;
	struct sockaddr	    rt_gateway;
	short		    rt_flags;
	short		    rt_refcnt;
	unsigned long	    rt_use;
	char		    rt_prov[IFNAMSIZ];
};

#define RTF_UP		0x1		/* route useable */
#define RTF_GATEWAY	0x2		/* destination is a gateway */
#define RTF_HOST	0x4		/* host entry (net otherwise) */
#define RTF_REINSTATE	0x8		/* re-instate route after timeout */
#define RTF_DYNAMIC	0x10		/* created dynamically (by redirect) */
#define RTF_MODIFIED	0x20		/* modified dynamically (by redirect)*/

#define RTF_SWITCHED	0x40		/* this route must be switched */
#define RTF_SLAVE	0x80		/* slave switched route */
#define RTF_REMOTE	0x100		/* route usedfor forwarded packets */
#define RTF_TOSWITCH	0x200		/* gateway is switched route */

#define RTF_SSSTATE	0x700		/* switched SLIP state */
#define SSS_NOCONN	0x000		/* unconnected */
#define SSS_DIALING	0x100		/* switch in progress */
#define SSS_OPENWAIT	0x200		/* short delay */
#define SSS_INUSE	0x300		/* in use locally */
#define SSS_CLEARWAIT	0x400		/* delayed hangup */
#define SSS_CALLFAIL	0x500		/* call failed recently */

#define RTF_USERMASK	(RTF_GATEWAY|RTF_HOST|RTF_SWITCHED)
/* flags settable by user */
#define SSS_GETSTATE(rt) ((rt)->rt_flags & RTF_SSSTATE)
#define SSS_SETSTATE(rt, new) \
	(rt)->rt_flags = (((rt)->rt_flags & ~RTF_SSSTATE) | new)

/*
 * Protocols for learning routes.  These values are from RFC 1158.
 */
#define RTP_OTHER	1
#define RTP_LOCAL	2
#define RTP_NETMGMT	3
#define RTP_ICMP	4
#define RTP_EGP		5
#define RTP_GGP		6
#define RTP_HELLO	7
#define RTP_RIP		8
#define RTP_IS_IS	9
#define RTP_ES_IS	10
#define RTP_CISCOIGRP	11
#define RTP_BBNSPFIGP	12
#define RTP_OSPF	13
#define RTP_BGP		14

/*
 * Routing statistics.
 */
struct rtstat {
	short	rts_badredirect;	/* bogus redirect calls */
	short	rts_dynamic;		/* routes created by redirects */
	short	rts_newgateway;		/* routes modified by redirects */
	short	rts_unreach;		/* lookups which failed */
	short	rts_wildcard;		/* lookups satisfied by a wildcard */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_ROUTE_ROUTE_H */
