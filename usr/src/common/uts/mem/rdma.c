/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/rdma.c	1.28"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/rdma.h>
#include <mem/tuneable.h>
#include <mem/uas.h>
#include <mem/vmparam.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/autotune.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifndef NO_RDMA

/*
 *	This file is responsible for handling arbitrary I/O requests whose
 *	buffers may include pages of memory the underlying I/O device cannot
 *	access.  In general it does this by selecting pages the I/O device
 *	can access, doing the I/O, and moving the data as needed to/from the
 *	original pages.
 *
 * Control flags in devflag:
 *	D_DMA			- The driver needs buffers to be DMA-able.
 */

rdma_mode_t rdma_mode;

/*
 * Default breakup control block used to convert I/O jobs to DMA-able memory
 * for the default DMA class which drivers get when they use D_DMA instead of
 * setting up their own control blocks.
 */
STATIC physreq_t rdma_dflt_preq = {
	/* phys_align */	1,
	/* phys_boundary */	0,
	/* phys_dmasize */	0,
	/* phys_max_scgth */	0
};
bcb_t rdma_dflt_bcb = {
	/* bcb_addrtypes */	BA_KVIRT|BA_UVIRT|BA_PAGELIST,
	/* bcb_flags */		0,
	/* bcb_max_xfer */	0,
	/* bcb_granularity */	1,
	/* bcb_physreqp */	&rdma_dflt_preq
};

STATIC struct rdma_bdevsw {
	int	(*d_strategy)();
} *rdma_bdevsw;

STATIC page_t *rdma_private_pages;
STATIC sv_t rdma_page_sv;
STATIC lock_t rdma_page_lock;

STATIC LKINFO_DECL(rdma_page_lkinfo, "MP:rdma:rdma_page_lock", 0);

/*
 * The rdma_chain structure is used to save b_iodone/b_misc
 * when we have to use our own iodone routine.
 */
struct rdma_chain {
	void	(*rdc_iodone)();
	void	*rdc_chain;
};

/* Page private data for pages owned by RDMA. */
struct rdma_private {
	boolean_t	rp_pageout;	/* page is from pageout private pool */
	struct rdma_chain rp_chain;	/* iodone chaining structure */
};
#define RDMA_PRIVATE(pp)	((struct rdma_private *)(pp)->p_pgprv_data)

#define	VALID_PP(pp)	((pp) >= pages && (pp) < epages)

STATIC int rdma_strategy(buf_t *);
#ifdef DEBUG
STATIC int rdma_stub_strategy(buf_t *);
#endif

STATIC uint_t rdma_dmapages;		/* count of non-DMAable pages */

/*
 * Externs
 */
extern page_t *page_freelist[NPAGETYPE];
extern page_t *page_cachelist[NPAGETYPE];
extern int page_freelist_size[NPAGETYPE];
extern int page_cachelist_size[NPAGETYPE];
extern int page_dirtylists_size[NPAGETYPE];
extern int page_ptype_map[];
extern int page_stype_map[];
extern mresvtyp_t kpg_mtypes[];
extern mresvtyp_t segkvn_mtype_dma;


/*
 * void
 * rdma_page_init(void)
 *	Initialize DMA_PAGE pages for restricted DMA (if needed).
 *
 * Calling/Exit State:
 *	Must be called from page_init() before the page free list creation,
 *	so it can separate the page pool into DMA-able and non-DMA-able pools.
 *
 *	Only called during system initialization, so it's single-threaded.
 */
void
rdma_page_init(void)
{
	page_t *pp;
	uint_t npages, ndmapool;
	int mem;

	npages = epages - pages;
	rdma_dmapages = 0;
	for (pp = pages; pp != epages; pp++) {
		if (DMA_PFN(pp->p_pfn)) {
			pp->p_physdma = 1;
			++rdma_dmapages;
		}
	}

	/*
	 * Decide if we need restricted DMA support enabled. However,
	 * since we cannot open the rootdev at this time, we have no
	 * way to decide between RDMA_SMALL and RDMA_LARGE. So we enter
	 * RDMA_LARGE at this time and will convert into model small
	 * should the need arise later.
	 */
	if (rdma_dmapages == npages) {
		rdma_mode = (tune.t_devnondma) ? RDMA_MICRO : RDMA_DISABLED;
		page_ptype_map[P_DMA] = STD_PAGE;
		kpg_mtypes[P_DMA] = M_KERNEL_ALLOC;
		segkvn_mtype_dma = M_REAL;
		page_ntype_used = 1;
	} else {
		rdma_mode = RDMA_LARGE;
		ndmapool = btop((11 * MAXBIOSIZE) / 2) +
			   (npages * tune.t_dma_percent / 100);
		/*
		 * Expand the DMA_PAGE pool when drivers are known to
		 * consume lots of DMAable memory.
		 */
		if (PHYSMEM_DRV_DMA())
			ndmapool *= 3;
		if (ndmapool >= rdma_dmapages) {
			ndmapool = rdma_dmapages;
		}

		/*
		 * Adjust memory counts.
		 */
		maxfreemem[STD_PAGE] -= ndmapool;
		maxfreemem[DMA_PAGE] = ndmapool;

		/*
		 * Now adjust the page types of ndmapool of the
		 * physically DMAable pages, making them DMA_PAGE
		 * pages.
		 */
		for (pp = pages; ndmapool != 0; pp++) {
			ASSERT(pp != epages);
			if (pp->p_physdma) {
				pp->p_type = DMA_PAGE;
				--ndmapool;
			}
		}
		page_ntype_used = 2;
	}
}

/*
 * void
 * rdma_convert(void)
 *	See if the system needs to switch to rdma_mode RDMA_SMALL.
 *	If so, do execute the switch.
 *
 * Calling/Exit State:
 *	Called from main() just before the call to vfs_mountroot().
 */

void
rdma_convert(void)
{
	page_t *pp;
	uint_t npages;

	if (rdma_mode != RDMA_LARGE)
		return;

	/*
	 * If the system has a large surplus of non-DMAable pages, then
	 * remain in model LARGE.
	 */
	npages = epages - pages;
	if (npages - rdma_dmapages >= rdma_dmapages * tune.t_lgdma_ratio / 100)
		return;

	/*
	 * If the root device has restricted DMA capabilities, then
	 * switch to model SMALL.
	 */
	if (rdma_root()) {
		rdma_convert_pages(B_FALSE);
	}
}

/*
 * void
 * rdma_convert_pages(boolean_t is_mini)
 *	Convert from rdma_mode RDMA_LARGE to rdma_mode RDMA_SMALL.
 *
 * Calling/Exit State:
 *	Called from rdma_convert(), which is called from main() just before
 *	the call to vfs_mountroot().
 *
 *	If is_mini is B_TRUE, then this functions was called by
 *	mcompat mcompatinit(), and this is a mini-kernel.
 */

void
rdma_convert_pages(boolean_t is_mini)
{
	page_t *pp;

	/*
	 * The following depends upon the system being in effect UNIPROC at
	 * this time. It also depends upon the system being single-threaded,
	 * which guarantees that no LWP can be waiting in any of the
	 * freemem_resv() family functions. The system will remain UNIPROC
	 * and single-threaded until main() creates new LWPs.
	 */
	DISABLE();
	for (pp = pages; pp != epages; pp++) {
		/*
		 * If the page if physically DMAable, but is in the
		 * STD_PAGE pool, then switch it to the PAD_PAGE pool.
		 *
		 * At this point we are not concerned with the order of
		 * the pages on the lists.
		 */
		if (pp->p_physdma && pp->p_type == STD_PAGE) {
			pp->p_type = PAD_PAGE;
			--maxfreemem[STD_PAGE];
			++maxfreemem[PAD_PAGE];
			if (pp->p_free) {
				ASSERT(!PAGE_IN_USE(pp));
				if (pp->p_mod) {
					--page_dirtylists_size[STD_PAGE];
					++page_dirtylists_size[PAD_PAGE];
					continue;
				} 
				if (pp->p_vnode == NULL) {
					page_sub(&page_freelist[STD_PAGE], pp);
					page_add(&page_freelist[PAD_PAGE], pp);
					--page_freelist_size[STD_PAGE];
					++page_freelist_size[PAD_PAGE];
				} else {
					page_sub(&page_cachelist[STD_PAGE], pp);
					page_add(&page_cachelist[PAD_PAGE], pp);
					--page_cachelist_size[STD_PAGE];
					--page_cachelist_size[PAD_PAGE];
				}
				--mem_freemem[STD_PAGE];
				++mem_freemem[PAD_PAGE];
			}
		}
	}

	/*
	 * recompute sections table
	 */
	page_cs_init();

	/*
	 * Enable use of the newly created PAD_PAGEs by adjusting
	 * freemem_resv() family tables.
	 */
	++page_ntype_used;
	page_ptype_map[0] = PAD_PAGE;
	page_stype_map[0] = STD_PAGE;
	page_stype_map[P_NODMA] = PAD_PAGE;
	ENABLE();

	/*
	 * Set the new mode.
	 */
	rdma_mode = RDMA_SMALL;

	/*
	 * Now, tell KMA to start using the PAD_PAGEs.
	 */
	kma_switch_small(is_mini);
}

/*
 * void
 * rdma_pool_init(void)
 *	Allocate private pool of DMA_PAGE pages for pageout (if needed).
 *
 * Calling/Exit State:
 *	Must be called after page_get() and mem_resv(...) are available.
 *
 *	Only called during system initialization, so it's single-threaded.
 */
void
rdma_pool_init(void)
{
	uint_t maxiopages;
	page_t *pp;
	mresvtyp_t mtype = M_DMA;

	if (rdma_mode == RDMA_DISABLED) {
		if (!physreq_prep(&rdma_dflt_preq, KM_NOSLEEP))
			goto nomem;
		return;
	} else if (rdma_mode == RDMA_MICRO) {
		mtype = M_KERNEL_ALLOC;
	}

	maxiopages = btopr(MAXBIOSIZE - 1) + 1;

	if (!mem_resv(maxiopages, mtype) ||
	    (rdma_private_pages = page_get(ptob(maxiopages), P_DMA | NOSLEEP))
			== NULL) {
nomem:
		/*
		 *+ The system has insufficient memory to allocate a
		 *+ private pool of DMA-able pages for the pageout daemon.
		 *+ The system needs to be (re)tuned, by adjusting one or
		 *+ more of the tunables, or more memory needs to be added.
		 */
		cmn_err(CE_PANIC, "not enough memory for rdma_pool_init");
		/* NOTREACHED */
	}

	/* Mark the private pages so we return them to the pool after use. */
	pp = rdma_private_pages;
	do {
		RDMA_PRIVATE(pp)->rp_pageout = B_TRUE;
	} while ((pp = pp->p_next) != rdma_private_pages);

	LOCK_INIT(&rdma_page_lock, VM_HIER_BASE, PLHI, &rdma_page_lkinfo,
		  KM_NOSLEEP);
	SV_INIT(&rdma_page_sv);

	/*
	 * Complete initialization of rdma_dflt_bcb/rdma_dflt_preq.
	 * Set phys_dmasize to one less than the number of bits in a
	 * physical address, thus forcing a DMA-ability requirement
	 * (RDMA_REQUIRED).
	 */
	rdma_dflt_preq.phys_dmasize = (NBBY * sizeof(paddr_t)) - 1;
	if (!physreq_prep(&rdma_dflt_preq, KM_NOSLEEP))
		goto nomem;
}


/*
 * void
 * rdma_fix_bswtbl(int major)
 *	Fix up a bdevsw entry, if necessary for Restricted-DMA.
 *
 * Calling/Exit State:
 *	major is a block device major number; that device's bdevsw entry
 *	will be modified to insert rdma_strategy() before the current
 *	strategy routine, if Restricted-DMA is enabled and it's a D_DMA
 *	device.
 *
 *	Caller guarantees exclusive access to the indicated bdevsw entry.
 */
void
rdma_fix_bswtbl(int major)
{
	ASSERT(rdma_mode != RDMA_DISABLED);

	if (bdevsw[major].d_strategy == nodev) {
#ifdef DEBUG
		rdma_bdevsw[major].d_strategy = rdma_stub_strategy;
#endif
		return;
	}

	ASSERT(bdevsw[major].d_flag != NULL);

	/* If it's not a DMA driver, we don't have to do anything. */
	if (!(*bdevsw[major].d_flag & D_DMA)) {
#ifdef DEBUG
		rdma_bdevsw[major].d_strategy = rdma_stub_strategy;
#endif
		return;
	}

	rdma_bdevsw[major].d_strategy = bdevsw[major].d_strategy;
	bdevsw[major].d_strategy = rdma_strategy;
}

/*
 * void
 * rdma_fix_swtbls(void)
 *	Fix up bdevsw entries, if necessary for Restricted-DMA.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single-threaded.
 *	Allocates the rdma_bdevsw shadow table and calls rdma_fix_bswtbl()
 *	for every configured major number.
 */
void
rdma_fix_swtbls(void)
{
	int major;

	ASSERT(rdma_mode != RDMA_DISABLED);

	rdma_bdevsw = (struct rdma_bdevsw *)
		kmem_zalloc(bdevswsz * sizeof(struct rdma_bdevsw), KM_NOSLEEP);
	if (rdma_bdevsw == NULL) {
		/*
		 *+ Boot-time allocation of data structures
		 *+ for support of Restricted DMA failed.
		 *+ This probably indicates that the system is
		 *+ configured with too little physical memory.
		 */
		cmn_err(CE_PANIC, "rdma_fix_swtbls: not enough memory");
	}

	for (major = bdevcnt; major-- != 0;)
		rdma_fix_bswtbl(major);
}


boolean_t
rdma_must_copy(const buf_t *bp, const physreq_t *preqp)
{
	page_t *pp;
	vaddr_t addr;
	paddr_t userpage;
	uint_t numpages;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(rdma_mode != RDMA_DISABLED);
	ASSERT(RDMA_REQUIREMENT(preqp) == RDMA_REQUIRED);

	if ((bp->b_flags & (B_PAGEIO|B_REMAPPED)) &&
	    !(bp->b_flags & B_WASPHYS)) {
		pp = bp->b_pages;
		numpages = bp->b_numpages;
		while (numpages-- != 0) {
			ASSERT(VALID_PP(pp));
			if (!DMA_PP(pp))
				return B_TRUE;
			pp = pp->p_next;
		}
		return B_FALSE;
	}

	for (addr = ((vaddr_t)bp->b_un.b_addr & PAGEMASK);
	     addr < (vaddr_t)bp->b_un.b_addr + bp->b_bcount; addr += PAGESIZE) {
		userpage = vtop((caddr_t)addr, bp->b_proc);
		if (!DMA_BYTE(userpage))
			return B_TRUE;
	}
	return B_FALSE;
}


/*
 * STATIC void
 * rdma_iodone(buf_t *bp)
 *	Buffer iodone routine used by rdma_strategy().
 *
 * Calling/Exit State:
 *	Called when I/O has completed on the given buffer, which was created
 *	by rdma_strategy().
 *
 * Remarks:
 *	Since ppcopyrange() cannot be called from interrupt level (and we
 *	wouldn't want to spend that much time at interrupt level, anyway),
 *	we put off processing this I/O completion until we're back at base
 *	level, if we would need to do the copy (for B_READ).
 */
STATIC void
rdma_iodone(buf_t *bp)
{
	page_t *pp, *nextpp, *origpp;
	uint_t off, count;
	boolean_t any_private = B_FALSE;
	pl_t pl;

	ASSERT(bp->b_flags & (B_PAGEIO|B_REMAPPED));
	ASSERT(!(bp->b_flags & B_DONE));
	ASSERT(bp->b_flags & B_KERNBUF);
	ASSERT(bp->b_misc != NULL);

	if ((bp->b_flags & B_READ) && servicing_interrupt()) {
		/*
		 * Don't handle reads at interrupt time, since we can't
		 * do the data copy at interrupt level.
		 */
		bdelaydone(bp);
		return;
	}

	/* Put back saved b_iodone/b_misc */
	bp->b_iodone = ((struct rdma_chain *)bp->b_misc)->rdc_iodone;
	bp->b_misc = ((struct rdma_chain *)bp->b_misc)->rdc_chain;

	/*
	 * Find the pages we substituted, and put the originals back.
	 */
	pp = bp->b_pages;
	off = ((uint_t)bp->b_un.b_addr & PAGEOFFSET);
		/*
		 * Note: when we compute the offset, we have to mask out
		 * the high order bits, in case the driver remapped the
		 * buffer with bp_mapin(), so that b_addr now holds the
		 * virtual address, not just the page offset.
		 */
	for (count = 0; count < bp->b_bufsize; count += PAGESIZE - off, off = 0,
					       pp = nextpp) {
		nextpp = pp->p_next;
		/*
		 * Substituted pages were marked as vnode == NULL, offset != 0.
		 */
		if (pp->p_vnode || pp->p_offset == 0)
			continue;
		origpp = (page_t *)pp->p_offset;
		/*
		 * Unlink the substituted page from the b_pages list, and
		 * link the original one back in its place.
		 */
		if (pp != nextpp) {
			pp->p_prev->p_next = nextpp->p_prev = origpp;
			origpp->p_prev = pp->p_prev;
			origpp->p_next = nextpp;
			pp->p_prev = pp->p_next = pp;
		}
		if (bp->b_pages == pp)
			bp->b_pages = origpp;
		/*
		 * If we're reading, copy the data now.
		 */
		if (bp->b_flags & B_READ) {
			uint_t	len;

			if (off + (len = bp->b_bufsize - count) > PAGESIZE)
				len = PAGESIZE - off;
			ppcopyrange(pp, origpp, off, len);
		}
		/*
		 * Free the substitution page.
		 */
		pp->p_offset = 0;
		if (RDMA_PRIVATE(pp)->rp_pageout) {
			/* pageout private page; put back into pool */
			pl = LOCK(&rdma_page_lock, PLHI);
			page_add(&rdma_private_pages, pp);
			UNLOCK(&rdma_page_lock, pl);
			any_private = B_TRUE;
		} else
			page_unlock(pp);
	}

	if (any_private)
		SV_BROADCAST(&rdma_page_sv, 0);

	/* Now do regular iodone processing */
	biodone(bp);
}


void
rdma_substitute_pages(buf_t *bp, const physreq_t *preqp)
{
	page_t *pp, *nextpp, *dmapp;
	uint_t off, count;
	uint_t pageget_flags;
	struct rdma_chain *rdcp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(rdma_mode != RDMA_DISABLED);
	ASSERT(RDMA_REQUIREMENT(preqp) == RDMA_REQUIRED);
	ASSERT(!(bp->b_flags & B_PHYS));
	ASSERT(bp->b_bufsize == bp->b_bcount);

	/*
	 * Check each page on the b_pages list to see if it's DMAable.
	 * If not, substitute a DMAable page and mark it so we put it back
	 * when we're done.
	 */
	dmapp = NULL;
	pp = bp->b_pages;
	off = (uint_t)bp->b_un.b_addr;
	pageget_flags = (IS_PAGEOUT() ? NOSLEEP : SLEEP) | P_DMA;
	for (count = 0; count < bp->b_bufsize; count += PAGESIZE - off, off = 0,
					       pp = nextpp) {
		nextpp = pp->p_next;
		if (DMA_PP(pp))
			continue;
		dmapp = page_get(PAGESIZE, pageget_flags);
		if (dmapp == NULL) {
			ASSERT(IS_PAGEOUT());
			/* Use pageout private pool. */
			(void) LOCK(&rdma_page_lock, PLHI);
			while ((dmapp = rdma_private_pages) == NULL) {
				SV_WAIT(&rdma_page_sv, PRIMEM, &rdma_page_lock);
				(void) LOCK(&rdma_page_lock, PLHI);
			}
			page_sub(&rdma_private_pages, dmapp);
			UNLOCK(&rdma_page_lock, PLBASE);
		} else
			RDMA_PRIVATE(dmapp)->rp_pageout = B_FALSE;
		ASSERT(dmapp->p_prev == dmapp);
		ASSERT(dmapp->p_next == dmapp);
		ASSERT(DMA_PP(dmapp));
		/*
		 * Unlink the original page from the b_pages list, and
		 * link the new one in its place.
		 */
		if (pp != nextpp) {
			pp->p_prev->p_next = nextpp->p_prev = dmapp;
			dmapp->p_prev = pp->p_prev;
			dmapp->p_next = nextpp;
			pp->p_prev = pp->p_next = pp;
		}
		if (bp->b_pages == pp)
			bp->b_pages = dmapp;
		/*
		 * If we're writing, copy the data now.
		 */
		if (!(bp->b_flags & B_READ)) {
			uint_t	len;

			if (off + (len = bp->b_bufsize - count) > PAGESIZE)
				len = PAGESIZE - off;
			ppcopyrange(pp, dmapp, off, len);
		}
		/*
		 * We need to mark the substituted page in such a way
		 * that we can tell it's been substituted.  We know it has
		 * no identity, so p_vnode == NULL.  In that case, p_offset
		 * is not used, so we'll use it; non-zero will indicate a
		 * substituted page.  We also make p_offset serve double duty
		 * as the back pointer to the original page.
		 */
		ASSERT(dmapp->p_vnode == NULL);
		ASSERT(dmapp->p_offset == 0);
		*(page_t **)&dmapp->p_offset = pp;
	}

	ASSERT(dmapp != NULL);

	/*
	 * We will have to put everything back to normal after the I/O
	 * completes.  We do this with a custom iodone routine.
	 *
	 * We need to save the original b_iodone field before
	 * setting it to our own iodone routine.  We do this by
	 * linking an rdma_chain struct onto the b_misc chain.
	 * Instead of kmem_alloc'ing this structure, we stuff it
	 * into the private data of one of the substituted pages.
	 */
	rdcp = &RDMA_PRIVATE(dmapp)->rp_chain;
	rdcp->rdc_iodone = bp->b_iodone;
	rdcp->rdc_chain = bp->b_misc;
	bp->b_misc = rdcp;
	bp->b_iodone = rdma_iodone;
}


/*
 * STATIC int
 * rdma_strategy(buf_t *bp)
 *	Restricted-DMA strategy routine.
 *
 * Calling/Exit State:
 *	Called with the buffer locked; remains the same on exit.
 *
 * Description:
 *	Intercepts strategy calls to DMA drivers when Restricted-DMA is
 *	enabled.  Any part of the buffer which is non-DMA-able must be
 *	replaced with DMA-able memory.
 */
STATIC int
rdma_strategy(buf_t *bp)
{
	major_t index;
	int (*strat)();

	ASSERT(rdma_mode != RDMA_DISABLED);
	ASSERT(!(bp->b_flags & B_PHYS));

	index = getmajor(bp->b_edev);		/* index back into bdevsw[] */

	ASSERT(bdevsw[index].d_strategy == rdma_strategy);

	strat = rdma_bdevsw[index].d_strategy; /* The actual strategy routine */

	if (rdma_must_copy(bp, &rdma_dflt_preq))
		rdma_substitute_pages(bp, &rdma_dflt_preq);

	return (*strat)(bp);
}

#ifdef DEBUG
/*
 * STATIC int
 * rdma_stub_strategy(buf_t *bp)
 *	Dummy strategy routine.
 *
 * Calling/Exit State:
 *	Should never be invoked.
 */
STATIC int
rdma_stub_strategy(buf_t *bp)
{
	cmn_err(CE_PANIC,"rdma_stub_strategy: bp %x edev %x\n", bp, bp->b_edev);
	/* NOTREACHED */
}
#endif

#endif /* NO_RDMA */
