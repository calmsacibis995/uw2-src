/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/ip/ip_f.h	1.4"
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
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */
#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_systm.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/byteorder.h>		/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_systm.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structure of an internet header, naked of options.
 *
 * We declare ip_len and ip_off to be short, rather than unsigned short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 *
 * 0	   4	   8		   16	   20	   24		 31
 * +-------+-------+---------------+-------------------------------+
 * | Vers  |Length |Type Of Service| Total Length		   |
 * +-------+-------+---------------+-------+-----------------------+
 * | Identification		   | Flags | Fragment Offset Field |
 * +---------------+---------------+-------+-----------------------+
 * | Time To Live  | Protocol	   | Header Checksum		   |
 * +---------------+---------------+-------------------------------+
 * | Source Internet Address					   |
 * +---------------------------------------------------------------+
 * | Destination Internet Address				   |
 * +---------------------------------------------------------------+
 */

struct ip {
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	ip_v:4;		/* version */
	unsigned char	ip_hl:4;	/* header length */
#elif BYTE_ORDER == LITTLE_ENDIAN
	unsigned char	ip_hl:4;	/* header length */
	unsigned char	ip_v:4;		/* version */
#else
#include "BYTE_ORDER must be defined in <net/inet/byteorder_f.h>"
#endif
	unsigned char	ip_tos;		/* type of service */
	short		ip_len;		/* total length */
	unsigned short	ip_id;		/* identification */
	short		ip_off;		/* fragment offset field */
	unsigned char	ip_ttl;		/* time to live */
	unsigned char	ip_p;		/* protocol */
	unsigned short	ip_sum;		/* checksum */
	struct in_addr	ip_src;		/* source address */
	struct in_addr	ip_dst;		/* destination address */
};

/*
 * Time stamp option structure.
 *
 * 0		   8		   16		   24	   28	 31
 * +---------------+---------------+---------------+-------+-------+
 * | Code = 68	   | Length	   | Pointer	   |OvrFlow| Flags |
 * +---------------+---------------+---------------+-------+-------+
 * | First Internet Address					   |
 * +---------------------------------------------------------------+
 * | First Timestamp						   |
 * +---------------------------------------------------------------+
 */
struct ip_timestamp {
	unsigned char	ipt_code;	/* IPOPT_TS */
	unsigned char	ipt_len;	/* size of structure (variable) */
	unsigned char	ipt_ptr;	/* index of current entry */
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	ipt_oflw:4;	/* overflow counter */
	unsigned char	ipt_flg:4;	/* flags, see below */
#elif BYTE_ORDER == LITTLE_ENDIAN
	unsigned char	ipt_flg:4;	/* flags, see below */
	unsigned char	ipt_oflw:4;	/* overflow counter */
#else
#include "BYTE_ORDER must be defined in <net/inet/byteorder_f.h>"
#endif
	union ipt_timestamp {
		n_long ipt_time[1];
		struct	ipt_ta {
			struct in_addr ipt_addr;
			n_long ipt_time;
		} ipt_ta[1];
	} ipt_timestamp;
};

#define RIP_ETSDU_SIZE	1
#define RIP_TIDU_SIZE	(16 * 1024)

/*
 * RIP_OPT_SIZE is based on the maximum size of all level SOL_SOCKET
 * and IPPROTO_IP options (including option header overhead).
 */

#define RIP_OPT_SIZE	(SOL_SOCKET_MAXSZ + IPPROTO_IP_MAXSZ)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_F_H */
