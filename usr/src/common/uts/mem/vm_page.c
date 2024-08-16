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

#ident	"@(#)kern:mem/vm_page.c	1.103"
#ident	"$Header: $"

/*
 * VM - physical page management.
 */

#include <fs/buf.h>
#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <mem/anon.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/pageidhash.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern void	poolrefresh_nudge(void);
extern void	poolrefresh_outofmem(void);
extern event_t	pageout_event;

STATIC page_t  *page_get_l(uint_t, uint_t, boolean_t *);
STATIC void	page_reuse(page_t *);
STATIC int	page_unfree_l(page_t *, boolean_t, enum page_lock_mode,
				u_int flags);
STATIC page_t  *page_lookup(vnode_t *, off_t);
STATIC page_t  *page_exists(const vnode_t *, off_t);
STATIC void	page_hashin(page_t *, vnode_t *, off_t);
STATIC void	page_hashout(page_t *);
STATIC void	page_unlock_l(page_t *);
STATIC void	page_setmod_l(page_t *);
STATIC void	wait_for_freemem(enum page_lock_mode);
STATIC void	outofmem(void);

#ifdef DEBUG
STATIC void	page_early_add(page_t **ppp, page_t *pp);
#else
#define page_early_add	page_add
#endif

#ifdef NO_RDMA

STATIC boolean_t freemem_resv_std(uint_t npages, uint_t flags, boolean_t *dropp,
    	    	    	      		enum page_lock_mode);
#define freemem_resv(npages, flags, npages_per_type, dropp) \
		((*(npages_per_type) = (npages)), \
		 freemem_resv_std(npages, flags, dropp, WRITE_LOCKED))
#define freemem_resv_one_type(npages, flags, typep, dropp) \
		(*(typep) = STD_PAGE, \
		 freemem_resv_std(npages, flags, dropp, WRITE_LOCKED))
#define freemem_resv_type(npages, type, flags, dropp, lockmode) \
		freemem_resv_std(npages, flags, dropp, lockmode)

STATIC void	freemem_unresv_std(uint_t npages);
#define freemem_unresv(npages, type)	\
		freemem_unresv_std(npages)

#else /* !NO_RDMA */

STATIC boolean_t freemem_resv(uint_t, uint_t, u_int *, boolean_t *); 
STATIC boolean_t freemem_resv_one_type(uint_t, uint_t, int *, boolean_t *);
STATIC boolean_t freemem_resv_type(uint_t, int, uint_t, boolean_t *,
			enum page_lock_mode);

STATIC void	freemem_unresv(uint_t, int);

#endif /* NO_RDMA */

lock_t	vm_pagefreelock;	/* mutex page free lists */
pl_t	vm_pagefreelockpl;	/* pl for current vm_pagefreelock holder */

rwlock_t vm_pageidlock;		/* mutex page id lists */

uint_t page_hashsz;		/* # slots in page hash table (power of 2) */
page_t **page_hash;		/* page hash table */

int	mem_freemem[NPAGETYPE];	/* # currently free pages of memory per type */
int	maxfreemem[NPAGETYPE];	/* max # free pages of memory per type */
sv_t	vm_waitmem;		/* wait for freemem to become available */

LKINFO_DECL(vmfreelkinfo, "MP:pages:vm_pagefreelock", 0);
LKINFO_DECL(vmidlkinfo, "MP:pages:vm_pageidlock", 0);
LKINFO_DECL(pagelkinfo, "MP:pages:p_uselock", 0);

#if defined(EXTRA_PAGE_STATS) || defined(lint)
struct page_tcnt pagecnt;
#endif

#ifdef DEBUG
#define PAGE_DEBUG 1
#endif

#ifdef PAGE_DEBUG
STATIC int do_checks = 0;
STATIC int do_check_vp = 1;
STATIC int do_check_free = 1;
STATIC int do_check_list = 1;
STATIC int do_check_pp = 1;
STATIC int do_check_pp_list = 1;

STATIC void call_debug(char *);
STATIC void page_vp_check(const vnode_t *);
STATIC void page_free_check(uint_t);
STATIC void page_list_check(const page_t *);
STATIC void page_pp_check(const page_t *);
STATIC void page_pp_list_check(const page_t *, page_t **);

#define	CHECK(vp)	if (do_checks && do_check_vp) page_vp_check(vp)
#define	CHECKFREE()	if (do_checks && do_check_free) page_free_check(0)
#define	CHECKLIST(pp)	if (do_checks && do_check_list) page_list_check(pp)
#define	CHECKPP(pp)	if (do_checks && do_check_pp) page_pp_check(pp)
#define	CHECKPPLIST(pp, plist) \
	if (do_checks && do_check_pp_list) page_pp_list_check(pp, plist)

#else /* PAGE_DEBUG */

#define	CHECK(vp)
#define	CHECKFREE()
#define	CHECKLIST(pp)
#define	CHECKPP(pp)
#define	CHECKPPLIST(pp, plist)

#endif /* PAGE_DEBUG */

/*
 * The logical page free list is maintained as four physical lists.
 *
 *	page_freelist:	 Free pages which are clean and have no file identity.
 *				Reuse these first.
 *	page_cachelist:	 Free pages which are clean and have file identity.
 *				Remain unused as long as possible so that
 *				they might be reclaimed.  Anon (swap) file
 *				pages are placed before non-anon file pages
 *				to give them reuse preference.
 *
 *	page_dirtyflist: Free pages which are dirty and have vnode identity
 *				belonging to a non-swap file.
 *				Must be cleaned before reuse.
 *	page_dirtyalist: Free pages which are dirty and have vnode identity
 *				belonging to a swap (anon) file.
 *				Must be cleaned before reuse.  Defer cleaning
 *				longer than non-swap pages in the hope they
 *				will be page_abort'd due to process exit.
 *				
 */

page_t *page_freelist[NPAGETYPE];  /* free, clean, non-file pages */
page_t *page_cachelist[NPAGETYPE]; /* free, clean, file pages */

page_t *page_dirtyflist;		  /* free, dirty, non-anon file pages */
page_t *page_dirtyalist;		  /* free, dirty, anon file pages */

int page_freelist_size[NPAGETYPE];
int page_cachelist_size[NPAGETYPE];

#ifdef DEBUG
/*
 * The size of lists of pages processed by pageout(): the 
 * page_dirtyflist and the page_dirtyalist. These sizes reflect
 * the total number of the pages summed across NPAGETYPE types.
 * Maintained for debugging purposes only.
 */
STATIC	int page_dirtyflist_size;
STATIC 	int page_dirtyalist_size;
#define	INCR_LIST_SIZE(x)	++(x)
#define DECR_LIST_SIZE(x)	--(x)
#else
#define	INCR_LIST_SIZE(x)
#define DECR_LIST_SIZE(x)
#endif

/*
 * page_dirtylists_size[] holds the sum of numbers of file and anon dirty 
 * pages for each of NPAGETYPE types.
 */
int	page_dirtylists_size[NPAGETYPE];

struct pp_chunk	*pagepool;		/* array of pagepool chunks */
struct pp_chunk *pp_first;		/* first chunk in search order */
uint_t n_pp_chunk;			/* # chunks in pagepool table */
#ifdef DEBUG
uint_t pagepool_size;			/* # chunks allocated for pagepool */
#endif

page_t *pages;				/* 1st pagepool page per type */
page_t *epages;				/* last pagepool page per type (+1) */

/*
 * The ``contiguous sections'' table is organized as an array of pointers
 * into the pages table. Each entry points to one of four types of places
 * in the pages table:
 *
 *	(a) the beginning of the table (i.e. page_cs_table[0] = pages),
 *	(b) to a place in the table where the page type is discontiguous
 *	    (i.e. where pp->p_type != (pp - 1)->p_type),
 *	(c) to a place in the table where the page frame numbers are
 *	    discontiguous (i.e. where pp->p_pfn != (pp - 1)->p_pfn - 1), or
 *	(d) the end of the table (epages).
 *
 * The size of the table is bounded by 4 + totalchunks. The term ``4''
 * comes about due to:
 *
 *	1 extra entry for the end of the table
 *	1 extra entry for a possible cut between STD_PAGEs and DMA_PAGEs
 *	1 extra entry for a possible cut between DMA_PAGEs amd PAD_PAGEs
 *	1 extra entry for a possible cut between PAD_PAGEs and STD_PAGEs
 */
page_t **page_cs_table;
#ifdef DEBUG
int page_cs_table_size;	/* number of entries in the page_cs_table */
#endif

STATIC page_t *page_swapreclaim_nextpp; /* pp at which to begin next scan. */
                                        /* Set in page_init */

STATIC lock_t page_swapreclaim_lck;     /* Lock serializing swapreclaim scans */
					/* Initialized in page_init */

LKINFO_DECL(pareclaimlckinfo, "MP:page:page_swapreclaim_lck", 0);


#ifndef NO_RDMA
/*
 * Number of page types in use.
 */
int page_ntype_used;

/*
 * Primary page types for each combination of PDMA|P_NODMA flags.
 * This table is correct for rdma_mode RDMA_LARGE. The table will
 * be adjusted should the system transition to another mode.
 */
int page_ptype_map[] = {
	STD_PAGE,	/* 0 */
			/* should be PAD_PAGE for RDMA_SMALL */
	0,
	DMA_PAGE,	/* P_DMA */
			/* should be STD_PAGE for RDMA_DISABLED */
			/* and RDMA_MICRO */
	0,
	STD_PAGE	/* P_NODMA */
};

/*
 * Secondary page types for each combination of PDMA|P_NODMA flags.
 * This table is correct for all rdma_mode(s) except RDMA_SMALL. It will
 * be adjusted should a transition occur to RDMA_SMALL.
 */
int page_stype_map[] = {
	NO_PAGE,	/* 0 */
			/* should be STD_PAGE for RDMA_SMALL */
	0,
	NO_PAGE,	/* P_DMA */
	0,
	NO_PAGE,	/* P_NODMA */
			/* should be PAD_PAGE for RDMA_SMALL */
};
#endif /* NO_RDMA */

#ifdef DEBUG
/*
 * additional DEUBG only counters
 */
STATIC uint_t page_swapreclaim_reclaims;       /* count of reclaims done */
STATIC uint_t page_swapreclaim_desperate;      /* times we were desperate */

#define PGSTAT_BUMP(count)	((count)++)

#else

#define PGSTAT_BUMP(count)

#endif

/*
 * void
 * page_init_chunk(page_t *pp, uint_t num_pp, paddr_t basepaddr,
 *		   page_t **plistp, boolean_t reversed)
 *
 *	Initialize physical page structures for a contiguous portion of memory.
 *
 * Calling/Exit State:
 *
 *	pp points to an array of num_pp page structures for the memory.
 *		The caller must allocate these.  This pp array must
 *		be virtually contiguous with the pp array supplied
 *		to the prior invocation of this function, if any.
 *
 *	num_pp is the number of page structures in the pp array.
 *
 *	basepaddr is the starting physical page address of the contiguous
 *		portion of physical memory being represented by these
 *		page structures.  This address must be PAGESIZE aligned
 *		(and hence also MMU_PAGESIZE aligned).
 *
 *	plistp is an optional pointer to a list of pages; if non-NULL,
 *		the pages initialized here will be read-locked instead of
 *		freed initially, and they will be added to this list.
 *
 *	reversed is true iff the plistp is to be built in reverse order
 *		(i.e. lowest physical address last).
 *
 *	This is called only at system initialization time, once for
 *	each contiguous chunk of physical memory that is to be managed
 *	by the page free lists.  The caller must subsequently call
 *	page_init() to finialize the initialization process.
 *
 * Description:
 *
 *	Since we cannot call the dynamic memory allocator yet,
 *	we have system initialization code allocate memory for
 *	the page structs and the hash tables for us.
 */
void
page_init_chunk(page_t *pp, uint_t num_pp, paddr_t basepaddr, page_t **plistp,
		boolean_t reversed)
{
	uint_t base_mmupfn;
	struct pp_chunk	**chunkpp;

	ASSERT(basepaddr % PAGESIZE == 0);
	ASSERT(n_pp_chunk < pagepool_size);

	base_mmupfn = mmu_btop(basepaddr);

	pagepool[n_pp_chunk].pp_pfn   = base_mmupfn;
	pagepool[n_pp_chunk].pp_epfn  = base_mmupfn + mmu_btopr(ptob(num_pp));
	pagepool[n_pp_chunk].pp_page  = pp;
	pagepool[n_pp_chunk].pp_epage = pp + num_pp;

	/*
	 * Link this chunk into the search order list.  Larger chunks
	 * go to the front of the list.
	 * This is done with the hope that when scanning for a particular
	 * page structure/page frame number we'll be more likely to find
	 * the one we're looking for quicker.
	 */
	chunkpp = &pp_first;
	while (*chunkpp != NULL) {
		if ((*chunkpp)->pp_epage - (*chunkpp)->pp_page <= num_pp)
			break;
		chunkpp = &(*chunkpp)->pp_next;
	}
	pagepool[n_pp_chunk].pp_next = *chunkpp;
	*chunkpp = &pagepool[n_pp_chunk];

	/*
	 * Add these pages to the count of maximum available pages.
	 */
	maxfreemem[STD_PAGE] += num_pp;

	/*
	 * The physical space for the page struct array has already been
	 * allocated and zeroed.  Initialize each page struct.
	 */
	while (num_pp-- != 0) {
		LOCK_INIT(&pp->p_uselock, VM_PUSELOCK_HIER, VM_PAGE_IPL,
				&pagelkinfo, KM_NOSLEEP);
		SV_INIT(&pp->p_wait);

		/* cache pfn values for speed of page_pptonum/page_numtopp */
		pp->p_pfn = base_mmupfn;
		pp->p_chidx = n_pp_chunk;

		/*
		 * If caller wants to keep the pages, WRITE lock them and add
		 * them to the caller's list.
		 */
		if (plistp != NULL) {
			pp->p_activecnt = 1;
			pp->p_invalid = 1;
			page_early_add(plistp, pp);
			if (reversed)
				*plistp = (*plistp)->p_next;
		}

		base_mmupfn += PAGESIZE / MMU_PAGESIZE;
		++pp;
	}

	++n_pp_chunk;
}


/*
 * void
 * page_init(void)
 *
 *	Finalize physical page structures initialization during system startup.
 *
 * Calling/Exit State:
 *
 *	This is called only once during system initialization and after
 *	already calling page_init_chunk() one or more times.
 */
void
page_init(void)
{
	page_t *pp;
	int type;

	ASSERT(n_pp_chunk != 0 && pp_first != NULL);
	ASSERT(n_pp_chunk <= pagepool_size);

	pages = pagepool[0].pp_page;
	epages = pagepool[n_pp_chunk - 1].pp_epage;
	ASSERT(epages - pages == maxfreemem[STD_PAGE]);

	/*
	 * Determine the number of pages that can be pplocked. This is
	 * normally given by the tunable PAGES_UNLOCK. However, if the value
	 * is unreasonable, then just use 10% of memory.
	 */
	if (pages_pp_maximum <= (tune.t_minamem + 20) ||
	    pages_pp_maximum > (epages - pages))
		pages_pp_maximum = (epages - pages) / 10;

	/*
	 * Verify that the hashing stuff has been initialized.
	 */
	if (page_hash == NULL || page_hashsz == 0) {
		/*
		 *+ A problem was found during system initialization
		 *+ of virtual memory data structures.  The page cache
		 *+ hash data structure was not initialized when expected.
		 *+ This indicates a kernel software error.  
		 *+ Corrective action:  none.
		 */
		cmn_err(CE_PANIC, "page_init: page_hash not initialized");
	}

	LOCK_INIT(&vm_pagefreelock, VM_PAGEFREE_HIER, VM_PAGE_IPL,
			&vmfreelkinfo, KM_NOSLEEP);

	RW_INIT(&vm_pageidlock, VM_PAGEID_HIER, VM_PAGE_IPL,
			&vmidlkinfo, KM_NOSLEEP);

	SV_INIT(&vm_waitmem);

#ifndef NO_RDMA
	rdma_page_init();
#endif

	/*
	 * Put all of the pages onto the free list, except those which
	 * are already locked.
	 */
	for (pp = pages; pp < epages; pp++) {

		if (PAGE_IS_LOCKED(pp))
			continue;

		/* page has no identity, put it on the front of the free list */
		pp->p_free = 1;
		type = page_type(pp);
		mem_freemem[type]++;
		page_freelist_size[type]++;
		page_add(&page_freelist[type], pp);
		ASSERT(!PAGE_IN_USE(pp));
	}

	page_cs_init();

	CHECKFREE();

	/*
	 * Initialize variables needed by page_swapreclaim.
	 */

	LOCK_INIT(&page_swapreclaim_lck, VM_SWAPREC_HIER, VM_SWAPREC_IPL,
		&pareclaimlckinfo, KM_NOSLEEP);
	page_swapreclaim_nextpp = pages;

}

/*
 * void
 * page_cs_init(void)
 *	Formulate the ``contiguous sections'' table.
 *
 * Calling/Exit State:
 *	The caller has arranged for the page pool to be quiescent.
 *
 * Remarks:
 *	Storage for the page_cs_table is allocated in kvm_init().
 */
void
page_cs_init(void)
{
	page_t **ppp;
	page_t *pp;

	page_cs_table[0] = pages;
	ppp = &page_cs_table[1];
	pp = pages + 1;
	while (pp != epages) {
#ifdef NO_RDMA
		if (pp->p_pfn != (pp - 1)->p_pfn + 1) {
#else
		if (pp->p_pfn != (pp - 1)->p_pfn + 1 ||
		    pp->p_type != (pp - 1)->p_type) {
#endif
			*ppp++ = pp;
			ASSERT(ppp - page_cs_table < page_cs_table_size);
		}
		++pp;
	}
	*ppp = epages;
}

/*
 * page_t *
 * page_get(uint_t bytes, uint_t flags)
 *
 *	Allocate enough pages for bytes of data.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDWRLOCK() and VM_PAGEFREELOCK() unlocked.
 *	They remain unlocked at return.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *		P_DMA
 *		      DMA-able pages required. Ignored if restricted DMA
 *		      support is not compiled into the kernel or if
 *		      rdma_mode == RDMA_DISABLED.
 *
 *		P_NODMA
 *		      STD_PAGEs are preferred over PAD_PAGEs. Ignored if
 *		      restricted DMA support is not compiled into the
 *		      kernel or if rdma_mode != RDMA_SMALL.
 *
 *		If neither of P_DMA or P_NODMA are specified, if restricted
 *		DMA support is compiled into the kernel, and if
 *		rdma_mode == RDMA_SMALL, then PAD_PAGES ars preferred over
 *		STD_PAGEs.
 * 
 *	On failure, returns NULL.
 *
 *	On success returns a doubly linked, circular list of pages.
 *	Each returned page has the following state:
 *		- page writer locked (p_invalid == 1 && p_activecnt == 1)
 *		- no vnode identity (p_vnode == NULL && p_offset == NULL)
 *		- PAGE_USEUNLOCK()'d
 *
 *	See also page_get_l() for locked version with more options.
 */
page_t *
page_get(uint_t bytes, uint_t flags)
{
	page_t *pp;
	boolean_t dropped_vmpagelocks;	/* dummy */
#ifdef DEBUG
	pl_t savpl;
#endif

	/* No flags besides NOSLEEP, P_DMA or P_NODMA */
	ASSERT(!(flags & ~(NOSLEEP|P_DMA|P_NODMA)));
	ASSERT((flags & (P_DMA|P_NODMA)) != (P_DMA|P_NODMA));
#ifdef DEBUG
	savpl = getpl();
#endif
	VM_PAGEIDWRLOCK();
	VM_PAGEFREELOCK();
	pp = page_get_l(bytes, flags, &dropped_vmpagelocks);
	VM_PAGEFREEUNLOCK();
	VM_PAGEIDUNLOCK();

	ASSERT(savpl == getpl());
	return (pp);
}


/*
 * STATIC page_t *
 * page_get_l(uint_t bytes, uint_t flags, boolean_t *dropped_vmpagelocks)
 *
 *	Allocate enough pages for bytes of data.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDWRLOCK() and VM_PAGEFREELOCK() locked.
 *	They remain locked at return. (Note: The id lock is also needed
 *	because of page_reuse()).
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *		P_RETURN_PAGEUSELOCKED
 *		      Page is returned PAGE_USELOCK()'d.  Caller must
 *		      PAGE_USEUNLOCK() the page *before* doing VM_PAGEUNLOCK().
 *		      (bytes must == PAGESIZE; i.e., a single page)
 *
 *		P_TRYPAGEUSELOCK
 *		      If a page cannot be immediately conditionally locked,
 *		      this function returns failure.  The caller must
 *		      also specify NOSLEEP.  The caller typically also
 *		      specifies P_RETURN_PAGEUSELOCKED.
 *		      (bytes must == PAGESIZE; i.e., a single page)
 *
 *		      This is typically used when the caller holds locks
 *		      whose locking order is out of order with respect to
 *		      PAGE_USELOCK().  But may also be used if the caller
 *		      wishes only a half-hearted attempt.
 *
 *		P_DMA
 *		      DMA-able pages required. Ignored if restricted DMA
 *		      support is not compiled into the kernel or if
 *		      rdma_mode == RDMA_DISABLED.
 *
 *		P_NODMA
 *		      STD_PAGEs are preferred over PAD_PAGEs. Ignored if
 *		      restricted DMA support is not compiled into the
 *		      kernel or if rdma_mode != RDMA_SMALL.
 *
 *		If neither of P_DMA or P_NODMA are specified, if restricted
 *		DMA support is compiled into the kernel, and if
 *		rdma_mode == RDMA_SMALL, then PAD_PAGES ars preferred over
 *		STD_PAGEs.
 *
 *	dropped_vmpagelocks is an out arg which is set on return:
 *		0 - Both the vm_pageidlock and the vm_pagefreelock were held
 *		    without ever dropping them.
 *		1 - Both the vm_pageidlock and the vm_pagefreelock were 
 *		    dropped due to blocking or lock contention.
 *		It will always be 0 if P_TRYPAGEUSELOCK and NOSLEEP
 *		were specified.
 *
 *	l.pageidlockpl could be reset in this routine if we have to return
 *	the page uselocked. In that case, it would be set to VM_PAGE_IPL.
 *
 *	On failure, returns NULL.
 *
 *	On success returns a doubly linked, circular list of pages.
 *	Each returned page has the following state:
 *		- page writer locked (p_invalid == 1 && p_activecnt == 1)
 *		- no vnode identity (p_vnode == NULL && p_offset == NULL)
 *		- PAGE_USEUNLOCK()'d
 *		  (unless P_RETURN_PAGEUSELOCKED specified, in which case the
 *		  caller must PAGE_USEUNLOCK(pp) *before* doing VM_PAGEUNLOCK())
 *
 *	See also page_get() for unlocked version.
 */
STATIC page_t *
page_get_l(uint_t bytes, uint_t flags, boolean_t *dropped_vmpagelocks)
{
    page_t *pp;
    page_t *ppnext;
    page_t *plist = NULL;
    uint_t npages;
    int triedcache;
    uint_t currentlist_size;
    uint_t npages_per_type[NPAGETYPE];
    int type;

#if (NOSLEEP == 0)
#error page_get_l() code assumes NOSLEEP != 0
#endif

    npages = btopr(bytes);

    ASSERT(VM_PAGEFREELOCK_OWNED());
    ASSERT(VM_PAGEIDLOCK_OWNED());
    ASSERT(!(flags & P_RETURN_PAGEUSELOCKED) || npages == 1);
    ASSERT(!(flags & P_TRYPAGEUSELOCK) || npages == 1);
    ASSERT(!(flags & P_TRYPAGEUSELOCK) || (flags & NOSLEEP));
    ASSERT(dropped_vmpagelocks != (boolean_t *)NULL);
    ASSERT(bytes != 0);

    /*
     * Count the number of calls and a histogram of number of pages
     */

    BUMPPGCOUNT(pagecnt.pc_get);
    BUMPPGCOUNT(pagecnt.pc_get_npages[(npages > PC_GET_CNT) ? PC_GET_CNT : npages]);

    /*
     * Reserve the requested number of pages on the free list.
     */

    if (!freemem_resv(npages, flags, npages_per_type, dropped_vmpagelocks)) {
    	ASSERT(flags & NOSLEEP);
	BUMPPGCOUNT(pagecnt.pc_get_fail_NOSLEEP);
	return ((page_t *)NULL);
    }
    
    /*
     * Pull the pages off the free list(s) and build the return list.
     */

    /*
     * First try to do it without dropping the id and free list locks by
     * skipping pages that can't be immediately TRYLOCKed.
     * If that's insufficient, do it the harder way.
     */

    for (type = 0; type < page_ntype_used; type++) {
    	if (npages_per_type[type] == 0)
    	    continue;

	pp = page_freelist[type];
	currentlist_size = page_freelist_size[type];
	triedcache = 0;

	ASSERT((pp == (page_t *)NULL) || !PAGE_IN_USE(pp));

	if (pp == NULL) {
    	    pp = page_cachelist[type];
	    currentlist_size = page_cachelist_size[type];
	    triedcache = 1;
	}

	ASSERT(pp->p_type == type);

	while (npages_per_type[type] != 0 && currentlist_size != 0) {

	    ppnext = pp->p_next;	/* remember next page on list */

	    /*
	     * If we can immediately lock the page, grab it.
	     * Otherwise a page_reclaim has got it; skip it.
	     *
	     * Note that, since we already hold higher order locks
	     * and they have a (potentially) stronger ipl level than
	     * PAGE_USELOCK(), we TRYLOCK the page using the
	     * stronger ipl level and retain that ipl level
	     * when we UNLOCK the page.
	     */
	    if (PAGE_TRYUSELOCK(pp) != INVPL) {
	    	page_reuse(pp);
    	    	page_add(&plist, pp);
		npages_per_type[type]--;
		npages--;

		if (flags & P_RETURN_PAGEUSELOCKED) {
		    /*
		     * Adjust cached ipl level so caller can
		     * release it via PAGE_USEUNLOCK().
		     */
		    l.puselockpl = l.vmpageidlockpl;
		    l.vmpageidlockpl = VM_PAGE_IPL;		
		    ASSERT(npages == 0);
		    CHECKFREE();
		    return (plist);
		} else {
		    l.puselockpl = VM_PAGE_IPL;
		    PAGE_USEUNLOCK(pp);
		}
	    }
    
	    /*
	     * Get next untried page from free lists.
	     *
	     * If we've looked at every page on the current list,
	     * switch to page_cachelist if that's not already the
	     * current list.
	     */

	    pp = ppnext;
	    if (--currentlist_size == 0 && !triedcache) {
    	    	pp = page_cachelist[type];
		currentlist_size = page_cachelist_size[type];
		triedcache = 1;
	    }
	}
    }

    /*
     * If we got all the requested pages, return success.
     */

    if (npages == 0) {
	CHECKFREE();
	return (plist);
    }

    /*
     * If we didn't get the requested pages, and the caller will
     * only let us trylock the pages, return failure.
     */

    if (flags & P_TRYPAGEUSELOCK) {
    	ASSERT(plist == NULL);		/* no garnered pages */
	ASSERT(npages == 1);		/* only one page requested */
	/*
	 * Cancel our memory reservation.  (We do this ourselves
	 * rather than call freemem_unresv() because we know that
	 * no additional requesters have blocked waiting for freemem
	 * since we did our reservation because we've held VM_PAGEFREELOCK()
	 * the entire time.)
	 */
	for (type = 0; type < page_ntype_used; type++)
	    mem_freemem[type] += npages_per_type[type];
	CHECKFREE();
	BUMPPGCOUNT(pagecnt.pc_get_fail_TRYLOCK);
	return ((page_t *)NULL);
    }

    /*
     * If we couldn't TRYLOCK sufficient pages without dropping
     * the id and the free list locks then do it the hard way.
     */

    BUMPPGCOUNT(pagecnt.pc_get_no_TRYLOCK);

    for (type = 0; type < page_ntype_used; type++) {

    	while (npages_per_type[type] != 0) {

	    /*
	     * Get first free page on free lists.
    	     */

	    if ((pp = page_freelist[type]) == NULL) {
		if ((pp = page_cachelist[type]) == NULL) {
    	    	    /*
		     *+ During memory allocation, the page
		     *+ free list counter was found to
		     *+ disagree with the actual contents
		     *+ of the free list.  This indicates a
		     *+ kernel software problem.
		     *+ Corrective action: none.
		     */
		    cmn_err(CE_PANIC, "page_get_l: freemem error");
		}
		ASSERT(pp->p_vnode != NULL);
	    } else
		ASSERT(pp->p_vnode == NULL);

	    if (PAGE_TRYUSELOCK(pp) != INVPL) {
		/*
		 * Immediately got the page lock.
		 * Adjust cached ipl level so we can release
		 * it via PAGE_USEUNLOCK().
		 */
		l.puselockpl = l.vmpageidlockpl;
		l.vmpageidlockpl = VM_PAGE_IPL;
	    } else {
		/*
		 * Couldn't immediately lock the page;
		 * page_reclaim() has got it.  If there's only
		 * enough freemem for one of us, page_reclaim()
		 * will defer to us (since we already
		 * decremented freemem thereby reserving it).
		 * If there is enough freemem for both of us,
		 * page_reclaim() will take this page.  So spin
		 * waiting to lock the page to determine which
		 * it is.
		 *
		 * Must unlock the id list and the free list locks 
		 * in order to PAGE_USELOCK(pp).  However, since we will
		 * release them out of order, adjust the cached
		 * ipl levels.
		 */

    	    	/* had to drop the freelk and the idlk */
		*dropped_vmpagelocks = B_TRUE;  

		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		PAGE_USELOCK(pp);
		VM_PAGEIDWRLOCK();
		VM_PAGEFREELOCK();

		if (!pp->p_free) {
		    /* lost race with page_reclaim */
		    l.vmpageidlockpl = l.puselockpl;	
		    l.puselockpl = VM_PAGE_IPL;
    	    	    PAGE_USEUNLOCK(pp);
		    continue;
		}
		ASSERT(!PAGE_IN_USE(pp));
    	    }

	    /* remove from free list & writer lock page */
	    page_reuse(pp);

	    page_add(&plist, pp);
	    npages_per_type[type]--;
	    npages--;

	    if (flags & P_RETURN_PAGEUSELOCKED) {
		/*
		 * Caller will do PAGE_USEUNLOCK().
		 */
		ASSERT(npages == 0);
		CHECKFREE();
		return (plist);
	    } else {
		ASSERT(VM_PAGEIDLOCK_OWNED());
		l.vmpageidlockpl = l.puselockpl;
		l.puselockpl = VM_PAGE_IPL;
		PAGE_USEUNLOCK(pp);
	    }
    	}   /* while loop */
    }	/* for loop */
    CHECKFREE();
    return (plist);
}


#define ALIGNED_PFN(pfn) \
		(((pfn) & start_align) == start_off && \
		 (((pfn) ^ ((pfn) + npages - 1)) & boundary) == 0)

#define FREECLEAN_PP(pp) ((pp)->p_free && !(pp)->p_mod)

/*
 * page_t *
 * page_get_aligned(uint_t bytes, paddr_t start_align, paddr_t start_off,
 *			paddr_t boundary, uint_t flags)
 *
 *	Special version of page_get() which enforces specified alignment
 *	constraints on the physical address.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They remain
 *	unlocked at return.
 *
 *	The first page is guaranteed to have its physical address satisfy:
 *		(paddr & (start_align - 1)) == start_off
 *	The remaining pages will be physically contigous.
 *	If boundary != 0, the remaining pages are guaranteed not to have:
 *		(paddr & (boundary - 1)) == 0
 *	(this is used to ensure pages do not cross a certain boundary).
 *
 *	The caller must specify alignment which is at least PAGESIZE aligned
 *	since that is the granularity of memory allocation.  Therefore,
 *	both start_align and start_off must be a multiple of PAGESIZE; i.e.,
 *		((start_align | start_off) & PAGEOFFSET) == 0
 *
 *	If boundary != 0, it must also be a multiple of PAGESIZE, and must
 *	be >= the number of bytes requested.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *		P_DMA
 *		      DMA-able pages required. Ignored if restricted DMA
 *		      support is not compiled into the kernel or if
 *		      rdma_mode == RDMA_DISABLED.
 *
 *		P_NODMA
 *		      STD_PAGEs are preferred over PAD_PAGEs. Ignored if
 *		      restricted DMA support is not compiled into the
 *		      kernel or if rdma_mode != RDMA_SMALL.
 *
 *		If neither of P_DMA or P_NODMA are specified, if restricted
 *		DMA support is compiled into the kernel, and if
 *		rdma_mode == RDMA_SMALL, then PAD_PAGES ars preferred over
 *		STD_PAGEs.
 *
 *	On failure, returns NULL.  (Only possible if NOSLEEP is specified.)
 *
 *	On success returns a doubly linked, circular list of pages.
 *	Each returned page has the following state:
 *		- page writer locked (p_invalid == 1 && p_activecnt == 1)
 *		- no vnode identity (p_vnode == NULL && p_offset == NULL)
 *		- PAGE_USEUNLOCK()'d
 *
 * Remarks:
 *
 *	If the caller specifies SLEEP, it is possible that this function
 *	will never return since all memory which matches the specified
 *	alignment constraints may be being used "permanently".  I.e.,
 *	the memory may be memory locked into a process address space which
 *	will never unlock it, or used in a long-term kmem_alloc().
 *
 *	In such a case, or in any case where the system must turn over
 *	lots of memory before the alignment constraints can be satisfied,
 *	this routine is currently implemented in such a way that it wakes up
 *	for every page that's freed and rescans all of physical memory.
 *	So it places a load on the system rather than staying quietly
 *	out of the way.
 */
page_t *
page_get_aligned(uint_t bytes, paddr_t start_align, paddr_t start_off,
		 paddr_t boundary, uint_t flags)
{
	struct pp_chunk *chp;
	page_t *pp;
	page_t *epage;
	page_t **ppp;
	page_t *plist;
	int	npages, numpages;
	uint_t	pfn;
	boolean_t  dropped_vmpagelocks;	/* dummy */
	int	type;
#ifdef DEBUG
	uint_t	foundcandidate = 0;
	pl_t 	savpl;
#endif

#if (NOSLEEP == 0)
#error page_get_aligned() code assumes NOSLEEP != 0
#endif

	ASSERT(bytes != 0);
	ASSERT(((start_align | start_off) & PAGEOFFSET) == 0);
	ASSERT((boundary & PAGEOFFSET) == 0);
	ASSERT(boundary == 0 || bytes <= boundary);
		/* no flags besides NOSLEEP, P_DMA or P_NODMA */
	ASSERT(!(flags & ~(NOSLEEP|P_DMA|P_NODMA)));
	ASSERT((flags & (P_DMA|P_NODMA)) != (P_DMA|P_NODMA));

	/*
	 * Convert to MMU_PAGESIZE units and masks to make it easy for
	 * ALIGNED_PFN() to compare against page_pptonum().
	 */
	start_align = mmu_btop(start_align - 1);
	start_off   = mmu_btop(start_off);
	if (boundary != 0)
		boundary = ~mmu_btop(boundary - 1);

#ifdef DEBUG
	savpl = getpl();
#endif
	VM_PAGEIDWRLOCK();    /* We need to call page_reuse() later */  
	ASSERT(l.vmpageidlockpl == savpl);
	VM_PAGEFREELOCK();

	BUMPPGCOUNT(pagecnt.pc_getaligned);

restart:
	npages = btopr(bytes);
	plist = NULL;

	/*
	 * Reserve the requested number of pages on the free list.
	 */

try_again:
	if (!freemem_resv_one_type(npages, flags, &type, 
			&dropped_vmpagelocks)) {
		ASSERT(flags & NOSLEEP);
		BUMPPGCOUNT(pagecnt.pc_getaligned_fail_NOSLEEP);
		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		return ((page_t *)NULL);
	}

	/*
	 * Scan pages looking for a large enough physically contiguous
	 * run of free, clean pages whose first page matches the
	 * requested physical address alignment.
	 *
	 * Note:  If pages are free but dirty, they are vnode pages
	 * that cannot be reused until someone cleans them.
	 */

	ppp = &page_cs_table[0];
	pp = *ppp;
	do {
		++ppp;
#ifndef NO_RDMA
		if (pp->p_type != type)
			continue;
#endif
		epage = *ppp - (npages - 1);
		while (pp < epage) {
			/* Skip page if not properly aligned */
			pfn = page_pptonum(pp);
			if (!ALIGNED_PFN(pfn)) {
				++pp;
				continue;
			}
#ifdef DEBUG
			foundcandidate = 1;
				
#endif
			/* A dirty page must have vnode identity */
			ASSERT(!(pp->p_mod && (pp->p_vnode == NULL)));

			/* Skip page if not free and clean */
			if (!FREECLEAN_PP(pp)) {
				++pp;
				continue;
			}
			numpages = npages;
			do {
				++pp;
				if (--numpages == 0)
					goto found_pages;
			} while (FREECLEAN_PP(pp));
			++pp;
		}
	} while ((pp = *ppp) != epages);
	ASSERT(foundcandidate);	/* should find at least one candidate */

	/*
	 * Can't find a big enough chunk with the proper alignment.
	 *
	 * Cancel our memory reservation.  (We do this ourselves
	 * rather than call freemem_unresv() because we know that
	 * no additional requesters have blocked waiting for freemem
	 * since we did our reservation because we've held VM_PAGEFREELOCK()
	 * the entire time.)
	 *
	 * Wait for any new memory to be freed and then retry.
	 */

	BUMPPGCOUNT(pagecnt.pc_getaligned_unavail);

	mem_freemem[type] += npages;	/* unreserve the pages */

	if (flags & NOSLEEP) {
		BUMPPGCOUNT(pagecnt.pc_getaligned_fail_NOSLEEP);
		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		return ((page_t *)NULL);    /* failure */
	}

	wait_for_freemem(WRITE_LOCKED);
	goto try_again;

	/*
	 * We found a big enough chunk with the proper alignemnt.
	 *
	 * Pull the pages off the free list and build the return list.
	 */

found_pages:

	while (npages) {

		pp--;

		if (PAGE_TRYUSELOCK(pp) == INVPL) {
			/*
			 * Couldn't immediately lock the page; page_reclaim()
			 * has got it.  If there's only enough freemem for
			 * one of us, page_reclaim() will defer to us (since
			 * we already decremented freemem thereby reserving
			 * it).  If there is enough freemem for both of us,
			 * page_reclaim() will take this page.  So spin
			 * waiting to lock the page to determine which it is.
			 *
			 * Must VM_PAGEUNLOCK() in order to PAGE_USELOCK(pp).
			 * However, since we will release them out of order,
			 * adjust the cached ipl levels.
			 */

			VM_PAGEFREEUNLOCK();
			VM_PAGEIDUNLOCK();
			ASSERT(getpl() == savpl);
			PAGE_USELOCK(pp);
			VM_PAGEIDWRLOCK();
			VM_PAGEFREELOCK();
			/* we are going to drop uselock out of order */
			l.vmpageidlockpl = l.puselockpl;

			if (!FREECLEAN_PP(pp)) {
				/*
				 * Lost race with page_reclaim.
				 */

				l.puselockpl = VM_PAGE_IPL;
				PAGE_USEUNLOCK(pp);

				/* unreserve any pages we didn't collect */
				freemem_unresv(npages, type);

				/* Free any pages we collected */
				VM_PAGEFREEUNLOCK();
				VM_PAGEIDUNLOCK();
				ASSERT(getpl() == savpl);
				page_list_unlock(plist);
				VM_PAGEIDWRLOCK();
				VM_PAGEFREELOCK();
				goto restart;
			}
		}  /* endif TRYLOCK() */

		page_reuse(pp);	/* remove from free list & writer lock page */
		page_add(&plist, pp);
		npages--;

		/*
		 * dropping uselock out of order here; we have already
		 * set the vmpageidlock pl correctly before.
		 */
		l.puselockpl = VM_PAGE_IPL;
		PAGE_USEUNLOCK(pp);
	}   /* end of while (npages) */

	CHECKFREE();
	VM_PAGEFREEUNLOCK();
	VM_PAGEIDUNLOCK();
	ASSERT(getpl() == savpl);
	return (plist);
}


#ifdef NO_RDMA

/*
 * STATIC int
 * freemem_resv_std(uint_t npages, uint_t flags, 
 *		boolean_t *dropped_vmpagelocks, enum page_lock_mode lockmode)
 *
 *	Reserve physical memory pages of STD_PAGE type from the page free 
 *	lists.
 *	(Note this is different from mem_resv().)
 *	This variant is used when only one page type is present in the 
 *	system.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() locked and VM_PAGEIDLOCK may also be
 *	locked indicated by the lockmode arg. It remains locked at return.
 *
 *	npages is the number of pages to reserve.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *	dropped_vmpagelocks is an out arg which is set on return:
 *		0 - Both the id and the free list locks were held without
 *		    ever dropping them.
 *		1 - Both the locks were dropped due to blocking.
 *		It will always be 0 if NOSLEEP was specified.
 *
 *	Returns true (non-zero) if the pages were successfully reserved.
 *	(dropped_vmpagelocks may be non-zero.)
 *
 *	Returns false (zero) if the pages were not successfully reserved.
 *	Only possible if NOSLEEP is specified.
 *	(dropped_vmpagelocks will be zero.)
 */
STATIC boolean_t
freemem_resv_std(uint_t npages, uint_t flags, boolean_t *dropped_vmpagelocks,
		enum page_lock_mode lockmode)
{
#if (NOSLEEP == 0)
#error freemem_resv_std() code assumes NOSLEEP != 0
#endif
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT((lockmode == NOT_LOCKED) || VM_PAGEIDLOCK_OWNED());
	ASSERT(dropped_vmpagelocks != (boolean_t *)NULL);
	ASSERT(npages != 0);

	*dropped_vmpagelocks = B_FALSE;	/* haven't VM_PAGEUNLOCK()'d yet */

	if (npages > maxfreemem[STD_PAGE]) {
		/*
		 *+ An attempt was made to allocate more pages of
		 *+ physical memory than could ever be available.
		 */
		cmn_err(CE_PANIC,
			"freemem_resv_std(): Request (%d) > maxfreemem (%d)",
			npages, maxfreemem[STD_PAGE]);
	}

	/*
	 * Wait until there is sufficient memory.
	 */

	while (mem_freemem[STD_PAGE] < npages) {
		if (flags & NOSLEEP) {
			return (B_FALSE);	/* failure */
		}
		wait_for_freemem(lockmode);
		/* wait_for_freemem() drops the locks */
		*dropped_vmpagelocks = B_TRUE;
	}

	/*
	 * There's enough free memory to satisfy our entire request.
	 * Reserve that memory now.  (This is a reservation instead
	 * of an actual allocation in the case where the caller needs
	 * to drop VM_PAGEFREELOCK before pulling all these pages off
	 * the freelists.)
	 */

	mem_freemem[STD_PAGE] -= npages;

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back. 
	 *
	 * PERF: Check against lotsfree rather than desfree?
	 */
	if (mem_freemem[STD_PAGE] < desfree)
		outofmem();

	return (B_TRUE);	/* success */
}

#else /* !NO_RDMA */

/*
 * STATIC int
 * freemem_resv(uint_t npages, uint_t flags, uint_t npages_per_type[],
 *		boolean_t *dropped_vmpagelocks)
 *
 *	Reserve physical memory pages from the page free lists.
 *	(Note this is different from mem_resv().)
 *	This variant picks pages of one or more types, based on the
 *	best fit with the specified flags.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDWRLOCK() locked.
 *	They remain locked at return.
 *
 *	npages is the number of pages to reserve.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *		P_DMA
 *		      DMA-able pages required. Ignored if restricted DMA
 *		      support is not compiled into the kernel or if
 *		      rdma_mode == RDMA_DISABLED.
 *
 *		P_NODMA
 *		      STD_PAGEs are preferred over PAD_PAGEs. Ignored if
 *		      restricted DMA support is not compiled into the
 *		      kernel or if rdma_mode != RDMA_SMALL.
 *
 *		If neither of P_DMA or P_NODMA are specified, if restricted
 *		DMA support is compiled into the kernel, and if
 *		rdma_mode == RDMA_SMALL, then PAD_PAGES ars preferred over
 *		STD_PAGEs.
 *
 *	npages_per_type is an out arg which is filled in with the number
 *	of pages of each type which were reserved; the sum of these
 *	counts will equal npages if returned successfully.
 *
 *	dropped_vmpagelocks is an out arg which is set on return:
 *		0 - vm_pagefreelock and vm_pageidlock were held without ever
 *		    dropping them.
 *		1 - Both the locks were dropped due to blocking.
 *		It will always be 0 if NOSLEEP was specified.
 *
 *	Returns true (non-zero) if the pages were successfully reserved.
 *	(dropped_vmpagelocks may be non-zero.)
 *
 *	Returns false (zero) if the pages were not successfully reserved.
 *	Only possible if NOSLEEP is specified.
 *	(dropped_vmpagelocks will be zero.)
 *
 * Remarks:
 *	 PERF: If freemem drops below desfree, we start the wheels turning
 *	       to get some back. Should we check against lotsfree rather
 *	       than desfree?
 */
STATIC boolean_t
freemem_resv(uint_t npages, uint_t flags, uint_t npages_per_type[],
	     boolean_t *dropped_vmpagelocks)
{
	int tcode, ptype, stype, tfreemem, hwm, surplus, temp;

#if (NOSLEEP == 0)
#error freemem_resv() code assumes NOSLEEP != 0
#endif
	ASSERT((flags & (P_DMA|P_NODMA)) != (P_DMA|P_NODMA));
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT(VM_PAGEIDLOCK_OWNED());
	ASSERT(dropped_vmpagelocks != (boolean_t *)NULL);
	ASSERT(npages != 0);

	*dropped_vmpagelocks = B_FALSE;
	npages_per_type[STD_PAGE] =
		npages_per_type[DMA_PAGE] = npages_per_type[PAD_PAGE] = 0;
	tcode = (flags & (P_DMA|P_NODMA));
	ptype = page_ptype_map[tcode];
	stype = page_stype_map[tcode];
	hwm = maxfreemem[ptype] / 2;

	/*
	 * Wait until there is sufficient memory.
	 */
	for (;;) {
		/*
		 * Optimization: Check for an allocation from the primary
		 *		 page pool, also checking that freemem is
		 *		 above the hwm threshold (which also
		 *		 means it's above the desfree threshold).
		 */
		tfreemem = mem_freemem[ptype];
		if (npages + hwm <= tfreemem) {
			/*
			 * The primary page pool contains enough memory
			 * to satisfy the request. Reserve it now (see
			 * comment at the end of this routine).
			 */
			mem_freemem[ptype] = tfreemem - npages;
			npages_per_type[ptype] = npages;
			return (B_TRUE);
		}

		/*
		 * Optimization 2: Check that both the primary and secondary
		 *	           pool together can satisfy the request
		 *		   without crossing the ``desfree' threshold
		 *		   see PERF comment above).
		 */
		tfreemem += mem_freemem[stype];
		if (npages + mem_desfree[ptype] + mem_desfree[stype] <=
		    tfreemem)
			break;

		/*
		 * If we can still satisfy the request, it will leave us
		 * with too little free memory, start the wheels turning
		 * to get some back (see PERF comment above). 
		 */
		if (npages <= tfreemem) {
			outofmem();
			break;
		}

		if (npages > maxfreemem[ptype] + maxfreemem[stype]) {
			/*
			 *+ An attempt was made to allocate more pages of
			 *+ physical memory than could ever be available.
			 */
			cmn_err(CE_PANIC,
				"freemem_resv: Request(%d) > max(%d)",
				npages, maxfreemem[ptype] + maxfreemem[stype]);
			/* NOTREACHED */
		}

		if (flags & NOSLEEP)
			return (B_FALSE);	/* failure */

		wait_for_freemem(WRITE_LOCKED);
		/* wait_for_freemem() drops the locks */
		*dropped_vmpagelocks = B_TRUE;
	}

	/*
	 * There's enough free memory to satisfy our entire request.
	 * Reserve that memory now.  (This is a reservation instead
	 * of an actual allocation in the case where the caller needs
	 * to VM_PAGEFREEUNLOCK() before pulling all these pages off
	 * the freelists.)
	 *
	 * Our preference policy for pages is:
	 *
	 *	1. Free pages of the primary page type,
	 *
	 *	2. Cache list pages of the primary page type, but not
	 *	   enough to bring it down below 50%. The reason for this
	 *	   is that draining the free pages of this type down too
	 *	   far will have the effect of reducing cache list
	 *	   residency time, with negative performance consequences.
	 *
	 *	3. Free or cache pages from the avail type (primary or
	 *	   secondary) which has more free pages left.
	 *
	 *	4. Any remaining free pages.
	 */
	if (stype != NO_PAGE) {
		ASSERT(page_cachelist_size[ptype] +
				page_freelist_size[ptype] ==
					mem_freemem[ptype]);

		/*
		 * First, use up all the free list pages
		 * of the primary page type.
		 */
		surplus = page_freelist_size[ptype];
		if (npages <= surplus)
			goto alloc_ptype;
		npages_per_type[ptype] = surplus;
		mem_freemem[ptype] -= surplus;
		npages -= surplus;

		/*
		 * Now, use up primary pages from the cache list, but do not
		 * drain it below maxfreemem[ptype] / 2.
		 */
		if (mem_freemem[ptype] > hwm) {
			surplus = mem_freemem[ptype] - hwm;
			if (npages <= surplus)
				goto alloc_ptype;
			mem_freemem[ptype] -= surplus;
			npages_per_type[ptype] += surplus;
			npages -= surplus;
		}

		/*
		 * Now use the pool with more free pages.
		 */
		if (mem_freemem[stype] >= mem_freemem[ptype]) {
			if (npages <= mem_freemem[stype]) {
				ptype = stype;
				goto alloc_ptype;
			}
		} else {
			if (npages <= mem_freemem[ptype])
				goto alloc_ptype;
			temp = ptype;
			ptype = stype;
			stype = temp;
		}
		npages_per_type[stype] += mem_freemem[stype];
		npages -= mem_freemem[stype];
		mem_freemem[stype] = 0;
	}

alloc_ptype:
	ASSERT(mem_freemem[ptype] >= npages);
	npages_per_type[ptype] += npages;
	mem_freemem[ptype] -= npages;

	return (B_TRUE);	/* success */
}

/*
 * STATIC int
 * freemem_resv_one_type(uint_t npages, uint_t flags, int *typep,
 *		 	 boolean_t_t *dropped_vmpagelocks)
 *
 *	Reserve physical memory pages from the page free lists,
 *	all of the same type.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDWRLOCK() locked.
 *	They remain locked at return.
 *
 *	npages is the number of pages to reserve.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *		P_DMA
 *		      DMA-able pages required. Ignored if restricted DMA
 *		      support is not compiled into the kernel or if
 *		      rdma_mode == RDMA_DISABLED.
 *
 *		P_NODMA
 *		      STD_PAGEs are preferred over PAD_PAGEs. Ignored if
 *		      restricted DMA support is not compiled into the
 *		      kernel or if rdma_mode != RDMA_SMALL.
 *
 *		If neither of P_DMA or P_NODMA are specified, if restricted
 *		DMA support is compiled into the kernel, and if
 *		rdma_mode == RDMA_SMALL, then PAD_PAGES ars preferred over
 *		STD_PAGEs.
 *
 *	typep is an out arg which is set on return to the page type
 *	(STD_PAGE, etc.) which was reserved.
 *
 *	dropped_vmpagelocks is an out arg which is set on return:
 *		0 - Both VM_PAGEFREELOCK() and VM_PAGEIDWRLOCK() were held
 *		    without ever dropping it
 *		1 - Both the locks were dropped due to blocking.
 *		It will always be 0 if NOSLEEP was specified.
 *
 *	Returns true (non-zero) if the pages were successfully reserved.
 *	(dropped_vmpagelocks may be non-zero.)
 *
 *	Returns false (zero) if the pages were not successfully reserved.
 *	Only possible if NOSLEEP is specified.
 *	(dropped_vmpagelocks will be zero.)
 */
STATIC boolean_t
freemem_resv_one_type(uint_t npages, uint_t flags, int *typep,
		      boolean_t *dropped_vmpagelocks)
{
	uint_t pmem_free, smem_free;
	int tcode, ptype, stype, type, tdesfree;

#if (NOSLEEP == 0)
#error freemem_resv_one_type() code assumes NOSLEEP != 0
#endif
	ASSERT((flags & (P_DMA|P_NODMA)) != (P_DMA|P_NODMA));
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT(VM_PAGEIDLOCK_OWNED());
        ASSERT(dropped_vmpagelocks != (boolean_t *)NULL);
	ASSERT(npages != 0);


	*dropped_vmpagelocks = B_FALSE;
	tcode = (flags & (P_DMA|P_NODMA));
	ptype = page_ptype_map[tcode];
	stype = page_stype_map[tcode];
	tdesfree = mem_desfree[ptype] + mem_desfree[stype];

	if (npages > maxfreemem[ptype] && npages > maxfreemem[stype]) {
		/*
		 *+ An attempt was made to allocate more pages of
		 *+ physical memory than could ever be available.
		 */
		cmn_err(CE_PANIC,
			"freemem_resv_one_type(): Request (%d) > "
			"maxfreemem[%d] (%d)",
			npages, type, maxfreemem[type]);
	}

	/*
	 * Wait until there is sufficient memory.
	 */

	for (;;) {
		pmem_free = mem_freemem[ptype];
		smem_free = mem_freemem[stype];
		if (pmem_free >= smem_free) {
			if (pmem_free >= npages) {
				type = ptype;
				break;
			}
		} else {
			if (smem_free >= npages) {
				type = stype;
				break;
			}
		}
		if (flags & NOSLEEP) {
			return (B_FALSE);	/* failure */
		}
		wait_for_freemem(WRITE_LOCKED);
		/* wait_for_freemem() drops the locks */
		*dropped_vmpagelocks = B_TRUE;
	}

	/*
	 * There's enough free memory to satisfy our entire request.
	 * Reserve that memory now.  (This is a reservation instead
	 * of an actual allocation in the case where the caller needs
	 * to VM_PAGEFREEUNLOCK() before pulling all these pages off
	 * the freelists.)
	 */

	mem_freemem[type] -= npages;
	*typep = type;

	/*
	 * If satisfying this request has left us with too little memory,
	 * start the wheels turning to get some back.
	 * PERF: Check against lotsfree instead of desfree?
	 */
	if (mem_freemem[stype] + mem_freemem[ptype] < tdesfree)
		outofmem();

	return (B_TRUE);	/* success */
}

/*
 * STATIC int
 * freemem_resv_type(uint_t npages, int type, uint_t flags,
 *		  boolean_t *dropped_vmpagelocks, enum page_lock_mode lockmode)
 *
 *	Reserve physical memory pages of a given type from the page free
 *	lists.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() locked. VM_PAGEIDLOCK() is indicated by
 *	lockmode arg. They remain locked at return.
 *
 *	npages is the number of pages to reserve.
 *	type is the page type (STD_PAGE, etc.) to reserve.
 *
 *	flags:
 *		SLEEP
 *		      Caller can tolerate blocking to wait for memory.
 *
 *		NOSLEEP
 *		      Opposite of SLEEP.  (It is an error to specify
 *		      both NOSLEEP and SLEEP.)
 *
 *	dropped_vmpagelocks is an out arg which is set on return:
 *		0 - VM_PAGEFREELOCK() and VM_PAGEIDLOCK() were held without
 *		    ever dropping them
 *		1 - Both the  locks were dropped due to blocking.
 *		It will always be 0 if NOSLEEP was specified.
 *
 *	Returns true (non-zero) if the pages were successfully reserved.
 *	(dropped_vmpagelocks may be non-zero.)
 *
 *	Returns false (zero) if the pages were not successfully reserved.
 *	Only possible if NOSLEEP is specified.
 *	(dropped_vmpagelocks will be zero.)
 */
STATIC boolean_t
freemem_resv_type(uint_t npages, int type, uint_t flags,
		boolean_t *dropped_vmpagelocks, enum page_lock_mode lockmode)
{
#if (NOSLEEP == 0)
#error freemem_resv_type() code assumes NOSLEEP != 0
#endif
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT(dropped_vmpagelocks != (boolean_t *)NULL);
	ASSERT(npages != 0);
 
	*dropped_vmpagelocks = B_FALSE;

	if (npages > maxfreemem[type]) {
		/*
		 *+ An attempt was made to allocate more pages of
		 *+ physical memory than could ever be available.
		 */
		cmn_err(CE_PANIC,
			"freemem_resv_type(): Request (%d) > "
			"maxfreemem[%d] (%d)",
			npages, type, maxfreemem[type]);
	}

	/*
	 * Wait until there is sufficient memory.
	 */

	for (;;) {
		if (mem_freemem[type] >= npages)
			break;
		if (flags & NOSLEEP) {
			return (B_FALSE);	/* failure */
		}
		wait_for_freemem(lockmode);
		/* wait_for_freemem() drops the locks */
		*dropped_vmpagelocks = B_TRUE;
	}

	/*
	 * There's enough free memory to satisfy our entire request.
	 * Reserve that memory now.  (This is a reservation instead
	 * of an actual allocation in the case where the caller needs
	 * to drop VM_PAGELOCK before pulling all these pages off
	 * the freelists.)
	 */
	mem_freemem[type] -= npages;

	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back. 
	 *
	 * PERF: Check against lotsfree rather than desfree?
	 */
	if (mem_freemem[type] < mem_desfree[type])
		outofmem();

	return (B_TRUE);	/* success */
}

#endif /* NO_RMDA */

/*
 * STATIC void
 * freemem_unresv(uint_t npages, int type)
 *
 *	Unreserve physical memory pages from the page free lists.
 *	(Note this is different from mem_unresv().)
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() locked.  It remains locked at return.
 *
 *	npages is the number of pages to unreserve.
 *	type is the page type (STD_PAGE, etc.) to unreserve.
 */
STATIC void
#ifndef NO_RDMA
freemem_unresv (uint_t npages, int type)
#else
freemem_unresv_std (uint_t npages)
#endif
{
	ASSERT(VM_PAGEFREELOCK_OWNED());

	ASSERT(npages != 0);

#ifdef NO_RDMA
	mem_freemem[STD_PAGE] += npages;
#else /* !NO_RDMA */
	mem_freemem[type] += npages;
#endif

	/* Wake anyone waiting for memory (either directly or indirectly) */

	if (SV_BLKD(&vm_waitmem))
		SV_BROADCAST(&vm_waitmem, 0);
	poolrefresh_nudge();
}


/*
 * STATIC void
 * wait_for_freemem(enum page_lock_mode lockmode)
 *
 *	Wait for more physical memory to become available.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() locked.  It remains locked at return.
 *	VM_PAGEIDLOCK() may or may not be held indicated by lockmode.
 *
 *	VM_PAGEFREELOCK() and VM_PAGEIDLOCK() will be dropped and reacquired
 *	in this routine.
 *
 *	Returns when more memory may be available.
 *	(Caller must recheck freemem.)
 */
STATIC void
wait_for_freemem(enum page_lock_mode lockmode)
{
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT((lockmode == NOT_LOCKED) || VM_PAGEIDLOCK_OWNED());

	/*
	 * 	Before waiting, we try to arrange to get more pages by
	 *	processing the I/O completion list and prodding the
	 *	pageout daemon.  However, there's nothing to guarantee
	 *	that these actions will provide enough pages to satisfy
	 *	the request.  In particular, the pageout daemon stops
	 *	running when freemem > lotsfree, so if npages > lotsfree
	 *	there's nothing going on that will bring freemem up to
	 *	a value large enough to satisfy the request.
	 */

	/*
	 * Given that we can wait, call cleanup directly to give
	 * it a chance to add pages to the free list.  This strategy
	 * avoids the cost of context switching to the pageout
	 * daemon unless it's really necessary.
	 */
	if (bclnlist != NULL) {
		VM_PAGEFREEUNLOCK();
    	    	if (lockmode != NOT_LOCKED)
			VM_PAGEIDUNLOCK();

		cleanup();

    	    	if (lockmode == WRITE_LOCKED)
			VM_PAGEIDWRLOCK();
		else if (lockmode == READ_LOCKED)
			VM_PAGEIDRDLOCK();
		VM_PAGEFREELOCK();
		return;		/* more memory may be available now */
	}

	/*
	 * There's nothing immediate waiting to become available.
	 * Signal out of memory.
	 */
	outofmem();

	/*
	 * Block waiting for more memory.
	 */

	if (lockmode != NOT_LOCKED) {
		/* out of order lock drop */
		l.vmpageidlockpl = vm_pagefreelockpl;
		VM_PAGEIDUNLOCK();
	}

	/* SV_WAIT VM_PAGEFREEUNLOCK()s at PLBASE */
	SV_WAIT(&vm_waitmem, PRIMEM-2, &vm_pagefreelock);

	if (lockmode == WRITE_LOCKED)
		VM_PAGEIDWRLOCK();
	else if (lockmode == READ_LOCKED)
		VM_PAGEIDRDLOCK();

	VM_PAGEFREELOCK();

	return;		/* more memory may be available now */
}

/*
 * STATIC void
 * outofmem(void)
 *
 *	Fire off daemons and such to attempt to get more free memory.
 *
 * Calling/Exit State:
 *
 *	Called when memory is low or unavailable, and more memory has
 *	been requested. VM_PAGEFREELOCK is held on entry and still held on
 *	exit. VM_PAGEIDLOCK() may or may not be held.
 */
STATIC void
outofmem(void)
{
	ASSERT(VM_PAGEFREELOCK_OWNED());

	/* Try to get private pools to release some of their memory. */
	poolrefresh_outofmem();

	/* kick-off pageout() here */
	if (freemem < lotsfree &&
	     (page_dirtyflist != NULL || page_dirtyalist != NULL))
		EVENT_SIGNAL(&pageout_event, 0);
}

/*
 * PAGE_RECLAIM(page_t *pp)
 *
 *	Attempt to reclaim the given page from the free list, if it is free.
 *
 * page_reclaim(page_t *pp)
 *
 *	Attempt to reclaim the given page from the free list.
 *
 * Calling/Exit State:
 *
 *	PAGE_RECLAIM() tolerates the page not being free.
 *	page_reclaim() requires the caller to guarantee the page is
 *		currently free.
 *
 *	Both are otherwise identical.
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDLOCK() unlocked.
 *	They remain unlocked on return. 
 *	Caller passes page PAGE_USELOCK()'d.  It is returned still locked.
 *
 *	Returns 1 if the page was successfully reclaimed from the free list.
 *
 *	Returns 0 if the page was already reserved by page_get().
 *	The caller must then PAGE_USEUNLOCK(pp) and forget about it.
 *	The page vnode identity (if any) is destroyed allowing the caller
 *	to create a new page with the same identity.
 *
 *	We (currently) always also have exclusive access to the page
 *	("effective" writer lock), but it is not required here since we
 *	aren't changing the page identity.
 *
 *	Also, currently the page always has vnode identity.
 *
 *	See page_reclaim_l() for locked versions.
 */
int
page_reclaim(page_t *pp)
{
	int i;

	VM_PAGEFREELOCK();
	i = page_reclaim_l(pp, NOT_LOCKED, P_LOOKUP_OR_CREATE);
	VM_PAGEFREEUNLOCK();
	ASSERT(!i || !pp->p_free);
	return (i);
}

/*
 * page_reclaim_l(page_t *pp, enum page_lock_mode locktype, u_int flags)
 *
 *	Attempt to reclaim the given page from the free list.
 *	Caller holds VM_PAGEFREELOCK() and may hold VM_PAGEIDLOCK(),
 *	indicated by the locktype argument.
 *
 * Calling/Exit State:
 *
 *	page_reclaim_l() requires the caller to guarantee the page is
 *		currently free.
 *
 *	Called with VM_PAGEFREELOCK() locked and may also hold VM_PAGEIDLOCK()
 *	indicated by locktype argument. They remain locked on return.
 *	Caller passes page PAGE_USELOCK()'d.  It is returned still locked.
 *
 *	Returns 1 if the page was successfully reclaimed from the free list.
 *
 *	Returns 0 if the page was already reserved by page_get().
 *	The caller must then PAGE_USEUNLOCK(pp) and forget about it.
 *	The page vnode identity (if any) is destroyed allowing the caller
 *	to create a new page with the same identity.
 *
 *	If flags specifies P_LOOKUP_OR_CREATE, then the page vnode identity
 *	 (if any) is destroyed allowing the caller to create a new page with
 *	the same identity.
 *
 *	We (currently) always also have exclusive access to the page
 *	("effective" writer lock), but it is not required here since we
 *	aren't changing the page identity.
 *
 *	Also, currently the page always has vnode identity.
 *
 *	See PAGE_RECLAIM() / page_reclaim() for unlocked versions.
 */
int
page_reclaim_l(page_t *pp, enum page_lock_mode locktype, u_int flags)
{
	int i;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT(PAGE_USELOCK_OWNED(pp));
       	ASSERT((locktype == NOT_LOCKED) || VM_PAGEIDLOCK_OWNED());
	ASSERT(pp->p_free);
	ASSERT(pp->p_vnode != (vnode_t *)NULL);

	i = page_unfree_l(pp, B_FALSE, locktype, flags);

	if (!i) {
		BUMPPGCOUNT(pagecnt.pc_reclaim_lost);
		return (0);	/* page is reserved; can't reclaim */
	}
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(!pp->p_free);

	/* Update statistics */
	BUMPPGCOUNT(pagecnt.pc_reclaim);

#ifdef DEBUG
	if (locktype != NOT_LOCKED)
		CHECK(pp->p_vnode);
#endif

	return (1);	/* successful reclaim */
}

/*
 * STATIC void
 * page_reuse(page_t *pp)
 *
 *	Remove a page from the free list, destroy it's vnode identity,
 *	and page writer lock it.
 *
 * Calling/Exit State:
 *
 *	Called with both VM_PAGEFREELOCK() and VM_PAGEIDWRLOCK() locked.
 *	They remain locked at return.
 *
 *	The caller passes the page in the following state:
 *      	- PAGE_USELOCK()'d
 *      	- on the free list
 *
 *	On return, the page is in the following state:
 *		- PAGE_USELOCK()'d
 *		- page writer locked
 *		- no vnode identity
 */
STATIC void
page_reuse(page_t *pp)
{
	ASSERT(VM_PAGEFREELOCK_OWNED());    	
	ASSERT(VM_PAGEIDLOCK_OWNED());
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(!PAGE_IN_USE(pp));

	if (!page_unfree_l(pp, B_TRUE, WRITE_LOCKED, P_LOOKUP_OR_CREATE)) {
		/*
		 *+ During memory allocation, a request for a page
		 *+ known to be available was refused.
		 *+ This indicates a kernel software problem.
		 *+ Corrective action: none.
		 */
		cmn_err(CE_PANIC, "page_reuse: page_unfree_l failed");
	}

	ASSERT(!pp->p_mod);	/* other ASSERTs made by page_unfree_l() */
	ASSERT(!pp->p_free);

	if (pp->p_vnode != NULL) {
		CHECK(pp->p_vnode);
		page_hashout(pp);	/* destroy old vnode association */
		BUMPPGCOUNT(pagecnt.pc_get_cache);
	} else {
		BUMPPGCOUNT(pagecnt.pc_get_free);
	}

	/* Establish a writer lock on the page.  */
	pp->p_activecnt = 1;
	pp->p_invalid = 1;
}


/*
 * STATIC int
 * page_unfree_l(page_t *pp, boolean_t freemem_resvd, 
 *		enum page_lock_mode locktype, u_int flags)
 *
 *	Attempt to remove a page from the free list.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and may also have VM_PAGEIDLOCK()
 *	locked indicated by locktype arg. They remain locked at return.
 *
 *	Caller passes page PAGE_USELOCK()'d.  It is returned still locked.
 *	Caller ensures the page is currently on the free list (p_free set).
 *
 *	If caller has already reserved this page from the free lists
 *	(decremented freemem) [page_reuse()], it passes B_TRUE for
 *	freemem_resvd (and the page must not be dirty, i.e., p_mod is
 *	zero); otherwise [page_reclaim()] it passes freemem_resvd B_FALSE.
 *
 *	Returns 1 (success) if the page was removed from the free list.
 *
 *	Returns 0 (failure) if the page was already reserved by page_get().
 *	This can only happen when the caller passes non-zero for mustdecrfreemem
 *	and the page is also clean (p_mod is zero).
 *	The caller must then PAGE_USEUNLOCK(pp) and forget about it.
 *	The page vnode identity (if any) is destroyed allowing the caller
 *	to create a new page with the same identity.
 */
STATIC int
page_unfree_l(page_t *pp, boolean_t freemem_resvd, 
		enum page_lock_mode locktype, u_int flags)
{
    boolean_t dropped_vmpagelocks;	/* dummy */
#ifdef NO_RDMA
    const int type = STD_PAGE;
#else
    int type = pp->p_type;
#endif

    ASSERT(VM_PAGEFREELOCK_OWNED());
    ASSERT((locktype == NOT_LOCKED) || VM_PAGEIDLOCK_OWNED());
    ASSERT(pp >= pages && pp < epages);
    ASSERT(PAGE_USELOCK_OWNED(pp));
    ASSERT(pp->p_free);

    /* The following should be true for a free page: */

    ASSERT(pp->p_nio == 0);
    ASSERT(pp->p_activecnt == 0);
    ASSERT(!pp->p_invalid);
    ASSERT(!pp->p_pageout);
    ASSERT(!pp->p_ioerr);
    ASSERT(pp->p_mapping == NULL);
    ASSERT(!PAGE_IN_USE(pp));

    if (pp->p_vnode == NULL) {
    	if (!freemem_resvd) {
    	    /*
	     * We never get here since freemem_resvd
	     * is set to B_FALSE only by page_reclaim() which only
	     * passes us pages with vnode identity.
	     */
    	    /*
	     *+ An attempt was made to reclaim an unused page.
	     *+ This indicates a kernel software problem.
	     *+ Corrective action:  none.
	     */
	    cmn_err(CE_PANIC,
		 "page_unfree_l: NULL p_vnode && mustdecrfreemem");
    	}
	page_sub(&page_freelist[type], pp);
	page_freelist_size[type]--;
    } else {
	if (pp->p_mod) {
	    if (IS_ANONVP(pp->p_vnode)) {
		page_sub(&page_dirtyalist, pp);
		DECR_LIST_SIZE(page_dirtyalist_size);
		} else {
		    page_sub(&page_dirtyflist, pp);
		    DECR_LIST_SIZE(page_dirtyflist_size);
		}
		--page_dirtylists_size[type];
		/*
		 * If the caller has already decremented freemem,
		 * it would be an error to unfree a dirty page
		 * for which freemem was never incremented.
		 */
		ASSERT(!freemem_resvd);
	} else {
		ASSERT(pp->p_timestamp == 0);

		if (!freemem_resvd &&
			!freemem_resv_type(1, type, NOSLEEP,
			    &dropped_vmpagelocks, locktype)) {
		    /* 
		     * If P_LOOKUP_CREATE is not set, then we don't need to
		     * destroy the vnode identity. We will just return failure.
		     */
		    if (!(flags & P_LOOKUP_OR_CREATE))
			return 0;

		    /*
		     * need to do some fancy stepping since we need to
		     * to acquire the idlock in writer mode.
		     */
		    switch (locktype) {
		    case WRITE_LOCKED:	/* do nothing */
			break;	

		    case READ_LOCKED:
		    	/* have to drop the free lock first */
			VM_PAGEFREEUNLOCK();
			/* have to upgrade to write lock */
			VM_PAGEIDUNLOCK();
			VM_PAGEIDWRLOCK();
			/* reacquire the free lock */
			VM_PAGEFREELOCK();
			break;

		    case NOT_LOCKED:
			/* have to drop the free lock first */
			VM_PAGEFREEUNLOCK();
			VM_PAGEIDWRLOCK();
			/* reacquire the free lock */
			VM_PAGEFREELOCK();
			break;
			
		    default:
			/*
			 *+ unknown locktype.
			 */
			cmn_err(CE_PANIC, "Unknown locktype in page_unfree_l");
			break;
		    }

		    /*
		     * Page already reserved by a racing
		     * page_get_l().  See comment in
		     * page_get_l() discussing the race
		     * with page_reclaim().
		     *
		     * Destroy vnode identity (and move
		     * to non-identity free list) so
		     * caller can recreate a new page
		     * with this identity.
		     */

		    page_sub(&page_cachelist[type], pp);
		    page_cachelist_size[type]--;
		    pp->p_free = 0;

		    page_hashout(pp);

		    ASSERT(VM_PAGEFREELOCK_OWNED());	
		    pp->p_free = 1;
		    page_freelist_size[type]++;
		    ASSERT(!PAGE_IN_USE(pp));	
		    page_add(&page_freelist[type], pp);

		    switch (locktype) {

		    case WRITE_LOCKED:	/* do nothing */
			break;	

		    case READ_LOCKED:
			/* need to drop the free lock */
			VM_PAGEFREEUNLOCK();
			/* have to downgrade to reader lock */
			VM_PAGEIDUNLOCK();
			VM_PAGEIDRDLOCK();
		    	/* have to re-acquire the free lock */
			VM_PAGEFREELOCK();
			break;

		    case NOT_LOCKED:
			VM_PAGEFREEUNLOCK();
			VM_PAGEIDUNLOCK();
		    	/* have to re-acquire the free lock */
			VM_PAGEFREELOCK();
			break;
		    }

		    return (0);   /* page already reserved */
		}  else {  /* if (!freemem_resvd ... ) */
		    page_sub(&page_cachelist[type], pp);
		    page_cachelist_size[type]--;
		}
	    } /* if page is not modified */
	} /* if vnode is not NULL */

	pp->p_free = 0;

	return (1);	/* success */
}

/*
 * void
 * page_free(page_t *pp, uint_t dontneed)
 *
 *	Free an unused page to the appropriate (clean/dirty) free list.
 *	(Called ONLY by page_unlock() and hat functions!)
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDLOCK() unlocked; they are
 *	returned unlocked.
 *
 *	"dontneed" is passed non-zero as a hint that the page is unlikely to
 *	be needed soon or ever.  A non-zero "dontneed" only has meaning
 *	(and should only be specified) if the page has vnode identity.
 *
 *	The caller passes the page in the following state:
 *		- PAGE_USELOCK()'d
 *		- !PAGE_IN_USE(pp)
 *		- not already on the free list
 *
 *	On return, the page is consumed.  The caller does not need to
 *	PAGE_USEUNLOCK() the page.
 *
 *	Note that the passed page has no outstanding reader/writer locks
 *	(since !PAGE_IN_USE(pp)), so this routine does no PAGE_BROADCAST()
 *	to wakeup page waiters.  The caller should have already done this
 *	when it dropped it's reader/writer lock.
 *
 * Remarks:
 *
 *	page_free() should only be called by functions which alter
 *	state which could cause PAGE_IN_USE(pp) to become false;
 *	i.e., functions which decrement pp->p_activecnt or set
 *	pp->p_mapping to NULL.  Thus only page_unlock() and certain
 *	hat functions should call page_free().
 *
 *	Other functions should call page_unlock() or hat_unload() et. al.
 *	to indicate they are done using a page.
 */
void
page_free(page_t *pp, uint_t dontneed)
{
	ASSERT(PAGE_USELOCK_OWNED(pp));
	VM_PAGEFREELOCK();
	page_free_l(pp, dontneed);
	VM_PAGEFREEUNLOCK();
	PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_free_l(page_t *pp, uint_t dontneed)
 *
 *	Free an unused page to the appropriate (clean/dirty) free list.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() locked; it is returned locked.
 *
 *	"dontneed" is passed non-zero as a hint that the page is unlikely to
 *	be needed soon or ever.  A non-zero "dontneed" only has meaning
 *	(and should only be specified) if the page has vnode identity.
 *
 *	The caller passes the page in the following state:
 *		- PAGE_USELOCK()'d
 *		- !PAGE_IN_USE(pp)
 *		- not already on the free list
 *
 *	On return, the page has been added to the appropriate free list.
 *	The caller must still PAGE_USEUNLOCK() the page.
 *
 *	Note that the passed page has no outstanding reader/writer locks
 *	(since !PAGE_IN_USE(pp)), so this routine does no PAGE_BROADCAST()
 *	to wakeup page waiters.  The caller should have already done this
 *	when it dropped it's reader/writer lock.
 *
 *	Calling this routine is discouraged.
 *	page_free() is the preferred interface to call.
 */
void
page_free_l(page_t *pp, uint_t dontneed)
{
	vnode_t *vp;
#ifdef NO_RDMA
	const int type = STD_PAGE;
#else
	int type = pp->p_type;
#endif

	ASSERT(VM_PAGEFREELOCK_OWNED());
	ASSERT(pp >= pages && pp < epages);
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(!SV_BLKD(&(pp)->p_wait));	/* no waiters on page */
	ASSERT(!pp->p_free);			/* page not already free */
	ASSERT(!pp->p_invalid);
	ASSERT(!pp->p_pageout);
	ASSERT(!pp->p_ioerr);
	ASSERT(pp->p_nio == 0);
	ASSERT(pp->p_mapping == NULL);
	ASSERT(!PAGE_IN_USE(pp));
	ASSERT(!dontneed || pp->p_vnode != NULL);

	BUMPPGCOUNT(pagecnt.pc_free);

	vp = pp->p_vnode;

	pp->p_free = 1;

	/*
	 * If page is dirty (and has vnode identity),
	 * add it to a the appropriate list for cleaning by pageout.
	 *
	 * In this case we wait until after the page is clean before
	 * bumping freemem.  We set p_timestamp:
	 *
	 *	if dontneed {
	 *		make the page appear old to get it
	 *		cleaned quickly and available for reuse.
	 *
	 *	} else if it's an anon page {
	 *		make it appear young to defer cleaning as long as
	 *		possible.  (We do this both to make I/O bandwidth
	 *		available for non-anon page cleaning and in the 
	 *		hopes that the page will be page_abort'd [due to
	 *		process exit or munmap] before the I/O takes place.)
	 *
	 *	} else / * it's a non-anon page * / {
	 *		if fsflush has already found it dirty and set
	 *		p_timestamp, leave it so that the page is sure
	 *		to be cleaned within the prescribed time (for
	 *		file system hardening).  Else set the time
	 *		now since we're the first to find the page dirty.
	 *	}
	 */

	if (pp->p_mod && vp) {
		if (IS_ANONVP(vp)) {
			/*
                         * Drop any real swap backing store associated
                      	 * with the page. We pass B_TRUE because if this
                         * page is still doubly associated we want the
                         * swapdoubles counter adjustd.
                         */
                        anon_freeswap(anon_pptoan(pp), B_TRUE);
			ANON_SAVE_HIST(anon_pptoan(pp), ANON_H_PAGEFREESWAP);

			page_add(&page_dirtyalist, pp);

			if (dontneed) {
				pp->p_timestamp = 1;	/* make very old */
				BUMPPGCOUNT(pagecnt.pc_free_dontneed);
			} else {
				/* move it to the tail of the list */
				page_dirtyalist = page_dirtyalist->p_next;
				pp->p_timestamp = lbolt;
			}
			INCR_LIST_SIZE(page_dirtyalist_size);
		} else {
			page_add(&page_dirtyflist, pp);

			if (dontneed) {
				pp->p_timestamp = 1;	/* make very old */
				BUMPPGCOUNT(pagecnt.pc_free_dontneed);
			} else {
				/* move it to the tail of the list */
				page_dirtyflist = page_dirtyflist->p_next;
				if (pp->p_timestamp == 0)
					pp->p_timestamp = lbolt;
			}
			INCR_LIST_SIZE(page_dirtyflist_size);
		}
		++page_dirtylists_size[type];
		BUMPPGCOUNT(pagecnt.pc_free_cache_dirty);
		return;
	}

	/*
	 * Page is either clean or it has no vnode identity.
	 */

	/*
	 * Now we add the page to the head of the free list.
	 * But if this page is associated with a paged vnode
	 * then we adjust the head forward so that the page is
	 * effectively at the end of the list.
	 */

	pp->p_mod = pp->p_timestamp = 0;

	if (vp == NULL) {
		/* page has no identity, put it on the front of the free list */
		ASSERT(!PAGE_IN_USE(pp));
		page_freelist_size[type]++;
		page_add(&page_freelist[type], pp);
		BUMPPGCOUNT(pagecnt.pc_free_free);
	} else {
		page_cachelist_size[type]++;
		page_add(&page_cachelist[type], pp);

		if (dontneed) {
			BUMPPGCOUNT(pagecnt.pc_free_dontneed);
		} else {
			/* move it to the tail of the list */
			page_cachelist[type] = page_cachelist[type]->p_next;
			BUMPPGCOUNT(pagecnt.pc_free_cache_clean);
		}
	}

	freemem_unresv(1, type);

	CHECKFREE();
}

/*
 * page_t *
 * page_create(vnode_t *vp, off_t off)
 *
 *	Create a page with the specified identity; mark it as dirty.
 *	Caller ensures there is not already a page with this identity.
 *
 * Calling/Exit State:
 *
 * 	Called with VM_PAGEFRELOCK() and VM_PAGEIDLOCK() unlocked. They
 * 	remain unlocked at return. Called with no spin LOCKs held.
 * 	The caller can tolerate blocking to wait for
 * 	memory.
 *
 *	Returns the resulting page PAGE_IS_RDLOCKED(pp)'d and
 *	marked dirty (p_mod is set).
 *
 * Description:
 *	The caller guarantees, by its own means, that no page with the
 *	specified identity exists.
 *
 * Remarks:
 *	This function is tailored for use by anonfs. It should not be
 *	called by other subsystems. Note that the page returned is
 *	preferably non-DMAable. This is the way anonfs wants it.
 *
 *	In order to guarantee that no page with the specified identity
 *	exists, it does not suffice for the caller to merely first call
 *	page_exists() and then this function, since the caller
 *	VM_PAGEIDUNLOCK() before calling us and that allows new pages with
 *	this identity to be created. Callers must have some other mechanism
 *	to ensure the page identity does not already exist.
 */
page_t *
page_create(vnode_t *vp, off_t off)
{
	page_t *pp;
	boolean_t dropped_vmpagelocks;	/* dummy */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	VM_PAGEIDWRLOCK();
	VM_PAGEFREELOCK();

	/*
	 * We always get the page returned PAGE_USELOCK()'d so we
	 * can call page_hashin() later.  
	 */

	pp = page_get_l(PAGESIZE, SLEEP | P_RETURN_PAGEUSELOCKED | P_NODMA,
			&dropped_vmpagelocks);
	ASSERT(pp != NULL);

	VM_PAGEFREEUNLOCK();

	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(pp->p_activecnt == 1);
	ASSERT(pp->p_invalid);		/* page is writer locked */
	ASSERT(!pp->p_free);

	page_hashin(pp, vp, off);	/* establish page vnode identity */

	VM_PAGEIDUNLOCK();

	page_setmod_l(pp);			/* mark page as dirty */
	pp->p_invalid = 0;			/* downgrade to READER lock */

	PAGE_USEUNLOCK(pp);

	ASSERT(!SV_BLKD(&(pp)->p_wait));	/* no waiters on page */

	return (pp);
}

/*
 * page_t *
 * page_lookup_or_create3(vnode_t *vp, off_t off, uint_t flags)
 *
 *	Find or create a page with the specified identity.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDLOCK() unlocked. 
 *	They remain unlocked at return.
 *
 *	Returns the resulting page either:
 *
 *		PAGE_IS_RDLOCKED(pp) in which case the page already contains
 *			valid data and has been reclaimed from the free list,
 *			if necessary.
 *	or:
 *		PAGE_IS_WRLOCKED(pp) in which case the caller must fill
 *			in the appropriate data before calling either
 *			page_unlock(pp) or page_downgrade_lock(pp).
 *
 *	In either case, the returned page is already PAGE_USEUNLOCK()'d.
 *
 * Remarks:
 *	Similar to page_lookup_or_create3() above, but also takes
 *	a flags parameter.
 */
page_t *
page_lookup_or_create3(vnode_t *vp, off_t off, uint_t flags)
{
	page_t *pp;
	boolean_t dropped_vmpagelocks;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

    	/* hold it write locked since we may call page_get_l */
top:	VM_PAGEIDWRLOCK();

	pp = page_lookup(vp, off);

	/*
	 * If we found the page, return it reader locked.
	 */

	if (pp) {
		ASSERT(!pp->p_invalid);
		ASSERT(!pp->p_free);
		pp->p_activecnt++;
		PAGE_USEUNLOCK(pp);
		ASSERT(getpl() == PLBASE);
		MET_ATCH(1);	/* successful attach */
		return (pp);
	}

	ASSERT(VM_PAGEIDLOCK_OWNED());

	/*
	 * Page doesn't already exist.  Create the new page writer locked.
	 */
    	VM_PAGEFREELOCK();
	pp = page_get_l(PAGESIZE, SLEEP | P_RETURN_PAGEUSELOCKED | flags,
			&dropped_vmpagelocks);

	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(pp->p_activecnt == 1);
	ASSERT(pp->p_invalid);			/* page is writer locked */

	if (dropped_vmpagelocks && page_exists(vp, off) != (page_t *)NULL) {

		/*
		 * Lost a race.  Someone else created a page with the
		 * same identity while we blocked (or dropped VM_PAGELOCK).
		 * Free our page and start over.
		 */

		pp->p_activecnt = 0;		   /* drop page writer lock */
		pp->p_invalid = 0;
		ASSERT(!SV_BLKD(&(pp)->p_wait));   /* no waiters on page */

		page_free_l(pp, 0);
		VM_PAGEFREEUNLOCK();
    	   	VM_PAGEIDUNLOCK();
		PAGE_USEUNLOCK(pp);
		ASSERT(getpl() == PLBASE);
		goto top;
	}
	VM_PAGEFREEUNLOCK();
	MET_ATCH_MISS(1);	/* attach attempt failed */

	/*
	 * Establish page vnode identity.
	 */

	page_hashin(pp, vp, off);

	VM_PAGEIDUNLOCK();
	PAGE_USEUNLOCK(pp);
	ASSERT(getpl() == PLBASE);

	return (pp);
}

/*
 * STATIC page_t *
 * page_lookup(vnode_t *vp, off_t off)
 *
 *	Find the page with the given identity and wait for valid data.
 *
 * Calling/Exit State:
 *
 *	Search the page ID hash list for the page with the given identity.
 *	Wait for valid data (i.e., wait for any writer lock to unlock).
 *	Reclaim from the page free list, if necessary.
 *
 *	Called with VM_PAGEIDWRLOCK() locked.
 *
 *	Returns the found page else NULL:
 *
 *	If a found page is returned, the page is PAGE_USELOCK()'d in a 
 *	state ready for the caller to establish a reader lock, and
 *	VM_PAGEIDUNLOCK()'ed.  The page has been reclaimed
 *	from the freelist; the caller is responsible for page_free()'ing
 *	the page if no read/write page lock is established.
 *
 *	If NULL is returned, VM_PAGEIDWRLOCK() is still held; the
 *	caller must release it via VM_PAGEIDUNLOCK().
 *
 *	This is similar to read locking a page but useful in cases where the
 *	caller wishes to quickly manipulate the page using a reader lock
 *	without sleeping.  Callers need not page_unlock() a page acquired
 *	via page_lookup(); they merely need to PAGE_USEUNLOCK() it.
 *
 *	Remarks:
 *	   PERF: Should page_exists_uselock() just be expanded in-line
 *		 in page_lookup() for performance? Then we could trylock
 *		 the page and (typically) hold VM_PAGEFREELOCK() across the
 *		 PAGE_RECLAIM() by using page_reclaim_l().
 *	   PERF: Expand page_lookup() inline in page_lookup_or_create()
 *		 since this is the only caller?
 *
 */
STATIC page_t *
page_lookup(vnode_t *vp, off_t off)
{
	page_t *pp;

again:	ASSERT(VM_PAGEIDLOCK_OWNED());

	pp = page_exists_uselock(vp, off, WRITE_LOCKED);

	if (pp == (page_t *)NULL) {
		return ((page_t *)NULL);      /* still VM_PAGEIDLOCK() held */
	}

	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(VM_PAGEIDLOCK_OWNED());

	VM_PAGEIDUNLOCK();

	/*
	 * Wait for valid data (i.e., wait for writer lock to unlock).  
	 * This allows the caller to establish a page reader lock.
	 */
	while (pp->p_invalid) {
		PAGE_WAIT(pp);
		/*
		 * If page identity has changed, start over.
		 */
		if (!PAGE_HAS_IDENTITY(pp, vp, off)) {
			ASSERT(PAGE_USELOCK_OWNED(pp));
			PAGE_USEUNLOCK(pp);

			VM_PAGEIDWRLOCK();

			goto again;
		}
	}
	if (pp->p_free) {
		if (!page_reclaim(pp)) {
			/*
			 * Page was free but already reserved
			 */
			ASSERT(PAGE_USELOCK_OWNED(pp));
			PAGE_USEUNLOCK(pp);

			VM_PAGEIDWRLOCK();
			goto again;
		}
		MET_ATCHFREE(1);	/* attached a free page */
	}
	return (pp);
}

/*
 * PERF:
 *	We could change page_lazy_create() to pass P_TRYPAGEUSELOCK to
 *	page_get_l() in addition to NOSLEEP.  This would guarantee that
 *	page_get_l() never dropped vm_pagelock and allow a change where
 *	the callers of page_lazy_create() locked VM_PAGEFREELOCK() and
 *	VM_PAGEIDLOCK(). This in turn would allow the callers to hold 
 *	these locks across multiple page_lazy_create() calls. This could
 *	be a win in the case of pvn_kluster().
 *
 *	On the other hand, if file system blocksize is <= PAGESIZE,
 *	or if it's a very small multiple of PAGESIZE (e.g., 2), then
 *	this probably isn't worth doing.
 */

/*
 * page_t *
 * page_lazy_create(vnode_t *vp, off_t off)
 *
 *	Create a page with the specified identity if it doesn't already exist
 *	and memory is readily available.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK() and VM_PAGEIDLOCK() unlocked. They
 *	remain unlocked at return.
 *
 *	Returns NULL if a page with the specified identity already exists
 *	or there was no memory readily available to create the page.
 *
 *	Otherwise it returns the newly created page PAGE_IS_WRLOCKED(pp)
 *	and the caller must fill in the appropriate data before calling
 *	either page_unlock(pp) or page_downgrade_lock(pp).
 */
page_t *
page_lazy_create(vnode_t *vp, off_t off)
{
	page_t *pp;
	boolean_t dropped_vmpagelocks;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	VM_PAGEIDWRLOCK();
	VM_PAGEFREELOCK();

	/*
	 * If memory is not plentiful, or if the page already exists,
	 * don't create a new page.
	 */

	if ((freemem <= minfree) || (page_exists(vp, off))) {
		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		return (NULL);
	}

	/*
	 * Page doesn't already exist.  Create the new page writer locked.
	 */

	pp = page_get_l(PAGESIZE, NOSLEEP | P_RETURN_PAGEUSELOCKED,
			&dropped_vmpagelocks);

	if (pp == NULL) {
		/*
		 * Because we specified NOSLEEP to page_get_l() and
		 * we checked available memory first, the only reason
		 * we could fail would be that minfree < minpagefree.
		 * This would imply that minfree was incorrectly
		 * configured.  But we tolerate this.
		 */
		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		ASSERT(getpl() == PLBASE);
		return (NULL);
	}

	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(pp->p_activecnt == 1);
	ASSERT(pp->p_invalid);		/* page is writer locked */

	if (dropped_vmpagelocks && page_exists(vp, off) != (page_t *)NULL) {
		/*
		 * Lost a race.  Someone else created a page with the
		 * same identity while we dropped VM_PAGELOCK.
		 * Free our page and quit.
		 */
		pp->p_activecnt = 0;		/* drop page writer lock */
		pp->p_invalid = 0;
		ASSERT(!SV_BLKD(&(pp)->p_wait));   /* no waiters on page */

		page_free_l(pp, 0);
		VM_PAGEFREEUNLOCK();
		VM_PAGEIDUNLOCK();
		PAGE_USEUNLOCK(pp);
		ASSERT(getpl() == PLBASE);
		return (NULL);
	}
	VM_PAGEFREEUNLOCK();

	/*
	 * Establish page vnode identity.
	 */

	page_hashin(pp, vp, off);
	VM_PAGEIDUNLOCK();
	PAGE_USEUNLOCK(pp);
	ASSERT(getpl() == PLBASE);

	return (pp);
}

/*
 * page_t *
 * page_exists_uselock(vnode_t *vp, off_t off, enum page_lock_mode lock_type)
 *
 *	Search the page ID hash list for the page with the given identity,
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDLOCK() locked (shared or exclusive mode indicated
 *	by argument lock_type).
 *
 *	Returns the found page else NULL:
 *
 *	The VM_PAGEIDLOCK() is returned locked. 
 *
 *	If a found page is returned, the page is PAGE_USELOCK()'d.
 *	No checking of page state (other than identity) is done.
 *
 *	NULL is returned, if page is not found.
 *
 *	Calling this routine directly is discouraged.
 *	page_lookup_or_create() is the preferred interface to call.
 */
page_t *
page_exists_uselock(vnode_t *vp, off_t off, enum page_lock_mode lock_type)
{
	page_t *pp;

again:	ASSERT(VM_PAGEIDLOCK_OWNED());

	pp = page_exists(vp, off);

	if (pp == (page_t *)NULL) {
		return ((page_t *)NULL);    /* VM_PAGEIDLOCK() still held */
	}

	if (PAGE_TRYUSELOCK(pp) == INVPL) {
		VM_PAGEIDUNLOCK();
		PAGE_USELOCK(pp);

		if (lock_type == WRITE_LOCKED)
			VM_PAGEIDWRLOCK();
		else
			VM_PAGEIDRDLOCK();

		if (!PAGE_HAS_IDENTITY(pp, vp, off)) {
			/* Lost the race after dropping VM_PAGELOCK */
			l.vmpageidlockpl = l.puselockpl;
			l.puselockpl = VM_PAGE_IPL;

			PAGE_USEUNLOCK(pp);
			goto again;
		}
	} else {	/* trylock succeeded */
		l.puselockpl = l.vmpageidlockpl;
		l.vmpageidlockpl = VM_PAGE_IPL;
	}

	return (pp);
}


/*
 * STATIC page_t *
 * page_exists(const vnode_t *vp, off_t off)
 *
 *	Search the page ID hash list for the page with the given identity.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAEIDLOCK() locked.  It remains locked at return.
 *	Returns the found page else NULL.  The returned page is not locked.
 *
 *	Other than identity, the returned page state is not checked; that
 *	is, it could be on a free list, p_invalid, p_pageout, etc. The
 *	caller must check the suitability of the page state.
 */
STATIC page_t *
page_exists(const vnode_t *vp, off_t off)
{
#ifdef EXTRA_PAGE_STATS

	page_t *pp;
	int len = 0;

	ASSERT(VM_PAGEIDLOCK_OWNED());

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (PAGE_HAS_IDENTITY(pp, vp, off))
			break;
	if (pp)
		BUMPPGCOUNT(pagecnt.pc_exist_hit);
	else
		BUMPPGCOUNT(pagecnt.pc_exist_miss);

	BUMPPGCOUNT(pagecnt.pc_exist_hashlen[(len > PC_HASH_CNT) ? PC_HASH_CNT : len]);
	return (pp);

#else /* EXTRA_PAGE_STATS */

	page_t *pp;

	ASSERT(VM_PAGEIDLOCK_OWNED());

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash)
		if (PAGE_HAS_IDENTITY(pp, vp, off))
			break;

	return (pp);

#endif /* EXTRA_PAGE_STATS */
}

/*
 * void
 * page_downgrade_lock(page_t *pp)
 *
 *	Downgrade a writer to a reader page lock.
 *
 * Calling/Exit State:
 *
 *	Marks page as having valid data (i.e., downgrades a writer lock).
 *
 *	Typically called by xxx_getapage() after filling data in a page
 *	returned via page_lookup_or_create().
 *
 *	Called with VM_PAGEIDLOCK(), VM_PAGEFREELOCK() and PAGE_USELOCK(pp)
 *	unlocked.
 *	They remain unlocked at return.
 */
void
page_downgrade_lock(page_t *pp)
{
	PAGE_USELOCK(pp);

	ASSERT(pp->p_invalid && pp->p_activecnt == 1);

	pp->p_invalid = 0;
	PAGE_BROADCAST(pp);		/* since we cleared p_invalid */

	PAGE_USEUNLOCK(pp);
}


/*
 * STATIC void
 * page_unlock_l(page_t *pp)
 *
 *	Release a page reader/writer lock.  Free the page if no longer 
 *	in use.
 *
 * Calling/Exit State:
 *
 *	Called with PAGE_USELOCK(pp) held. VM_PAGEIDLOCK() & VM_PAGEFREELOCK()
 *	are unlocked.
 *
 *	On return, the page is consumed.  The caller does not need to
 *	PAGE_USEUNLOCK() the page.
 *
 *	If the page has vnode identity, this indicates that the data
 *	in the page is now valid.
 */
STATIC void
page_unlock_l(page_t *pp)
{
	ASSERT(PAGE_USELOCK_OWNED(pp));

	ASSERT(pp->p_activecnt > 0);
	ASSERT(!pp->p_invalid || pp->p_activecnt == 1);

	pp->p_invalid = 0;	/* if we hold a writer lock, drop it */

	if (--pp->p_activecnt == 0) {
		PAGE_BROADCAST(pp);
	}
	if (!PAGE_IN_USE(pp)) {
		page_free(pp, 0);
	} else {
		PAGE_USEUNLOCK(pp);
	}
}


/*
 * void
 * page_unlock(page_t *pp)
 *
 *	Release a page reader/writer lock.  Free the page if no longer in use.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK(), VM_PAGEIDLOCK() and 
 *	PAGE_USELOCK(pp) all unlocked.
 *
 *	On return, the page is consumed.  The caller does not need to
 *	PAGE_USEUNLOCK() the page.
 *
 *	If the page has vnode identity, this indicates that the data
 *	in the page is now valid.
 */
void
page_unlock(page_t *pp)
{
	PAGE_USELOCK(pp);
	page_unlock_l(pp);
}


/*
 * void
 * page_list_unlock(page_t *plist)
 *
 *	Release page reader/writer locks on a list of pages.
 *	Free the pages if no longer in use.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEFREELOCK(), VM_PAGEIDLOCK() and 
 *	PAGE_USELOCK(pp) all unlocked.
 *
 *	On return, the pages are consumed.  The caller does not need to
 *	PAGE_USEUNLOCK() the pages.
 *
 *	If a page has vnode identity, this indicates that the data
 *	in the page is now valid.
 */
void
page_list_unlock(page_t *plist)
{
	page_t *pp;

	while ((pp = plist) != NULL) {
		page_sub(&plist, pp);
		page_unlock(pp);
	}
}


/*
 * void
 * page_find_unlock(vnode_t *vp, off_t off, uint_t flags)
 *
 *	Release a page reader lock based on <vp, off> identity.
 *	Free the page if no longer in use.
 *
 * Calling/Exit State:
 *
 *	Caller ensures that a page already exists with the specified
 *	<vnode, offset> identity and that the page is reader locked.
 *	This function finds the page and drops the reader lock.
 *
 *	flags:
 *		P_SETMOD
 *		      Mark the page as dirty (modified) before unlocking.
 *
 *	Called with VM_PAGEIDLOCK() & VM_PAGEFREELOCK() unlocked.
 */
void
page_find_unlock(vnode_t *vp, off_t off, uint_t flags)
{
	page_t *pp;

	ASSERT(!(flags & ~P_SETMOD));	/* no flags besides P_SETMOD */

	VM_PAGEIDRDLOCK();
	pp = page_exists_uselock(vp, off, READ_LOCKED);

	ASSERT(pp != NULL);
	ASSERT(PAGE_IS_RDLOCKED(pp));
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(VM_PAGEIDLOCK_OWNED());

	VM_PAGEIDUNLOCK();

	if (flags & P_SETMOD) {
		page_setmod_l(pp);	/* mark page as dirty */
	}
	page_unlock_l(pp);
}

/*
 * void
 * page_abort_l(page_t *pp)
 *
 *	Remove the identity of a page and return it to the free list.
 *
 * Calling/Exit State:
 *
 *	Caller passes the page in the following state:
 *		- PAGE_USELOCK()'d
 *		- not on free list (!p_free)
 *		- *either*:
 *			p_activecnt == 0 && p_invalid == 0    [no locks]
 *		  or:
 *			p_activecnt == 1 && p_invalid == 1    [writer locked]
 *	VM_PAGEFREELOCK() and VM_PAGEIDLOCK() is not held on entry or exit.
 *	
 *	On return, the page is consumed.  The caller does not need to
 *	PAGE_USEUNLOCK() the page.
 */
void
page_abort_l(page_t *pp)
{
	ASSERT(pp >= pages && pp < epages);
	ASSERT(!pp->p_free);
	ASSERT(!pp->p_pageout);
	ASSERT((pp->p_invalid == 1 && pp->p_activecnt == 1) ||
		(pp->p_invalid == 0 && pp->p_activecnt == 0));
	ASSERT(PAGE_USELOCK_OWNED(pp));

	if (pp->p_mapping)
		hat_pageunload(pp);	/* Remove any current mappings */

	/* Drop any page writer lock we may have */

	pp->p_invalid = 0;
	pp->p_activecnt = 0;
	PAGE_BROADCAST(pp);

	if (pp->p_vnode) {
		VM_PAGEIDWRLOCK();  
		page_hashout(pp);   /* Destroy any current vnode identity */
		VM_PAGEIDUNLOCK();
	}

	VM_PAGEFREELOCK();
	page_free_l(pp, 0);
	VM_PAGEFREEUNLOCK();
	PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_abort(page_t *pp)
 *
 *	Remove the identity of a page and return it to the free list.
 *
 * Calling/Exit State:
 *
 *	Caller passes the page in the following state:
 *		- not on free list (!p_free)
 *		- *either*:
 *			p_activecnt == 0 && p_invalid == 0    [no locks]
 *		  or:
 *			p_activecnt == 1 && p_invalid == 1    [writer locked]
 *	VM_PAGEFREELOCK() and VM_PAGEIDLOCK() is not held on entry or exit.
 *	
 *	On return, the page is consumed.
 */
void
page_abort(page_t *pp)
{
	ASSERT(pp >= pages && pp < epages);

	PAGE_USELOCK(pp);
	page_abort_l(pp);
}

/*
 * void
 * page_abort_identity(page_t *pp, vnode_t *vp, off_t offset)
 *
 *	If the page has the specified identity, remove the identity of
 *	the page and return it to the free list.  It is a conditional
 *	form of page_abort().
 *
 * Calling/Exit State:
 *
 *	Caller with no spin LOCKs held and returns that way. The caller
 *	is prepared to block.
 *
 *	Upon the return, the page no longer has the specified identity. (It
 *	has either a different identity or no identity.) If the identity
 *	was destroyed by this routine (as opposed to already destroyed by
 *	another agent), the page was added to the free list.
 *
 * Description:
 *
 *	This was designed for anonfs. It is useful for callers that control
 *	the creation of pages with the specified identity and can hence
 *	guarantee that only the specified page, or no page, could have the
 *	identity. Upon return, the caller then knows that, since the
 *	specified page no longer has the specified identity, no page in the
 *	system has that identity.
 */
void
page_abort_identity(page_t *pp, vnode_t *vp, off_t offset)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(pp >= pages && pp < epages);

	PAGE_USELOCK(pp);

	/*
	 * If the page no longer has the specified identity
	 * then someone else already did our job for us.
	 */

	if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
		PAGE_USEUNLOCK(pp);
		return;
	}

	/*
	 * Wait for all other page locks to drain so we can
	 * establish an effective page writer lock.
	 */

	while (pp->p_activecnt) {
		PAGE_WAIT(pp);

		/*
		 * If the page no longer has the specified identity
		 * then someone else already did our job for us.
		 */
		if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
			PAGE_USEUNLOCK(pp);
			return;
		}
	}

	/*
	 * If we can easily reclaim it from the free list (if necessary)
	 * then abort the identity.
	 */

	if (PAGE_RECLAIM(pp)) {
		page_abort_l(pp);
	} else {
		/*
		 * If PAGE_RECLAIM() failed to get the page,
		 * it at least destroyed the identity for us.
		 */
		ASSERT(!PAGE_HAS_IDENTITY(pp, vp, offset));
		PAGE_USEUNLOCK(pp);
	}
}

/*
 * void
 * page_assign_identity(page_t *pp, vnode_t *vp, off_t off)
 *	Function to assign the identity given by (vp, off)
 *	to the specified page (pp).
 *
 * Calling/Exit State:
 *	The specified page (pp) has no identity, and is write locked.
 *
 *	The caller guarantees, though its own means, that no page
 *	exists, or being concurrently created, with the same identity.
 *
 * Description:
 *	This function is tailored for anonfs, which has tight control over
 *	the identities of its pages.
 */

void
page_assign_identity(page_t *pp, vnode_t *vp, off_t off)
{
	ASSERT(PAGE_IS_WRLOCKED(pp));
	ASSERT(pp->p_vnode == NULL);
	ASSERT(pp->p_offset == 0);

	PAGE_USELOCK(pp);
	VM_PAGEIDWRLOCK();
	ASSERT(page_exists(vp, off) == NULL);
	page_hashin(pp, vp, off);
	VM_PAGEIDUNLOCK();
	PAGE_USEUNLOCK(pp);
}

/*
 * STATIC void
 * page_hashin(page_t *pp, vnode_t *vp, off_t offset)
 *
 *	Add page to the hash/vp chains for <vp, offset>.
 *
 * Calling/Exit State:
 *
 *	Caller holds VM_PAGEIDWRLOCK(), passes page pp:
 *		- PAGE_USELOCK()'d
 *		- with no current vnode identity
 *		- not on free list (!p_free)
 *		- *either*:
 *			p_activecnt == 0 && p_invalid == 0	[no locks]
 *		  or:
 *			p_activecnt == 1 && p_invalid == 1	[writer locked]
 *
 *	Returned state is identical except page now has identity.
 */
STATIC void
page_hashin(page_t *pp, vnode_t *vp, off_t offset)
{
	page_t **hpp;

	ASSERT(VM_PAGEIDLOCK_OWNED());
	ASSERT(vp != (vnode_t *)NULL);
	ASSERT(pp >= pages && pp < epages);
	ASSERT((pp->p_invalid == 1 && pp->p_activecnt == 1) ||
	       (pp->p_invalid == 0 && pp->p_activecnt == 0));
	ASSERT(page_exists(vp, offset) == (page_t *)NULL);
	ASSERT(pp->p_vnode == (vnode_t *)NULL);
	ASSERT(pp->p_offset == 0);

	pp->p_vnode = vp;
	pp->p_offset = offset;

	hpp = &page_hash[PAGE_HASHFUNC(vp, offset)];
	pp->p_hash = *hpp;
	*hpp = pp;

	/*
	 * Add the page to the end of the v_pages linked list.
	 * This allows pvn_getdirty_range(), et al. to ignore all new
	 * pages added while it was sleeping; thereby guaranteeing
	 * that it will terminate.
	 */
	if (vp->v_pages == NULL) {
		vp->v_pages = pp->p_vpnext = pp->p_vpprev = pp;
	} else {
		pp->p_vpnext = vp->v_pages;
		pp->p_vpprev = vp->v_pages->p_vpprev;
		vp->v_pages->p_vpprev = pp;
		pp->p_vpprev->p_vpnext = pp;
	}
	CHECKPP(pp);
}

/*
 * STATIC void
 * page_hashout(page_t *pp)
 *
 *	Remove page from the hash and vp chains and remove vp association.
 *
 * Calling/Exit State:
 *
 *	Caller holds VM_PAGEIDWRLOCK() and passes page pp:
 *		- PAGE_USELOCK()'d
 *		- with a current vnode identity
 *		- not on free list (!p_free)
 *		- *either*:
 *			p_activecnt == 0 && p_invalid == 0	[no locks]
 *	  	or:
 *			p_activecnt == 1 && p_invalid == 1	[writer locked]
 *
 *	Returned state is identical except page now has no vnode identity.
 */
STATIC void
page_hashout(page_t *pp)
{
	page_t **hpp, *hp;
	vnode_t *vp;

	ASSERT(VM_PAGEIDLOCK_OWNED());
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(pp >= pages && pp < epages);
	ASSERT(pp->p_vnode != (vnode_t *)NULL);
	ASSERT((pp->p_invalid == 1 && pp->p_activecnt == 1) ||
		(pp->p_invalid == 0 && pp->p_activecnt == 0));
	ASSERT(!pp->p_free);
	CHECKPP(pp);

	vp = pp->p_vnode;

	/*
	 * inform anonfs/memfs it's page is being destroyed
	 */
	if (vp->v_flag & VSWAPBACK) {
		if (IS_ANONVP(vp))
			anon_hashout(pp);
		else
			memfs_hashout(pp);
	}

	hpp = &page_hash[PAGE_HASHFUNC(vp, pp->p_offset)];
	for (;;) {
		hp = *hpp;
		if (hp == pp)
			break;
		ASSERT(hp != NULL);
		hpp = &hp->p_hash;
	}
	*hpp = pp->p_hash;

	pp->p_hash = NULL;
	pp->p_vnode = NULL;
	pp->p_offset = 0;

	/*
	 * Remove this page from the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	CHECKPP(pp);
	if (vp->v_pages == pp)
		vp->v_pages = pp->p_vpnext;		/* go to next page */

	if (vp->v_pages == pp)
		vp->v_pages = NULL;			/* page list is gone */
	else {
		pp->p_vpprev->p_vpnext = pp->p_vpnext;
		pp->p_vpnext->p_vpprev = pp->p_vpprev;
	}
	pp->p_vpprev = pp->p_vpnext = NULL;
}


/*
 * void
 * page_add(page_t **ppp, page_t *pp)
 *
 *	Add page (pp) to the front of the linked list of pages (*ppp)
 *	using p_next/p_prev pointers for the list.
 *
 * Calling/Exit State:
 *
 *	On entry, *ppp points to the first element in the list (or NULL if
 *	the list is empty).  On exit, *ppp points to pp which has been added
 *	to the list.
 *
 *	The caller is responsible for ensuring exclusive access to the list.
 */
void
page_add(page_t **ppp, page_t *pp)
{
	ASSERT(pp >= pages && pp < epages);

	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
	} else {
		pp->p_next = *ppp;
		pp->p_prev = (*ppp)->p_prev;
		(*ppp)->p_prev = pp;
		pp->p_prev->p_next = pp;
	}
	*ppp = pp;
	CHECKPP(pp);
}

#ifdef DEBUG
/*
 * STATIC void
 * page_early_add(page_t **ppp, page_t *pp)
 *
 *	Similar to page_add, but may be called during page pool
 *	initialization.
 *
 * Calling/Exit State:
 *
 *	On entry, *ppp points to the first element in the list (or NULL if
 *	the list is empty).  On exit, *ppp points to pp which has been added
 *	to the list.
 *
 *	The caller is responsible for ensuring exclusive access to the list.
 */
STATIC void
page_early_add(page_t **ppp, page_t *pp)
{
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
	} else {
		pp->p_next = *ppp;
		pp->p_prev = (*ppp)->p_prev;
		(*ppp)->p_prev = pp;
		pp->p_prev->p_next = pp;
	}
	*ppp = pp;
}
#endif /* DEBUG */

/*
 * void
 * page_sub(page_t **ppp, page_t *pp)
 *
 *	Remove page (pp) from the linked list of pages (*ppp)
 *	using p_next/p_prev pointers for the list.
 *
 * Calling/Exit State:
 *
 *	On entry, *ppp points to the first element in the list.
 *	On exit, pp has been removed from the list and *ppp has
 *	been updated to point to the new list (possibly NULL).
 *
 *	The caller is responsible for ensuring exclusive access to the list.
 */
void
page_sub(page_t **ppp, page_t *pp)
{
	ASSERT(*ppp != NULL);
	ASSERT(pp != NULL);
	ASSERT(pp >= pages && pp < epages);

	CHECKPP(pp);
	CHECKPPLIST(pp, ppp);

	if (*ppp == pp)
		*ppp = pp->p_next;		/* go to next page */

	if (*ppp == pp)
		*ppp = NULL;			/* page list is gone */
	else {
		pp->p_prev->p_next = pp->p_next;
		pp->p_next->p_prev = pp->p_prev;
	}
	pp->p_prev = pp->p_next = pp;		/* make pp a list of one */
}

/*
 * void
 * page_sortadd(page_t **ppp, page_t *pp)
 *
 *	Add page (pp) to the linked list of vnode pages (*ppp)
 *	sorted by p_offset using p_next/p_prev pointers for the list.
 *
 * Calling/Exit State:
 *
 *	On entry, *ppp points to the first element in the list (or NULL if
 *	the list is empty).  The list must already be sorted in 
 *	increasing p_offset order.  pp->p_vnode must already be set.
 *
 *	On exit, pp has been added to the appropriate position in the list
 *	and *ppp has been updated to point to the new head of the list.
 *
 *	The caller is responsible for ensuring exclusive access to the list.
 */
void
page_sortadd(page_t **ppp, page_t *pp)
{
	page_t *p1;
	off_t off;

	ASSERT(pp >= pages && pp < epages);
	CHECKLIST(*ppp);
	CHECKPP(pp);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
		*ppp = pp;
	} else {
		/*
		 * Figure out where to add the page to keep list sorted
		 */
		p1 = *ppp;
		if (pp->p_vnode != p1->p_vnode || p1->p_vnode == NULL ||
		    pp->p_vnode == NULL) {
			/*
			 *+ An internal kernel software error was detected.
			 *+ Corrective action:  none.
			 */
			cmn_err(CE_PANIC, "page_sortadd: bad vp");
		}

		off = pp->p_offset;
		if (off < p1->p_prev->p_offset) {
			do {
				if (off == p1->p_offset) {
					/*
					 *+ An internal kernel software error
					 *+ was detected.
					 *+ Corrective action:  none.
					 */
					cmn_err(CE_PANIC,
						"page_sortadd: same offset");
				}
				if (off < p1->p_offset)
					break;
				p1 = p1->p_next;
			} while (p1 != *ppp);
		}

		/* link in pp before p1 */
		pp->p_next = p1;
		pp->p_prev = p1->p_prev;
		p1->p_prev = pp;
		pp->p_prev->p_next = pp;

		if (off < (*ppp)->p_offset)
			*ppp = pp;		/* pp is at front */
	}
	CHECKLIST(*ppp);
}


/*
 * STATIC void
 * page_setmod_l(page_t *pp)
 *
 *	Change the page status to indicate the page has been modified.
 *	Caller holds PAGE_USELOCK(pp).
 *
 * Calling/Exit State:
 *
 *	Caller passes the page in the following state:
 *		- PAGE_USELOCK(pp)'d
 *		- read or write locked
 *		- not on free list (!p_free)
 *		
 *	On return, the page is still both read/write locked and
 *	PAGE_USELOCK(pp)'d.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remains unlocked at return.
 */
STATIC void
page_setmod_l(page_t *pp)
{
	ASSERT(PAGE_USELOCK_OWNED(pp));
	ASSERT(pp->p_activecnt > 0);	/* page read/write locked */
	ASSERT(!pp->p_free);		/* page not free */
	pp->p_mod = 1;
	pp->p_timestamp = lbolt;
}

/*
 * void
 * page_setmod(page_t *pp)
 *
 *	Change the page status to indicate the page has been modified.
 *
 * Calling/Exit State:
 *
 *	This is used when the system modifies a page via a mechanism which
 *	doesn't cause the modify bit to be set (e.g., pagezero() or via
 *	a hidden hat translation).
 *
 *	Caller passes the page in the following state:
 *		- read or write locked
 *		- not on free list (!p_free)
 *		- PAGE_USELOCK(pp) unlocked
 *		
 *	On return, the page is still locked.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 */
void
page_setmod(page_t *pp)
{
	PAGE_USELOCK(pp);
	page_setmod_l(pp);
	PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_setmod_iowait(page_t *pp)
 *
 *	Change the page status to indicate the page has been modified.
 *	Wait for any pending pageouts to complete.
 *
 * Calling/Exit State:
 *
 *	This is used when a filesystem has changed the backing store for a
 *	page, and needs to make sure the old backing store is no longer in use,
 *	and that the page eventually gets written to the new backing store.
 *
 *	Caller passes the page in the following state:
 *		- read or write locked
 *		- not on free list (!p_free)
 *		- PAGE_USELOCK(pp) unlocked
 *		
 *	On return, the page is still locked.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 */
void
page_setmod_iowait(page_t *pp)
{
	PAGE_USELOCK(pp);
	page_setmod_l(pp);
	ASSERT(!pp->p_invalid || !pp->p_pageout);
	while (pp->p_pageout)
		PAGE_WAIT(pp);
	PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_find_iowait(vnode_t *vp, off_t off)
 *
 *	If the page with the given identity exists, wait for I/O to complete.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDLOCK() & VM_PAGEFREELOCK() unlocked. They remain
 *	unlocked at return.
 */
void
page_find_iowait(vnode_t *vp, off_t off)
{
	page_t *pp;

	VM_PAGEIDRDLOCK();

	pp = page_exists_uselock(vp, off, READ_LOCKED);

	ASSERT(VM_PAGEIDLOCK_OWNED());
	VM_PAGEIDUNLOCK();

	if (pp == (page_t *)NULL)
		return;

	ASSERT(PAGE_USELOCK_OWNED(pp));

	/*
	 * Wait for any outstanding I/O to complete.
	 */
	while (pp->p_invalid || pp->p_pageout) {
		PAGE_WAIT(pp);
		/*
		 * If page identity has changed, I/O must previously have
		 * completed, so there's nothing left to do.
		 */
		if (!PAGE_HAS_IDENTITY(pp, vp, off))
			break;
	}
	PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_find_unload(vnode_t *vp, off_t off)
 *
 *	Ensure that if the page with the given identity exists,
 *	there are no writable visible hat mappings to the page.
 *
 * Calling/Exit State:
 *
 *	This is used, e.g., when a file system creates a hole in a currently
 *	writable page as is sometimes the case when EOF is extended within
 *	a page.
 *
 *	Waits until an effective writer lock can be acquired on the page,
 *	or the given page does not exist.
 *		
 *	On return, the page is no longer locked and there are no
 *	writable visible hat mappings to this page.  (There may be
 *	writable hidden mappings and/or readable visible mappings.)
 *
 *	Called with VM_PAGEIDLOCK() & VM_PAGEFREELOCK() unlocked. They remain
 *	unlocked at return.
 */
void
page_find_unload(vnode_t *vp, off_t off)
{
	page_t *pp;

	VM_PAGEIDRDLOCK();

	pp = page_exists_uselock(vp, off, READ_LOCKED);

	ASSERT(VM_PAGEIDLOCK_OWNED());
	VM_PAGEIDUNLOCK();

	if (pp == (page_t *)NULL)
		return;

	ASSERT(PAGE_USELOCK_OWNED(pp));

	/*
	 * Wait for locks on the page to go away.
	 */
	while (pp->p_activecnt) {
		PAGE_WAIT(pp);
		/*
		 * If page identity has changed, all translations must have
		 * been unloaded, so there's nothing left to do.
		 */
		if (!PAGE_HAS_IDENTITY(pp, vp, off)) {
			PAGE_USEUNLOCK(pp);
			return;
		}
	}

	/*
	 * If page is free, it must not have any translations.
	 */
	if (pp->p_free) {
		PAGE_USEUNLOCK(pp);
		return;
	}

	/*
	 * We now have an effective writer lock on the page.
	 */
	ASSERT(pp->p_activecnt == 0 && !pp->p_invalid);

	/*
	 * Rather than downgrading all writable visible translations
	 * to read-only (which would be a problem on some architectures)
	 * we take the simplistic approach of removing all visible
	 * translations.
	 */
	hat_pageunload(pp);

	if (!PAGE_IN_USE(pp))
		page_free(pp, 0);
	else
		PAGE_USEUNLOCK(pp);
}

/*
 * void
 * page_find_zero(vnode_t *vp, off_t off)
 *
 *	Ensure that if the page with the given identity exists,
 *	the page content past "off" is zeroed.
 *
 * Calling/Exit State:
 *
 *	This is used, e.g., when a file system extends the file size
 *	leaving holes in the page where the old EOF used to be.
 *
 *	Waits until an effective reader lock can be acquired on the page,
 *	or the given page does not exist.
 *		
 *	Called with VM_PAGEIDLOCK() & VM_PAGEFREELOCK() unlocked. They remain
 *	unlocked at return.
 */
void
page_find_zero(vnode_t *vp, off_t off)
{
	page_t *pp;
	off_t roff;

	roff = off & PAGEMASK;

	VM_PAGEIDRDLOCK();

	pp = page_exists_uselock(vp, roff, READ_LOCKED);

	ASSERT(VM_PAGEIDLOCK_OWNED());
	VM_PAGEIDUNLOCK();

	if (pp == (page_t *)NULL)
		return;

	ASSERT(PAGE_USELOCK_OWNED(pp));

	/*
	 * Wait for locks on the page to go away.
	 */
	while (pp->p_invalid) {
		PAGE_WAIT(pp);
		/*
		 * If page identity has changed, all translations must have
		 * been unloaded, so there's nothing left to do.
		 */
		if (!PAGE_HAS_IDENTITY(pp, vp, roff)) {
			PAGE_USEUNLOCK(pp);
			return;
		}
	}

	/*
	 * We now have an effective reader lock on the page.
	 */
	ASSERT(!pp->p_invalid);

	pagezero(pp, off & PAGEOFFSET, PAGESIZE - (off & PAGEOFFSET));

	PAGE_USEUNLOCK(pp);
}

/*
 * page_t *
 * page_numtopp(uint_t pfn)
 *
 *	Translate a hardware physical page frame number into its
 *	corresponding page_t, if any.
 *
 * Calling/Exit State:
 *
 *	pfn is the page frame number of an MMU_PAGESIZE sized page.
 *
 *	Returns a pointer to the corresponding page_t, if any,
 *	else returns NULL.
 */
page_t *
page_numtopp(uint_t pfn)
{
	struct pp_chunk *chp = pp_first;

	while (pfn < chp->pp_pfn || pfn >= chp->pp_epfn) {
		chp = chp->pp_next;
		if (chp == NULL)
			return (page_t *)NULL;
	}
	return &chp->pp_page[(pfn - chp->pp_pfn) / (PAGESIZE/MMU_PAGESIZE)];
}


/*
 * page_t *
 * page_numtopp_idx_lowres(uint_t pfn, uint_t idx, uint_t idx_res)
 *
 *	Translate a hardware physical page frame number into its
 *	corresponding page_t, using the chunk index # to speed lookup.
 *
 * Calling/Exit State:
 *
 *	pfn is the page frame number of an MMU_PAGESIZE sized page.
 *
 *	idx is the low-order bits of the index into the pagepool array
 *		of the chunk which contains the indicated page.
 *
 *	idx_res is the resolution of the idx value; i.e. if n bits are
 *		used to hold idx, idx_res should be (2^n - 1).
 *
 *	Returns a pointer to the corresponding page_t.
 *	Unlike with page_numtopp(), the pfn is required to have a
 *	corresponding page_t.
 *
 * Remarks:
 *	Typically, the idx value will have been cached by the HAT in a
 *	small number of bits in the PTE, or other per-translation data
 *	structure, from the p_chidx field of the page when the translation
 *	was loaded.
 *
 *	This interface should not be used for HAT hidden mappings, as
 *	there may be no corresponding page_t.
 *
 *	Normally, this will not be called directly.  Instead, the
 *	page_numtopp_idx() macro should be used.
 */
page_t *
page_numtopp_idx_lowres(uint_t pfn, uint_t idx, uint_t idx_res)
{
	struct pp_chunk *chp = &pagepool[idx];

	while (pfn < chp->pp_pfn || pfn >= chp->pp_epfn) {
		chp += idx_res;
		ASSERT(chp - pagepool < n_pp_chunk);
	}
	return &chp->pp_page[(pfn - chp->pp_pfn) / (PAGESIZE/MMU_PAGESIZE)];
}


/*
 * page_t *
 * page_phystopp(paddr_t paddr)
 *
 *	Translate a physical address into its corresponding page_t, if any.
 *
 * Calling/Exit State:
 *
 *	paddr is a kernel physical address.
 *
 *	Returns a pointer to the corresponding page_t, if any,
 *	else returns NULL.
 */
page_t *
page_phystopp(paddr_t paddr)
{
	return (page_numtopp(mmu_btop(paddr)));
}


/*
 * paddr_t
 * page_pptophys(const page_t *pp)
 *
 *	Translate a page_t to the physical address of the corresponding page.
 *
 * Calling/Exit State:
 *
 *	Returns the physical address of the page corresponding to "pp".
 */
paddr_t
page_pptophys(const page_t *pp)
{
	return (mmu_ptob(page_pptonum(pp)));
}


/*
 * page_t *
 * getnextpg(buf_t *bp, const page_t *pp)
 *
 *	Get next page from a B_PAGEIO buffer header.
 *
 * Calling/Exit State:
 *
 *	Given a buffer header, bp, and a pointer to the page, pp,
 *	returned from the previous call to getnextpg(), the next page
 *	is returned.  If pp is NULL, the first page in the page list
 *	is returned.  If the end of the list is reached, NULL is returned.
 *
 * Remarks:
 *
 *	This is a DDI/DKI routine.
 */
page_t *
getnextpg(buf_t *bp, const page_t *pp)
{
	if (pp == NULL) {
		return (bp->b_pages);		/* return the first page */
	} else if (pp->p_next == bp->b_pages) {
		return ((page_t *)NULL);	/* we're at the last page */
	} else {
		CHECKPPLIST(pp, &bp->b_pages);
		return (pp->p_next);		/* return the next page */
	}
}

/*
 * void
 * page_swapreclaim(boolean_t desperate)
 *
 *      Reclaim swap space from swap backed (anonfs/memfs) pages which
 *	no longer need it.
 *
 * Calling/Exit State:
 *
 *      Called with no locks held and returns the same way. Acquires
 *      p_uselock of pages as they are scanned. Uses page_swapreclaim_lck
 *      to serialize between regular and `desperate' callers.
 *
 *      Normally called once a second via pooldaemaon() with `desperate'
 *      set to B_FALSE. Also called as needed from anon_pageout with
 *      `desperate' equal to B_TRUE. Effects of `desperate' setting are
 *      described in ``Description'' section below.
 *
 *      Returns no useful value (void).
 *
 * Description:
 *
 *	A page of a swap backed file system (anonfs or memfs) is said to be
 *	`doubly associated' if it is present both in memory and on the swap
 *	device. Double associations arise because (1) pages are not aborted
 *	when they are cleaned, and (2) swap space is not necessary freed
 *	when pageins occur.
 *
 *	The current VM implementation of SVR4 tolerates a certain amount of
 *	`double association' as an optimization (since it save us the
 *	trouble of swapping these pages if they are clean when unloaded and
 *	allows us to possibly reclaim them later without going back to
 *	swap, as noted above). If the system runs low on real memory and
 *	needs to clean pages then it no longer becomes possible to tolerate
 *	significant numbers of doubly associated pages. Ultimately, when
 *	all of real and swap memory is committed, it cannot tolerate the
 *	existence of any of these pages. This follows from the fact that
 *	mem_resv(..., M_SWAP) is willing to issue as many M_SWAP
 *	reservations as there are pages in physical memory plus the swap
 *	files. The purpose of page_swapreclaim() is to allow all the holders
 *	of M_SWAP reservations to make use of them.
 *
 *	When `desperate' is B_FALSE, this routine makes an estimate of how
 *	many pages need to be scanned, and then scans this many pages (if
 *	the estimate is non-zero).  If desperate, then all pages are scanned.
 *
 *	ANONFS:
 * 
 *		Anon pages which are read in from swap via an S_WRITE fault
 *		give up their swap space immediately since these pages are
 *		immediately modified and the swap copy is stale. Dirty anon
 *		pages which are freed give up their swap space at that
 *		time. Pages which are read in from swap via an S_READ fault
 *		are allowed to keep their swap space since the page is
 *		clean and we may be able to later free these pages without
 *		cleaning them again. In a similiar manner, pages pushed to
 *		swap keep real memory tied up until they are reused under a
 *		new identity. Such pages are considered to be `doubly
 *		associated' since they are consuming both a page of real
 *		and a page of swap memory.
 *
 *		When `desperate' is equal to B_FALSE, double association
 *		will only be broken for anonfs pages which are already
 *		dirty, provided that the pseudo ANON_T_LOCK can be
 *		obtained. Otherwise, double association will be broken for
 *		all non-free anonfs pages for which the pseudo ANON_T_LOCK
 *		can be obtained.
 *
 *  	MEMFS:
 *  
 * 		Like anonfs, memfs creates a doubly associated page the
 * 		first time it cleans a page to swap. However, unlike
 * 		anonfs, memfs will not break the double association for an
 * 		S_WRITE fault . Thus, memfs is totally reliant upon this
 * 		routine to break double association for it.
 * 
 *		This routine take no action on memfs pages when `desperate'
 *		is equal to B_FALSE. Otherwise, it breaks double
 *		association on all non-free memfs pages for which the
 *		MEM_T_LOCK can be obtained.
 *
 * PERF: When all swap files fill up, this routine can be called multiple
 *	 times each time the pageout daemon gets to run. One calls will be
 *	 generated by anon_pageout(). Multiple calls can be generated by
 *	 memfs_doputpage(). Since all of these calls will be in
 *	 `desperate' mode, the result is multiple scans of the entire page
 *	 pool.
 */
void
page_swapreclaim(boolean_t desperate)
{
        page_t *pp;
        uint_t pgs_to_scan, doubles, swapfree, dirty, totpages;
	anon_t *ap;

	ASSERT(getpl() == PLBASE);

	/*
	 * (1) In desperate mode, we scan all pages. Otherwise,
	 *
	 * (2) If there are no swapdoubles, then there is nothing to do.
	 * 
	 * (3) If there is more swap space than reservations at this
	 *     time, then there is no need to break doubles.
	 *
	 * (4) If there are insufficient dirty free pages to consume the
	 *     swap space, then do nothing.
	 *
	 * (5) Estimate how many pages we need to scan based upon the
	 *     number of dirty pages and the number of doubles present.
	 *
	 * Note that the unlocked tests of nswappg, nswappgfree, and
	 * anoninfo.ani_resv are acceptable in the non-desperate case, since
	 * this is only part of an optimization.
	 */

	if (desperate) {
		ASSERT(swapdoubles != 0);
		(void) LOCK(&page_swapreclaim_lck, VM_SWAPREC_IPL);
		PGSTAT_BUMP(page_swapreclaim_desperate);
		pgs_to_scan = (epages - pages);
	} else {
		doubles = swapdoubles;
		swapfree = nswappgfree;
		dirty = page_dl_size;
		if (doubles == 0 || nswappg > anoninfo.ani_resv ||
		    dirty <= swapfree ||
		    TRYLOCK(&page_swapreclaim_lck, VM_SWAPREC_IPL) == INVPL)
			return;
		totpages = (epages - pages);
                pgs_to_scan = totpages * (dirty - swapfree) / doubles;
		if (pgs_to_scan > totpages)
			pgs_to_scan = totpages;
	}

        pp = page_swapreclaim_nextpp;

        while (pgs_to_scan) {

                /*
		 * For anonfs, this acquisition of the PAGE_USELOCK,
		 * together with the tests of p_invalid and p_pageout
		 * acquires the pseudo ANON_T_LOCK lock (in trylock mode).
		 * For memfs, this code acquires the pseudo MEM_T_LOCK.
                 */

                PAGE_USELOCK(pp);

                if (!PAGE_IS_WRLOCKED(pp) && 
		    pp->p_pageout == 0 && pp->p_vnode != NULL &&
		    (pp->p_vnode->v_flag & VSWAPBACK)) {

			/*
			 * Check if page is anonfs or memfs.
			 */
			if (IS_ANONVP(pp->p_vnode)) {
				ap = anon_pptoan(pp);
				ASSERT(ap != (anon_t *)NULL);

				/*
				 * Page is anonfs. Is it doubly associated?
				 */
				if (!ANON_HAS_SWAP(ap))
					goto skip_page;

				/*
				 * Look at all unmodified and unfree pages.
				 * Pages on the dirtyalist have already
				 * had double association broken. Clean
				 * free pages are reuseable. We rely upon
				 * the resuse mechanism to break double
				 * association.
				 * 
                                 * If we are not desperate and the page is not
                                 * already marked modified, look to see if
                                 * this page is modified without actually
                                 * syncing the bit. If we are desperate then
                                 * we don't care and simply blast p_mod into
                                 * the page. We rely on the fact that if this
                                 * page is NOT really modified that the p_mod
                                 * bit won't be cleared by subseqent calls to
                                 * either hat_pagegetmod or hat_pagesyncmod.
                                 */

				if (!pp->p_mod && !pp->p_free) {
					if (!desperate)
						hat_pagegetmod(pp);
					else
						pp->p_mod = 1;
						/*
						 * p_timestamp is note used
						 * for anonfs pages
						 */
				}

                                if (pp->p_mod) {
                                        anon_freeswap(ap, B_TRUE);
					ANON_SAVE_HIST(ap, ANON_H_RECBRK);
                                        PGSTAT_BUMP(page_swapreclaim_reclaims);
                                }
                        } else {
				/*
				 * Page is memfs.
				 *
				 * We must be desperate to take any action
				 * at all (so as not to interfere with
				 * klustering). We skip clean free pages
				 * because they are reusable. We rely upon
				 * the resuse mechanism to break double
				 * association. We skip dirty free pages
				 * since they will soon become clean free
				 * pages.
				 */
				ASSERT(IS_MEMFSVP(pp->p_vnode));
				if (!desperate || pp->p_free ||
				    !memfs_has_swap(pp))
					goto skip_page;

				/*
				 * Inline expansion of page_setmod_l(pp).
				 *
				 * The actual page_setmod_l() function
				 * ASSERTS that the page is locked. While we
				 * hold an effective READer lock here, we
				 * don't actually have the page locked.
				 */
				pp->p_mod = 1;
				pp->p_timestamp = lbolt;

				memfs_freeswap(pp);
				PGSTAT_BUMP(page_swapreclaim_reclaims);
			}
                }
skip_page:
                PAGE_USEUNLOCK(pp);

                /*
                 * Handle wrap
                 */

                if (++pp == epages)
                        pp = pages;

                --pgs_to_scan;
        }

        page_swapreclaim_nextpp = pp;

        UNLOCK(&page_swapreclaim_lck, PLBASE);
} 

/*
 * int
 * page_anonpageout(int pgs, clock_t stamp, clock_t interval, page_t **ppp)
 *      Pull up to pgs worth of pages from the page_dirtyalist and
 *      return them on their own page list (ppp) prepped for pageout.
 *      The actual number of pages prepped is returned.
 *
 * Calling/Exit State:
 *      Called without any special MP locks held and returns the same
 *      way. Acquires VM_PAGEFREELOCK() during processing along with the
 *      p_uselock of pages we are interested in.
 *
 *      On exit, the page list pointer passed in by our caller is
 *      set to point at the list of prepped pages (unfreed, marked
 *      p_pageio, read locked) and the return value indicates the
 *      actual number of pages on the list.
 *
 * Description:
 *      Pages on the page_dirtyagelist are scanned under the aegis
 *      of the VM_PAGEFREELOCK(). Pages which have not been on the list
 *      long enough to meet the aging criteia of IS_PAGE_AGED() are
 *      skipped. Since this list is FIFO, once a page of insufficient
 *      age is found, the search is discontinued since only newer pages
 *      will be found after this point.
 *
 *      We attempt to acquire the p_uselock of pages once they have met
 *      the IS_PAGE_AGED() criterion. Since this is an out-of-lockorder
 *      acquisition and since finding the lock held is an indication that
 *      another agent is attempting to reclaim the page, we skip pages
 *      whose p_uselock we cannot acquire immediately.
 *
 *      Pages which are successfully TRYLOCKed are then prepped for
 *      pageout: p_pageio is set, the page is read locked and unfreed.
 *      Finally the page is added to the caller supplied page list.
 *	Since the list we are scanning is a circular one, we remember the
 *	first page that we could not successfully TRYLOCK, and if we come 
 *	around to the same page, then we are done with processing. This 
 *	strategy relies on the fact that the entire processing was performed 
 *	under the cover of VM_PAGEFREELOCK(), which barred any other agents
 *	from altering the list.
 */
int
page_anonpageout(int pgs, clock_t stamp, clock_t interval, page_t **ppp)
{
        int i = 0;
        page_t *pp;
        page_t *remember_pp = NULL;
	page_t *npp; 

        VM_PAGEFREELOCK();
	npp = page_dirtyalist;
	while ((i != pgs) && ((pp = npp) != remember_pp)) {
		if (page_dirtyalist == NULL) 
			break;

		ASSERT(page_dirtyalist_size > 0);
		ASSERT(pp != NULL);

		/*
		 * Remember p_next here because it will be invalid if
		 * we page_add the current page to the caller's page list.
		 */
		npp = pp->p_next;

                /*
                 * Check this page against timestamp passed by
                 * our caller. Since pages on the dirtyalist
                 * are timestamped in FIFO order, once we find a
                 * page which is not fully aged, there is little point
                 * in going on. Break out and return.
                 */

                if (!IS_PAGE_AGED(pp, stamp, interval)) {
                        break;
                }

                /*
                 * TRYLOCK the page. Someone may be attempting a reclaim
                 * elsewhere and has locked the page prepatory to moving
                 * if off the dirtyalist. This is a race we're happy to lose.
                 */

                if (PAGE_TRYUSELOCK(pp) == INVPL) {
                        /*
                         * Someone has interest in this page, so just
                         * skip it.  They'll probably reclaim it.  If
                         * not, we'll clean it eventually. If this is the 
			 * first time that we're skipping a page, remember it 
			 * so that if we see it again, it is time to quit.
                         */
			if (remember_pp == NULL) {
				remember_pp = pp;
			}
			continue;
                }

                /*
                 * Immediately got the page lock.
                 * Adjust cached ipl level so we can release
                 * it via PAGE_USEUNLOCK().
                 */

		l.puselockpl = VM_PAGE_IPL;

                /*
                 * Because page is free, it has:
                 *      - no translations (p_mapping == NULL),
                 *      - no reader/writer locks
                 */

                ASSERT(!PAGE_IN_USE(pp));
                ASSERT(pp->p_free);
                ASSERT(!pp->p_invalid);
		
                /*
                 * Remove page from page_dirtyalist, clearing p_mod
	 	 * after this page is moved off dirtyalis.t Its OK to 
		 * at that point because we are committed to the pageout. 
		 *
		 * Because page is on dirtyalist, it is:
		 *	- p_mod 
		 *
		 * Note we pass 1 to indicate that we know that freemem
		 * does NOT need to be adjusted for this page which has
		 * NOT been cleaned yet and is not reflected in the global
		 * freemem counter. This happens AFTER the I/O. 
		 */

		ASSERT(pp->p_mod);

		/* clears p_free */
		page_unfree_l(pp, B_FALSE, NOT_LOCKED, P_LOOKUP_OR_CREATE);

		ASSERT(!pp->p_free);

                /*
                 * Establish page reader lock for pageout and mark
                 * the page p_pageout. Reset p_timestamp
                 *
                 * Drop the PAGE_USELOCK spinlock.
                 */

		pp->p_mod = 0;
		pp->p_timestamp = 0;
                pp->p_pageout = 1;
                pp->p_activecnt++;
                PAGE_USEUNLOCK(pp);

                page_add(ppp, pp);  /* add to list for VOP_PUTPAGELIST */
                i++;
        }

        VM_PAGEFREEUNLOCK();

        return (i);
}

/*
 * boolean_t
 * page_cache_query(vnode_t *vp, off_t off)
 *
 *	Examine the page cache and return a boolean indicating the existence
 *	of a page under the indicated <vp, offset> identity.
 *
 * Calling/Exit State:
 *
 *      No special MP state is required for entry or is held on exit.
 *
 *      If the cache contains and entry for the passed <vp, offset>
 *      pair then B_TRUE is returned, otherwise B_FALSE is returned.
 *
 * Remarks:
 *      The indication passed back is guaranteed to be stale unless the
 *      caller prevents this by external means.
 */
boolean_t
page_cache_query(vnode_t *vp, off_t off)
{
	boolean_t ret;

	VM_PAGEIDRDLOCK();

	ret = (page_exists(vp, off) ? B_TRUE : B_FALSE);

	VM_PAGEIDUNLOCK();
	return ret;
}

/*
 * void
 * page_uselock(page_t *pp)
 *	Lock a page
 *
 * Calling/Exit State:
 *	See PAGE_USELOCK in page.h.
 *
 * Remarks:
 *	Procedure call used to preserve compatibility for
 *	non-VM modules.  Not to be called directly, only
 *	through a macro.
 */
void
page_uselock(page_t *pp)
{

	PAGE_USELOCK(pp);
}

/*
 * void
 * page_useunlock(page_t *pp)
 *	Unlock a page
 *
 * Calling/Exit State:
 *	See PAGE_USEUNLOCK in page.h.
 *
 * Remarks:
 *	Procedure call used to preserve compatibility for
 *	non-VM modules.  Not to be called directly, only
 *	through a macro.
 */
void
page_useunlock(page_t *pp)
{
	PAGE_USEUNLOCK(pp);
}

/*
 * pl_t
 * page_tryuselock(page_t *pp)
 *	Try to lock a page
 *
 * Calling/Exit State:
 *	See PAGE_TRYUSELOCK in page.h
 *
 * Remarks:
 *	Procedure call used to preserve compatibility for
 *	non-VM modules.  Not to be called directly, only
 *	through a macro.
 */
pl_t
page_tryuselock(page_t *pp)
{

	return PAGE_TRYUSELOCK(pp);
}

#ifdef	DEBUG

/*
 * boolean_t
 * page_uselock_owned(page_t *pp)
 *	Indicate if lock is owned by calling context
 *
 * Calling/Exit State:
 *	See PAGE_USELOCK_OWNED in page.h
 *
 * Remarks:
 *	Procedure call used to preserve compatibility for
 *	non-VM modules.  Not to be called directly, only
 *	through a macro.
 */
boolean_t
page_uselock_owned(page_t *pp)
{

	return PAGE_USELOCK_OWNED(pp);
}

/*
 * boolean_t
 * page_uselock_locked(page_t *pp)
 *	Indicate if lock is locked by any context
 *
 * Calling/Exit State:
 *	See PAGE_USELOCK_LOCKED in page.h
 *
 * Remarks:
 *	Procedure call used to preserve compatibility for
 *	non-VM modules.  Not to be called directly, only
 *	through a macro.
 */
boolean_t
page_uselock_locked(page_t *pp)
{

	return PAGE_USELOCK_LOCKED(pp);
}

#endif	/* DEBUG */

#ifdef PAGE_DEBUG

/*
 * Debugging routine only!
 */

extern void	call_demon(void);
STATIC int call_demon_flag = 1;

/*
 * void
 * call_debug(char *mess)
 *	Debugging Routine.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
call_debug(char *mess)
{
	/*
	 *+ print out debug msg as a warning.
	 */
	cmn_err(CE_WARN, mess);
	if (call_demon_flag)
		call_demon();
}

/*
 * void
 * page_vp_check(const vnode_t *vp)
 *	Debugging Routine. Verify that vp->v_pages contains only proper pages.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
page_vp_check(const vnode_t *vp)
{
	page_t *pp;
	int count = 0;
	int err = 0;

	if (vp == NULL)
		return;

	if ((pp = vp->v_pages) == NULL) {
		/* random check to see if no pages on this vp exist */
		if ((pp = page_exists(vp, (off_t)0)) != NULL) {
			cmn_err(CE_CONT,
				"page_vp_check: pp=0x%x on NULL vp list\n", vp);
			call_debug("page_vp_check");
		}
		return;
	}

	do {
		/*
		 * If the page is not for this vnode,
		 * and it's not a marker page, then it's bad.
		 */
		if (pp->p_vnode != vp
		 && ((char *)pp->p_vnode < (char *)KVBASE
		 || (char *)pp->p_vnode >= etext)) {
			cmn_err(CE_CONT, "pp=0x%x pp->p_vnode=0x%x, vp=0x%x\n",
			    pp, pp->p_vnode, vp);
			err++;
		}
		if (pp->p_vpnext->p_vpprev != pp) {
			cmn_err(CE_CONT,
			    "pp=0x%x != p_vpnext->p_vpprev=0x%x (p_vpnext=0x%x)\n",
			    pp,  pp->p_vpnext->p_vpprev, pp->p_vpnext);
			err++;
		}
		if (++count > 10000) {
			cmn_err(CE_CONT, "vp loop\n");
			err++;
			break;
		}
		pp = pp->p_vpnext;
	} while (err == 0 && pp != vp->v_pages);

	if (err)
		call_debug("page_vp_check");
}

/*
 * void page_free_check(uint_t flag)
 * Debugging routine only!
 * Verify that the free lists contain only proper pages.
 *
 *	If flag set then stricter checking is enforced:
 *		- announce pages which are PAGE_USELOCK()'d
 *		- require *exactly* freemem pages on the free lists.
 * Calling/Exit State:
 *	None.
 */
STATIC void
page_free_check(uint_t flag)
{
	boolean_t freemem_err = B_FALSE;
	int err = 0;
	int count[NPAGETYPE];
	int type;
	page_t *pp;

	for (type = 0; type < page_ntype_used; type++) {
		count[type] = 0;
		if (page_freelist[type] != NULL) {
			pp = page_freelist[type];
			do {
				if (!pp->p_free || pp->p_mod ||
				    pp->p_vnode != (vnode_t *)NULL) {
					err++;
					cmn_err(CE_CONT, "page_freelist: "
						"BAD pp = 0x%x\n", pp);
				}
				if (flag && PAGE_USELOCK_LOCKED(pp)) {
					cmn_err(CE_CONT, "page_freelist: "
						"USELOCK'd pp = 0x%x\n", pp);
				}
				count[type]++;
				pp = pp->p_next;
			} while (pp != page_freelist[type]);
		}
		if (page_cachelist[type] != NULL) {
			pp = page_cachelist[type];
			do {
				if (!pp->p_free || pp->p_mod ||
				    pp->p_vnode == (vnode_t *)NULL) {
					err++;
					cmn_err(CE_CONT, "page_cachelist: "
						"BAD pp = 0x%x\n", pp);
				}
				if (flag && PAGE_USELOCK_LOCKED(pp)) {
					cmn_err(CE_CONT, "page_cachelist: "
						"USELOCK'd pp = 0x%x\n", pp);
				}
				count[type]++;
				pp = pp->p_next;
			} while (pp != page_cachelist[type]);
		}
	}
	if (page_dirtyflist != NULL) {
		pp = page_dirtyflist;
		do {
			if (!pp->p_free || !pp->p_mod ||
			    pp->p_vnode == (vnode_t *)NULL ||
			    IS_ANONVP(pp->p_vnode)) {
				err++;
				cmn_err(CE_CONT, "page_dirtyflist: "
					"BAD pp = 0x%x\n", pp);
			}
			if (flag && PAGE_USELOCK_LOCKED(pp)) {
				cmn_err(CE_CONT, "page_dirtyflist: "
					"USELOCK'd pp = 0x%x\n", pp);
			}
			pp = pp->p_next;
		} while (pp != page_dirtyflist);
	}
	if (page_dirtyalist != NULL) {
		pp = page_dirtyalist;
		do {
			if (!pp->p_free || !pp->p_mod ||
			    pp->p_vnode == (vnode_t *)NULL ||
			    !IS_ANONVP(pp->p_vnode)) {
				err++;
				cmn_err(CE_CONT, "page_dirtyalist: "
					"BAD pp = 0x%x\n", pp);
			}
			if (flag && PAGE_USELOCK_LOCKED(pp)) {
				cmn_err(CE_CONT, "page_dirtyalist: "
					"USELOCK'd pp = 0x%x\n", pp);
			}
			pp = pp->p_next;
		} while (pp != page_dirtyalist);
	}

	/*
	 * We signal a problem if:
	 *	- an error was seen
	 *	- we have fewer pages than freemem states
	 *	  (We could have more if we're racing with page_get().
	 *	- flag indicates strict checking and we don't have
	 *	  exactly freemem pages.
	 */
	for (type = 0; type < page_ntype_used; type++) {
		if (count[type] < mem_freemem[type] ||
		    (flag && count[type] != mem_freemem[type]))
			freemem_err = B_TRUE;
	}
	if (err || freemem_err) {
		cmn_err(CE_CONT, "page_free_check:  %d bad pages,\n", err);
		for (type = 0; type < page_ntype_used; type++) {
			cmn_err(CE_CONT, "\tcount[%d] = %x, freemem[%d] = %x\n",
				type, count[type], type, mem_freemem[type]);
		}
		call_debug("page_free_check");
	}
}

/*
 * void page_list_check(const page_t *plist)
 * Debugging routine only!
 * Verify that the list is properly sorted by offset on same vp
 * 
 * Calling/Exit State:
 *	None.
 */
STATIC void
page_list_check(const page_t *plist)
{
	const page_t *pp = plist;

	if (pp == NULL)
		return;
	while (pp->p_next != plist) {
		if (pp->p_next->p_offset <= pp->p_offset ||
		    pp->p_vnode != pp->p_next->p_vnode) {
			cmn_err(CE_CONT, "pp = 0x%x <0x%x, 0x%x> pp next = 0x%x <0x%x, 0x%x>\n",
			    pp, pp->p_vnode, pp->p_offset, pp->p_next,
			    pp->p_next->p_vnode, pp->p_next->p_offset);
			call_debug("page_list_check");
		}
		pp = pp->p_next;
	}
}

/*
 * void age_pp_list_check(const page_t *pp, page_t **plist)
 * Debugging routine only! Verify that the page is on the page list.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
page_pp_list_check(const page_t *pp, page_t **plist)
{
	const page_t *pp1 = *plist;

	if (pp1 != NULL) {
		do {
			if (pp1 == pp)
				return;
			pp1 = pp1->p_next;
		} while (pp1 != *plist);
	}
	cmn_err(CE_CONT, "pp 0x%x not on page list 0x%x\n", pp, plist);
	call_debug("page_pp_list_check");
}

/*
 * void page_pp_check(const page_t *pp)
 * Debugging routine only!
 * Verify that pp is actually on vp page list.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
page_pp_check(const page_t *pp)
{
	page_t *p1;
	vnode_t *vp;
	boolean_t acquired = B_FALSE;
	pl_t ospl;

	if (!VM_PAGEIDLOCK_OWNED()) {
		ospl = RW_TRYRDLOCK(&vm_pageidlock, VM_PAGE_IPL);
		if (ospl == INVPL)
			return;
		acquired = B_TRUE;
	}

	if ((vp = pp->p_vnode) == (vnode_t *)NULL)
		goto unlock;

	if ((p1 = vp->v_pages) == (page_t *)NULL) {
		cmn_err(CE_CONT, "pp = 0x%x, vp = 0x%x\n", pp, vp);
		call_debug("NULL vp page list");
		goto unlock;
	}

	do {
		if (p1 == pp)
			goto unlock;
	} while ((p1 = p1->p_vpnext) != vp->v_pages);

	cmn_err(CE_CONT, "page 0x%x not on vp 0x%x page list\n", pp, vp);
	call_debug("vp page list");

unlock:
	if (acquired)
		RW_UNLOCK(&vm_pageidlock, ospl);
}
#endif /* PAGE_DEBUG */


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void print_page(const page_t *pp)
 * 	Debugging print routines.
 *
 * Calling/Exit State:
 *	None.
 *
 */
void
print_page(const page_t *pp)
{
	vnode_t *vp = pp->p_vnode;

	debug_printf("pp 0x%x ", pp);

	/* check for marker page */
	if ((vaddr_t)vp >= KVBASE && (char *)vp < etext) {
		debug_printf("marker for 0x%x lwp 0x%x\n",
				 vp, pp->p_offset);
		debug_printf(" vpnext 0x%x vpprev 0x%x\n",
				 pp->p_vpnext, pp->p_vpprev);
		return;
	}

	debug_printf("mapping 0x%x nio %d activecnt %d",
			 pp->p_mapping, pp->p_nio, pp->p_activecnt);
	if (SV_BLKD(&(pp)->p_wait))
		debug_printf(" (WAITERS)");
#ifdef DEBUG
	if (PAGE_USELOCK_LOCKED(pp))
		debug_printf(" USELOCKED");
#endif
	debug_printf("%s%s%s%s%s\n", 
			 (pp->p_invalid) ? " INVALID" : "" ,
			 (pp->p_free)    ? " FREE"    : "" ,
			 (pp->p_pageout) ? " PAGEOUT" : "" ,
			 (pp->p_mod)     ? " MOD"     : "" ,
			 (pp->p_ioerr)   ? " IOERR"   : "" );

	debug_printf(" type %d\n", pp->p_type);
	debug_printf(" vnode 0x%x, offset 0x%x", vp, pp->p_offset);
	if (vp) {
		if (IS_ANONVP(vp))
			debug_printf(" (ANONVP)");
		debug_printf("  v_flag 0x%x, v_count %d, v_type %d",
				 vp->v_flag, vp->v_count, vp->v_type);
	}
	debug_printf("\n next 0x%x, prev 0x%x, vpnext 0x%x vpprev 0x%x\n",
			 pp->p_next, pp->p_prev, pp->p_vpnext, pp->p_vpprev);
}

/*
 * void print_vnode_pages(const vnode_t *pp)
 * 	Debugging print routines.
 *
 * Calling/Exit State:
 *	None.
 *
 */
void
print_vnode_pages(const vnode_t *vp)
{
	page_t *pp;

	debug_printf("v_pages 0x%x for vnode 0x%x:\n",
			 vp->v_pages, vp);
	if ((pp = vp->v_pages) == NULL)
		return;
	do {
		print_page(pp);
	} while ((pp = pp->p_vpnext) != vp->v_pages);
}

/*
 * void conv_phystopp(uint_t v)
 * 	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
conv_phystopp(uint_t v)
{
	page_t *pp;

	debug_printf("pfn=0x%x, ", pfnum(v));
	pp = page_phystopp(v);
	if (pp)
		debug_printf("pp=0x%x\n", pp);
	else
		debug_printf("pp=NULL\n");
}

/*
 * void conv_phystopp(uint_t v)
 * 	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
conv_pptonum(const page_t *pp)
{
	debug_printf("pfn=0x%x, ", page_pptonum(pp));
	debug_printf("phys=0x%x\n", page_pptophys(pp));
}

/*
 * void find_page_by_id(const vnode_t *vp, off_t off)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
find_page_by_id(const vnode_t *vp, off_t off)
{
	page_t *pp;
	boolean_t found = B_FALSE;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash) {
		if (PAGE_HAS_IDENTITY(pp, vp, off)) {
			print_page(pp);
			found = B_TRUE;
		}
	}
	if (!found)
		debug_printf("not found\n");
}

/*
 * void print_pagepool(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_pagepool(void)
{
	struct pp_chunk *chp;

	debug_printf("\nPagepool Chunk Table:\n");
	for (chp = pp_first; chp != NULL; chp = chp->pp_next) {
		debug_printf("  (%d)  PFN 0x%x-0x%x  PP 0x%x-0x%x\n",
			chp - pagepool,
			chp->pp_pfn, chp->pp_epfn - 1,
			chp->pp_page, chp->pp_epage - 1);
	}
	debug_printf("\n");
}

/*
 * void print_page_plocal_stats(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_page_plocal_stats(void)
{
	uint_t i;
	struct plocalmet *plp;
	uint_t preatch = 0;
	uint_t atch = 0;
	uint_t atchfree = 0;
	uint_t atchfree_pgout = 0;
	uint_t atchmiss = 0;
	uint_t pgin = 0;
	uint_t pgpgin = 0;
	uint_t pgout = 0;
	uint_t pgpgout = 0;
	uint_t swpout = 0;
	uint_t pswpout = 0;
	uint_t vpswpout = 0;
	uint_t swpin = 0;
	uint_t pswpin = 0;
	uint_t virscan = 0;
	uint_t virfree = 0;
	uint_t physfree = 0;
	uint_t pfault = 0;
	uint_t vfault = 0;
	uint_t sftlock = 0;


	debug_printf("eng\tmpv_preatch  mpv_atch  mpv_atchfree  mpv_atchfree_pgout"
		      " mvp_atchmiss\n");

	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("%2d\t %6d\t  %6d\t%6d\t\t    %6d\t %6d\n",
			i, 
			plp->metp_vm.mpv_preatch,
			plp->metp_vm.mpv_atch,
			plp->metp_vm.mpv_atchfree,
			plp->metp_vm.mpv_atchfree_pgout,
			plp->metp_vm.mpv_atchmiss);
		preatch += plp->metp_vm.mpv_preatch;
		atch += plp->metp_vm.mpv_atch;
		atchfree += plp->metp_vm.mpv_atchfree;
		atchfree_pgout += plp->metp_vm.mpv_atchfree_pgout;
		atchmiss += plp->metp_vm.mpv_atchmiss;
		if (debug_output_aborted())
			return;
	}
	debug_printf("  \t ------\t   ------\t------\t\t    ------\t ------\n"
		     "  \t %6d\t   %6d\t%6d\t\t    %6d\t %6d\n",
		preatch, atch, atchfree, atchfree_pgout, atchmiss);

	debug_printf("\n\neng\tmpv_pgin  mpv_pgpgin  mpv_pgout  mvp_pgpgout\n");

	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("%2d\t  %6d      %6d     %6d       %6d\n",
			i, plp->metp_vm.mpv_pgin,
			plp->metp_vm.mpv_pgpgin,
			plp->metp_vm.mpv_pgout,
			plp->metp_vm.mpv_pgpgout);
		pgin += plp->metp_vm.mpv_pgin;
		pgpgin += plp->metp_vm.mpv_pgpgin;
		pgout += plp->metp_vm.mpv_pgout;
		pgpgout += plp->metp_vm.mpv_pgpgout;
		if (debug_output_aborted())
			return;
	}
	debug_printf("  \t  ------      ------     ------       ------\n"
		     "  \t  %6d      %6d     %6d       %6d\n",
		pgin, pgpgin, pgout, pgpgout);

	debug_printf("\n\neng\tmpv_swpout  mpv_pswpout  mpv_vpswpout"
		      "  mvp_swpin  mvp_pswpin\n");

	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("%2d\t  %6d\t %6d        %6d\t  %6d      %6d\n",
			i, plp->metp_vm.mpv_swpout,
			plp->metp_vm.mpv_pswpout,
			plp->metp_vm.mpv_vpswpout,
			plp->metp_vm.mpv_swpin,
			plp->metp_vm.mpv_pswpin);
		swpout += plp->metp_vm.mpv_swpout;
		pswpout += plp->metp_vm.mpv_pswpout;
		vpswpout += plp->metp_vm.mpv_vpswpout;
		swpin += plp->metp_vm.mpv_swpin;
		pswpin += plp->metp_vm.mpv_pswpin;
		if (debug_output_aborted())
			return;
	}
	debug_printf("  \t  ------\t ------        ------"
		      "\t  ------      ------\n"
		     "  \t  %6d\t %6d        %6d"
		      "\t  %6d      %6d\n",
		swpout, pswpout, vpswpout, swpin, pswpin);

	debug_printf("\n\neng\tmpv_virscan  mpv_virfree  mpv_physfree\n");

	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("%2d\t  %6d\t  %6d\t%6d\n",
			i, plp->metp_vm.mpv_virscan,
			plp->metp_vm.mpv_virfree,
			plp->metp_vm.mpv_physfree);
		virscan += plp->metp_vm.mpv_virscan;
		virfree += plp->metp_vm.mpv_virfree;
		physfree += plp->metp_vm.mpv_physfree;
		if (debug_output_aborted())
			return;
	}
	debug_printf("  \t  ------\t  ------\t------\n"
		     "  \t  %6d\t  %6d\t%6d\n",
		virscan, virfree, physfree);

	debug_printf("\n\neng\tmpv_pfault  mpv_vfault  mpv_sftlock\n");

	for (i = 0; i < Nengine; i++) {
		plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("%2d\t  %6d\t%6d\t     %6d\n",
			i, plp->metp_vm.mpv_pfault,
			plp->metp_vm.mpv_vfault,
			plp->metp_vm.mpv_sftlock);
		pfault += plp->metp_vm.mpv_pfault;
		vfault += plp->metp_vm.mpv_vfault;
		sftlock += plp->metp_vm.mpv_sftlock;
		if (debug_output_aborted())
			return;
	}
	debug_printf("  \t  ------\t------\t     ------\n"
		     "  \t  %6d\t%6d\t     %6d\n",
		pfault, vfault, sftlock);
}

/*
 * void print_page_info(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_page_info(void)
{
	int in_use, is_free, active, mapped;
	page_t *pp;

	in_use = is_free = active = mapped = 0;
	for (pp = pages; pp < epages; pp++) {
		if (PAGE_IN_USE(pp)) {
			++in_use;
#ifdef DEBUG
			if (pp->p_free && !PAGE_USELOCK_LOCKED(pp)) {
				debug_printf("\nWARNING: PAGE IN USE "
					      "and FREE:\n");
				print_page(pp);
			}
#endif
			if (pp->p_activecnt)
				++active;
			if (pp->p_pageout) {
				debug_printf("\nIN PAGEOUT:\n");
				print_page(pp);
			}
			if (pp->p_mapping)
				++mapped;
		}
		if (pp->p_free) {
			++is_free;
		}
#ifdef DEBUG
		if (PAGE_USELOCK_LOCKED(pp)) {
			debug_printf("\nPAGE USELOCKED:\n");
			print_page(pp);
		}
#endif
	}
	debug_printf("\t%d pages in use\n", in_use);
	debug_printf("\t%d pages free\n", is_free);
	debug_printf("\t%d pages active\n", active);
	debug_printf("\t%d pages mapped\n", mapped);
}

/*
 * void print__active_pages(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_active_pages(void)
{
	page_t *pp;

	for (pp = pages; pp < epages; pp++) {
		if (pp->p_activecnt)
			print_page(pp);
	}
}

/*
 * void print_mapped_pages(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_mapped_pages(void)
{
	page_t *pp;

	for (pp = pages; pp < epages; pp++) {
		if (pp->p_mapping)
			print_page(pp);
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef EXTRA_PAGE_STATS

/*
 * void print_page_stats(void)
 *	Debugging routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_page_stats(void)
{
	uint_t i;

#define X(v)    debug_printf("%6d  "#v"\n", pagecnt.v)

	X(pc_free);
	X(pc_free_dontneed);
	X(pc_free_free);
	X(pc_free_cache_clean);
	X(pc_free_cache_dirty);
	X(pc_get);
	X(pc_get_fail_NOSLEEP);
	X(pc_get_fail_TRYLOCK);
	X(pc_get_no_TRYLOCK);
	X(pc_get_cache);
	X(pc_get_free);
	X(pc_getaligned);
	X(pc_getaligned_fail_NOSLEEP);
	X(pc_getaligned_unavail);
	X(pc_reclaim);
	X(pc_reclaim_pgout);
	X(pc_reclaim_lost);
	X(pc_exist_hit);
	X(pc_exist_miss);

	for (i = 0; i < PC_HASH_CNT+1; i++) {
		debug_printf("pc_exist_hashlen[%2d]: %6d\n", i,
				pagecnt.pc_exist_hashlen[i]);
	}

	for (i = 0; i < PC_GET_CNT+1; i++) {
		debug_printf("pc_get_npages[%2d]: %6d\n", i,
				pagecnt.pc_get_npages[i]);
	}
#undef X
}

#endif /* EXTRA_PAGE_STATS */
