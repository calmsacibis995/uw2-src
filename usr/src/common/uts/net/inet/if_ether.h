/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifndef	_NET_INET_IF_ETHER_H	/* wrapper symbol for kernel use */
#define _NET_INET_IF_ETHER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/if_ether.h	1.8"
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
 *		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *	(c) 1986,1987,1988,1989	 Sun Microsystems, Inc.
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 * All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.	 The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.	This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/if_arp.h>		/* REQUIRED */
#include <net/inet/if_ether_f.h>	/* PORTABILITY */
#include <net/inet/in.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if.h>			/* REQUIRED */
#include <net/if_arp.h>			/* REQUIRED */
#include <netinet/if_ether_f.h>		/* PORTABILITY */
#include <netinet/in.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else /* user */
/*
 * The following include is for compatibility with SunOS 3.x and
 * 4.3bsd.  Newly written programs should include it separately.
 */
#include <net/if_arp.h>			/* SVR4.0COMPAT */
#include <netinet/if_ether_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#define ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define ETHERTYPE_IP		0x0800	/* IP protocol */
#define ETHERTYPE_ARP		0x0806	/* Addr. resolution protocol */
#define ETHERTYPE_REVARP	0x8035	/* Reverse ARP */

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define ETHERTYPE_TRAIL		0x1000	/* Trailer packet */
#define ETHERTYPE_NTRAILER	16

#define ETHERMTU	1500
#define ETHERMIN	(60-14)

#define arp_hrd	ea_hdr.ar_hrd
#define arp_pro	ea_hdr.ar_pro
#define arp_hln	ea_hdr.ar_hln
#define arp_pln	ea_hdr.ar_pln
#define arp_op	ea_hdr.ar_op

/*
 * multicast address structure
 *
 * Keep a reference count for each multicast address so
 * addresses loaded into chip are unique.
 */
struct	mcaddr {
	ether_addr_t	mc_enaddr;	/* multicast address */
	unsigned short	mc_count;	/* reference count */
};
#define MCADDRMAX	64		/* multicast addr table length */
#define MCCOUNTMAX	4096		/* multicast addr max ref count */

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.	 For example, each ec_softc or il_softc
 * begins with this structure.
 *
 * The structure contains a pointer to an array of multicast addresses.
 * This pointer is NULL until the first successful SIOCADDMULTI ioctl
 * is issued for the interface.
 */
struct arpcom {
	struct ifnet	ac_if;		/* network-visible interface */
	ether_addr_t	ac_enaddr;	/* ethernet hardware address */
	struct in_addr	ac_ipaddr;	/* copy of ip address */
	struct mcaddr	*ac_mcaddr;	/* table of multicast addrs */
	unsigned short	ac_nmcaddr;	/* count of M/C addrs in use */
	struct in_addr	ac_lastip;	/* cache of last ARP lookup */
	ether_addr_t	ac_lastarp;	/* result of the last ARP */
	int		ac_mintu;	/* minimum transfer unit */
	int		ac_addrlen;	/* length of address */
	int		ac_mactype;	/* type of network */
};

/*
 * Internet to ethernet address resolution table.
 */
struct	arptab {
	struct	in_addr	at_iaddr;	/* internet address */
	union {
	    ether_addr_t atu_enaddr;	/* ethernet address */
	    long	 atu_tvsec;	/* timestamp if incomplete */
	} at_union;
	unsigned char	at_timer;	/* minutes since last reference */
	unsigned char	at_flags;	/* flags */
#ifdef _KERNEL
	mblk_t	       *at_hold;	/* last pkt until resolved/timeout */
#else /* _KERNEL */
	void	       *at_hold;	/* last pkt until resolved/timeout */
#endif /* _KERNEL */
};

#define at_enaddr at_union.atu_enaddr
#define at_tvsec at_union.atu_tvsec

/*
 * Compare two Ethernet addresses.
 */

#ifndef ether_cmp
#define ether_cmp(a, b) (bcmp(a, b, sizeof (ether_addr_t)))
#endif

/*
 * Copy Ethernet addresses from a to b.
 */

#ifndef ether_copy
#define ether_copy(a, b) (bcopy(a, b, sizeof (ether_addr_t)))
#endif

/*
 * Copy IP addresses from a to b
 */

#ifndef ip_copy
#define ip_copy(a, b) (bcopy(a, b, sizeof (struct in_addr)))
#endif

#ifdef	_KERNEL
ether_addr_t etherbroadcastaddr;

extern struct arptab *arptnew(struct in_addr *);

#define BPTOETHER_ARP(bp) \
	((struct ether_arp *)BPTOSTRUCTPTR((bp), _ALIGNOF_ETHER_ARP))

#endif	/* _KERNEL */

#ifdef __STDC__
extern char *ether_ntoa(ether_addr_t);
extern ether_addr_t *ether_aton(char *);
extern int ether_ntohost(char *, ether_addr_t);
extern int ether_hostton(char *, ether_addr_t);
extern int ether_line(char *, ether_addr_t, char *);
#else
extern char *ether_ntoa();
extern ether_addr_t *ether_aton();
extern int ether_ntohost();
extern int ether_hostton();
extern int ether_line();
#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IF_ETHER_H */
