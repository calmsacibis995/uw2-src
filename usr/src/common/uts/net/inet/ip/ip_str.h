/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_STR_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_STR_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ip/ip_str.h	1.11"
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
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_var.h>		/* REQUIRED */
#include <net/inet/route/route.h>	/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if.h>			/* REQUIRED */
#include <net/route.h>			/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_var.h>		/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#elif defined(_KMEMUSER)

#include <net/route_kern.h>		/* REQUIRED */
#include <netinet/in_f.h>		/* REQUIRED */
#include <netinet/in_var.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * Definitions for stream driver control of the Internet Protocol.
 * This module defines the structures related to controlling the
 * streams interface itself, the structures related to various other
 * protocol elements are in other files.
 */

#define NIP		8		/* Number of minor devices supported */
#define IP_PROVIDERS	16		/* Max Number of link level service
					 * providers */
#define IP_SAP		0x800		/* SAP for IP protocol - currently
					 * enet type */
#define ARP_SAP		0x806		/* SAP for ARP */

struct ip_pcb {
	queue_t	*ip_rdq;		/* Upper read queue for this client */
	unsigned short	ip_proto;	/* Protocol number set with N_BIND */
	unsigned short	ip_state;	/* State flags, see below */
	atomic_int_t 	ip_rdqref;	/* Protects ip_rdq while lock dropped */
};

#define IPOPEN	1		/* Minor device open when set */
#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* The description of each link service */
struct ip_provider {
	char		 name[IFNAMSIZ]; /* provider name (e.g., emd1) */
	queue_t		*qbot;		/* lower write queue */
	int		 l_index;	/* unique ID of lower stream */
	int		 if_flags;	/* up/down, broadcast, etc. */
	int		 if_metric;	/* routing metric (external only) */
	int		 if_maxtu;	/* maximum transmission unit */
	int		 if_mintu;	/* minimum transmission unit */
	int		 if_spsize;	/* slug mode size */
	int		 if_spthresh;	/* slug mode threshold */
	struct in_ifaddr ia;		/* address chain structure maintained
					 * by if */

#define SOCK_INADDR(sock) (&(((struct sockaddr_in *)(sock))->sin_addr))
#define PROV_INADDR(prov) SOCK_INADDR(&((prov)->ia.ia_ifa.ifa_addr))

/*
 * The following defines are vestiges of the socket based IP
 * implementation
 */
#define if_addr	     ia.ia_ifa.ifa_addr /* interface address */
#define if_broadaddr ia.ia_broadaddr	/* broadcast address */
#define if_dstaddr   ia.ia_dstaddr	/* other end of p-to-p link */

	time_t		 if_lastchange;	/* time of status change for SNMP */
	atomic_int_t	 qbot_ref;	/* reference count */
};

/*
 * A special version of the unitdata request to be sent down through ip -> it
 * contains various ip specific extensions to the base structure
 */

struct ip_unitdata_req {
	unsigned long	dl_primitive;	/* always N_UNITDATA_REQ */
	unsigned long	dl_dest_addr_length; /* dest NSAP addr length - 4 for
					      * ip */
	unsigned long	dl_dest_addr_offset; /* dest NSAP addr offset */
	unsigned long	dl_reserved[2];
	mblk_t	       *options;	/* options for ip */
	struct route	route;		/* route for packet to follow */
	int		flags;
	unsigned char	tos;
	unsigned char	ttl;
	struct in_addr	ip_addr;	/* the ip destination addr */
};

struct ip_unitdata_ind {
	long            PRIM_type;	/* always N_UNITDATA_REQ */
	long            RA_length;	/* dest NSAP addr length - 4 for ip */
	long            RA_offset;	/* dest NSAP addr offset */
	long            LA_length;	/* dest NSAP addr length - 4 for ip */
	long            LA_offset;	/* dest NSAP addr offset */
	long            SERV_class;	/* service class */
	mblk_t 		*options;	/* input options for ip */
	struct ip_provider *provider;	/* input provider for ip */
};

#endif /* _KERNEL || _KMEMUSER */ 

/*
 * ip_ctlmsg is the structure used to send control messages to the
 * client upper level protocols.  These messages are actually
 * generated by ICMP and are passed down to IP to distribute among the
 * clients.
 */

struct ip_ctlmsg {
	int		command;
	struct in_addr	dst_addr;
	struct in_addr	src_addr;
	int		proto;
	char		data[8];
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_STR_H */
