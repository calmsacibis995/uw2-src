/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ROUTE_ROUTE_MP_H	/* wrapper symbol for kernel use */
#define _NET_INET_ROUTE_ROUTE_MP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/route/route_mp.h	1.1"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

extern rwlock_t *rthost_rwlck[RTHASHSIZ];
extern rwlock_t *rtnet_rwlck[RTHASHSIZ];

#define RTSTAT_INC(s)	(++rtstat.s)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_ROUTE_ROUTE_MP_H */
