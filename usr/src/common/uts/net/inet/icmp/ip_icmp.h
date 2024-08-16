/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ICMP_IP_ICMP_H	/* wrapper symbol for kernel use */
#define _NET_INET_ICMP_IP_ICMP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/icmp/ip_icmp.h	1.6"
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

#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_systm.h>		/* REQUIRED */
#include <net/inet/ip/ip.h>		/* REQUIRED */
#include <net/inet/icmp/ip_icmp_f.h>	/* PORTABILITY */
#include <net/inet/icmp/icmp_var.h>	/* COMPATIBILITY */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_systm.h>		/* REQUIRED */
#include <netinet/ip.h>			/* REQUIRED */
#include <netinet/ip_icmp_f.h>		/* PORTABILITY */
#include <netinet/icmp_var.h>		/* COMPATIBILITY */
#include <sys/types.h>			/* REQUIRED */

#else /* user */

#include <netinet/ip_icmp_f.h>		/* PORTABILITY */
#include <netinet/icmp_var.h>		/* COMPATIBILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Lower bounds on packet lengths for various types.  For the error
 * advice packets must first insure that the packet is large enought
 * to contain the returned ip header.  Only then can we do the check
 * to see if 64 bits of packet data have been returned, since we need
 * to check the returned ip header length.
 */
#define ICMP_MINLEN	8				/* abs minimum */
#define ICMP_TSLEN	(8 + 3 * sizeof (n_time))	/* timestamp */
#define ICMP_MASKLEN	12				/* address mask */
#define ICMP_ADVLENMIN	(8 + sizeof (struct ip) + 8)	/* min */
#define ICMP_ADVLEN(p)	(8 + ((p)->icmp_ip.ip_hl << 2) + 8)
	/* N.B.: must separately check that ip_hl >= 5 */

#define ICMP_INFOTYPE(type) \
	((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO || \
	(type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY || \
	(type) == ICMP_IREQ || (type) == ICMP_IREQREPLY || \
	(type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ICMP_IP_ICMP_H */
