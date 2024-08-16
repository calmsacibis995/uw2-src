/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ICMP_ICMP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_ICMP_ICMP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/icmp/icmp_kern.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_systm.h>		/* REQUIRED */
#include <net/inet/ip/ip.h>		/* REQUIRED */

#define BPTOICMP(bp) ((struct icmp *)BPTOSTRUCTPTR((bp), _ALIGNOF_ICMP))

#define BPTOICMPSTAT(bp) \
	((struct icmpstat *)BPTOSTRUCTPTR((bp), _ALIGNOF_ICMPSTAT))

extern struct icmpstat icmpstat;

extern int icmpstartup(void);
extern void icmp_error(struct ip *, unsigned char, unsigned char, queue_t *,
		       struct in_addr *);

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ICMP_ICMP_KERN_H */
