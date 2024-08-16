/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IF_ETHER_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IF_ETHER_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/if_ether_f.h	1.3"
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
 * Ethernet address - 6 octets
 *
 * 0		   8		   16		   24		 31
 * +---------------+---------------+---------------+---------------+
 * | Host Address (octets 0-3)					   |
 * +---------------+---------------+---------------+---------------+
 * | Host Address (octets 4-5)	   |
 * +---------------+---------------+
 */
typedef unsigned char ether_addr_t[6];

/*
 * Ethernet type field
 *
 * 0				 15
 * +-------------------------------+
 * | Ethernet Type		   |
 * +-------------------------------+
 */

typedef unsigned short	ether_type_t;

/*
 * Structure of a 10Mb/s Ethernet header.
 *
 * 0		   8		   16		   24		 31
 * +---------------+---------------+---------------+---------------+
 * | Destination Host Address (octets 0-3)			   |
 * +---------------+---------------+---------------+---------------+
 * | Dest Host Address (octets 4-5)| Src Host Address (octets 0-1) |
 * +---------------+---------------+---------------+---------------+
 * | Source Host Address (octets 2-5)				   |
 * +---------------+---------------+---------------+---------------+
 * | Ethernet Type		   |
 * +---------------+---------------+
 */
struct ether_header {
	ether_addr_t	ether_dhost;
	ether_addr_t	ether_shost;
	ether_type_t	ether_type;
};

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 *
 * 0		   8		   16		   24		 31
 * +---------------+---------------+---------------+---------------+
 * | Hardware Address Format	   | Protocol Format		   |
 * +---------------+---------------+---------------+---------------+
 * | H Addr Length | Prot Addr Len | Operation			   |
 * +---------------+---------------+---------------+---------------+
 * | Sender Hardware Address (Octets 0-3)			   |
 * +---------------+---------------+---------------+---------------+
 * | Sender Hard Addr (Octets 4-5) | Sender INET Addr (Octets 0-1) |
 * +---------------+---------------+---------------+---------------+
 * | Sender INET Addr (Octets 2-3) | Target Hard Addr (Octets 0-1) |
 * +---------------+---------------+---------------+---------------+
 * | Target Hardware Address (Octets 2-5)			   |
 * +---------------+---------------+---------------+---------------+
 * | Target Internet Address (Octets 0-3)			   |
 * +---------------+---------------+---------------+---------------+
 */
struct ether_arp {
	struct arphdr	ea_hdr;		/* fixed-size header */
	ether_addr_t	arp_sha;	/* sender hardware address */
	unsigned char	arp_spa[4];	/* sender protocol address */
	ether_addr_t	arp_tha;	/* target hardware address */
	unsigned char	arp_tpa[4];	/* target protocol address */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IF_ETHER_F_H */
