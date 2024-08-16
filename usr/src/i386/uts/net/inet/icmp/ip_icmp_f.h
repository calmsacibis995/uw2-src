/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ICMP_IP_ICMP_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_ICMP_IP_ICMP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/icmp/ip_icmp_f.h	1.3"
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
 * Interface Control Message Protocol Definitions.
 * Per RFC 792, September 1981.
 *
 * 0		   8		   16				 31
 * +---------------+---------------+-------------------------------+
 * | Type	   | Code	   | Checksum			   |
 * +---------------+---------------+-------------------------------+
 */

/*
 * Structure of an icmp header.
 */
struct icmp {
	unsigned char	icmp_type;	/* type of message, see below */
	unsigned char	icmp_code;	/* type sub code */
	unsigned short	icmp_cksum;	/* ones complement cksum of struct */
	union {
		unsigned char ih_pptr;	/* ICMP_PARAMPROB */
		struct in_addr ih_gwaddr; /* ICMP_REDIRECT */
		struct ih_idseq {
			n_short	icd_id;
			n_short	icd_seq;
		} ih_idseq;
		int ih_void;
	} icmp_hun;
#define icmp_pptr	icmp_hun.ih_pptr
#define icmp_gwaddr	icmp_hun.ih_gwaddr
#define icmp_id		icmp_hun.ih_idseq.icd_id
#define icmp_seq	icmp_hun.ih_idseq.icd_seq
#define icmp_void	icmp_hun.ih_void
	union {
		struct id_ts {
			n_time its_otime;
			n_time its_rtime;
			n_time its_ttime;
		} id_ts;
		struct id_ip  {
			struct ip idi_ip;
			/* options and then 64 bits of data */
		} id_ip;
		unsigned long	id_mask;
		char	id_data[1];
	} icmp_dun;
#define icmp_otime	icmp_dun.id_ts.its_otime
#define icmp_rtime	icmp_dun.id_ts.its_rtime
#define icmp_ttime	icmp_dun.id_ts.its_ttime
#define icmp_ip		icmp_dun.id_ip.idi_ip
#define icmp_mask	icmp_dun.id_mask
#define icmp_data	icmp_dun.id_data
};

#define ICMP_ETSDU_SIZE	1
#define ICMP_TIDU_SIZE	(16 * 1024)

/*
 * ICMP_OPT_SIZE is based on the maximum size of all level SOL_SOCKET
 * and IPPROTO_IP options (including option header overhead).
 */

#define ICMP_OPT_SIZE	(SOL_SOCKET_MAXSZ+IPPROTO_IP_MAXSZ)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ICMP_IP_ICMP_F_H */
