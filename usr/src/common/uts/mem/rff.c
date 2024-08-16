/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/rff.c	1.1"
#ident	"$Header: $"

/*
 * Rotating First Fit allocator
 *
 * These routines manage blocks of memory which are sub-allocated in
 * (small) pieces using a rotating first-fit algorithm.
 */

#include <mem/rff.h>
#include <util/debug.h>
#include <util/list.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * void
 * rff_init(rff_t *rffp)
 *	Initialize rotating first-fit allocation.
 *
 * Calling/Exit State:
 *	(*rffp) is a privately-held structure for maintaining a pool of
 *	memory for allocation by rff_alloc().
 */
void
rff_init(rff_t *rffp)
{
	INITQUE(&rffp->rff_freelist.rpp_freelink);
	rffp->rff_cursor = &rffp->rff_freelist;
}


/*
 * void
 * rff_add_chunk(rff_t *rffp, void *chunk, size_t csize)
 *	Add a chunk of memory to a rotating first-fit allocation pool.
 *
 * Calling/Exit State:
 *	(*rffp) is an rrf_t structure for maintaining a pool of memory
 *	for allocation by rff_alloc(); the caller is responsible for
 *	serializing access to this structure.
 *
 *	chunk points to csize bytes of memory which is to be added into
 *	the pool for (*rffp).
 */
void
rff_add_chunk(rff_t *rffp, void *chunk, size_t csize)
{
	rff_piece_t *rpp;

	ASSERT(chunk != NULL);
	ASSERT(csize > 2 * RFF_HDRSIZE);

	/*
	 * Initialize the chunk to have two pieces:
	 * one free piece which covers most of the chunk,
	 * followed by one zero-length "allocated" chunk
	 * which is used to terminate the list of pieces.
	 */
	rpp = chunk;
	rpp->rpp_size = (ushort_t)(csize -= RFF_HDRSIZE);
	rpp->rpp_prevsize = 0;
	insque(&rpp->rpp_freelink,
	       &rffp->rff_freelist.rpp_freelink);
	rffp->rff_cursor = rpp;
	rpp = RFF_NEXTPIECE(rpp);
	rpp->rpp_size = 0;
	rpp->rpp_prevsize = (ushort_t)csize;
}


/*
 * void *
 * rff_alloc(rff_t *rffp, size_t size)
 *	Allocate memory using rotating first-fit.
 *
 * Calling/Exit State:
 *	Returns a pointer to size bytes of memory allocated out of the
 *	space maintained by rffp, or NULL on failure.
 *
 *	size must be less than or equal to RFF_MAXSIZE.
 *	size may not be zero.
 *
 *	The caller is responsible for serializing access to this rff
 *	resource (*rffp).
 */
void *
rff_alloc(rff_t *rffp, size_t size)
{
	rff_piece_t *rpp, *rpp2;
	ushort_t asize;

	ASSERT(size != 0);
	ASSERT(size <= RFF_MAXSIZE);

	/* Compute actual allocation size, including header. */
	asize = RFF_ALLOCSIZE(size);

	/* Look for an existing free block which is big enough. */
	rpp2 = rpp = rffp->rff_cursor;
	while (rpp->rpp_size < asize) {
		rpp = RFF_NEXTFREE(rpp);
		if (rpp == rpp2)
			return NULL;
	}

	/*
	 * See if there's enough room to split the space into two pieces.
	 */
	if (rpp->rpp_size - asize >= RFF_MINFREE) {
		/*
		 * Split this piece into two pieces.  Leave the first one
		 * on the free list and return the second one.
		 */
		RFF_NEXTPIECE(rpp)->rpp_prevsize = asize;
		rpp->rpp_size -= asize;
		rpp2 = RFF_NEXTPIECE(rpp);
		rpp2->rpp_size = 0;
		rpp2->rpp_prevsize = rpp->rpp_size;
		rffp->rff_cursor = rpp;
		rpp = rpp2;
	} else {
		rpp->rpp_size -= asize;
		rffp->rff_cursor = RFF_NEXTFREE(rpp);
		remque(&rpp->rpp_freelink);
	}

	return RFF_DATA(rpp);
}


/*
 * void
 * rff_free(rff_t *rffp, void *addr, size_t size)
 *	Free memory allocated by rff_alloc().
 *
 * Calling/Exit State:
 *	The procp and size arguments must be the same as those passed
 *	to the rff_alloc() call which allocated the object at addr.
 *
 *	This routine does not block.
 *	Ublock locks are used internally, but no locks are required on
 *	entry or held on exit.
 */
void
rff_free(rff_t *rffp, void *addr, size_t size)
{
	rff_piece_t *rpp, *rpp2, *prpp;
	size_t asize;

	ASSERT(addr != NULL);
	ASSERT(size != 0);
	ASSERT(size <= RFF_MAXSIZE);

	/* Compute actual allocation size, including header. */
	asize = RFF_ALLOCSIZE(size);

	rpp = RFF_DATATOPIECE(addr);

	ASSERT(!RFF_ISFREE(rpp));

	rpp->rpp_size += asize;

	rpp2 = RFF_NEXTPIECE(rpp);

	ASSERT(rpp2->rpp_prevsize == rpp->rpp_size);

	/* Try coalescing with previous piece. */
	if (rpp->rpp_prevsize != 0 &&
	    RFF_ISFREE(prpp = RFF_PREVPIECE(rpp))) {
		rpp2->rpp_prevsize = (prpp->rpp_size += rpp->rpp_size);
		/* Try coalescing with next piece, too. */
		if (RFF_ISFREE(rpp2)) {
			RFF_NEXTPIECE(rpp2)->rpp_prevsize =
					(prpp->rpp_size += rpp2->rpp_size);
			/* Unlink second piece from free list. */
			remque(&rpp2->rpp_freelink);
			if (rffp->rff_cursor == rpp2)
				rffp->rff_cursor = prpp;
		}
	} else {
		/* Try coalescing with next piece. */
		if (RFF_ISFREE(rpp2)) {
			RFF_NEXTPIECE(rpp2)->rpp_prevsize =
					(rpp->rpp_size += rpp2->rpp_size);
			/* Relocate free list pointers. */
			rpp->rpp_freelink.flink =
					&RFF_NEXTFREE(rpp2)->rpp_freelink;
			rpp->rpp_freelink.rlink =
					&RFF_PREVFREE(rpp2)->rpp_freelink;
			RFF_PREVFREE(rpp)->rpp_freelink.flink =
						&rpp->rpp_freelink;
			RFF_NEXTFREE(rpp)->rpp_freelink.rlink =
						&rpp->rpp_freelink;
			if (rffp->rff_cursor == rpp2)
				rffp->rff_cursor = rpp;
		} else {
			/* Link onto free list behind the cursor. */
			insque(&rpp->rpp_freelink,
			       rffp->rff_cursor->rpp_freelink.rlink);
		}
	}
}
