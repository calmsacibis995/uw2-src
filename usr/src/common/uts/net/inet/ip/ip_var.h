/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_VAR_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_VAR_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ip/ip_var.h	1.5"
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
 *	STREAMware TCP/IP Release
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
#include <net/inet/byteorder.h>		/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_pcb.h>		/* REQUIRED */
#include <net/inet/ip/ip.h>		/* REQUIRED */
#include <net/inet/ip/ip_var_f.h>	/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if.h>			/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_pcb.h>		/* REQUIRED */
#include <netinet/ip.h>			/* REQUIRED */
#include <netinet/ip_var_f.h>		/* REQUIRED */
#include <sys/byteorder.h>		/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else  /* user */

#include <netinet/ip_var_f.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Get a pointer to an IP header from a pointer to a fragment */

#define IPHDR(ip)	((struct ip *)(ip))
#define IPASFRAG(ip)	((struct ipasfrag *)(ip))

/*
 * Structure stored in mbuf in inpcb.ip_options and passed to
 * ip_output when ip options are in use.  The actual length of the
 * options (including ipopt_dst) is in m_len.
 */
#define MAX_IPOPTLEN	40

struct ipoption {
	struct in_addr ipopt_dst;	/* first-hop dst if source routed */
	char ipopt_list[MAX_IPOPTLEN];	/* options proper */
};

struct ipstat {
	unsigned int	ips_total;	/* total packets received */
	unsigned int	ips_badsum;	/* checksum bad */
	unsigned int	ips_tooshort;	/* packet too short */
	unsigned int	ips_toosmall;	/* not enough data */
	unsigned int	ips_badhlen;	/* ip header length < data size */
	unsigned int	ips_badlen;	/* ip length < ip header length */
	unsigned int	ips_fragments;	/* fragments received */
	unsigned int	ips_fragdropped; /* frags dropped/dups out of space */
	unsigned int	ips_fragtimeout; /* fragments timed out */
	unsigned int	ips_forward;	/* packets forwarded */
	unsigned int	ips_cantforward; /* pkts rcvd for unreachable dest */
	unsigned int	ips_redirectsent; /* packets forwarded on same net */
	unsigned int	ips_unknownproto; /* unknown protocol */
	unsigned int	ips_inerrors;	/* input errors */
	unsigned int	ips_indelivers;	/* packets delivered */
	unsigned int	ips_outrequests; /* ip packets sent */
	unsigned int	ips_outerrors;	/* output errors */
	unsigned int	ips_noroutes;	/* no route found */
	unsigned int	ips_reasms;	/* packets reassembled */
	unsigned int	ips_pfrags;	/* packets fragmented */
	unsigned int	ips_fragfails;	/* packets which couldn't be fragged */
	unsigned int	ips_frags;	/* fragments created */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IP_IP_VAR_H */
