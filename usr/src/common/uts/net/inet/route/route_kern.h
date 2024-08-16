/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ROUTE_ROUTE_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_ROUTE_ROUTE_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/route/route_kern.h	1.1"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>			/* REQUIRED */
#include <io/stream.h>			/* REQUIRED */
#include <net/inet/in_f.h>		/* REQUIRED */
#include <net/inet/af.h>		/* REQUIRED */
#include <net/inet/in_systm_f.h>	/* REQUIRED */
#include <net/inet/route/route.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <netinet/in_f.h>		/* REQUIRED */
#include <net/af.h>			/* REQUIRED */
#include <netinet/in_systm_f.h>		/* REQUIRED */
#include <net/route.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * The following flags are used in route functions such as rtfree() and
 * rtrequest() to signify if the calling function should acquire the
 * the route lock.
 */
#define MP_NOLOCK	0
#define MP_LOCK		1

#define BPTORTSTAT(bp) ((struct rtstat *)BPTOSTRUCTPTR((bp), _ALIGNOF_RTSTAT))
#define BPTORTE(bp) \
	((struct rte *)BPTOSTRUCTPTR((bp), _ALIGNOF_RTE))
#define BPTORTENTRY(bp) \
	((struct rtentry *)BPTOSTRUCTPTR((bp), _ALIGNOF_RTENTRY))

extern mblk_t	*rthost[RTHASHSIZ];
extern mblk_t	*rtnet[RTHASHSIZ];

extern struct rtstat	rtstat;

extern int	rtstartup(void);

extern void	rt_inethash(struct in_addr, struct afhash *);
extern void	rtalloc(struct route *);
extern void	rtfree(mblk_t *, int);
extern void	rtredirect(struct in_addr, struct in_addr, int, struct in_addr,
			int, int, time_t);
extern void	rtioctl(int, mblk_t *);
extern int	rtrequest(int, mblk_t *, int);
extern void	rtinit(struct in_addr, struct in_addr, int, int, int, int,
			time_t);
extern void	rtcopy(struct route *, struct route *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ROUTE_ROUTE_KERN_H */
