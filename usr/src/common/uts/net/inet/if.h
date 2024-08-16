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

#ifndef	_NET_INET_IF_H	/* wrapper symbol for kernel use */
#define _NET_INET_IF_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/if.h	1.12"
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
 *	STREAMware TCP/IP Release 1.0
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
 */

#ifdef _KERNEL_HEADERS

#include <net/socket.h>			/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/ksynch.h>			/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define IFNAMSIZ	16

#define LOOPM_ID	2100		/* Module ID for Loopback stream */
#define ENETM_ID	2101		/* Module ID for Ethernet stream */
#define SLIPM_ID	2102		/* Module ID for SLIP stream */
#define IFX25M_ID	2103		/* Module ID for IP/X.25 stream */

/*
 * Note the mbuf chain described below is not supported by
 * SVRV.
 */

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three
 * parameters:
 *	(*ifp->if_output)(ifp, m, dst)
 * Here m is the mbuf chain to be sent and dst is the destination
 * address.  The output routine encapsulates the supplied datagram if
 * necessary, and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and
 * either places it on the input queue of a internetwork datagram
 * routine and posts the associated software interrupt, or passes the
 * datagram to a raw packet input routine.
 *
 * Routines exist for locating interfaces by their addresses or for
 * locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.	These routines live in the files if.c and route.c
 */

/*
 * Structure defining a queue for a network interface.
 */
struct ifnet {
	char	       *if_name;	/* name, e.g. ``emd'' or ``lo'' */
	short		if_unit;	/* sub-unit for lower level driver */
	short		if_mtu;		/* maximum transmission unit */
	short		if_flags;	/* up/down, broadcast, etc. */
	short		if_timer;	/* time 'til if_watchdog called */
	unsigned short	if_promisc;	/* net # of reqs for promisc mode */
	int	if_metric;		/* routing metric (external only) */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
	struct	ifqueue {
		void	*ifq_head;	/* used to be struct mbuf * */
		void	*ifq_tail;	/* used to be struct mbuf * */
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
	} if_snd;			/* output queue */
/* procedure handles */
	int	(*if_init)();		/* init routine */
	int	(*if_output)();		/* output routine */
	int	(*if_ioctl)();		/* ioctl routine */
	int	(*if_reset)();		/* bus reset routine */
	int	(*if_watchdog)();	/* timer routine */
/* generic interface statistics */
	int	if_ipackets;		/* packets received on interface */
	int	if_ierrors;		/* input errors on interface */
	int	if_opackets;		/* packets sent on interface */
	int	if_oerrors;		/* output errors on interface */
	int	if_collisions;		/* collisions on csma interfaces */
/* end statistics */
	struct	ifnet *if_next;
	struct	ifnet *if_upper;	/* next layer up */
	struct	ifnet *if_lower;	/* next layer down */
	int	(*if_input)();		/* input routine */
	int	(*if_ctlin)();		/* control input routine */
	int	(*if_ctlout)();		/* control output routine */
};

#define IFF_UP		0x1		/* interface is up */
#define IFF_BROADCAST	0x2		/* broadcast address valid */
#define IFF_DEBUG	0x4		/* turn on debugging */
#define IFF_LOOPBACK	0x8		/* is a loopback net */
#define IFF_POINTOPOINT	0x10		/* interface is point-to-point link */
#define IFF_NOTRAILERS	0x20		/* avoid use of trailers */
#define IFF_RUNNING	0x40		/* resources allocated */
#define IFF_NOARP	0x80		/* no address resolution protocol */
#define IFF_PROMISC	0x100		/* receive all packets */
#define IFF_ALLMULTI	0x200		/* receive all multicast packets */
#define IFF_INTELLIGENT 0x400		/* protocol code on board */
#define IFF_ONEPACKET	0x800		/* slug mode */
#define IFF_PRIVATE	0x8000		/* do not advertise */

struct if_onepacket {
	int	spsize;			/* short packet size */
	int	spthresh;		/* short packet threshold */
};

/* flags set internally only: */
#define IFF_CANTCHANGE \
		(IFF_BROADCAST | IFF_POINTOPOINT | IFF_RUNNING | IFF_PROMISC)

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup
 * level 1) input routines have queues of messages stored on ifqueue
 * structures (defined above).	Entries are added to and deleted from
 * these structures by these macros.
 */
#define IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define IF_DROP(ifq)		((ifq)->ifq_drops++)
#define IF_ENQUEUE(ifq, m) { \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_act = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
}

#define IF_PREPEND(ifq, m) { \
	(m)->m_act = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}

/*
 * Packets destined for level-1 protocol input routines
 * have a pointer to the receiving interface prepended to the data.
 * IF_DEQUEUEIF extracts and returns this pointer when dequeueing the packet.
 * IF_ADJ should be used otherwise to adjust for its presence.
 */
#define IF_ADJ(m) {					\
	(m)->m_off += sizeof (struct ifnet *);		\
	(m)->m_len -= sizeof (struct ifnet *);		\
	if (!(m)->m_len) {				\
		void *n;				\
		MFREE((m), n);				\
		(m) = n;				\
	}						\
}

#define IF_DEQUEUEIF(ifq, m, ifp) {			\
	(m) = (ifq)->ifq_head;				\
	if (m) {					\
		if (((ifq)->ifq_head = (m)->m_act) == 0)\
			(ifq)->ifq_tail = 0;		\
		(m)->m_act = 0;				\
		(ifq)->ifq_len--;			\
		(ifp) = *(mtod((m), struct ifnet **));	\
		IF_ADJ(m);				\
	}						\
}

#define IF_DEQUEUE(ifq, m) {				\
	(m) = (ifq)->ifq_head;				\
	if (m) {					\
		if (((ifq)->ifq_head = (m)->m_act) == 0)\
			(ifq)->ifq_tail = 0;		\
		(m)->m_act = 0;				\
		(ifq)->ifq_len--;			\
	}						\
}

#define IFQ_MAXLEN	50
#define IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * The ifstats structures are linked by each unit into a chain.	 Note that
 * interfaces have to know their own name to play this game.
 */

struct ifstats {
	struct ifstats *ifs_next;	/* next if on chain */
	char	       *ifs_name;	/* interface name */
	short		ifs_unit;	/* unit number */
	short		ifs_active;	/* non-zero if this if is running */
	struct ifaddr  *ifs_addrs;	/* list of addresses */
	short		ifs_mtu;	/* Maximum transmission unit */

	/* generic interface statistics */
	int		ifs_ipackets;	/* packets received on interface */
	int		ifs_ierrors;	/* input errors on interface */
	int		ifs_opackets;	/* packets sent on interface */
	int		ifs_oerrors;	/* output errors on interface */
	int		ifs_collisions;	/* collisions on csma interfaces */
	/* end statistics */
	/* MIB-II */
	struct ifs_mib {
		int		im_iftype;
#define IFOTHER			1
#define IFDDN_X25		4
#define IFRFC877_X25		5
#define IFETHERNET_CSMACD	6
#define IFISO88023_CSMACD	7
#define IFISO88025_TOKENRING	9
#define IFPPP			23
#define IFLOOPBACK		24
#define IFSLIP			28
		int		im_ifspeed;
		struct ifs_ins {
			unsigned long	ii_ifinoctets;
			unsigned long	ii_ifinucastpkts;
			unsigned long	ii_ifinnucastpkts;
			unsigned long	ii_ifindiscards;
			unsigned long	ii_ifinunkprotos;
		} ifs_ins;
		struct ifs_outs {
			unsigned long	io_ifoutoctets;
			unsigned long	io_ifoutucastpkts;
			unsigned long	io_ifoutnucastpkts;
			unsigned long	io_ifoutdiscards;
		} ifs_outs;
	} ifs_mib;

#define iftype		ifs_mib.im_iftype
#define ifspeed		ifs_mib.im_ifspeed
#define ifinoctets	ifs_mib.ifs_ins.ii_ifinoctets
#define ifinucastpkts	ifs_mib.ifs_ins.ii_ifinucastpkts
#define ifinnucastpkts	ifs_mib.ifs_ins.ii_ifinnucastpkts
#define ifindiscards	ifs_mib.ifs_ins.ii_ifindiscards
#define ifinunkprotos	ifs_mib.ifs_ins.ii_ifinunkprotos
#define ifoutoctets	ifs_mib.ifs_outs.io_ifoutoctets
#define ifoutucastpkts	ifs_mib.ifs_outs.io_ifoutucastpkts
#define ifoutnucastpkts	ifs_mib.ifs_outs.io_ifoutnucastpkts
#define ifoutdiscards	ifs_mib.ifs_outs.io_ifoutdiscards
	/* MIB-II */
};

typedef struct ifstats ifstats_t;


#ifdef _KERNEL

extern struct ifstats *ifstats;		/* COMPATIBILITY */

/*
 * Lock macros for IFSTATS chain.  These remain for compatibility with old
 * drivers which might access the ifstats chain directly.  Drivers should
 * use ifstats_attach()/ifstats_detach() instead.
 */

extern rwlock_t *ifstats_rwlck;		/* COMPATIBILITY */

#define IFSTATS_RDLOCK()	oldspl=RW_RDLOCK(ifstats_rwlck, plstr)
#define IFSTATS_RDLOCKx()	RW_RDLOCK(ifstats_rwlck, plstr)
#define IFSTATS_WRLOCK()	oldspl=RW_WRLOCK(ifstats_rwlck, plstr)
#define IFSTATS_WRLOCKx()	RW_WRLOCK(ifstats_rwlck, plstr)
#define IFSTATS_UNLOCK()	RW_UNLOCK(ifstats_rwlck, oldspl)
#define IFSTATS_UNLOCKx()	RW_UNLOCK(ifstats_rwlck, plstr)

#ifdef __STDC__
extern void	ifstats_attach(struct ifstats *);
extern struct ifstats	*ifstats_detach(struct ifstats *);
#else
extern void	ifstats_attach();
extern struct ifstats	*ifstats_detach();
#endif        /* __STDC__ */

#endif /* _KERNEL */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct sockaddr ifa_addr;	/* address of interface */
	union {
		struct sockaddr ifu_broadaddr;
		struct sockaddr ifu_dstaddr;
	} ifa_ifu;
#define ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address */
#define ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of p-to-p link */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifstats *ifa_ifs;	/* back-pointer to interface stats */
	struct	ifaddr *ifa_next;	/* next address for interface */
};

/*
 * Interface request structure used for socket ioctl's.	 All interface
 * ioctl's must have parameter definitions which begin with ifr_name.
 * The remainder may be interface specific.
 */
struct	ifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "emd1" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		char	ifru_oname[IFNAMSIZ];	/* other if name */
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		char	ifru_data[1];		/* interface dependent data */
		char	ifru_enaddr[6];
		struct	if_onepacket ifru_onepacket;
	} ifr_ifru;
#define ifr_addr	ifr_ifru.ifru_addr	/* address */
#define ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define ifr_oname	ifr_ifru.ifru_oname	/* other if name */
#define ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define ifr_data	ifr_ifru.ifru_data	/* for use by interface */
#define ifr_enaddr	ifr_ifru.ifru_enaddr	/* ethernet address */
#define ifr_onepacket	ifr_ifru.ifru_onepacket	/* slug mode data */
};

/*
 * Structure used in SIOCGIFCONF request.  Used to retrieve interface
 * configuration for machine (useful for programs which must know all
 * networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

#ifdef _KERNEL
#define BPTOIFREQ(bp) ((struct ifreq *)BPTOSTRUCTPTR((bp), _ALIGNOF_IFREQ))
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IF_H */
