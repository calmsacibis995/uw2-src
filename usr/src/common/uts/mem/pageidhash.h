/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_PAGEIDHASH_H	/* wrapper symbol for kernel use */
#define _MEM_PAGEIDHASH_H	/* subject to change without notice */

#ident	"@(#)kern:mem/pageidhash.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * Page identity hash tables are powers-of-two in size.  This size is
 * computed at sysinit time by the pageid_compute_hashsz() function.
 *
 * PAGEID_HASHAVELEN is the average length desired for the hash chain
 * attached to a particular hash table slot.  This feeds into the
 * hash size computation in pageid_compute_hashsz().
 *
 * PAGEID_HASHVPSHIFT is defined so that (1 << PAGEID_HASHVPSHIFT) is
 * the approximate size of a vnode struct.
 *
 * PAGEID_RAND_SIZE is the size of pageid_hash_rand[], a table of random
 * numbers used in the PAGEID_HASHFUNC hash function computation.
 */

#define	PAGEID_HASHAVELEN		4
#define	PAGEID_HASHVPSHIFT		7
#define PAGEID_RAND_SIZE		64
#define PAGEID_RAND_SHIFT		6	/* log2(PAGEID_RAND_SIZE) */

#define	PAGEID_HASHFUNC(vp, off, hashsz) \
		((((off) >> PAGESHIFT) ^ \
		  ((uint_t)(vp) >> (PAGEID_HASHVPSHIFT + PAGEID_RAND_SHIFT)) ^ \
		  pageid_hash_rand[((uint_t)(vp) >> PAGEID_HASHVPSHIFT) & \
					(PAGEID_RAND_SIZE - 1)]) & \
			((hashsz) - 1))

extern ushort_t pageid_hash_rand[PAGEID_RAND_SIZE];

extern uint_t pageid_compute_hashsz(uint_t nitems);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_PAGEIDHASH_H */
