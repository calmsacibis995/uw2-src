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

#ifndef _NET_INET_IN_PCB_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_PCB_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in_pcb.h	1.15"
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
 *	STREAMware TCP/IP Release 1.0
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
#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/route/route.h>	/* REQUIRED */
#include <net/inet/route/route_kern.h>	/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <net/route.h>			/* REQUIRED */
#include <net/route_kern.h>		/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#elif defined(_KMEMUSER)

#include <net/route_kern.h>		/* REQUIRED */
#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_pcb.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

#define	INET_RETRY_CNT	1000000

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct inpcb {
	struct inpcb	*inp_next;
	struct inpcb	*inp_prev;
	/* pointers to other pcb's */
	struct inpcb	*inp_head;	/* pointer back to chain of inpcb's
					 * for this protocol */
	short		inp_state;	/* old so_state from sockets */
	short		inp_tstate;	/* TLI state for this endpoint */
	short		inp_error;	/* error on this pcb */
	minor_t		inp_minor;	/* minor device number allocated */
	queue_t		*inp_q;		/* queue for this minor dev */
	struct in_addr	inp_faddr;	/* foreign host table entry */
	struct in_addr	inp_laddr;	/* local host table entry */
	unsigned short	inp_fport;	/* foreign port */
	unsigned short	inp_lport;	/* local port */
#define inp_proto	inp_lport	/* overload port field for protocol */
	void		*inp_ppcb;	/* pointer to per-protocol pcb */
	struct route	inp_route;	/* placeholder for routing entry */
	mblk_t		*inp_options;	/* IP options */
	mblk_t		*inp_inpopts;	/* recvd IP options */
	unsigned short	inp_protoopt;	/* old so_options from sockets */
	unsigned short	inp_linger;	/* time to linger while closing */
	unsigned short	inp_protodef;	/* old pr_flags from sockets */
	unsigned short	inp_iocstate;	/* state for transparent ioctls */
	int		inp_addrlen;	/* address length client likes */
	int		inp_family;	/* address family client likes */
	int		inp_iptos;	/* IP type of service */
	rwlock_t	*inp_rwlck;	/* protects this in_pcb */
	atomic_int_t	inp_qref;	/* inp_q reference count */
	int		inp_closing;	/* this in_pcb is closing down */
	unsigned int	inp_flags;
#define INPF_FREE	0x000001	/* OK for sweeper to free this inp */
#define INPF_BSWAP	0x000002	/* AF_INET_BSWAP used for connect */
#define INPF_TLI	0x000004	/* TLI compatibility mode */
};

#ifdef _KERNEL
/*
 * common structure to hold bottom queue information
 */
struct in_bot {
	queue_t *bot_q;			/* points to bottom queue */
	int	bot_index;		/* mux id of lower stream */
	lock_t	*bot_lck;		/* lock protects bot_q and bot_ref */
	atomic_int_t	bot_ref;	/* reference count */
};
#endif /* _KERNEL */
#endif /* _KERNEL || _KMEMUSER */

/* this structure is for passing pcb information to netstat via ioctl */

struct pcbrecord {
	void		*inp_addr;	/* address of this pcb */
	struct in_addr	inp_faddr;	/* foreign host table entry */
	struct in_addr	inp_laddr;	/* local host table entry */
	unsigned short	inp_fport;	/* foreign port */
	unsigned short	inp_lport;	/* local port */
	unsigned long	t_outqsize;	/* tcp's output qsize */
	unsigned long	t_iqsize;	/* tcp's input qsize */
	short		t_state;	/* tcp state */
};

/*
 * inp_iocstate tells us which transparent ioctl we are in the process
 * of handling.	 inp_iocstate is usually set when the M_IOCTL message
 * for a transparent ioctl first seen.	It is used to decide what to do
 * when the subsequent associated M_IOCDATA message(s) arrive.
 * inp_iocstate == 0 means we are not currently processing any
 * transparent ioctls.
 */
#define INP_IOCS_DONAME		1

#define INPLOOKUP_WILDCARD	1
#define INPLOOKUP_SETLOCAL	2

#ifdef _KERNEL
extern struct inpcb *in_pcballoc(struct inpcb *, lkinfo_t *, int);
extern int in_pcbbind(struct inpcb *, unsigned char *, int len);
extern int in_pcbconnect(struct inpcb *, unsigned char *, int);
extern void in_pcbdisconnect(struct inpcb *);
extern void in_pcbdetach(struct inpcb *, pl_t);
extern struct inpcb *in_pcblookup(struct inpcb *, struct in_addr,
				  unsigned short, struct in_addr,
				  unsigned short, int);

extern struct in_addr zeroin_addr;

#define QTOINP(q) ((struct inpcb *)(q)->q_ptr)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IN_PCB_H */
