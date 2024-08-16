/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_RFF_H	/* wrapper symbol for kernel use */
#define _MEM_RFF_H	/* subject to change without notice */

#ident	"@(#)kern:mem/rff.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/list.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/list.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structures for managing rff pools.
 *
 * Each chunk of memory in a pool (a pool contains zero or more chunks)
 * is divided into one or more pieces, which are either currently allocated
 * (by an rff_alloc() caller) or free.
 *
 * Each piece has an rff_piece_t structure as a header.  These are linked
 * together in a doubly-linked list, which is in ascending address order.
 * However, instead of two pointers for these links, each rff piece begins
 * with two size counts, the first (rpp_size) for the size, in bytes, of
 * that piece (including the header), and the second (rpp_prevsize) for the
 * size of the previous piece; these counts can be added to or subtracted
 * from the address of the piece to find the next or previous piece.  This
 * list (the next/prev list) is used to facilitate coalescing of adjacent
 * freed pieces.  It also indicates (via rpp_size) the size of a free piece.
 * This list is zero-terminated on both ends.
 *
 * Another link pointer, union'ed with the rff piece data, is used to thread
 * together all of the free pieces for quicker allocation search time.  This
 * list can span multiple memory chunks.  The next/prev list, on the other
 * hand, is local to each chunk.
 */

typedef struct rff_piece {
	ushort_t	rpp_size;	/* size of this piece, inc. header */
	ushort_t	rpp_prevsize;	/* size of prev piece, inc. header */
	union {
		long		_rpp_align;	/* ensure long alignment */
		char		_rpp_data[1];	/* allocated data */
		list_t		_rpp_freelink;	/* freelist links when free */
	} _rpp_u2;
} rff_piece_t;

#define rpp_data	_rpp_u2._rpp_data
#define rpp_freelink	_rpp_u2._rpp_freelink

typedef struct rff {
	rff_piece_t	rff_freelist;	/* head of freelist */
	rff_piece_t	*rff_cursor;	/* next piece to try allocating */
} rff_t;

/*
 * RFF_MINFREE -- minimum size of a free piece.
 */
#define RFF_MINFREE	sizeof(rff_piece_t)

/*
 * RFF_HDRSIZE -- size of an rff piece header.
 */
#define RFF_HDRSIZE	((ulong_t)offsetof(rff_piece_t, rpp_data))

/*
 * RFF_MAXSIZE -- maximum size of an rff piece.
 */
#define RFF_MAXSIZE	(((USHRT_MAX / sizeof(long)) * sizeof(long)) - \
			  RFF_HDRSIZE)

/*
 * ushort_t
 * RFF_ALLOCSIZE(size_t size)
 *	Computes the allocation size (piece size) needed to provide
 *	size bytes of data.
 */
#define RFF_ALLOCSIZE(size)	\
		(RFF_HDRSIZE + \
		 (ushort_t)(((size) + sizeof(long) - 1) / sizeof(long)) * \
		  sizeof(long))

/*
 * boolean_t
 * RFF_ISFREE(rff_piece_t *rpp)
 *	Returns B_TRUE if the piece is currently free.
 */
#define RFF_ISFREE(rpp)	((rpp)->rpp_size >= RFF_MINFREE)

/*
 * rff_piece_t *
 * RFF_NEXTPIECE(rff_piece_t *rpp)
 *	Returns a pointer to the next piece after the given piece.
 */
#define RFF_NEXTPIECE(rpp)	\
		((rff_piece_t *)((vaddr_t)(rpp) + (rpp)->rpp_size))

/*
 * rff_piece_t *
 * RFF_PREVPIECE(rff_piece_t *rpp)
 *	Returns a pointer to the previous piece before the given piece.
 */
#define RFF_PREVPIECE(rpp)	\
		((rff_piece_t *)((vaddr_t)(rpp) - (rpp)->rpp_prevsize))

/*
 * rff_piece_t *
 * RFF_DATATOPIECE(void *datap)
 *	Given a pointer to a piece's data buffer, returns a pointer
 *	to the piece.
 */
#define RFF_DATATOPIECE(datap)	\
		((rff_piece_t *)((vaddr_t)(datap) - RFF_HDRSIZE))

/*
 * rff_piece_t *
 * RFF_LINKTOPIECE(list_t *linkp)
 *	Given a pointer to a piece's freelist link, returns a pointer
 *	to the piece.
 */
#define RFF_LINKTOPIECE(linkp)	\
		((rff_piece_t *)((vaddr_t)(linkp) - RFF_HDRSIZE))

/*
 * rff_piece_t *
 * RFF_NEXTFREE(rff_piece_t *rpp)
 *	Returns a pointer to the next piece on the freelist.
 */
#define RFF_NEXTFREE(rpp)	\
		RFF_LINKTOPIECE((rpp)->rpp_freelink.flink)

/*
 * rff_piece_t *
 * RFF_PREVFREE(rff_piece_t *rpp)
 *	Returns a pointer to the previous piece on the freelist.
 */
#define RFF_PREVFREE(rpp)	\
		RFF_LINKTOPIECE((rpp)->rpp_freelink.rlink)

/*
 * void *
 * RFF_DATA(rff_piece_t *rpp)
 *	Returns a pointer to the data for the piece.
 */
#define RFF_DATA(rpp)		\
		((void *)(rpp)->rpp_data)

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern void rff_init(rff_t *rffp);
extern void rff_add_chunk(rff_t *rffp, void *chunk, size_t csize);
extern void *rff_alloc(rff_t *rffp, size_t size);
extern void rff_free(rff_t *rffp, void *addr, size_t size);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_RFF_H */
