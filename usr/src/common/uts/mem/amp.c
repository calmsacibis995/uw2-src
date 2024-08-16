/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/amp.c	1.4"
#ident	"$Header: $"

/*
 * VM - segment driver support for maps of anonymous memory.
 */

#include <mem/amp.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>


/*
 *+ Per-anon_map lock. Protects the anon_map against races
 *+ by segments sharing the map.
 */
LKINFO_DECL(segvn_amlockinfo, "MS:amp:am_lock", 0);


/*
 * struct anon_map *
 * amp_alloc(size_t size)
 * 	Allocate and initialize an anon_map structure of the
 *	specified size.
 *
 * Calling/Exit State:
 *	Returns a pointer to the anon_map.
 *
 *	Caller holds no spin locks, and is therefore prepared to block.
 */
struct anon_map *
amp_alloc(size_t size)
{
	struct anon_map *amp;
	uint_t npages;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * round up to page sizes
	 */
	npages = btopr(size);
	size = ptob(npages);

	amp = kmem_alloc(sizeof(struct anon_map), KM_SLEEP);
	amp->am_refcnt = 1;
	amp->am_size = size;
	amp->am_anon = kmem_zalloc((size_t)(npages * sizeof(anon_t *)),
				   KM_SLEEP);
#ifdef DEBUG
	amp->am_npageget = 0;
	LOCK_INIT(&amp->am_lock, VM_AMLOCK_HIER, VM_AMLOCK_IPL,
		  &segvn_amlockinfo, KM_SLEEP);
#endif /* DEBUG */

	return (amp);
}

/*
 * void
 * amp_release(struct anon_map *amp, uint_t index, size_t size)
 * 	Caller gives up its rights on an anon_map.
 *
 * Calling/Exit State:
 *	This routine does not block.
 *
 *	The caller holds the AS write lock for the affected address space.
 *
 * Description:
 *	If there are no more references to this anon_map structure, then
 *	deallocate the structure after freeing up all the anon slot pointer.
 */
void
amp_release(struct anon_map *amp, uint_t index, size_t size)
{
	ASSERT((amp->am_size & PAGEOFFSET) == 0);

	/*
	 * Free up all the anon slot pointers that the caller was using.
	 */
	anon_free(&amp->am_anon[index], size);

	/*
	 * Since all sharing segments live in the same AS, and since this
	 * AS is write locked, we can decrement the refcnt without acquiring
	 * the am_lock.
	 */
	if (--amp->am_refcnt == 0) {
#ifdef DEBUG
		LOCK_DEINIT(&amp->am_lock);
#endif /* DEBUG */
		kmem_free(amp->am_anon, btop(amp->am_size) * sizeof(anon_t *));
		kmem_free(amp, sizeof(struct anon_map));
	}
}
