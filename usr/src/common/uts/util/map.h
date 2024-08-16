/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MAP_H	/* wrapper symbol for kernel use */
#define _UTIL_MAP_H	/* subject to change without notice */

#ident	"@(#)kern:util/map.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Generic support for resource allocation maps.
 *
 * Resource maps are structured as follows, but DDI/DKI drivers must not
 * depend on any of these details; they should only access maps through the
 * DDI/DKI function interfaces.
 *
 *		struct map	X[]	.m_size		.m_addr
 *				---	------------	-----------
 *				[0]	mapsize(X)	mapwant(X)
 *					# X[] unused	sleep count
 *
 *				[1]	flags		synchronization ptr
 *
 *		  mapstart(X)->	[2]	# units		unit number
 *				 :	    :		  :
 *				[ ]	    0
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

struct map
{
	ulong_t	m_size;		/* number of units available */
	ulong_t	m_addr;		/* address of first available unit */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#if defined(__STDC__)
extern ulong_t rmalloc(struct map *, size_t);
extern ulong_t rmalloc_wait(struct map *, size_t);
extern struct map *rmallocmap(ulong_t);
extern void rmfree(struct map *, size_t, ulong_t);
extern void rmfreemap(struct map *);
#else
extern ulong_t rmalloc();
extern ulong_t rmalloc_wait();
extern struct map *rmallocmap();
extern void rmfree();
extern void rmfreemap();
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MAP_H */
