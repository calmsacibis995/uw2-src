/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)kern-i386sym:mem/vm_sysinit_p.c	1.11"
#ident	"$Header: $"

/*
 * VM - system initialization.
 */

#include <io/cfg.h>
#include <mem/hatstatic.h>
#include <mem/page.h>
#include <mem/vmparam.h>
#include <svc/autotune.h>
#include <svc/memory.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

STATIC page_t *page_init_mc_chunk(page_t *, int, uint_t);

extern size_t mod_obj_size;
extern vaddr_t mod_obj_kern;
extern paddr_t pmemptr;		/* Next unused physical address */
extern size_t totalmem;


/*
 * uint_t
 * get_pagepool_size(uint_t *nchunkp)
 *
 *  Find total pages left in memory not in use by kernel text or data.
 *  The memory used by the symbol table is also included in the page pool.
 *
 * Calling/Exit State:
 *
 *   topmem:    Physical address of where memory ceases to be
 *                          available to the page pool.
 *                          All pages >= topmem are unavailable to the
 *                          page pool.
 *
 * Description:
 *
 *   Loop through possible memory controller clicks, counting the
 *   clicks which are present and which is not in use.
 *   
 *   This only calculates an upper bound on the number of pages
 *   that the page pool will need to manage, since subsequent
 *   allocations could reduce this before we initialize the page pool.
 */

uint_t
get_pagepool_size(uint_t *nchunkp)
{
	uint_t mc, totalpages;
	paddr_t mc_baddr;  /* beginning physical byte address of this run */
	paddr_t mc_eaddr;  /* ending physical byte address plus 1 of this run */
	paddr_t lo_kpaddr;
	boolean_t in_chunk;

	totalpages = 0;
	*nchunkp = 0;

	ASSERT(btop(topmem) == btopr(topmem));	/* topmem on a page boundary */

	/*
	 * Retrieve physical address of next unallocated physical page.
	 * Since it is MMU_PAGESIZE aligned, convert it to PAGESIZE aligned.
	 */

	lo_kpaddr = ptob(btopr(pmemptr));
	in_chunk = B_FALSE;

	for (mc = 0; mc < CD_LOC->c_mmap_size; mc++) {

		if (MC_MMAP(mc, CD_LOC)) {

			/* Have a memory controller click here */

			mc_baddr = (paddr_t) (mc * MC_CLICK);
			mc_eaddr = mc_baddr + MC_CLICK;

			if (mc_eaddr <= lo_kpaddr) {
				/* entire mc click already calloc'd */
				if (in_chunk) {
					++*nchunkp;
					in_chunk = B_FALSE;
				}
				continue;
			}
			if (mc_baddr >= topmem) {
				/* entire mc click above allowed range */
				break;
			}

			in_chunk = B_TRUE;

			totalpages += btop(MC_CLICK);
			
			if (mc_baddr < lo_kpaddr) {
				/* mc click partially calloc'd */
				totalpages -= btopr(lo_kpaddr - mc_baddr);
			}

			if (topmem < mc_eaddr) {
				/* mc click partially disallowed */
				totalpages -= btopr(mc_eaddr - topmem);
			}
		} else if (in_chunk) {
			++*nchunkp;
			in_chunk = B_FALSE;
		}
	}
	if (in_chunk) {
		++*nchunkp;
		in_chunk = B_FALSE;
	}

	if (mod_obj_kern != NULL) {
		totalpages += btopr(mod_obj_size);
		++*nchunkp;
	}

	return totalpages;
}

/*
 * void
 * pagepool_init(page_t *pp, int pgarraysize)
 *
 * 	Allocate page structures for the unused memory pages.
 *
 * Calling/Exit State:
 *
 *  	pp passed points to the calloc'ed page array.
 *  	pgarraysize is number of page structs allocated.
 *  	lo_kpaddr is the next available physical address. 
 *
 * Description:
 *
 * 	Determine which memory controller clicks are present,
 * 	group them into physically contiguous runs, and
 * 	allocate page structures for each such run.
 */
void
pagepool_init(page_t *pp, int pgarraysize)
{
	int mc;
	int start_mc_chunk;
        uint_t num_mc_in_chunk;
	page_t *savpp = pp;
	extern vaddr_t mod_obj_kern;
	extern struct page *mod_obj_plist;

	/*
	 * Retrieve physical address of next unallocated physical page.
	 * Since it is MMU_PAGESIZE aligned, convert it to PAGESIZE aligned.
	 */

	start_mc_chunk = -1;
	num_mc_in_chunk = 0;
	for (mc = 0; mc < CD_LOC->c_mmap_size; mc++) {
		if (MC_MMAP(mc, CD_LOC)) {
			/* Have a memory controller there */
			num_mc_in_chunk++;
			if (start_mc_chunk == -1)
				start_mc_chunk = mc;
		} else {
			/*
			 * Don't have a memory controller there.
			 * Allocate page structures for the prior
			 * memory controller chunk, if any.
			 */
			if (start_mc_chunk != -1) {
				pp = page_init_mc_chunk(pp, start_mc_chunk,
							num_mc_in_chunk);
				start_mc_chunk = -1;
				num_mc_in_chunk = 0;
			}
		}
	}

	/* If any leftover, do it. */

	if (start_mc_chunk != -1)
		pp = page_init_mc_chunk(pp, start_mc_chunk, num_mc_in_chunk);

	if (mod_obj_kern != NULL) {
		page_init_chunk(pp, btopr(mod_obj_size),
				kvtophys(mod_obj_kern), &mod_obj_plist,
				B_FALSE);
	}

	tunemem = TUNE_ROUNDUP(totalmem);

	/*
	 * ASSERT:  We didn't run out of page structures
	 *          (and hence run into the page hash structures).
	 */
	if ((char *)pp > (char *)(savpp + pgarraysize)) {
        	/*
                 *+ A problem was found during system initialization
                 *+ of virtual memory data structures.  This indicates
                 *+ a kernel software error.  Corrective action:  none.
		 */
		cmn_err(CE_PANIC,"Invalid page struct and page hash tables");
	}
}

/*
 * STATIC page_t *
 * page_init_mc_chunk(page_t *pp, int start_mc_chunk, uint_t num_mc)
 *
 *	Allocate page pool data structure for a range of
 *	memory controller clicks.
 *
 * Calling/Exit State:
 *
 *	pp point to an array of page structures.  Must be big enough
 *		to hold all pages represented by start_mc_chunk.
 *
 *	start_mc_chunk is the starting memory controller click number
 *		of a contiguous run of clicks.
 *
 *	num_mc is the number of memory controller clicks in the contiguous run.
 *
 *	Returns the value of pp updated to point to the page structure
 *	immediately following the last page structure consumed by this
 *	run of memory controller clicks.  (Suitable for a subsequent call
 *	to this function.)
 */

STATIC page_t *
page_init_mc_chunk(page_t *pp, int start_mc_chunk, uint_t num_mc)
{
	uint_t num_pp;	/* number of pages in this run of memory controllers */
	paddr_t baddr;	/* beginning physical byte address of this run */
	paddr_t eaddr;	/* ending physical byte address plus 1 of this run */


	ASSERT(start_mc_chunk >= 0);
	ASSERT(num_mc != 0);

	baddr = (paddr_t) (start_mc_chunk * MC_CLICK);
	eaddr = baddr + num_mc * MC_CLICK;
	
	/*
	 * Ignore all physical memory up to (not including) pmemptr.
	 * It is already allocated by palloc.
	 */

	if (eaddr <= pmemptr)
		return pp;
	
	if (baddr < pmemptr)
		baddr = ptob(btopr(pmemptr));

	/*
	 * Ignore all physical memory beyond (including) topmem.
	 * It is beyond the user configured allowable range.
	 */

	if (baddr >= topmem)
		return pp;

	if (eaddr > topmem)
		eaddr = ptob(btop(topmem));


	num_pp = btop(eaddr - baddr);

	page_init_chunk(pp, num_pp, baddr, NULL, B_FALSE);

	return pp + num_pp;
}
