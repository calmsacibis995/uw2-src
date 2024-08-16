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

#ident	"@(#)kern:mem/vm_sysinit.c	1.43"
#ident	"$Header: $"

/*
 * VM - system initialization.
 */

#include <mem/anon.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pageidhash.h>
#include <mem/pvn.h>
#include <mem/seg_kmem.h>
#include <mem/seg_kvn.h>
#include <mem/seg_map.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <mem/zbm.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern uint_t get_pagepool_size(uint_t *nchunkp);
extern void pagepool_init(page_t *pp, int pgarraysize);
extern void rmem_init(void);
extern void sched_init(void);
extern size_t mod_obj_size;
extern void ublock_calloc(void);



STATIC void carve_kvspace(uint_t);

#ifdef DEBUG
       void	 print_carve_kspace(void);
#endif /* DEBUG */


/*
 * The following are kernel configuration parameters to request
 * the size of the kernel virtual space managed by each of the
 * kernel segment managers.
 *
 * See carve_kvspace() for a discussion of how these are used.
 */

extern ulong_t segkmem_bytes;
extern ulong_t segkmem_percent;

extern ulong_t segmap_bytes;
extern ulong_t segmap_percent;

extern ulong_t segkvn_bytes;
extern ulong_t segkvn_percent;

/*
 * Starting kernel virtual address and size in bytes of each
 * kernel address space segment.  Calculated below.
 */

vaddr_t segkmem_addr;
ulong_t segkmem_size;
vaddr_t segmap_addr;
ulong_t segmap_size;
vaddr_t segkvn_addr;
ulong_t segkvn_size;

ulong_t unused_kvsize;	/* leftover kernel virtual space after the above */

LKINFO_DECL(kas_lkinfo, "MEM: kas_slplck: kas sleep lock", 0);

/*
 * Calculation of nominal kernel segment size.
 */
#define seg_nominal_size(bytes, totalpages, percent)	\
	ptob(btopr(bytes) + ((totalpages) * percent) / 100)

/*
 * void
 * kvm_init(void)
 *
 *	Initialize virtual memory system.
 *
 * Calling/Exit State:
 *
 *	On entry, calloc() is still functioning.
 *	On exit, calloc() has stopped functioning and kmem_alloc()
 *	is now functioning.
 *
 * Description:
 *
 *	Initialize the kernel address space with its segment managers.
 *	Initialize the page pool.  Initialize and enable kmem_alloc().
 */

void
kvm_init(void)
{
	uint_t totalpages, totalchunks;
	page_t *pp;
#ifdef DEBUG
	extern uint_t pagepool_size;
#endif

	/*
	 * Kernel segment are not permitted to extend to the highest
	 * machine address, so that address expresssion of the form
	 * seg->s_base + seg->s_size do not overflow for the kernel address
	 * space.
	 */
#ifndef lint
	ASSERT(KVAVEND != 0);
#endif

	/*
	 * Find total pages left in memory not yet palloc()'d.
	 *
	 * Loop through possible memory controller clicks, counting the
	 * clicks which are present and which haven't been palloc()'d.
	 *
	 * This only calculates an upper bound on the number of pages
	 * that the page pool will need to manage, since subsequent
	 * palloc()'s could reduce this before we initialize the page pool.
	 */

	totalpages = get_pagepool_size(&totalchunks);
#ifdef DEBUG
	pagepool_size = totalchunks;
#endif

	/*
	 * Find size of page hash structures
	 */

	page_hashsz = pageid_compute_hashsz(totalpages);

	/*
	 * Allocate pagepool and page structures and page hash array.
	 */

	pagepool = calloc(totalchunks * sizeof(struct pp_chunk));
	page_cs_table = calloc((4 + totalchunks) * sizeof(page_t *));
#ifdef DEBUG
	page_cs_table_size = 4 + totalchunks;
#endif
	pp = calloc(totalpages * sizeof(page_t));
	page_hash = calloc(page_hashsz * sizeof(page_t *));
	pvn_memresv_hash = calloc(page_hashsz * sizeof(pvn_memresv_entry *));

	/*
	 * Allocate segment structures for all kernel segments.
	 */

	segkmap = calloc(sizeof(struct seg));
	kpgseg = calloc(sizeof(struct seg));
	segkvn = calloc(sizeof(struct seg));

	/*
	 * We need to call certain segment managers while calloc()
	 * is still enabled in order to allow them to calloc()
	 * their data structures.  However, we also need to
	 * supply (some of) them with an indication of how much
	 * kernel virtual address space they'll be managing, and
	 * we won't know this for sure until we disable further
	 * calloc().  (Since calloc() uses up additional kernel
	 * virtual address space.)
	 *
	 * So we calculate remaining kernel virtual space now
	 * (before disabling calloc()) and use this as an
	 * upper bound on the amount of virtual space we'll
	 * give to each segment manager.  Later we disable
	 * calloc() and recalculate the actual amount.
	 */

	carve_kvspace(totalpages);

	/*
	 * At this point we can call segment manager calloc-time init
	 * functions, which need a preliminary, upperbound indication
	 * on the size of kernel virtual space they will be managing.
	 *
	 * --------------------------------------------------------
	 * At this point, calloc() and hat_statpt_alloc() are still
	 * functioning; kmem_[z]alloc() is not.
	 * --------------------------------------------------------
	 */

	segkmap->s_size = segmap_size;
	segmap_calloc(segkmap);
	kpgseg->s_size = segkmem_size;
	kpg_calloc();
	segkvn->s_size = segkvn_size;
	segkvn_calloc();
	kma_calloc();
	ublock_calloc();

	/*
	 * init hat visible mappings
	 *
	 *	The mapping must be large enough to accomodate both
	 *	seg_kvn and seg_map, plus any possible "alignment hole"
	 *	between them.
	 */
	hat_kas_mapping_init(segkvn_size + segmap_size +
			     ptob(btopr(MAXBSIZE) - 1));

	/*
	 * Just before we disable calloc(), carve up the kernel address space
	 * again.  These will be the final assignments.
	 */

	carve_kvspace(totalpages);

	hat_static_stopcalloc();	/* disable calloc() */

#ifdef DEBUG
	print_carve_kspace();
#endif

	/*
	 * -------------------------------------------------------
	 * At this point, hat_statpt_alloc() is still functioning;
	 * page_get(), calloc() and kmem_[z]alloc() are not.
	 * -------------------------------------------------------
	 */
	RWSLEEP_INIT(&(kas.a_rwslplck), 0, &kas_lkinfo, KM_NOSLEEP);
	(void) seg_attach(&kas, segmap_addr, segmap_size, segkmap);
	segmap_create(segkmap);
	(void) seg_attach(&kas, segkmem_addr, segkmem_size, kpgseg);
	segkmem_create(kpgseg);
	(void) seg_attach(&kas, segkvn_addr, segkvn_size, segkvn);
	segkvn_create(segkvn);

	/* Set the base address for kernel visible mappings. */
	hat_vis_ptaddr_init(segkvn_addr);

	hat_static_stoppalloc();	/* disable palloc() */

	/*
	 * -------------------------------------------------------
	 * At this point, none of the allocators are functioning.
	 * -------------------------------------------------------
	 */

	/*
	 * Initialize the page pool.
	 *
	 * Determine which memory controller clicks are present,
	 * group them into physically contiguous runs, and
	 * allocate page structures for each such run.
	 */
	pagepool_init(pp, totalpages);

	page_init();	/* finalize page struct allocation */

	/* Initialize memory resource reservation accounting. */
	rmem_init();

	/* Remaining KPG and KMA initialization. */
	kpg_init();
	kma_init();

	/*
	 * -------------------------------------------------------
	 * At this point, kmem_[z]alloc() is functioning;
	 * calloc() and hat_statpt_alloc() are not.
	 * -------------------------------------------------------
	 */

	/*
	 * Initialize anonfs: create first anode based on freemem.
	 * Must be first KMA user.
	 *
	 * Note we check freemem for sufficient memory pages for
	 * tune.t_minamem (for file pages), minpagefree (for pager to clean
	 * pages), and tune.t_kmem_resv (for kmem).
	 *
	 * ###laz: need to do something better here. Can we sanely readjust
	 * the counters or make a better `fail-safe' check than this?
	 */

	if (freemem <= tune.t_minamem + tune.t_kmem_resv) {
		/*
		 *+ The system has insufficient memory to boot because
		 *+ freemem is less than or equal to the the memory space
		 *+ required by the sum of the tunables MINPAGEFREE, MINAMEM,
		 *+ and KMEM_RESV. The system needs to be (re)tuned, by
		 *+ adjusting one or more of the tunables, or more memory
		 *+ needs to be added.
		 */
		cmn_err(CE_PANIC, "freemem insufficient");
		/* NOTREACHED */
	}

	/*
	 * anon will make available:
	 *	(max_freemem() - tune.t_minamem) for KMA use
	 *	(max_freemem() - tune.t_minamem - tune.t_kmem_resv)
	 *		for normal use
	 *	(max_freemem() - tune.t_minamem - tune.t_kmem_resv -
	 *		pages_ukma_maximum) for discretionary kma use
	 *	(max_freemem() - tune.t_minamem - tune.t_kmem_resv -
	 *		pages_pp_maximum) for user lockdown
	 *
	 * The last two parameters to anon_conf() are part of an
	 * optimization in which the otherwise unused portion of the pages
	 * table is given to anon (for use as anon sees fit).
	 */
	anon_conf(max_freemem() - tune.t_minamem, tune.t_kmem_resv,
		  pages_pp_maximum, pages_dkma_maximum, epages,
		  (totalpages - (epages - pages)) * sizeof(page_t));

	/* Obtain reservations for the kernel's symbol table */
	if (!mem_resv(btopr(mod_obj_size), M_BOTH)) {
		/*
		 *+ The system has insufficient memory to configure in its
		 *+ own symbol table during the boot. The system needs to be
		 *+ (re)tuned, by adjusting one or more of the tunables, or
		 *+ more memory needs to be added.
		 */
		cmn_err(CE_PANIC, "mod obj reservation failed");
		/* NOTREACHED */
	}

	/* swap space management initialization */
	swap_init();

#ifndef NO_RDMA
	/* Allocate RDMA local pool, if necessary. */
	rdma_pool_init();
#endif

	/* Final initialization for kernel segment managers. */
	segmap_init(segkmap);
	segkvn_init(segkvn);

	hat_init();	/* dynamic HAT inititialization */
	pvn_init();	/* paged vnode routine inititialization */
	pageout_init(); /* set up pageout() */
	sched_init();	/* swapper parameters */
}

#define segsizedecr(bytes, minbytes) \
		((btopr(bytes) > btopr(minbytes)) ? PAGESIZE : 0)
/*
 * STATIC void
 * carve_kvspace(uint_t totalpages)
 *
 *	Carve up remaining kernel virtual address space among the
 *	various kernel segment managers.
 *
 * Calling/Exit State:
 *
 *	On entry, the following global values must already be set:
 *
 *	segkmem_bytes	- requested size of segkmem's virtual space
 *			  in bytes, or zero.
 *	segkmem_percent	- requested size of segkmem's virtual space
 *			  as a percentage (* 100) of available
 *			  kernel virtual space, or zero.
 *
 *	Similarly, the following also must already be set:
 *
 *		segmap_bytes, segmap_percent,
 *		segkvn_bytes, segkvn_percent
 *
 *	On return, the following global values are set:
 *
 *	segkmem_addr	- starting kernel virtual address of segkmem
 *	segmap_addr	- starting kernel virtual address of segmap
 *	segkvn_addr	- starting kernel virtual address of segkvn
 *
 *	segkmem_size	- size in bytes of segkmem's virtual space
 *	kpg_cellsize	- size of a segkmem cell
 *	segkvn_cellsize	- size of a segkvn cell
 *	segmap_size	- size in bytes of segmap's virtual space
 *	segkvn_size	- size in bytes of segkvn's virtual space
 *
 *	unused_kvsize	- size in bytes of unused virtual space
 *
 * Description:
 *
 *	The basic algorithm is:
 *
 *	For each segment manager (segxxx):
 *		segment-size = segxxx_bytes + physical-memory * segxxx_percent
 *
 *	If the total kernel virtual space consumed by all segment
 *	managers exceeds the available space, reduce each segment
 *	managers space by approximately equal amounts until they
 *	all fit.
 *
 *	If some space cannot be used due to alignment considerations,
 *	then try to shuffle space to a segment which can use it.
 *
 *	Finally, guarantee that the values computed are non-increasing
 *	with each successive invocation.
 */
	
STATIC void
carve_kvspace(uint_t totalpages)
{
	vaddr_t lo_kvaddr, hi_kvaddr;
	ulong_t kvsize, desired_kvsize, i, maxbsize;
	ulong_t pad;
	static int invocations = 0;
	static ulong_t last_kmemsize, last_msize, last_kvnsize;

	/*
	 * The remaining kernel virtual space is bounded by
	 * hat_static_nextvaddr() on the low end and by
	 * KVAVEND on the high end.
	 */

	lo_kvaddr = ptob(btopr(hat_static_nextvaddr()));
	hi_kvaddr = KVAVEND;
	kvsize = hi_kvaddr - lo_kvaddr;

	/*
	 * Calculate desired byte sizes of each segment based on configuration
	 * parameters and the available physical memory.
	 */

	segkmem_size = seg_nominal_size(segkmem_bytes, totalpages,
					segkmem_percent);
	segmap_size = seg_nominal_size(segmap_bytes, totalpages,
				       segmap_percent);
	segkvn_size = seg_nominal_size(segkvn_bytes, totalpages,
					segkvn_percent);

	/*
	 * If the desired total kernel virtual space size exceeds the
	 * available size, reduce the desired sizes proportionally (subject
	 * to the minimum size requirement of each segment).
	 */

 	desired_kvsize = segkmem_size + segmap_size + segkvn_size;

	if (ptob((btopr(segkmem_bytes) + btopr(segmap_bytes) + 
			btopr(segkvn_bytes))) > kvsize) {
		/*
		 *+ Insufficient kernel virtual space was left
		 *+ to accommodate the kernel segment drivers. 
		 *+ This may indicate that required minimum virtual
		 *+ space for kernel segments is larger than can
		 *+ be accommodated. The minimum required virtual
		 *+ space is set by following tunable parameters:
		 *+ SEGKMEM_BYTES, SEGKVN_BYTES, and SEGMAP_BYTES. 
		 *+ Adjust as needed, and then rebuild the kernel.
		 */
		cmn_err(CE_PANIC, "carve_kvspace: shortfall");
		/* NOTREACHED */
	}

	while (desired_kvsize > kvsize) {

	 	i = ((segkmem_size * kvsize) / desired_kvsize);
		segkmem_size = ptob(btopr(MAX(i, segkmem_size)));

	 	i = ((segmap_size * kvsize) / desired_kvsize);
		segmap_size = ptob(btopr(MAX(i, segmap_size)));

	 	i = ((segkvn_size * kvsize) / desired_kvsize);
		segkvn_size = ptob(btopr(MAX(i, segkvn_size)));

		i = segkmem_size + segmap_size + segkvn_size;

		if (i == desired_kvsize) {
			/* 
			 * we must have a rounding problem here, so 
			 * must force small size reductions equally 
			 */
			segkmem_size -= segsizedecr(segkmem_size, 
						segkmem_bytes);
			segmap_size  -= segsizedecr(segmap_size, 
						segmap_bytes);
			segkvn_size  -= segsizedecr(segkvn_size, 
						segkvn_bytes);
		}
 		desired_kvsize = segkmem_size + segmap_size + segkvn_size;
	}

	/*
	 * Compute kpg_cellsize and segkvn_cellsize.
	 * N.B. We cannot change ZBM cell sizes on the second pass through
	 * carve_kvspace.
	 */
	if (++invocations == 1) {
		kpg_cellsize = zbm_cell_size(segkmem_size);
		segkvn_cellsize = zbm_cell_size(segkvn_size);
	}

	/*
	 * segkmem can only use a multiple of kpg_cellsize pages.
	 * Any space above that can be given away to segkvn.
	 */
	i = segkmem_size % ptob(kpg_cellsize);
	segkmem_size -= i;
 	segkvn_size += i;

	/*
	 * segkmem_size must be non-increasing from last invocation of
 	 * carve_kvspace.  Any space above that can be given away to segkvn.
	 */
	if (invocations > 1 && segkmem_size > last_kmemsize) {
		segkvn_size += segkmem_size - last_kmemsize;
		segkmem_size = last_kmemsize;
	}

	/*
 	 * segkvn can only use a multiple of segkvn_cellsize pages.
 	 * Any space above that can be given away to segmap.
 	 */
 	i = segkvn_size % ptob(segkvn_cellsize);
 	segkvn_size -= i;
 	segmap_size += i;
 
 	/*
 	 * segkvn_size must be non-increasing from last invocation of
 	 * carve_kvspace.  Any space above that can be given away to
 	 * segmap.
 	 */
 	if (invocations > 1 && segkvn_size > last_kvnsize) {
 		segmap_size += segkvn_size - last_kvnsize;
 		segkvn_size = last_kvnsize;
 	}
 
 	/*
	 * segmap must start at a MAXBSIZE aligned address.
	 * Any space left below must be discarded.
	 */

	maxbsize = ptob(btopr(MAXBSIZE));
 	pad = (lo_kvaddr + segkmem_size + segkvn_size) % maxbsize;
 	if (pad != 0) {
 		pad = maxbsize - pad;
 		segmap_size -= pad;
	}

	/*
	 * segmap can only use multiples of MAXBSIZE.
	 * Any space above that must be discarded;
	 */

	i = segmap_size % maxbsize;
	segmap_size -= i;

	/*
	 * segmap_size must be non-increasing from last invocation of
	 * carve_kvspace.  Any space above that must be discarded.
	 */
	if (invocations > 1 && segmap_size > last_msize) {
		segmap_size = last_msize;
	}

	/*
	 * All sizes are integral multiples of pages.
	 */

	ASSERT(segkmem_size % PAGESIZE == 0);
	ASSERT(segkvn_size  % PAGESIZE == 0);
	ASSERT(segmap_size % PAGESIZE == 0);

	/*
	 * Calculate remaining unused kernel virtual space.
	 */

 	i = segkmem_size + segkvn_size + segmap_size;
	ASSERT(i <= kvsize);
	unused_kvsize = kvsize - i;

	/*
	 * Calculate starting address of each segment.
	 */

	segkmem_addr = lo_kvaddr;
	segkvn_addr = segkmem_addr + segkmem_size;
	segmap_addr = segkvn_addr + segkvn_size + pad;

 	ASSERT(segkmem_addr < segkvn_addr);
 	ASSERT(segkvn_addr < segmap_addr);
	ASSERT(segmap_size <= hi_kvaddr - segmap_addr);

	/*
	 * save sizes for second pass through
	 */
	last_kmemsize = segkmem_size;
	last_kvnsize = segkvn_size;
	last_msize = segmap_size;
}

/*
 * void
 * kvm_kmadv(void)
 *	Call kmem_advise() for VM data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
kvm_kmadv(void)
{
	extern void hat_kmadv(void);
	extern void segkvn_kmadv(void);
	extern void segvn_kmadv(void);
	extern void ublock_kmadv(void);

	kmem_advise(sizeof(struct as));
	kmem_advise(sizeof(struct seg));
	hat_kmadv();
	segkvn_kmadv();
	segvn_kmadv();
	ublock_kmadv();
}

/*
 * void
 * kvm_postroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
kvm_postroot(void)
{
	extern void poolrefresh(void *);
	extern void pageout(void *);
	extern void sched_swapin(void *);
	extern void sched_sleeper_search(void *);
	extern void sched_elapsed_time_age(void *);
	extern void sched_force_unload(void *);
	extern void start_schedpaging(void);

	/* Spawn system daemons */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 poolrefresh, NULL);
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 pageout, NULL);
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 sched_swapin, NULL);
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 sched_sleeper_search, NULL);
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 sched_elapsed_time_age, NULL);
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 sched_force_unload, NULL);

	/* Kick off pageout periodic timeout */
	start_schedpaging();
}


#ifdef DEBUG

/*
 * void
 * print_carve_kspace(void)
 *
 * Calling/Exit State:
 *
 *	Print the value of kernel segment addresses & sizes for debugging.
 */

void
print_carve_kspace(void)
{
#define X(v)	cmn_err(CE_CONT, "^0x%x	"#v"\n", v)

	cmn_err(CE_CONT, "^\n");

	X(segkmem_addr);
	X(segkmem_size);
	X(segkvn_addr);
	X(segkvn_size);
	X(segmap_addr);
	X(segmap_size);
	cmn_err(CE_CONT, "^\n");

	X(unused_kvsize);
}

#endif /* DEBUG */
