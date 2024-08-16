/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifndef	_NET_INET_IF_ARP_H	/* wrapper symbol for kernel use */
#define _NET_INET_IF_ARP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/if_arp.h	1.5"
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

#include <net/inet/if_arp_f.h>		/* PORTABILITY */
#include <net/socket.h>			/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if_arp_f.h>		/* PORTABILITY */
#include <sys/socket.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else /* user */

#include <net/if_arp_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Types of hardware addresses
 */
#define ARPHRD_ETHER	1		/* ethernet hardware address */
#define ARPHRD_802	6		/* 802 hardware type 	 */

/*
 * ARP Operation types
 */
#define ARPOP_REQUEST	1		/* request to resolve address */
#define ARPOP_REPLY	2		/* response to previous request */
#define REVARP_REQUEST	3		/* Reverse ARP request */
#define REVARP_REPLY	4		/* Reverse ARP reply */

/*
 * ARP ioctl request
 */
struct arpreq {
	struct sockaddr	arp_pa;		/* protocol address */
	struct sockaddr	arp_ha;		/* hardware address */
	int		arp_flags;	/* flags */
};

#ifdef _KERNEL
#define BPTOARPREQ(bp) ((struct arpreq *)BPTOSTRUCTPTR((bp), _ALIGNOF_ARPREQ))
#define BPTOARPHDR(bp) ((struct arphdr *)BPTOSTRUCTPTR((bp), _ALIGNOF_ARPHDR))
#endif /* _KERNEL */

/* arp_flags and at_flags field values */
#define ATF_INUSE	0x01		/* entry in use */
#define ATF_COM		0x02		/* completed entry (enaddr valid) */
#define ATF_PERM	0x04		/* permanent entry */
#define ATF_PUBL	0x08		/* publish entry (respond for other
					 * host) */
#define ATF_USETRAILERS	0x10		/* has requested trailers */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IF_ARP_H */
