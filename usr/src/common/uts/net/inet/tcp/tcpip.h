/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCPIP_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCPIP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcpip.h	1.7"
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
 *		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *	(c) 1986,1987,1988,1989	 Sun Microsystems, Inc.
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 * All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.	 The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.	This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/ip/ip_var.h>		/* REQUIRED */
#include <net/inet/tcp/tcp.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <netinet/ip_var.h>		/* REQUIRED */
#include <netinet/tcp.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * TCP+IP header, after IP options removed.
 */
struct tcpiphdr {
	struct	ipovly ti_i;		/* overlaid IP structure */
	struct	tcphdr ti_t;		/* TCP header */
};

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#define ti_next		ti_i.ih_next
#define ti_mblk		ti_i.ih_mblk
#define ti_x1		ti_i.ih_x1
#define ti_pr		ti_i.ih_pr
#define ti_len		ti_i.ih_len
#define ti_src		ti_i.ih_src
#define ti_dst		ti_i.ih_dst
#define ti_sport	ti_t.th_sport
#define ti_dport	ti_t.th_dport
#define ti_seq		ti_t.th_seq
#define ti_ack		ti_t.th_ack
#define ti_x2		ti_t.th_x2
#define ti_off		ti_t.th_off
#define ti_flags	ti_t.th_flags
#define ti_win		ti_t.th_win
#define ti_sum		ti_t.th_sum
#define ti_urp		ti_t.th_urp

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCPIP_H */
