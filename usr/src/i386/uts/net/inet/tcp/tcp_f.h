/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/tcp/tcp_f.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/byteorder.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	unsigned long tcp_seq;

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 *
 * 0	   4	   8		   16	   20	   24		 31
 * +-------------------------------+-------------------------------+
 * | Source Port		   | Destination Port		   |
 * +-------------------------------+-------------------------------+
 * | Sequence Number						   |
 * +---------------------------------------------------------------+
 * | Acknowledgement Number					   |
 * +-------+-------+---------------+-------------------------------+
 * | Offset| RSRVD | Flags	   | Window			   |
 * +-------+-------+---------------+-------------------------------+
 * | Chucksum			   | Urgent Pointer		   |
 * +-------------------------------+-------------------------------+
 */

struct tcphdr {
	unsigned short	th_sport;	/* source port */
	unsigned short	th_dport;	/* destination port */
	tcp_seq		th_seq;		/* sequence number */
	tcp_seq		th_ack;		/* acknowledgement number */
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	th_off:4;	/* data offset */
	unsigned char	th_x2:4;	/* (unused) */
#elif BYTE_ORDER == LITTLE_ENDIAN
	unsigned char	th_x2:4;	/* (unused) */
	unsigned char	th_off:4;	/* data offset */
#else
#include "BYTE_ORDER must be defined in <net/inet/byteorder_f.h>"
#endif
	unsigned char	th_flags;
	unsigned short	th_win;		/* window */
	unsigned short	th_sum;		/* checksum */
	unsigned short	th_urp;		/* urgent pointer */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_F_H */
