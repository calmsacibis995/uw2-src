/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_VAR_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_VAR_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/ip/ip_var_f.h	1.7"
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

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/byteorder.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/byteorder.h>		/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Overlay for IP header used by other protocols (TCP, UDP).
 */
struct ipovly {
	caddr_t		ih_next;	/* for protocol sequence q's */
	mblk_t	       *ih_mblk;
	unsigned char	ih_x1;		/* (unused) */
	unsigned char	ih_pr;		/* protocol */
	short		ih_len;		/* protocol length */
	struct in_addr	ih_src;		/* source internet address */
	struct in_addr	ih_dst;		/* destination internet address */
};

/*
 * IP reassembly queue structure.  Each fragment being reassembled is
 * attached to one of these structures.	 They are timed out after
 * ipq_ttl drops to 0, and may also be reclaimed if memory becomes
 * tight.
 *
 * This used to be a kernel structure, but the snmp command requires
 * it.
 */
struct ipq {
	struct ipq	*next;		/* to next other reass headers */
	struct ipq	*prev;		/* to previous other reass headers */
	unsigned char	 ipq_ttl;	/* time for reass q to live */
	unsigned char	 ipq_p;		/* protocol of this fragment */
	unsigned short	 ipq_id;	/* sequence id for reassembly */
	struct ipasfrag *ipq_next;	/* to ip headers of fragments */
	struct in_addr	 ipq_src;
	struct in_addr	 ipq_dst;
	lock_t		*ipq_lck;
	int		 ipq_refcnt;
};

/*
 * IP header, when holding a fragment.
 *
 * 0	   4	   8		   16	   20	   24		 31
 * +-------+-------+---------------+-------------------------------+
 * | Vers  |Length |More Fragments | Total Length		   |
 * +-------+-------+---------------+-------+-----------------------+
 * | Identification		   | Flags | Fragment Offset Field |
 * +---------------+---------------+-------+-----------------------+
 * | Time To Live  | Protocol	   | Header Checksum		   |
 * +---------------+---------------+-------------------------------+
 *
 * Note: ipf_next must be at same offset as ipq_next above
 */
struct ipasfrag {
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	 ip_v:4;
	unsigned char	 ip_hl:4;
#elif BYTE_ORDER == LITTLE_ENDIAN
	unsigned char	 ip_hl:4;
	unsigned char	 ip_v:4;
#else
#include "BYTE_ORDER must be defined in <net/inet/byteorder_f.h>"
#endif
	unsigned char	 ipf_mff;	/* copied from (ip_off&IP_MF) */
	short		 ip_len;
	unsigned short	 ip_id;
	short		 ip_off;
	unsigned char	 ip_ttl;
	unsigned char	 ip_p;
	unsigned short	 ip_sum;
	struct ipasfrag *ipf_next;	/* next fragment */
	mblk_t		*ipf_mblk;	/* The mblk header for this data */
};

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_VAR_F_H */
