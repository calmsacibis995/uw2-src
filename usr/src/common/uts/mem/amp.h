/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _AMP_H	/* wrapper symbol for kernel use */
#define _AMP_H	/* subject to change without notice */

#ident	"@(#)kern:mem/amp.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * VM - segment driver support for maps of anonymous memory.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The anon_map structure is used by the seg_vn segment driver to manage
 * anonfs backed PRIVATE mappings. Anon structures (`slots') are pointed
 * to and managed via a dynamically allocated array of pointers.
 * The am_anon field is used for this purpose.
 *
 * If a segment using an anon_map is partially unmapped in its middle
 * making two new segments, it becomes necessary to indicate which portion of
 * the anon array is still in use by each segment. This information is
 * managed on a per-segment basis (since segments may be sharing the anon
 * map). Similiar effects occur when the segment is trimmed (unmapped) from
 * either end.
 *
 * Because trimming, hole-punching, and concatenation (optimization)
 * operations cause a segment to use less than the entire range of the
 * initially created anon array, we need to store the initial size of the
 * array in the anon_map structure (am_size) so that we can free the entire
 * array when the reference count on the anon_map finally goes to zero.
 * The combination of am_size and svd_anon_index tells us how much unused
 * anon map space is available for concatenation with an adjacent segment.
 *
 * NOTES on locking for the anon_map structure:
 * -------------------------------------------
 *
 * am_npageget			mutexed by am_lock (DEBUG case only).
 *
 * am_refcnt, am_size, am_anon	mutexed by the AS lock
 *
 * Since the anon_map is never shared across address spaces, it suffices
 * to use the AS lock to mutex the anon map fields. Anon map instantiation
 * as well as changes to the anon map slots are mutexed by segment level
 * locking (see kern:mem/seg_vn.h).
 */
struct anon_map {
	ushort_t am_refcnt;	/* reference count on this structure */
	size_t	am_size;	/* size in bytes mapped by the anon array */
	struct	anon **am_anon;	/* pointer to an array of anon * pointers */
#ifdef DEBUG
	lock_t	am_lock;	/* lock for this data structure */
	uint_t	am_npageget;	/* number of anon_getpage(s) */
#endif /* DEBUG */
};

#ifdef _KERNEL

struct page;
struct anon_map;
extern struct anon_map *amp_alloc(size_t);
extern void amp_release(struct anon_map *, uint_t, size_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _AMP_H */
