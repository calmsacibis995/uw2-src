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

#ifndef _NET_INET_IN_VAR_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_VAR_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in_var.h	1.4"
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

#include <net/inet/af.h>		/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/in_pcb.h>		/* REQUIRED */
#include <net/inet/in_systm.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <net/af.h>			/* REQUIRED */
#include <net/if.h>			/* REQUIRED */
#include <netinet/in_pcb.h>		/* REQUIRED */
#include <netinet/in_systm.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Interface address, Internet version.	 One of these structures
 * is allocated for each interface with an Internet address.
 * The ifaddr structure contains the protocol-independent part
 * of the structure and is assumed to be first.
 */
struct in_ifaddr {
	struct ifaddr	  ia_ifa;	/* protocol-independent info */
#define ia_addr		  ia_ifa.ifa_addr
#define ia_broadaddr	  ia_ifa.ifa_broadaddr
#define ia_dstaddr	  ia_ifa.ifa_dstaddr
#define ia_ifp		  ia_ifa.ifa_ifp
	unsigned long	  ia_net;	/* network number of interface */
	unsigned long	  ia_netmask;	/* mask of net part */
	unsigned long	  ia_subnet;	/* subnet number, including net */
	unsigned long	  ia_subnetmask; /* mask of net + subnet */
	struct in_addr	  ia_netbroadcast; /* broadcast addr for logical net */
	int		  ia_flags;
	struct in_ifaddr *ia_next;	/* to next inet addresses in list */
};

/*
 * Given a pointer to an in_ifaddr (ifaddr), return a pointer to the
 * addr as a sockadd_in.
 */
#define IA_SIN(ia) ((struct sockaddr_in *)(&((struct in_ifaddr *)ia)->ia_addr))
/*
 * ia_flags
 */
#define IFA_ROUTE	0x01		/* routing entry installed */

/*
 * This structure is for passing interface info to netstat via ioctls,
 * contains no pointers
 */
struct ifrecord {
	char		ifs_name[IFNAMSIZ]; /* interface name */
	short		ifs_unit;	/* unit number */
	short		ifs_active;	/* non-zero if this if is running */
	short		ifs_addrcnt;	/* no. of addr in the list below */
	struct in_ifaddr ifs_addrs[10];	/* list of addresses */
	short		ifs_mtu;	/* Maximum transmission unit */
	int		ifs_ipackets;	/* packets received on interface */
	int		ifs_ierrors;	/* input errors on interface */
	int		ifs_opackets;	/* packets sent on interface */
	int		ifs_oerrors;	/* output errors on interface */
	int		ifs_collisions;	/* collisions on csma interfaces */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IN_VAR_H */
