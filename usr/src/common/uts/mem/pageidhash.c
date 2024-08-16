/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/pageidhash.c	1.3"
#ident	"$Header: $"

#include <mem/pageidhash.h>
#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/types.h>

/*
 * The pageid_hash_rand array is used as part of the PAGEID_HASHFUNC()
 * hash function computation, to inject some randomness.
 */
ushort_t pageid_hash_rand[PAGEID_RAND_SIZE] = {
	0x310f, 0x5ceb, 0x8b5a, 0xa938, 0x92b6, 0x0348, 0x4901, 0xf402,
	0x18a4, 0xb4bf, 0xc289, 0xa9a9, 0x589b, 0x8aa6, 0x89bb, 0x715c,
	0x4c63, 0x2abd, 0xc1f8, 0xec22, 0x6c3d, 0x0549, 0x79b8, 0xda48,
	0xf318, 0x3b56, 0x0523, 0x5143, 0xb2b7, 0xc713, 0x14b2, 0xe3c7,
	0x23ff, 0xa00c, 0x8cff, 0xb6b5, 0xa354, 0xd600, 0xaab8, 0xbbf8,
	0x8abf, 0x6d41, 0x65a2, 0xe35b, 0xf7e7, 0xef5d, 0x54b7, 0x444b,
	0x1a1b, 0x16af, 0x306d, 0x8658, 0x1bf8, 0xaa25, 0x60a0, 0x0f11,
	0xe57c, 0x65c4, 0x6054, 0x9833, 0x2cd7, 0x7507, 0x7bfa, 0x50d6
};


/*
 * uint_t
 * pageid_compute_hashsz(uint_t nitems)
 *	Compute a hash table size.
 *
 * Calling/Exit State:
 *	Called at sysinit time, while still single-threaded.
 */
uint_t
pageid_compute_hashsz(uint_t nitems)
{
	uint_t hashsz;
	int bit;

	/*
	 * Compute desired number of hash slots to cover nitems items,
	 * based on a target average hash length of PAGEID_HASHAVELEN.
	 */
	hashsz = (nitems + PAGEID_HASHAVELEN - 1) / PAGEID_HASHAVELEN;

	/*
	 * Establish a minimum hash table size of 64.
	 */
	if (hashsz < 64)
		hashsz = 64;

	/*
	 * Round hash table size to nearest power of two.
	 */
	bit = BITMASK1_FLS(&hashsz);
	ASSERT(bit >= 1);
	if (BITMASK1_TEST1(&hashsz, bit - 1))
		bit++;
	BITMASK1_INIT(&hashsz, bit);

	return hashsz;
}
