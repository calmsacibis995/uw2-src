/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IF_ARP_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IF_ARP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/if_arp_f.h	1.4"
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

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
 *
 * 0		   8		   16				 31
 * +-------------------------------+-------------------------------+
 * | Hardware Address Format	   | Protocol Format		   |
 * +---------------+---------------+-------------------------------+
 * | H Addr Length | Prot Addr Len | Operation			   |
 * +---------------+---------------+-------------------------------+
 */
struct arphdr {
	unsigned short	ar_hrd;		/* format of hardware address */
	unsigned short	ar_pro;		/* format of protocol address */
	unsigned char	ar_hln;		/* length of hardware address */
	unsigned char	ar_pln;		/* length of protocol address */
	unsigned short	ar_op;		/* ARP Operation */
	/*
	 * The remaining fields are variable in size, according to the
	 * sizes above, and are defined as appropriate for specific
	 * hardware/protocol combinations.  (E.g., see
	 * <netinet/if_ether.h>.)
	 */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IF_ARP_F_H */
