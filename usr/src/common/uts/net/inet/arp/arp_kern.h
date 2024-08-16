/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ARP_ARP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_ARP_ARP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/arp/arp_kern.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define QTOAPPPCB(q) ((struct app_pcb *)(q)->q_ptr)
#define QTOARPPCB(q) ((struct arp_pcb *)(q)->q_ptr)

#define N_ARP	10

extern struct app_pcb app_pcb[];
extern struct arp_pcb arp_pcb[];

extern struct arptab arptab[];
extern int arptab_nb;
extern int arptab_bsiz;

extern int appwput(queue_t *, mblk_t *);
extern int arpstartup(void);
extern int arpresolve(struct app_pcb *, mblk_t *, unsigned char *, int *);
extern void arpioctl(queue_t *, mblk_t *);

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ARP_ARP_KERN_H */
