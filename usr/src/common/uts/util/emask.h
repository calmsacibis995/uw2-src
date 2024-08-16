/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_EMASK_H	/* wrapper symbol for kernel use */
#define _UTIL_EMASK_H	/* subject to change without notice */

#ident	"@(#)kern:util/emask.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Support for "engine-masks", bit arrays of size MAXNUMCPU bits,
 * one per engine.
 *
 * The basic operations, EMASK_XXX(), correspond to the equivalent BITMAPX_XXX
 * operations (see util/bitmasks.h).  Depending on how large MAXNUMCPU is,
 * EMASK_XXX is mapped to BITMAP1_XXX, BITMAP2_XXX or BITMAPN_XXX.  This allows
 * a single EMASK_XXX to be used, but the actual implementation is tailored
 * for optimal efficiency.
 *
 * Available operations are:
 *	CLRALL		Clear all bits in the bit array.
 *	SETALL		Set all bits in the bit array.
 *	TESTALL		Return non-zero if any bit is set.
 *	CLR1		Clear an individual bit.
 *	SET1		Set an individual bit.
 *	TEST1		Test one bit; return non-zero if bit is set.
 *	CLRN		Clear all bits in dst_emask which are set in src_emask.
 *	SETN		Set all bits in dst_emask which are set in src_emask.
 *	TESTN		Return non-zero if any bit is set in both bit arrays.
 *	INIT		Clear all bits except one specified bit, which is set.
 *	FFS		"Find First Set"; return the bit number of the first
 *			set bit, starting from 0; if none, return -1.
 *	FLS		"Find Last Set"; return the bit number of the highest
 *			set bit; if none, return -1.
 *	FFSCLR		"Find First Set and Clear"; return the bit number
 *			of the first set bit, starting from 0, after clearing
 *			that bit; if no bits were set, return -1.
 *	FLSCLR		"Find Last Set and Clear"; return the bit number
 *			of the highest set bit, starting from 0, after clearing
 *			that bit; if no bits were set, return -1.
 *
 * In addition, there are some operations to combine regular engine-masks
 * with special single-engine engine-masks (see emask_single_t, below).
 * They are:
 *
 *	S_INIT		Set the value of an emask_single_t to have the (one)
 *			specified bit set.
 *	CLRS		Clear the bit in dst_emask which is set in src_emask_s.
 *	SETS		Set the bit in dst_emask which is set in src_emask_s.
 *	CLRS		Return non-zero if a bit is set in dst_emask which is
 *			set in src_emask_s.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/bitmasks.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/bitmasks.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

	/* Number of words (ints) needed for MAXNUMCPU bits */
#define EMASK_WORDS	BITMASK_NWORDS(MAXNUMCPU)

/*
 * An emask_t is the regular engine-mask structure.  It holds a set of engines.
 */
typedef struct emask {
	uint_t	em_bits[EMASK_WORDS];
} emask_t;

/*
 * An emask_single_t is a special engine-mask structure optimized for the
 * case of only one engine bit set.  An emask_single_t can only be used with
 * EMASK_S_INIT() or as the src argument of EMASK_CLRS(), EMASK_SETS() or
 * EMASK_TESTS().
 */
typedef struct emask_single {
#if (EMASK_WORDS > 1)
	uint_t	em_idx;		/* Word index into bitmask */
#endif
	uint_t	em_mask;	/* em_bits[em_idx] */
} emask_single_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

#if (EMASK_WORDS == 1)

#define EMASK_CLRALL(emask)	BITMASK1_CLRALL((emask)->em_bits)
#define EMASK_SETALL(emask)	BITMASK1_SETALL((emask)->em_bits)
#define EMASK_TESTALL(emask)	BITMASK1_TESTALL((emask)->em_bits)
#define EMASK_CLR1(emask, engnum)	\
		BITMASK1_CLR1((emask)->em_bits, engnum)
#define EMASK_SET1(emask, engnum)	\
		BITMASK1_SET1((emask)->em_bits, engnum)
#define EMASK_TEST1(emask, engnum)	\
		BITMASK1_TEST1((emask)->em_bits, engnum)
#define EMASK_CLRN(dst_emask, src_emask)	\
		BITMASK1_CLRN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_SETN(dst_emask, src_emask)	\
		BITMASK1_SETN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_TESTN(dst_emask, src_emask)	\
		BITMASK1_TESTN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_INIT(emask, engnum)	\
		BITMASK1_INIT((emask)->em_bits, engnum)
#define EMASK_FFS(emask)	BITMASK1_FFS((emask)->em_bits)
#define EMASK_FLS(emask)	BITMASK1_FLS((emask)->em_bits)
#define EMASK_FFSCLR(emask)	BITMASK1_FFSCLR((emask)->em_bits)
#define EMASK_FLSCLR(emask)	BITMASK1_FLSCLR((emask)->em_bits)

#elif (EMASK_WORDS == 2)

#define EMASK_CLRALL(emask)	BITMASK2_CLRALL((emask)->em_bits)
#define EMASK_SETALL(emask)	BITMASK2_SETALL((emask)->em_bits)
#define EMASK_TESTALL(emask)	BITMASK2_TESTALL((emask)->em_bits)
#define EMASK_CLR1(emask, engnum)	\
		BITMASK2_CLR1((emask)->em_bits, engnum)
#define EMASK_SET1(emask, engnum)	\
		BITMASK2_SET1((emask)->em_bits, engnum)
#define EMASK_TEST1(emask, engnum)	\
		BITMASK2_TEST1((emask)->em_bits, engnum)
#define EMASK_CLRN(dst_emask, src_emask)	\
		BITMASK2_CLRN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_SETN(dst_emask, src_emask)	\
		BITMASK2_SETN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_TESTN(dst_emask, src_emask)	\
		BITMASK2_TESTN((dst_emask)->em_bits, (src_emask)->em_bits)
#define EMASK_INIT(emask, engnum)	\
		BITMASK2_INIT((emask)->em_bits, engnum)
#define EMASK_FFS(emask)	BITMASK2_FFS((emask)->em_bits)
#define EMASK_FLS(emask)	BITMASK2_FLS((emask)->em_bits)
#define EMASK_FFSCLR(emask)	BITMASK2_FFSCLR((emask)->em_bits)
#define EMASK_FLSCLR(emask)	BITMASK2_FLSCLR((emask)->em_bits)

#else

#define EMASK_CLRALL(emask)	BITMASKN_CLRALL((emask)->em_bits, EMASK_WORDS)
#define EMASK_SETALL(emask)	BITMASKN_SETALL((emask)->em_bits, EMASK_WORDS)
#define EMASK_TESTALL(emask)	BITMASKN_TESTALL((emask)->em_bits, EMASK_WORDS)
#define EMASK_CLR1(emask, engnum)	\
		BITMASKN_CLR1((emask)->em_bits, engnum)
#define EMASK_SET1(emask, engnum)	\
		BITMASKN_SET1((emask)->em_bits, engnum)
#define EMASK_TEST1(emask, engnum)	\
		BITMASKN_TEST1((emask)->em_bits, engnum)
#define EMASK_CLRN(dst_emask, src_emask)	\
		BITMASKN_CLRN((dst_emask)->em_bits, (src_emask)->em_bits, \
			      EMASK_WORDS)
#define EMASK_SETN(dst_emask, src_emask)	\
		BITMASKN_SETN((dst_emask)->em_bits, (src_emask)->em_bits, \
			      EMASK_WORDS)
#define EMASK_TESTN(dst_emask, src_emask)	\
		BITMASKN_TESTN((dst_emask)->em_bits, (src_emask)->em_bits, \
			      EMASK_WORDS)
#define EMASK_INIT(emask, engnum)	\
		BITMASKN_INIT((emask)->em_bits, EMASK_WORDS, engnum)
#define EMASK_FFS(emask)	BITMASKN_FFS((emask)->em_bits, EMASK_WORDS)
#define EMASK_FLS(emask)	BITMASKN_FLS((emask)->em_bits, EMASK_WORDS)
#define EMASK_FFSCLR(emask)	BITMASKN_FFSCLR((emask)->em_bits, EMASK_WORDS)
#define EMASK_FLSCLR(emask)	BITMASKN_FLSCLR((emask)->em_bits, EMASK_WORDS)

#endif

#if (EMASK_WORDS == 1)

#define EMASK_S_INIT(emask_s, engnum) \
		BITMASK1_INIT(&(emask_s)->em_mask, engnum)
#define EMASK_CLRS(dst_emask, src_emask_s)	\
		BITMASK1_CLRN((dst_emask)->em_bits, &(src_emask_s)->em_mask)
#define EMASK_SETS(dst_emask, src_emask_s)	\
		BITMASK1_SETN((dst_emask)->em_bits, &(src_emask_s)->em_mask)
#define EMASK_TESTS(dst_emask, src_emask_s)	\
		BITMASK1_TESTN((dst_emask)->em_bits, &(src_emask_s)->em_mask)

#else

#define EMASK_S_INIT(emask_s, engnum) \
		(((emask_s)->em_idx = (engnum) / NBITPW), \
		 BITMASK1_INIT(&(emask_s)->em_mask, (engnum) % NBITPW))
#define EMASK_CLRS(dst_emask, src_emask_s)	\
		BITMASK1_CLRN(&(dst_emask)->em_bits[(src_emask_s)->em_idx], \
			      &(src_emask_s)->em_mask)
#define EMASK_SETS(dst_emask, src_emask_s)	\
		BITMASK1_SETN(&(dst_emask)->em_bits[(src_emask_s)->em_idx], \
			      &(src_emask_s)->em_mask)
#define EMASK_TESTS(dst_emask, src_emask_s)	\
		BITMASK1_TESTN(&(dst_emask)->em_bits[(src_emask_s)->em_idx], \
			      &(src_emask_s)->em_mask)

#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_EMASK_H */
