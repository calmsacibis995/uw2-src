/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/seg_kvn.c	1.30"
#ident	"$Header: $"

/*
 * MS (Segment Management) - seg_kvn routines
 *
 *	seg_kvn provides mappings of vnodes into the kernel virtual
 *	address space. Users of seg_kvn use the segkvn_vp_mapin() and
 *	segkvn_mapout() primitives to create and destroy the vnode
 *	mappings, respectively.
 *
 *	Users of seg_kvn are required either to (i) lock the mapping into
 *	memory, or (ii) to protect all accesses to seg_kvn managed memory
 *	with BOTH a seg_kvn lock [obtained using segkvn_lock()] and a
 *	fault catcher [obtained using CATCH_FAULTS(CATCH_SEGKVN_FAULT) and
 *	END_CATCH() macros]. More specifically, the locking paradigms are:
 *
 * 		SEGKVN_FUTURE_LOCK
 * 
 *			The requirement for locking arises from delayed
 *			TLB shootdown processing associated with the aging
 *			of seg_kvn memory.
 *
 * 			The SEGKVN_FUTURE_LOCK is designed to do the
 *			minimal work to address this requirement.
 *			It locks a seg_kvn segment against aging.
 *			Thus, it does not actually bring any pages
 *			into memory, or otherwise allocate REAL memory.
 *			Rather, it (i) locks the segment against aging,
 *			and (ii) mem_resv(s) REAL memory.
 *
 *			The paradigm for using SEGKVN_FUTURE_LOCK is
 *			as follows:
 *
 *			fc = segkvn_lock(mapcookie, SEGKVN_FUTURE_LOCK);
 *			if (fc) {
 *				... process error ...
 *			}
 *			CATCH_FAULTS(CATCH_SEGKVN_FAULT) {
 *				... access seg_kvn memory ...
 *			}
 *			errno = END_CATCH();
 *			if (errno) {
 *				... process error ...
 *			}
 *			segkvn_unlock(mapcookie, SEGKVN_FUTURE_LOCK);
 *
 *		SEGKVN_MEM_LOCK
 *
 *			This implements a true memory lock, faulting all
 *			pages into memory. Thus, once a memory lock is
 *			obtained, it is not necessary to use CATCH_FAULTS.
 */

#include <fs/memfs/memfs.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_kvn.h>
#include <mem/zbm.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>

/*
 * The following is the correct mresvtyp_t for locking SEGKVN_DMA segments
 * in rdma_mode(s) RDMA_SMALL and RDMA_LARGE. It will be changed to
 * M_REAL should the system transition to RDMA_DISABLED or RDMA_MICRO.
 */
mresvtyp_t segkvn_mtype_dma = M_DMA;
#endif

/*
 * external data
 */

extern int avefree;
extern int verylowfree;
extern clock_t segkvn_age_time;

/*
 * Global data
 *
 * Two Level Address Space Structure:
 *
 *	Users of kas (e.g. the kernel page fault code) never lock it.
 *	Therefore, it is not possible for seg_kvn to attach/detach
 *	segments directly to/from kas during normal operation. Therefore,
 *	seg_kvn attaches exactly one segment (segkvn) to kas at
 *	initialization. Associated with segment segkvn is an address space
 *	(segkvn_as). seg_kvn attaches a segment to segkvn_as for each
 *	mapping which supports hardware faults. Thus, segkvn_as is used
 *	to translate from the virtual address to the mapping (kmp)
 *	structure when processing hardware faults.
 *
 *	Segkvn and the segkvn_as attached segments share the same
 *	seg_ops vector (segkvn_ops). However, only the segkvn_as attached
 *	segments have private data (in the form of struct kmp).
 */

struct seg *segkvn;				/* segment attached to kas */
int segkvn_cellsize;				/* cell size for ZBM */

/*
 * Private STATIC data
 */
STATIC struct as *segkvn_as;			/* private address space */
STATIC rwlock_t segkvn_as_mutex;		/* lock for segkvn_as */
STATIC struct kmp_hdr segkvn_aganchor;		/* age list anchor */
STATIC lock_t segkvn_mutex;			/* private data lock */
STATIC clock_t segkvn_normal_age_time;		/* normal aging time */
STATIC clock_t segkvn_fast_age_time;		/* fast aging time */
STATIC clock_t segkvn_desperate_age_time;	/* desperate aging time */
STATIC zbm_t segkvn_zbm;			/* virtual space alloc info */

#define SEGKVN_AGANCHOR		((struct kmp *)(&segkvn_aganchor))

#ifdef DEBUG
/*
 * Private debug data
 */
uint_t segkvn_naging;				/* length of the age list */
#endif	/* DEBUG */

/*
 * Private seg op routines.
 */
STATIC faultcode_t segkvn_fault(struct seg *, vaddr_t, uint_t,
				enum fault_type, enum seg_rw);
STATIC faultcode_t segkvn_kmp_fault(struct kmp *, vaddr_t, uint_t,
				    enum fault_type, enum seg_rw);
STATIC int segkvn_kluster(struct seg *, vaddr_t, int);
STATIC void segkvn_badop(void);
STATIC boolean_t segkvn_lazy_shootdown(struct seg *seg, vaddr_t addr);

STATIC struct seg_ops segkvn_ops = {
	(int (*)())segkvn_badop,	/* unmap */
	(void (*)())segkvn_badop,	/* free */
	segkvn_fault,
	(int (*)())segkvn_badop,	/* setprot */
	(int (*)())segkvn_badop,	/* checkprot */
	segkvn_kluster,
	(int (*)())segkvn_badop,	/* sync */
	(int (*)())segkvn_badop,	/* incore */
	(int (*)())segkvn_badop,	/* lockop */
	(int (*)())segkvn_badop,	/* dup */
	(void (*)())segkvn_badop,	/* childload */
	(int (*)())segkvn_badop,	/* getprot */
	(off_t (*)())segkvn_badop,	/* getoffset */
	(int (*)())segkvn_badop,	/* gettype */
	(int (*)())segkvn_badop,	/* getvp */
	(void (*)())segkvn_badop,	/* age */
	segkvn_lazy_shootdown,
	(int (*)())segkvn_badop		/* memory */
};

/*
 * other private routines
 */
STATIC void segkvn_adj_aging(struct kmp *, uint_t flag);
STATIC void segkvn_outofmem(void);

/*
 *+ RW spin lock to protect the segkvn_as segment chain
 */
STATIC LKINFO_DECL(segkvn_as_mutex_lkinfo, "MS:seg_kvn:segkvn_as_mutex", 0);

/*
 *+ Global seg_kvn mutex
*/
STATIC LKINFO_DECL(segkvn_mutex_lkinfo, "MS:seg_kvn:private data mutex", 0);

extern int mem_avail;
/*
 * void
 * segkvn_kmadv(void)
 *	Call kmem_advise() for segkvn data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
segkvn_kmadv(void)
{
	kmem_advise(sizeof(struct kmp));
}

/*
 * void
 * segkvn_calloc(void)
 *      calloc the segkvn_cell(s) and the bitmap
 *
 * Calling/Exit State:
 *      - called only once while calloc is still alive during
 *        system initialization
 *      - segkvn->s_size will have been set to an upper bound on the eventual
 *        segment size
 */

void
segkvn_calloc(void)
{
        zbm_calloc(&segkvn_zbm, segkvn->s_size, segkvn_cellsize);
}

/*
 * void
 * segkvn_create(struct seg *seg)
 *	Create kernel page tables for use by seg_kvn segments.
 *
 * Calling/Exit State:
 *	Called after calloc time (but while hat static page tables are still
 *	being allocated).  At this time we have the exact size and virtual
 *	address allocated to segkvn.
 */

void
segkvn_create(struct seg *seg)
{
	/*
	 * Allocate static page tables.
	 */
	hat_statpt_alloc(seg->s_base, seg->s_size);

	/*
	 * bind our ops into segkvn
	 */
	seg->s_ops = &segkvn_ops;
}

/*
 * void
 * segkvn_init(struct seg *seg)
 *	Final seg_kvn initialization.
 *
 * Calling/Exit State:
 *	Called from kvm_init after KMA enabled.
 */

/* ARGSUSED */
void
segkvn_init(struct seg *seg)
{
	/*
	 * allocate a private address space for managing seg_kvn space
	 */
	segkvn_as = as_alloc();

	/*
	 * initialize the locks
	 */
	RW_INIT(&segkvn_as_mutex, VM_SEGKVN_HIER, VM_SEGKVN_IPL,
		&segkvn_as_mutex_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&segkvn_mutex, VM_SEGKVN_HIER+1, VM_SEGKVN_IPL,
		  &segkvn_mutex_lkinfo, KM_NOSLEEP);

	/*
	 * initialize the aging list
	 */
	SEGKVN_AGANCHOR->kmp_next =
			SEGKVN_AGANCHOR->kmp_last = SEGKVN_AGANCHOR;

	/*
	 * pre-compute some aging times
	 */
	segkvn_normal_age_time = (segkvn_age_time * 2) / 5;
	segkvn_fast_age_time = (segkvn_normal_age_time * 2) / 5;
	segkvn_desperate_age_time = (segkvn_fast_age_time * 2) / 5;

	/*
	 * Initialize the zoned bit map.
	 */
	zbm_init(&segkvn_zbm, segkvn->s_base, segkvn->s_size, segkvn_outofmem);
}

/*
 * STATIC void
 * segkvn_outofmem(void)
 *	Routine for ZBM to call when it cannot allocate virtual space.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	There is nothing to do in this case.
 */
STATIC void
segkvn_outofmem(void)
{
}

/*
 * vaddr_t
 * segkvn_vp_mapin(size_t forelen, size_t midlen, size_t aftlen, vnode_t *vp,
 *                off_t off, uint_t flags, void **mapcookie)
 *      Create a mapping of vnode ``vp'', starting at offset ``off'',
 *      and of size ``midlen'' bytes into segkvn virtual space.
 *
 * Calling/Exit State:
 *      This function allocates ptob(btopr(forelen + midlen + aftlen))
 *      bytes of segkvn virtual space. forlen and aftlen are both required
 *      to be a multiple of PAGESIZE. midlen must be non-zero. If a
 *      non-zero value is specified for forelen, then forlen bytes of segkvn
 *      virtual space before the mapping of page <vp, off> will be left
 *      vacant for use by the caller. Similarly, a non-zero value of aftlen
 *      indicates that space is to be left vacant following the vnode
 *      mapping.
 *
 *      The address returned is the kernel virtual address at which page
 *      <vp, off> is mapped. The mapping of vnode vp continues up to, but
 *      excluding, page <vp, btop(btopr(off + midlen))>. The return value
 *	is NULL if segkvn_vp_mapin() fails.
 *
 *	The following flags are supported:
 *	
 *		SEGKVN_NOFAULT	Indicates that the user agrees to memory
 *				lock the mapping (using segkvn_lock(...,
 *				SEGKVN_MEM_LOCK) before accessing it through
 *				kernel virtual. Specifically, mappings
 *				created using the SEGKVN_NOFAULT flag do not
 *				support hardware faults.
 *
 *		SEGKVN_KLUSTER	Indicates that seg_kvn is to advise
 *				klustered getpage operations to the
 *				underlying file system even when the result
 *				of such klustering is to get pages
 *				outside the range managed by seg_kvn.
 *
 *		SEGKVN_DMA	If vp is NULL, then the memfs vnode to
 *				be created will allocate P_DMA pages.
 *
 *	If vp is NULL, then segkvn_vp_mapin() will create a vnode of size
 *	midlen+off using memfs_create_unnamed().
 *
 *	If vp is non-NULL, then a VN_HOLD() will be placed on the vnode.
 *
 *      An opaque cookie is return in location *cookie for use with the
 *      segkvn_vp_mapout(), segkvn_lock(), and segkvn_unlock()
 *      interfaces.
 *
 *      No spin LOCKs are held on entry to or exit from this function.
 *
 * Description:
 *      A user of forlen/aftlen space needs to note that segkvn virtual is
 *      backed by kernel visible translations.
 */

vaddr_t
segkvn_vp_mapin(size_t forelen, size_t midlen, size_t aftlen, vnode_t *vp,
               off_t off, uint_t flags, void **mapcookie)
{
	vaddr_t segbase;
	struct kmp *kmp;
	uint_t npages;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT((forelen & PAGEOFFSET) == 0);
	ASSERT(midlen != 0);
	ASSERT((aftlen & PAGEOFFSET) == 0);
	ASSERT((flags & ~(SEGKVN_NOFAULT|SEGKVN_KLUSTER|SEGKVN_DMA)) == 0);
	ASSERT(mapcookie);

	/*
	 * first, allocate an address range for this mapping
	 */
	npages = btopr(forelen + midlen + aftlen);
	if ((segbase = zbm_alloc(&segkvn_zbm, npages, KM_NOSLEEP)) == NULL)
		return ((vaddr_t)NULL);

	/*
	 * If the user has not passed in a vnode, then create a memfs vnode.
	 * Else if the user has passed in a vnode, then place a hold on it.
	 */
	if (vp == NULL) {
		vp = memfs_create_unnamed(off + midlen,
					  SEGKVN_DMA_FLAGS(flags));
		if (vp == NULL) {
			zbm_free(&segkvn_zbm, segbase, npages);
			return ((vaddr_t)NULL);
		}
	} else {
		VN_HOLD(vp);
	}

	/*
	 * Allocate and initialize a kmp structure
	 */
	kmp = (struct kmp *)kmem_alloc(sizeof(struct kmp), KM_SLEEP);
	kmp->kmp_seg.s_ops = &segkvn_ops;
	kmp->kmp_seg.s_data = kmp;
	kmp->kmp_base = segbase;
	kmp->kmp_npages = npages;
	kmp->kmp_vp = vp;
	kmp->kmp_off = off;
	kmp->kmp_nlocks = kmp->kmp_nmlocks = 0;
	kmp->kmp_flags = (flags & SEGKVN_KLUSTER) ? SEGKVN_KMP_KLUST : 0;
#ifndef NO_RDMA
	kmp->kmp_mtype = SEGKVN_MEM_TYPE(flags);
#endif
	SV_INIT(&kmp->kmp_sv);
#ifdef DEBUG
	/*
	 * zero out debug information
	 */
	bzero((void *)&kmp->kmp_debug, sizeof(struct kmp_debug));
#endif

	/*
	 * Create a segment and attach it to segkvn_as if the user
	 * so requests.
	 */
	if (flags & SEGKVN_NOFAULT) {
		kmp->kmp_seg.s_base = segbase + forelen;
		kmp->kmp_seg.s_size = ptob(npages) - forelen - aftlen;
		kmp->kmp_seg.s_as = NULL;
	} else {
		/*
		 * must write lock the seg_kvn address space
		 */
		(void) RW_WRLOCK_PLMIN(&segkvn_as_mutex);

		/*
		 * now attach the segment to the kernel address space
		 */
		if (seg_attach(segkvn_as, segbase + forelen,
			       ptob(npages) - forelen - aftlen,
			       &kmp->kmp_seg) < 0) {
			/*
			 *+ segkvn_vp_mapin failed to attach a newly allocated
			 *+ segment to segkvn_as. This indicates an internal
			 *+ error in the operating system. The basic integrity
			 *+ of the operating system may have been compromised
			 *+ by malfunctioning software or hardware. Operator
			 *+ action: unknown.
			 */
			cmn_err(CE_PANIC, "segkvn_vp_mapin");
		}

		/*
		 * Remember that this was the most recently touched kernel
		 * segment.
		 */
		segkvn_as->a_seglast = &kmp->kmp_seg;

		/*
		 * All done with the segkvn_as address space.
		 */
		RW_UNLOCK_PLMIN(&segkvn_as_mutex, PLBASE);
	}

	/*
	 * Return the kmp (as an opaque cookie) and the segment base
	 * address to the client.
	 */
	*mapcookie = (void *)kmp;
	return (kmp->kmp_seg.s_base);
}

/*
 * void
 * segkvn_mapout(void *mapcookie)
 *      Destroy a segkvn mapping previously created by segkvn_vp_mapin().
 *
 * Calling/Exit State:
 *      ``mapcookie'' is an opaque value previously returned by
 *	segkvn_vp_mapin().
 *
 *      This routine can block in only one way: within the VN_RELE() of the
 *      mapped vnode. If the caller has made private arrangements with the
 *      vnodes's file system to guarantee that VN_RELE() will not block,
 *      then this routine is non-blocking.
 *
 * Remarks:
 *      For some file systems, it might suffice to abort the mapped pages
 *      in order to guarantee that VN_RELE() will not block. This can be
 *      achieved through segkvn_unlock(), by specifying the SEGKVN_DISCARD
 *      flag.
 */

void
segkvn_mapout(void *mapcookie)
{
	struct kmp *kmp = mapcookie;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kmp != (struct kmp *)NULL);
	ASSERT(kmp->kmp_seg.s_ops == &segkvn_ops);

	if (kmp->kmp_nlocks != 0) {
		/*
		 *+ An attempt was made to mapout a locked seg_kvn mapping.
		 *+ This indicates that some software subsystem within the
		 *+ operating system does not obey the correct seg_kvn
		 *+ interface protocol.
		 */
		cmn_err(CE_PANIC, "segkvn_mapout: locked");
	}
	ASSERT(kmp->kmp_nmlocks == 0);

	/*
	 * Remove from the aging list and unload the translations, if
	 * required. However, we can defer the shootdown till we need to
	 * reuse some kernel virtual space. ZBM will take care of the
	 * shootdown for us at that time.
	 *
	 * The non-LOCKed access to kmp_flags is valid at this time because:
	 *
	 * (1) the client has ceased access to this mapping, so that the kmp
	 *     is privately held by us, with the single exception of
	 *     segkvn_age(), and
	 *
	 * (2) The segkvn_age() function does not change the SEGKVN_HAT_LOAD
	 *     flag.
	 */
	if (kmp->kmp_flags & SEGKVN_HAT_LOAD) {
		/*
		 * We must wait if the mapping is aging.
		 */
		(void) LOCK_PLMIN(&segkvn_mutex);
		while (kmp->kmp_flags & SEGKVN_AGING) {
			SEGKVN_INC_STAT(kmp->kmp_nagewaits);
			SV_WAIT(&kmp->kmp_sv, VM_SEGKVN_PRI, &segkvn_mutex);
			(void) LOCK_PLMIN(&segkvn_mutex);
		}

		/*
		 * Remove mapping from the aging list.
		 */
		segkvn_adj_aging(kmp, 0);
		UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

		/*
		 * Now, unload all translations. Now that the segment is
		 * privately held by us, the segkvn_mutex is no longer
		 * needed.
		 */
		hat_kas_unload(kmp->kmp_seg.s_base, kmp->kmp_seg.s_size,
			       HAT_NOFLAGS);
	}

	/*
	 * Detach the segment from segkvn_as (if so attached).
	 */
	if (kmp->kmp_seg.s_as != NULL) {
		ASSERT(kmp->kmp_seg.s_as == segkvn_as);

		/*
		 * must write lock the seg_kvn address space
		 */
		(void) RW_WRLOCK_PLMIN(&segkvn_as_mutex);
		seg_detach(&kmp->kmp_seg);
		RW_UNLOCK_PLMIN(&segkvn_as_mutex, PLBASE);
	}

	/*
	 * free the address range in the zoned bit map
	 */
	zbm_free(&segkvn_zbm, kmp->kmp_base, kmp->kmp_npages);

	/*
	 * release the vnode
	 */
	VN_RELE_CRED(kmp->kmp_vp, sys_cred);

	/*
	 * Now, free the kmp itself
	 */
	kmem_free(kmp, sizeof(struct kmp));
}

/*
 * faultcode_t
 * segkvn_lock(void *mapcookie, uint_t flags)
 *      Lock a segkvn mapping, either just against aging, or lock
 *      it into memory.
 *
 * Calling/Exit State:
 *      ``mapcookie'' is an opaque value previously returned by
 *	segkvn_vp_mapin().
 *
 *      flags must include either:
 *              SEGKVN_FUTURE_LOCK - lock the segment against aging
 *              SEGKVN_MEM_LOCK    - lock the segment into physical memory
 *      flags may optionally also include:
 *              SEGKVN_NO_MEMRESV  - don't take any M_REAL reservations
 *
 *      Called at BASE ipl with no LOCKs held and returns the same way.
 *      Returns 0 on success, or a faultcode on failure.
 */
faultcode_t
segkvn_lock(void *mapcookie, uint_t flags)
{
	struct kmp *kmp = mapcookie;
	faultcode_t fc = 0;
	uint_t nresv = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kmp->kmp_seg.s_ops == &segkvn_ops);

	/*
	 * We always lock the mapping against aging.
	 * We must wait if the mapping is currently aging.
	 */
	(void) LOCK_PLMIN(&segkvn_mutex);
	while (kmp->kmp_flags & SEGKVN_AGING) {
		SEGKVN_INC_STAT(kmp->kmp_nagewaits);
		SV_WAIT(&kmp->kmp_sv, VM_SEGKVN_PRI, &segkvn_mutex);
		(void) LOCK_PLMIN(&segkvn_mutex);
	}
	ASSERT(kmp->kmp_nlocks >= kmp->kmp_nmlocks);

	/*
	 * Must reserve real memory, if not already locked.
	 * Must shoot down the TLB if potentially inconsistent.
	 */
	if (++kmp->kmp_nlocks == 1) {
		if (!(flags & SEGKVN_NO_MEMRESV)) {
			nresv = btop(kmp->kmp_seg.s_size);
			if (!mem_resv(nresv, SEGKVN_KMP_MTYPE(kmp))) {
				kmp->kmp_nlocks = 0;
				UNLOCK_PLMIN(&segkvn_mutex, PLBASE);
				return (FC_MAKE_ERR(ENOMEM));
			}
			SEGKVN_ADD_STAT(kmp->kmp_nreserved, nresv);
		}
		if (kmp->kmp_flags & SEGKVN_HAS_COOKIE) {
			hat_shootdown(kmp->kmp_cookie, HAT_HASCOOKIE);
			kmp->kmp_flags &= ~SEGKVN_HAS_COOKIE;
		}

		/*
		 * Mark the kmp structure to indicate that translations
		 * might be loaded.
		 */
		kmp->kmp_flags |= SEGKVN_HAT_LOAD;
	}

	/*
	 * in the memory locking case, also fault in the pages
	 */
	if (flags & SEGKVN_MEM_LOCK) {
		if (kmp->kmp_nmlocks == 0) {
			/*
			 * The segment is already locked against aging.
			 * So, all we need to do is fault in the pages.
			 */
			UNLOCK_PLMIN(&segkvn_mutex, PLBASE);
			fc = segkvn_kmp_fault(kmp, kmp->kmp_seg.s_base,
					      kmp->kmp_seg.s_size,
					      F_INVAL, S_OTHER);
			/*
			 * faulting complete (with or without error)
			 */
			(void) LOCK_PLMIN(&segkvn_mutex);
			ASSERT(!(kmp->kmp_flags & SEGKVN_AGING));
			if (fc) {
				/*
				 * error case, so undo everything
				 */
				if (--kmp->kmp_nlocks == 0 && nresv != 0) {
					mem_unresv(nresv,
						   SEGKVN_KMP_MTYPE(kmp));
					SEGKVN_SUB_STAT(kmp->kmp_nreserved,
							nresv);
				}
			} else {
				++kmp->kmp_nmlocks;

				/*
				 * memory locked segments are not aged
				 */
				if (kmp->kmp_flags & SEGKVN_AGELIST)
					segkvn_adj_aging(kmp, SEGKVN_NOFLAGS);
			}
		} else {
			/*
			 * already memory locked, so just bump the count
			 */
			++kmp->kmp_nmlocks;
		}
	}

	UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

	return (fc);
}

/*
 * void
 * segkvn_unlock(void *mapcookie, uint_t flags)
 *      Release a lock on a seg_kvn segment.
 *
 * Calling/Exit State:
 *      ``mapcookie'' is an opaque value previously returned by
 *	segkvn_vp_mapin().
 *
 *      flags must include either:
 *              SEGKVN_FUTURE_LOCK - a future lock is being released
 *              SEGKVN_MEM_LOCK    - a memory lock is being released
 *      flags may optionally also include:
 *              SEGKVN_NO_MEMRESV  - don't release any M_REAL reservations
 *              SEGKVN_DONTNEED    - immediately unload translations
 *              SEGKVN_DISCARD     - don't bother to preserve data
 *
 *      Called at BASE ipl level with no LOCKs held and returns the same way.
 */
void
segkvn_unlock(void *mapcookie, uint_t flags)
{
	struct kmp *kmp = mapcookie;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kmp->kmp_seg.s_ops == &segkvn_ops);

	/*
	 * release a memory lock, if so requested
	 */
	(void) LOCK_PLMIN(&segkvn_mutex);

	ASSERT(kmp->kmp_nlocks >= kmp->kmp_nmlocks);
	if (flags & SEGKVN_MEM_LOCK) {
		ASSERT(kmp->kmp_nmlocks != 0);
		--kmp->kmp_nmlocks;
	}

	/*
	 * Release aging lock
	 */
	ASSERT(kmp->kmp_nlocks != 0);
	if (--kmp->kmp_nlocks == 0) {
		/*
		 * unreserve REAL memory if all aging locks are gone
		 */
		if (!(flags & SEGKVN_NO_MEMRESV)) {
			mem_unresv(btop(kmp->kmp_seg.s_size),
				   SEGKVN_KMP_MTYPE(kmp));
			SEGKVN_SUB_STAT(kmp->kmp_nreserved,
					btop(kmp->kmp_seg.s_size));
		}

		if (flags & (SEGKVN_DONTNEED|SEGKVN_DISCARD)) {
			hat_kas_unload(kmp->kmp_seg.s_base,
				       kmp->kmp_seg.s_size,
				       (flags & SEGKVN_DISCARD) ?
						HAT_DONTNEED|HAT_CLRMOD :
						HAT_DONTNEED);

			/*
			 * Remove it from the age list if it is already
			 * there. It won't be there if the client consistently
			 * uses the HAT_DONTNEED flag. However, we have no way
			 * to guarantee such behavior.
			 */
			if (kmp->kmp_flags & SEGKVN_AGELIST)
				segkvn_adj_aging(kmp, 0);

			/*
			 * Mark the kmp structure, indicating that
			 * no translations are loaded.
			 */
			kmp->kmp_flags &= ~SEGKVN_HAT_LOAD;
		} else {
			/*
			 * Place on the aging list, if not already there.
			 */
			segkvn_adj_aging(kmp, SEGKVN_AGELIST);
		}
	}

	UNLOCK_PLMIN(&segkvn_mutex, PLBASE);
}

/*
 * STATIC faultcode_t
 * segkvn_fault(struct seg *seg, vaddr_t addr, uint_t len,
 *		enum fault_type type, enum seg_rw rw);
 *	Process a hardware fault on a seg_kvn segment.
 *
 * Calling/Exit State:
 *	Called at BASE ipl with no LOCKs held and returns that way.
 *	The affected segment must have been locked against aging.
 *	The only fault types supported are F_INVAL and F_PROT.
 *
 * Description:
 *	This code is called for segments attached to kas (currently
 *	this means segkvn only). Its function is to locate the correct
 *	kmp structure from the fault address, and dispatch the fault
 *	to segkvn_kmp_fault().
 */
STATIC faultcode_t
segkvn_fault(struct seg *seg, vaddr_t addr, uint_t len,
	     enum fault_type type, enum seg_rw rw)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(seg->s_ops == &segkvn_ops);
	ASSERT(seg->s_as == &kas);

	(void) RW_RDLOCK_PLMIN(&segkvn_as_mutex);
	seg = as_segat(segkvn_as, addr);
	RW_UNLOCK_PLMIN(&segkvn_as_mutex, PLBASE);
	if (seg == NULL)
		return (FC_NOMAP);
	return (segkvn_kmp_fault(SEGKVN_TO_KMP(seg), addr, len, type, rw));
}

/*
 * STATIC faultcode_t
 * segkvn_kmp_fault(struct kmp *kmp, vaddr_t addr, uint_t len,
 *		    enum fault_type type, enum seg_rw rw)
 *	Process a fault on a seg_kvn mapping.
 *
 * Calling/Exit State:
 *	Called at BASE ipl with no LOCKs held and returns that way.
 *	The affected mapping must have been locked against aging.
 *	The only fault type supported is F_INVAL.
 */
STATIC faultcode_t
segkvn_kmp_fault(struct kmp *kmp, vaddr_t addr, uint_t len,
		 enum fault_type type, enum seg_rw rw)
{
	int err;
	uint_t prot;
	page_t *pp;
	page_t **ppp;
	page_t *plp[PVN_KLUSTER_NUM + 1];
	vaddr_t end_addr, vp_addr;
	off_t vp_off;
	size_t get_len;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kmp->kmp_seg.s_ops == &segkvn_ops);

	/*
	 * No SOFTLOCKing supported.
	 */
	switch (type) {
	case F_INVAL:
	case F_PROT:
		break;
	default:
		return (FC_MAKE_ERR(EINVAL));
	}

	/*
	 * The segment should have been "aging locked" for the access to
	 * be valid.
	 */
	ASSERT(kmp->kmp_nlocks != 0 && !(kmp->kmp_flags & SEGKVN_AGING));

	/*
	 * truncate address to a page boundary
	 */
	addr &= PAGEMASK;

	/*
	 * per-page fault processing loop
	 */
	end_addr = addr + len;
	while (addr < end_addr) {
		/*
		 * Try to get all remaining pages in the fault range at
		 * once, plus perhaps some surrounding pages, up to a maximum
		 * of PVN_KLUSTER_NUM pages.
		 */
		get_len = MIN(len, PVN_KLUSTER_SZ);
		vp_off = kmp->kmp_off + (addr - kmp->kmp_seg.s_base);
		SEGKVN_INC_STAT(kmp->kmp_ngetpage);
		err = VOP_GETPAGE(kmp->kmp_vp, vp_off, get_len,
				  &prot, plp, PVN_KLUSTER_SZ,
				  &kmp->kmp_seg, addr, rw, sys_cred);
		if (err)
			return (FC_MAKE_ERR(err));

		/*
		 * At this time the pl array has some of the needed pages
		 * in the fault address range, plus (possibly) having some
		 * adjacent pages (klustered in).
		 *
		 * The page list returned by VOP_GETPAGE is already in order
		 * of increasing p_offset.
		 */
		ppp = plp;
		pp = *ppp++;
		ASSERT(vp_off >= pp->p_offset);
		do {
			ASSERT(PAGE_IS_RDLOCKED(pp));
			if (pp->p_offset >= kmp->kmp_off &&
			    pp->p_offset < kmp->kmp_off + kmp->kmp_seg.s_size) {
				vp_addr = (pp->p_offset - kmp->kmp_off) +
					  kmp->kmp_seg.s_base;
				hat_kas_memload(vp_addr, pp, prot);
			}
			page_unlock(pp);
			pp = *ppp++;
			ASSERT(vp_addr <= addr);
			if (vp_addr == addr) {
				addr += PAGESIZE;
				len -= PAGESIZE;
			}
		} while (pp);
	}

	return (0);
}

/*
 * void
 * segkvn_age(void)
 *	Age out old (not recently referenced) pages, based upon the clock.
 *
 * Calling/Exit State:
 *	Called once a second (or more often under memory exhaustion
 *	conditions) from the poolrefresh daemon.
 *	Called at BASE ipl with no LOCKs held and returns that way.
 */

void
segkvn_age(void)
{
	struct kmp *kmp;
	clock_t age_time, over_time, stamp;
	uint_t flag;
	vaddr_t base;
	boolean_t tlbi;
	TLBScookie_t cookie;
	int lmin_mem_notinuse;
	boolean_t do_wakeup;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * OPTIMIZATION: quick exit if the list is empty
	 */
	if (SEGKVN_AGANCHOR->kmp_next == SEGKVN_AGANCHOR)
		return;

	/*
	 * Adjust the aging speed to the demand on memory.
	 * We examine the counts without locking, as the results are just
	 * an approximation, used only for advice.
	 */
	age_time = segkvn_age_time;
	over_time = SEGKVN_INFINITY;
	lmin_mem_notinuse = mem_avail;
	if (lmin_mem_notinuse < lotsfree) {
		if (lmin_mem_notinuse < desfree) {
			if (lmin_mem_notinuse < verylowfree ||
			    avefree < minfree) {
				age_time = segkvn_desperate_age_time;
				over_time = SEGKVN_DESPERATE;
			} else {
				age_time = segkvn_fast_age_time;
				over_time = segkvn_normal_age_time;
			}
		} else {
			age_time = segkvn_normal_age_time;
			over_time = segkvn_age_time;
		}
	}

	/*
	 * Age the oldest non-aging-locked segment,
	 * or all segments if desperate.
	 */
	do_wakeup = B_FALSE;
	(void) LOCK_PLMIN(&segkvn_mutex);
	kmp = SEGKVN_AGANCHOR->kmp_next;
	while (kmp != SEGKVN_AGANCHOR &&
	       lbolt - kmp->kmp_stamp >= age_time) {
		ASSERT(kmp->kmp_flags & SEGKVN_AGELIST);
		ASSERT(!(kmp->kmp_flags & SEGKVN_AGING));
		ASSERT(kmp->kmp_nmlocks == 0);
		if (kmp->kmp_nlocks != 0) {
			kmp = kmp->kmp_next;
		} else {
			cookie = hat_getshootcookie();
			stamp = kmp->kmp_stamp;
			flag = ~(kmp->kmp_flags & SEGKVN_HAS_AGED);
			segkvn_adj_aging(kmp, flag);
			kmp->kmp_flags |= SEGKVN_AGING;
			SEGKVN_INC_STAT(kmp->kmp_nagings);
			UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

			base = kmp->kmp_seg.s_base;
			tlbi = hat_kas_agerange(base,
						base + kmp->kmp_seg.s_size);

			(void) LOCK_PLMIN(&segkvn_mutex);
			kmp->kmp_flags &= ~SEGKVN_AGING;
			if (tlbi) {
				kmp->kmp_flags |= SEGKVN_HAS_COOKIE;
				kmp->kmp_cookie = cookie;
			}

			/*
			 * remember to wakeup any LWPs waiting for aging
			 * to complete
			 */
			if (SV_BLKD(&kmp->kmp_sv)) {
				do_wakeup = B_TRUE;
			}

			/*
			 * if this segment was aging into "over time"
			 * then continue aging other segments on the list
			 */
			if (lbolt - stamp < over_time) {
				break;
			}

			/*
			 * We must start at the list head again because we
			 * dropped the segkvn_mutex (so that anything may
			 * have happened to the list).
			 */
			kmp = SEGKVN_AGANCHOR->kmp_next;
		}
	}

	UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

	if (do_wakeup)
		SV_BROADCAST(&kmp->kmp_sv, 0);

	return;
}

/*
 * STATIC int
 * segkvn_kluster(struct seg *seg, vaddr_t addr, int delta)
 *	Check to see if it makes sense to do kluster/read ahead to
 *	addr + delta relative to the mapping at addr.  We assume here
 *	that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * Calling/Exit State:
 *	For segkvn we approve of this action, as long as the
 *	page stays within the segment. However, if SEGKVN_KMP_KLUST is
 *	set, then we always approve of klustering.
 */

STATIC int
segkvn_kluster(struct seg *seg, vaddr_t addr, int delta)
{
	struct kmp *kmp = SEGKVN_TO_KMP(seg);

	if (kmp->kmp_flags & SEGKVN_KMP_KLUST)
		return (0);		/* always kluster */

	if (addr + delta < seg->s_base ||
	    addr + delta >= (seg->s_base + seg->s_size))
		return (-1);		/* exceeded segment bounds */

	return (0);
}

/*
 * STATIC void
 * segkvn_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */

STATIC void
segkvn_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ seg_kvn segment driver.  This indicates an internal error in
	 *+ the operating system. The basic integrity of the operating
	 *+ system may have been compromised by malfunctioning software or
	 *+ hardware. Operator action: unknown.
	 */
	cmn_err(CE_PANIC, "segkvn_badop");
	/*NOTREACHED*/
}

/*
 * boolean_t
 * segkvn_lazy_shootdown(struct seg *seg, vaddr_t addr)
 *	It is basically a no-op.
 *
 * Calling/Exit State:
 *	Always returns B_FALSE indicating that lazy shootdown is not possible
 *	and immediate shootdown needs to be done by the caller.
 */
/* ARGSUSED */
STATIC boolean_t
segkvn_lazy_shootdown(struct seg *seg, vaddr_t addr)
{
	return B_FALSE;
}

/*
 * STATIC void
 * segkvn_adj_aging(struct kmp *kmp, uint_t flag)
 *	Adjust the aging state of a segkvn segment, adding or deleting
 *	it from the aging list, as appropriate.
 *
 * Calling/Exit State:
 *	Called with segkvn_mutex LOCKed as BASE ipl and returns that way.
 *
 *	kmp	A pointer to the private data of the segment.
 *
 *	flag	Indicates the operation requested as follows:
 *
 *	    0			Segment is no longer to be aged.
 *	    SEGKVN_AGELIST	Segment must be aged (twice more).
 *	    SEGKVN_HAS_AGED	Segment has aged once and must therefore
 *				be aged once more.
 */

STATIC void
segkvn_adj_aging(struct kmp *kmp, uint_t flag)
{
	struct kmp *next, *last;

	ASSERT(LOCK_OWNED(&segkvn_mutex));
	ASSERT(getpl() == VM_SEGKVN_IPL);

	/*
	 * First take care of the case where the segment is already on the
	 * aging list.
	 */
	if (kmp->kmp_flags & SEGKVN_AGELIST) {
		if (flag == SEGKVN_AGELIST) {
			/*
			 * The segment may have just been accessed by
			 * by the user. Just make sure it gets aged twice
			 * more.
			 */
			kmp->kmp_flags &= ~SEGKVN_HAS_AGED;
			return;
		}

		/*
		 * Remove segment from the aging list.
		 */
		next = kmp->kmp_next;
		last = kmp->kmp_last;
		next->kmp_last = last;
		last->kmp_next = next;
		kmp->kmp_flags &= ~(SEGKVN_AGELIST|SEGKVN_HAS_AGED);
		SEGKVN_DEC_STAT(segkvn_naging);
	}

	/*
	 * Add the segment to the aging list, if so required.
	 */
	if (flag != SEGKVN_NOFLAGS) {
		last = SEGKVN_AGANCHOR->kmp_last;
		last->kmp_next = kmp;
		SEGKVN_AGANCHOR->kmp_last = kmp;
		kmp->kmp_next = SEGKVN_AGANCHOR;
		kmp->kmp_last = last;
		kmp->kmp_stamp = lbolt;
		kmp->kmp_flags |= (SEGKVN_AGELIST | flag);
		SEGKVN_INC_STAT(segkvn_naging);
	}
}

#ifdef DEBUG

/*
 * void
 * segkvn_audit(void *mapcookie, struct segkvn_dbginfo *stats)
 *	Audit a seg_kvn segment, asserting consistency of structure.
 *
 * Calling/Exit State:
 *	The caller passes in an opaque pointer to the kmp,
 *	and stats (a pointer to a segkvn_dbginfo structure).
 *	On exit, this segkvn_dbginfo structure contains the returned
 *	information.
 *
 *	Called at BASE ipl with no LOCKs held and returns that way.
 */

void
segkvn_audit(void *mapcookie, struct segkvn_dbginfo *stats)
{
	off_t off, end_off;
	struct kmp *kmp = mapcookie;
	extern void zbm_audit(const zbm_t *);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kmp->kmp_seg.s_ops == &segkvn_ops);


	/*
	 * count up the incore pages
	 */
	stats->kmpi_nincore = 0;
	off = kmp->kmp_off;
	end_off = off + kmp->kmp_seg.s_size;
	while (off < end_off) {
		if (page_cache_query(kmp->kmp_vp, off))
			++stats->kmpi_nincore;
		off += PAGESIZE;
	}

	/*
	 * some basic correctness assertions
	 */
	ASSERT(kmp->kmp_base <= kmp->kmp_seg.s_base);
	ASSERT(kmp->kmp_seg.s_base + kmp->kmp_seg.s_size <=
	       kmp->kmp_base + ptob(kmp->kmp_npages));
	(void) LOCK_PLMIN(&segkvn_mutex);
	ASSERT(kmp->kmp_nlocks >= kmp->kmp_nmlocks);
	if (kmp->kmp_nlocks != 0) {
		ASSERT(kmp->kmp_nreserved == btop(kmp->kmp_seg.s_size));
		ASSERT((kmp->kmp_flags &
		       (SEGKVN_AGING|SEGKVN_HAS_COOKIE)) == 0);
	} else {
		ASSERT(kmp->kmp_nreserved == 0);
	}
	if (kmp->kmp_nmlocks != 0) {
		ASSERT((kmp->kmp_flags &
		       (SEGKVN_AGELIST|SEGKVN_HAS_AGED)) == 0);
	}

	/*
	 * return data to user
	 */
	stats->kmpi_ngetpage = kmp->kmp_ngetpage;
	stats->kmpi_nagewaits = kmp->kmp_nagewaits;
	stats->kmpi_nagings = kmp->kmp_nagings;
	stats->kmpi_nreserved = kmp->kmp_nreserved;
	stats->kmpi_nlocks = kmp->kmp_nlocks;
	stats->kmpi_nmlocks = kmp->kmp_nmlocks;
	stats->kmpi_flags = kmp->kmp_flags;

	UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

	/*
	 * audit the ZBM data
	 */
	zbm_audit(&segkvn_zbm);
}

/*
 * uint_t
 * segkvn_aging_audit(void)
 *	Audit the seg_kvn aging list.
 *
 * Calling/Exit State:
 *	Returns the number of processes on the aging list.
 *
 *	Called at BASE ipl with no LOCKs held and returns that way.
 */
uint_t
segkvn_aging_audit(void)
{
	uint_t naging = 0;
	struct kmp *kmp, *last;
	clock_t stamp = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK_PLMIN(&segkvn_mutex);

	kmp = SEGKVN_AGANCHOR->kmp_next;
	last = SEGKVN_AGANCHOR;
	while (naging < segkvn_naging && kmp != SEGKVN_AGANCHOR) {
		ASSERT(kmp->kmp_flags & SEGKVN_AGELIST);
		ASSERT(kmp->kmp_nmlocks == 0);
		ASSERT(kmp->kmp_last == last);
		ASSERT(kmp->kmp_stamp >= stamp);
		++naging;
		last = kmp;
		kmp = kmp->kmp_next;
		stamp = kmp->kmp_stamp;
	}
	ASSERT(naging == segkvn_naging);
	ASSERT(kmp == SEGKVN_AGANCHOR);
	ASSERT(SEGKVN_AGANCHOR->kmp_last == last);
	ASSERT(lbolt >= last->kmp_stamp);

	UNLOCK_PLMIN(&segkvn_mutex, PLBASE);

	return (naging);
}

#endif /* DEBUG */

#if defined(DEBUG)

/*
 * void
 * print_segkvn_zbm(void)
 * 	Routine to print out the internal ZBM data structures for segkvn.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_segkvn_zbm(void)
{
	extern void print_zbm(const zbm_t *);

	print_zbm(&segkvn_zbm);
}

/*
 * void
 * print_segkvn_stats(void)
 *	Routine to print out the ZBM statistics for segkvn.
 *
 * Calling/Exit State:
 *	none
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_segkvn_stats(void)
{
	extern void print_zbm_stats(const zbm_t *);

	print_zbm_stats(&segkvn_zbm);
}

#endif /* DEBUG */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_kmp(const struct kmp *kmp)
 *	Debug function to print the contents of a seg_kvn mapping structure.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_kmp(const struct kmp *kmp)
{
	char buf[128];

	debug_printf("contents of kmp structure at %x:\n", kmp);
	debug_printf("    next = %x, last = %x\n", kmp->kmp_next,
		     kmp->kmp_last);
	debug_printf("    seg.s_base = %x, seg.s_size = %x\n",
		     kmp->kmp_seg.s_base, kmp->kmp_seg.s_size);
	debug_printf("    base = %x, npages = %x\n", kmp->kmp_base,
		     kmp->kmp_npages);
	debug_printf("    kmp_vp = %x, kmp_off = %x\n", kmp->kmp_vp,
		     kmp->kmp_off);
	debug_printf("    nlocks = %d, nmlocks = %d\n",
		     kmp->kmp_nlocks, kmp->kmp_nmlocks);
	buf[0] = '\0';
	if (kmp->kmp_flags & SEGKVN_AGING)
		strcat(buf, "AGING ");
	if (kmp->kmp_flags & SEGKVN_HAS_COOKIE)
		strcat(buf, "HAS_COOKIE ");
	if (kmp->kmp_flags & SEGKVN_AGELIST)
		strcat(buf, "AGELIST ");
	if (kmp->kmp_flags & SEGKVN_HAS_AGED)
		strcat(buf, "HAS_AGED ");
	if (kmp->kmp_flags & SEGKVN_HAT_LOAD)
		strcat(buf, "HAT_LOAD ");
	if (kmp->kmp_flags & SEGKVN_KMP_KLUST)
		strcat(buf, "KLUSTER ");
	if (kmp->kmp_flags & ~SEGKVN_ALL_FLAGS)
		strcat(buf, "*** illegal flags *** ");
	debug_printf("    flags = %x ( %s)\n", kmp->kmp_flags, buf);
#ifndef NO_RDMA
	debug_printf("    mtype = %x\n", kmp->kmp_mtype);
#endif
#ifdef DEBUG
	debug_printf("    # of getpage calls = %d, # of age waits = %d\n",
		     kmp->kmp_ngetpage, kmp->kmp_nagewaits);
	debug_printf("    nagings = %d, nreserved = %d\n",
		     kmp->kmp_nagings, kmp->kmp_nreserved);
#endif /* DEBUG */
}

#endif /* DEBUG || DEBUG_TOOLS */
