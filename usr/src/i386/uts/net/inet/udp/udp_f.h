/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_UDP_UDP_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_UDP_UDP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/udp/udp_f.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * UDP protocol header.  Per RFC 768, September, 1981.
 *
 * 0		   		   16		   	   	 31
 * +-------------------------------+-------------------------------+
 * | Source Port		   | Destination Port  		   |
 * +-------------------------------+-------------------------------+
 * | Length			   | Checksum			   |
 * +-------------------------------+-------------------------------+
 */

struct udphdr {
	unsigned short	uh_sport;	/* source port */
	unsigned short	uh_dport;	/* destination port */
	short		uh_ulen;	/* UDP length */
	unsigned short	uh_sum;		/* UDP checksum */
};

#define UDP_TSDU_SIZE	(65508)	/* maximum TSDU size */
#define UDP_TIDU_SIZE	(65508)	/* maximum TIDU size */

/*
 * UDP_OPT_SIZE is based on the maximum size of all level SOL_SOCKET
 * and IPPROTO_IP options (including option header overhead).
 */

#define UDP_OPT_SIZE	(SOL_SOCKET_MAXSZ + IPPROTO_IP_MAXSZ)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_UDP_UDP_F_H */
