/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern:mem/vm_seg.c	1.15"

/*
 * VM - segment management.
 */

#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * struct seg *
 * seg_alloc(struct as *as, vaddr_t base, u_int size)
 * 	Allocate a segment to cover [base, base+size] and attach it to the 
 *	specified address space.
 *
 * Calling/Exit State:
 * 	Caller guarantees the stability of the AS by holding the AS lock.
 *
 *	On success, returns address of newly allocated segment; on failure
 *	returns NULL.
 */
struct seg *
seg_alloc(struct as *as, vaddr_t base, u_int size)
{
	struct seg *new;
	vaddr_t segbase;
	u_int segsize;
	extern int valid_va_range();

	segbase = (vaddr_t)((u_int)base & PAGEMASK);
	segsize =
	    (((u_int)(base + size) + PAGEOFFSET) & PAGEMASK) - (u_int)segbase;

	if (!valid_va_range(&segbase, &segsize, segsize, AH_LO))
		return ((struct seg *)NULL);	/* bad virtual addr range */

	if ((as != &kas) && !VALID_USR_RANGE(segbase, segsize) ) 
		return ((struct seg *)NULL);	/* bad virtual addr range */

	new = (struct seg *)kmem_zalloc(sizeof(struct seg), KM_SLEEP);
	if (seg_attach(as, segbase, segsize, new) < 0) {
		kmem_free((caddr_t)new, sizeof(struct seg));
		return ((struct seg *)NULL);
	}
	/* caller must fill in ops, data */
	return (new);
}

/*
 * int
 * seg_attach(struct as *as, vaddr_t base, u_int size, struct seg *seg)
 * 	Attach a segment to the address space.  Used by seg_alloc()
 * 	and for kernel startup to attach to static segments.
 *
 * Calling/Exit State:
 * 	Caller guarantees the stability of the AS by holding the AS lock.
 *
 * 	Returns result of as_addseg opaquely.
 */
int
seg_attach(struct as *as, vaddr_t base, u_int size, struct seg *seg)
{

	seg->s_as = as;
	seg->s_base = base;
	seg->s_size = size;

	/*
	 * as_addseg() will add the segment at the appropraite point
	 * in the list. It will return -1 if there is overlap with
	 * an already existing segment.
	 */

	return (as_addseg(as, seg));
}

/*
 * void
 * seg_unmap(struct seg *seg)
 * 	Unmap a segment and free it from its associated address space.
 *
 * Calling/Exit State:
 *	Caller guarantees stability of address space by holding the AS lock
 *	or other out of band means.
 *
 *	Returns nothing useful to the caller.
 *
 * Description:
 * 	This should be called by anybody who's finished with a whole segment's
 * 	mapping. Just calls s_ops->unmap() on the whole mapping . It is the
 * 	responsibility of the segment driver to unlink the the segment
 * 	from the address space, and to free public and private data structures
 * 	associated with the segment. (This is typically done by a call to 
 * 	seg_free()).
 *
 * Remarks:
 *	It is very difficult for this call to fail.
 */
void
seg_unmap(struct seg *seg)
{
	/* Shouldn't have called seg_unmap if mapping isn't yet established */
	ASSERT(seg->s_data != NULL);

	/* Unmap the whole mapping */
	SOP_UNMAP(seg, seg->s_base, seg->s_size);
}

/*
 * void
 * seg_detach(struct seg *seg)
 * 	Detach the segment from its associated as. 
 *
 * Calling/Exit State:
 *	Caller guarantees stability of address space by holding the AS lock
 *	or other out of band means.
 */
void
seg_detach(struct seg *seg)
{
	struct as *as = seg->s_as;
	struct seg *next_seg, *prev_seg;

	DISABLE_PRMPT();
	next_seg = seg->s_next;
	if (as->a_segs == seg)
		as->a_segs = next_seg;			/* go to next seg */

	if (as->a_segs == seg) {
		as->a_segs = NULL;			/* seg list is gone */
		as->a_seglast = NULL;
	} else {
		prev_seg = seg->s_prev;
		prev_seg->s_next = next_seg;
		next_seg->s_prev = prev_seg;
		if (prev_seg->s_base + prev_seg->s_size == seg->s_base)
			as->a_seglast = prev_seg;
		else
			as->a_seglast = next_seg;
	}
	ENABLE_PRMPT();
}

/*
 * void
 * seg_free(struct seg *seg)
 * 	Free the segment from its associated as. 
 *
 * Calling/Exit State:
 *	Caller guarantees stability of address space by holding the AS lock
 *	or other out of band means.
 *
 *	Returns nothing useful to the caller.
 *
 * Description:
 * 	This should only be called if a mapping to the segment has not yet 
 *	been established (e.g., if an error occurs in the middle of doing 
 *	an as_map when the segment has already been partially set up) or if 
 *	it has already been deleted (e.g., from a segment driver unmap routine 
 *	if the unmap applies to the entire segment). If the mapping is 
 *	currently set up then seg_unmap() should  be called instead. 
 *
 * Remarks:
 *	It is veru difficult for this call to fail.
 */
void
seg_free(struct seg *seg)
{
	seg_detach(seg);

	/*
	 * If the segment private data field is NULL,
	 * then segment driver is not attached yet.
	 */
	if (seg->s_data != NULL)
		SOP_FREE(seg);

	kmem_free((caddr_t)seg, sizeof(struct seg));
}

#ifdef DEBUG

/*
 * Translate addr into page number within segment.
 */
u_int
seg_page(seg, addr)
	struct seg *seg;
	vaddr_t addr;
{

	return ((u_int) btop((addr) - (seg)->s_base));
}

/*
 * Return number of pages in segment.
 */
u_int
seg_pages(seg)
	struct seg *seg;
{

	return ((u_int) btopr((seg)->s_size));
}

#endif	/* DEBUG */
