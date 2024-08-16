/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ARP_ARP_H	/* wrapper symbol for kernel use */
#define _NET_INET_ARP_ARP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/arp/arp.h	1.6"
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
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989	 Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *		  All rights reserved.
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/if_ether.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <net/if.h>			/* REQUIRED */
#include <netinet/if_ether.h>		/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * PCB's shared by app module and arp driver
 */

#ifdef _KERNEL

struct app_pcb {
	queue_t		*app_q;		/* pointer to our read queue */
	struct arp_pcb	*arp_pcb;	/* cross pointer to our arp module */
	char		app_uname[IFNAMSIZ];	/* enet unit name */
	struct arpcom	app_ac;		/* common structure for this unit */
	int		app_closing;	/* has a close started for this pcb */
	atomic_int_t	app_inuse;	/* uses not protected by qprocsoff */
};

struct arp_pcb {
	queue_t		*arp_qtop;	/* upstream read queue */
	queue_t		*arp_qbot;	/* downstream write queue */
	int		arp_index;	/* mux index for link */
	struct app_pcb	*app_pcb;	/* cross pointer to app module */
	char		arp_uname[IFNAMSIZ];	/* enet unit name */
	mblk_t		*arp_saved;	/* saved input request */
	int		arp_flags;	/* flags */
	atomic_int_t	arp_qbot_inuse;	/* reference count */
	atomic_int_t	arp_qtop_inuse;	/* reference count */
};

#else   /* _KERNEL */

struct app_pcb {
	void		*app_q;		/* pointer to our read queue */
	struct arp_pcb	*arp_pcb;	/* cross pointer to our arp module */
	char		app_uname[IFNAMSIZ];	/* enet unit name */
	struct arpcom	app_ac;		/* common structure for this unit */
	int		app_closing;	/* has a close started for this pcb */
	int		app_inuse;	/* uses not protected by qprocsoff */
};

struct arp_pcb {
	void		*arp_qtop;	/* upstream read queue */
	void		*arp_qbot;	/* downstream write queue */
	int		arp_index;	/* mux index for link */
	struct app_pcb	*app_pcb;	/* cross pointer to app module */
	char		arp_uname[IFNAMSIZ];	/* enet unit name */
	void		*arp_saved;	/* saved input request */
	int		arp_flags;	/* flags */
	int		arp_qbot_inuse;	/* reference count */
	int		arp_qtop_inuse;	/* reference count */
};

#endif /* _KERNEL */

extern int arptab_size;		    /* for ARP command */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ARP_ARP_H */
