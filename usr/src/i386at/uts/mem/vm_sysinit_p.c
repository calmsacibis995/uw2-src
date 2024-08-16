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

#ident	"@(#)kern-i386at:mem/vm_sysinit_p.c	1.26"
#ident	"$Header: $"

/*
 * VM - system initialization.
 */

#include <fs/memfs/memfs_mnode.h>
#include <mem/page.h>
#include <mem/seg_kmem.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <svc/autotune.h>
#include <svc/bootinfo.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

extern size_t mod_obj_size;
extern vaddr_t mod_obj_kern;
extern page_t *mod_obj_plist;
extern size_t resmgr_rdata_size;
extern size_t resmgr_size;
extern page_t *resmgr_obj_plist;
extern boolean_t phystokvmem;
extern size_t totalmem;

extern void pse_pagepool_init(void);
extern page_t	**memfs_getparm( struct krdata *, vaddr_t *, size_t *);

extern	paddr_t	bootinfo_loc;

STATIC page_t	**memfsroot_plist;
#define pbootinfo	(*((struct bootinfo *)bootinfo_loc))

/*
 * Count up the number of pages in use
 */
int pages_in_use;

#ifndef NO_RDMA

/*
 * kernel symbols in reverse page order?
 */
STATIC boolean_t mod_reversed;

#else /* NO_RDMA */

#define mod_reversed	B_FALSE

#endif /* NO_RDMA */

/*
 * STATIC void
 * pagepool_scan_kobject(void (*func)(), vaddr_t vaddr, size_t objsize, 
  *			void *arg)
 * 	Scan the kernel object, breaking it up into physically contiguous
 *	chunks.
 *
 * Calling/Exit State:
 *	``func'' will be called for each identified chunk.
 *	''vaddr'' is virtual address to start scan.
 *	''objsize'' is size in bytes of of area.
 *	``arg'' is an opaque argument which is passed onto chunk.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	In the restricted DMA case, the object is typically allocated
 *	from pages whose physically addresses are in reverse order from
 *	their virtual addresses. Therefore, this function can identify chunks
 *	that are either in low to high or in high to low physical address order.
 */
 
STATIC void
pagepool_scan_kobject(void (*func)(), vaddr_t objvaddr, size_t objsize, void *arg)
{
	
	paddr_t paddr, lower_paddr, higher_paddr;
	size_t size;

	lower_paddr = kvtophys(objvaddr);
	higher_paddr = lower_paddr + PAGESIZE;
	size = PAGESIZE;

	for (;;) {
		if (size >= objsize) {
			(*func)(lower_paddr, higher_paddr - lower_paddr, arg);
			break;
		}
		objvaddr += PAGESIZE;
		size += PAGESIZE;
		paddr = kvtophys(objvaddr);
		if (paddr == higher_paddr) {
			higher_paddr += PAGESIZE;
#ifndef NO_RDMA
			ASSERT(!mod_reversed);
		} else if (paddr + PAGESIZE == lower_paddr) {
			lower_paddr = paddr;
			ASSERT(size == 2 * PAGESIZE || mod_reversed);
			mod_reversed = B_TRUE;
#endif /* NO_RDMA */
		} else {
			(*func)(lower_paddr, higher_paddr - lower_paddr, arg);
			lower_paddr = paddr;
			higher_paddr = lower_paddr + PAGESIZE;
		}
	}
}

/*
 * STATIC void
 * pagepool_inc(paddr_t chunk_base, size_t chunk_size, void *arg)
 *	Counting function called by pagepool_scan_kobject on behalf
 *	of get_pagepool_size.
 *
 * Calling/Exit State:
 *	``arg'' is treated as a pointer to a count. The count is incremented.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 */

/* ARGSUSED */
STATIC void
pagepool_inc(paddr_t chunk_base, size_t chunk_size, void *arg)
{
	int *countp = arg;

	++(*countp);
}

/*
 * uint_t
 * get_pagepool_size(uint_t *nchunkp)
 *
 *  Find total pages left in memory not unused by kernel text and data.
 *  The memory used by the symbol table is also included in the page pool.
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description:
 *
 *   Loop through the memNOTused list to find free chunks.
 *   Then loop through the memused array to find chunks used by
 *   symbol table.
 *   
 *   This only calculates an upper bound on the number of pages
 *   that the page pool will need to manage, since subsequent
 *   allocations could reduce this before we initialize the page pool.
 */
uint_t
get_pagepool_size(uint_t *nchunkp)
{
	uint_t totalpages = 0;
	int nchunk = 0;
	int	j;
	vaddr_t	addr;
	size_t	size;
#ifdef NO_RDMA
	struct unusedmem *mnotused = memNOTused;
#else /* !NO_RDMA */
	struct unusedmem *mnotused;
	struct unusedmem ***listp;
	static struct unusedmem **mem_list[] =
			{&memNOTused, &memNOTusedNDMA, 0};
#endif /* NO_RDMA */

	pse_pagepool_init();
#ifndef NO_RDMA
	for (listp = mem_list; *listp != NULL; ++listp) {
		mnotused = **listp;
#endif /* NO_RDMA */
		while (mnotused != NULL) {
			totalpages += btop(mnotused->extent);
			++nchunk;
			mnotused = mnotused->next;
		}
#ifndef NO_RDMA
	}
#endif /* NO_RDMA */
	*nchunkp = nchunk;

	ASSERT(btop(totalmem) > totalpages);
	pages_in_use = btop(totalmem) - totalpages;

	if (mod_obj_kern != NULL && mod_obj_size != 0) {
		totalpages += btopr(mod_obj_size);
		pagepool_scan_kobject(pagepool_inc, mod_obj_kern, 				mod_obj_size, nchunkp);
	}

	/*
	 * Scan for kernel raw data segments next
	 * checking if any kernel raw data sections loaded.
	 */
	for (j=0; j <= pbootinfo.memusedcnt; j++) 
		if ( pbootinfo.memused[j].flags & B_MEM_KRDATA ){
			for(j = 0; pbootinfo.kd[j].paddr != 0; j++){
				switch (pbootinfo.kd[j].type) {
				case RM_DATABASE:
					totalpages += btopr(pbootinfo.kd[j].size);
					pagepool_scan_kobject(
						pagepool_inc, 
						pbootinfo.kd[j].vaddr, 
						pbootinfo.kd[j].size, nchunkp);
					break;
				case MEMFSROOT_META:
					memfs_getparm( &pbootinfo.kd[j], 
							&addr, &size);
					totalpages += btopr(size);
					pagepool_scan_kobject( pagepool_inc, 
						addr, size, nchunkp);
					break;
				case MEMFSROOT_FS:
					while(memfs_getparm(&pbootinfo.kd[j],
						&addr,&size)!=0 ){
						totalpages += btopr(size);
						pagepool_scan_kobject( 
						       pagepool_inc,
							addr, size, nchunkp );
						};

					break;
				};
			};
			break;
		}

	return totalpages;
}

/*
 * STATIC void
 * pagepool_sym_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
 *	Initialize a pagepool chunk for the symbol table.
 *
 * Calling/Exit State:
 *	``arg'' is treated as a pointer to a (page_t **). *arg is treated
 *	as the current pointer into the pages table.  It is incremented
 *	following the chunk initialization.
 * 
 *	``chunk_base'' and ``chunk_size'' describe the physical chunk to be
 *	initialized.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	This function is called by pagepool_scan_kobject on behalf
 *	of pagepool_init.
 */

STATIC void
pagepool_sym_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
{
	page_t **ppp = arg;
	int num_pp = btop(chunk_size);

	page_init_chunk(*ppp, num_pp, chunk_base, &mod_obj_plist, mod_reversed);
	*ppp += num_pp;

	/*
	 * phystokv support hack. Can't make the symbol table
	 * pageable if the pages are above the PHYSKV
	 * range.
	 */
	if (phystokvmem &&
	    chunk_base + chunk_size > PHYSTOKV_FULL_COMPAT_RANGE) {
		mod_obj_pagesymtab = MOD_NOPAGE;
		mod_symtab_lckcnt++;
	}
}

/*
 * STATIC void
 * pagepool_memfsroot_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
 *	Initialize a pagepool chunk for the memfs meta data.
 *
 * Calling/Exit State:
 *	``arg'' is treated as a pointer to a (page_t **). *arg is treated
 *	as the current pointer into the pages table.  It is incremented
 *	following the chunk initialization.
 * 
 *	``chunk_base'' and ``chunk_size'' describe the physical chunk to be
 *	initialized.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	This function is called by pagepool_scan_kobject on behalf
 *	of pagepool_init.
 */

STATIC void
pagepool_memfsroot_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
{
	page_t **ppp = arg;
	int num_pp = btop(chunk_size);

	page_init_chunk(*ppp, num_pp, chunk_base, 
			memfsroot_plist, mod_reversed);
	*ppp += num_pp;
}

/*
 * STATIC void
 * pagepool_rscmgt_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
 *	Initialize a pagepool chunk for the resource manager meta data.
 *
 * Calling/Exit State:
 *	``arg'' is treated as a pointer to a (page_t **). *arg is treated
 *	as the current pointer into the pages table.  It is incremented
 *	following the chunk initialization.
 * 
 *	``chunk_base'' and ``chunk_size'' describe the physical chunk to be
 *	initialized.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	This function is called by pagepool_scan_kobject on behalf
 *	of pagepool_init.
 */

STATIC void
pagepool_rscmgt_chunk(paddr_t chunk_base, size_t chunk_size, void *arg)
{
	page_t **ppp = arg;
	int num_pp = btop(chunk_size);

	page_init_chunk(*ppp, num_pp, chunk_base, &resmgr_obj_plist, mod_reversed);
	*ppp += num_pp;

}

/*
 * STATIC page_t *
 * pagepool_dolist(page_t *pp, struct unusedmem * mnotused)
 *
 * 	Allocate page structures for the unused memory pages on one of
 *	``chunk lists'' left by the pstart module.
 *
 * Calling/Exit State:
 *
 * 	pp is the current pointer into the calloc'ed page array.
 *
 *	returns the number of pages configured into the page pool
 *
 * Description:
 *
 *	Scan down the list, calling ``page_init_chunk'' for each chunk.
 */
STATIC page_t *
pagepool_dolist(page_t *pp, struct unusedmem * mnotused)
{
	int num_pp;
	paddr_t extent;

	while (mnotused != (struct unusedmem *)NULL) {
		extent = mnotused->extent;
		if (phystokvmem) {
			if (mnotused->base >= PHYSTOKV_FULL_COMPAT_RANGE) {
				break;
			}
			if (mnotused->base + extent >
			    PHYSTOKV_FULL_COMPAT_RANGE) {
				ASSERT(PHYSTOKV_FULL_COMPAT_RANGE > 
				       mnotused->base);
				extent = PHYSTOKV_FULL_COMPAT_RANGE - 
					 mnotused->base;
			}
		}
		num_pp = btop(extent);
		if (v.v_maxpmem != 0 && pages_in_use + num_pp > v.v_maxpmem) {
			num_pp = v.v_maxpmem - pages_in_use;
			if (num_pp <= 0)
				break;
		}
		page_init_chunk(pp, num_pp, mnotused->base, NULL, B_FALSE);
		pp += num_pp;
		pages_in_use += num_pp;
		mnotused = mnotused->next;
	}

	return pp;
}

/*
 * void
 * pagepool_init(page_t *pp, int pgarraysize)
 *
 * 	Allocate page structures for the unused memory pages.
 *
 * Calling/Exit State:
 *
 * 	pp is the calloc'ed page array.
 *	pgarraysize is the size of the page array.
 *
 * Description:
 *
 * Determine which memory controller clicks are present,
 * group them into physically contiguous runs, and
 * allocate page structures for each such run.
 */
void
pagepool_init(page_t *pp, int pgarraysize)
{
	page_t *savpp = pp;
	int	j;
	vaddr_t	addr;
	size_t	size;
#ifndef NO_RDMA
	struct unusedmem *mnup, *next_mnup, **mnupp, *mnuNDMAp, *next_mnuNDMAp;
#endif

	/*
	 * Scan the symbol table first, so that a MAXPMEM restriction, if
	 * in force, does not to apply to the symbol table.
	 */
	if (mod_obj_kern != NULL && mod_obj_size != 0)
		pagepool_scan_kobject(pagepool_sym_chunk, mod_obj_kern, 
						mod_obj_size, &pp);

	/*
	 * Scan for kernel raw data segments next.
	 */
	for (j=0; j <= pbootinfo.memusedcnt; j++) 
		if ( pbootinfo.memused[j].flags & B_MEM_KRDATA ){
			for(j = 0; pbootinfo.kd[j].paddr != 0; j++){
				switch (pbootinfo.kd[j].type) {
				case RM_DATABASE:
					pagepool_scan_kobject(
						pagepool_rscmgt_chunk, 
						pbootinfo.kd[j].vaddr, 
						pbootinfo.kd[j].size, &pp);
					resmgr_rdata_size=ptob(btopr(pbootinfo.kd[j].size));
					resmgr_size=pbootinfo.kd[j].size;
					break;
				case MEMFSROOT_META:
					memfsroot_plist = memfs_getparm( 
						&pbootinfo.kd[j], &addr, &size);
					pagepool_scan_kobject(
						pagepool_memfsroot_chunk, 
						addr, size, &pp);
					break;
				case MEMFSROOT_FS:
					while((memfsroot_plist = memfs_getparm( 						&pbootinfo.kd[j],&addr,&size))!=0 )
						pagepool_scan_kobject( 
						       pagepool_memfsroot_chunk,
							addr, size, &pp );

					break;
				};
			};
			break;
		}

#ifndef NO_RDMA
	/*
	 * Compress the memNOTusedNDMA list into the memNOTused list,
	 * concatenating physically contiguous pieces.
	 */
	mnuNDMAp = memNOTusedNDMA;
	while (mnuNDMAp != NULL) {
		next_mnuNDMAp = mnuNDMAp->next;
		mnupp = &memNOTused;
		mnup = memNOTused;
		while (mnup != NULL) {
			next_mnup = mnup->next;

			if (mnuNDMAp->base + mnuNDMAp->extent ==
			    mnup->base) {
				mnup->base = mnuNDMAp->base;
				mnup->extent += mnuNDMAp->extent;
				goto next_NDMA;
			}

			if (mnuNDMAp->base < mnup->base)
				break;

			if (mnup->base + mnup->extent == mnuNDMAp->base) {
				mnup->extent += mnuNDMAp->extent;
				if (next_mnup != NULL &&
				    mnup->base + mnup->extent ==
				    next_mnup->base) {
					mnup->extent += next_mnup->extent;
					mnup->next = next_mnup->next;
				}
				goto next_NDMA;
			}

			mnupp = &mnup->next;
			mnup = next_mnup;
		}
		*mnupp = mnuNDMAp;
		mnuNDMAp->next = mnup;

next_NDMA:
		mnuNDMAp = next_mnuNDMAp;
	}
#endif /* NO_RDMA */

	pp = pagepool_dolist(pp, memNOTused);

	/*
	 * Clip back totalmem to the pages actually in use.
	 * Also set tunemem to this same quantity (rounded to the next
	 * highest 1M).
	 */
	totalmem = ptob(pages_in_use);
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
		cmn_err(CE_PANIC, "Invalid page struct and page hash tables");
	}
}
