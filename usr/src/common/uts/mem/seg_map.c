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

#ident	"@(#)kern:mem/seg_map.c	1.57"
#ident	"$Header: $"

/*
 * VM - generic vnode mapping segment.
 *
 * The segmap driver is used only by the kernel to get faster (than seg_vn)
 * mappings [lower routine overhead; more persistent cache] to random
 * vnode/offsets.
 */

#include <fs/buf.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pageidhash.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_map_u.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

/*
 * external data
 */
extern int avefree;
extern int verylowfree;
extern int mem_avail;
extern clock_t segmap_age_time;
extern ulong_t segmap_agings;

/*
 * Private seg op routines.
 */
STATIC faultcode_t segmap_fault(struct seg *seg, vaddr_t addr, uint_t len,
				enum fault_type type, enum seg_rw rw);
STATIC int segmap_kluster(struct seg *seg, vaddr_t addr, int delta);
STATIC void segmap_badop(void);
STATIC boolean_t segmap_lazy_shootdown(struct seg *seg, vaddr_t addr);

STATIC struct seg_ops segmap_ops = {
	(int (*)())segmap_badop,	/* unmap */
	(void (*)())segmap_badop,	/* free */
	segmap_fault,
	(int (*)())segmap_badop,	/* setprot */
	(int (*)())segmap_badop,	/* checkprot */
	segmap_kluster,
	(int (*)())segmap_badop,	/* sync */
	(int (*)())segmap_badop,	/* incore */
	(int (*)())segmap_badop,	/* lockop */
	(int (*)())segmap_badop,	/* dup */
	(void (*)())segmap_badop,	/* childload */
	(int (*)())segmap_badop,	/* getprot */
	(off_t (*)())segmap_badop,	/* getoffset */
	(int (*)())segmap_badop,	/* gettype */
	(int (*)())segmap_badop,	/* getvp */
	(void (*)())segmap_badop,	/* age */
        segmap_lazy_shootdown,          /* lazy_shootdown */
	(int (*)())segmap_badop,	/* memory */
};

/*
 * Private STATIC data
 */
STATIC clock_t segmap_normal_age_time;		/* normal aging time */
STATIC clock_t segmap_fast_age_time;		/* fast aging time */
STATIC clock_t segmap_desperate_age_time;	/* desperate aging time */
STATIC ulong_t segmap_normal_agings;		/* normal: MAX chunks to age */
STATIC ulong_t segmap_fast_agings;		/* fast: MAX chunks to age */
STATIC ulong_t segmap_desperate_agings;		/* desperate: MAX to age */

/*
 * Lock information.
 */

	/*+ Segmap list and hash table lock */
STATIC LKINFO_DECL(segmap_list_lkinfo, "MS:segmap:smd_list_lock", 0);
	/*+ Per-smap lock for simultaneous faults */
STATIC LKINFO_DECL(segmap_sm_lkinfo, "MS:segmap:sm_lock", 0);

/*
 * Private segmap routines.
 */

/*
 * STATIC void
 * segmap_smapadd(struct segmap_data *smd, struct smap *smp)
 *	Add an smap slot onto the free list.
 *
 * Calling/Exit State:
 *	Caller must hold the smd_list_lock.
 *
 * Description:
 *	The smap is added to the front of the free list if it is not currently
 *	file-associated (i.e. has sm_vp == NULL).  Otherwise it is added to
 *	the end of the free list.
 */
#define segmap_smapadd(smd, smp) { \
	if (smd->smd_free == (struct smap *)NULL) { \
		smd->smd_free = smp->sm_next = smp->sm_prev = smp; \
		if (smp->sm_vp != (vnode_t *)NULL) \
			smd->smd_age = smp; \
	} else { \
		smp->sm_next = smd->smd_free; \
		smp->sm_prev = smd->smd_free->sm_prev; \
		smd->smd_free->sm_prev = smp; \
		smp->sm_prev->sm_next = smp; \
		if (smp->sm_vp == (vnode_t *)NULL) \
			smd->smd_free = smp; \
		else { \
			smd->smd_free = smp->sm_next; \
			if (smd->smd_age == (struct smap *)NULL) \
				smd->smd_age = smp; \
		} \
	} \
}

/*
 * STATIC void
 * segmap_smapsub(struct segmap_data *smd, struct smap *smp)
 *	Remove an smap slot from the free list.
 *
 * Calling/Exit State:
 *	Caller must hold the smd_list_lock.
 */
#define segmap_smapsub(smd, smp) { \
	if (smd->smd_free == smp && (smd->smd_free = smp->sm_next) == smp) \
		smd->smd_age = smd->smd_free = (struct smap *)NULL; \
	else { \
		smp->sm_prev->sm_next = smp->sm_next; \
		smp->sm_next->sm_prev = smp->sm_prev; \
		if (smd->smd_age == smp) { \
			if ((smd->smd_age = smp->sm_next) == smd->smd_free) \
				smd->smd_age = (struct smap *)NULL; \
		} \
		smp->sm_prev = smp->sm_next = smp; \
	} \
}

/*
 * Hash function for segmap: use a modified PAGEID hash function.
 * Note that we have to eliminate any component of the offset which is
 * finer grained than MAXBSIZE, to make sure all offsets within an smap
 * chunk hash to the same slot.
 */
#define SMAP_HASHFUNC(smd, vp, off) \
		PAGEID_HASHFUNC(vp, (off) >> (MAXBSHIFT - PAGESHIFT), \
				(smd)->smd_hashsz)

STATIC void segmap_hashin(struct segmap_data *smd, struct smap *smp,
			  vnode_t *vp, off_t off);
STATIC void segmap_hashout(struct segmap_data *smd, struct smap *smp);

/*
 * Global variables.
 */
struct seg *segkmap;	/* pointer to segment structure for segmap segment */

/*
 * Statistics for segmap operations.
 */
#ifdef DEBUG
struct segmapcnt {
	int	smc_fault;	/* number of segmap_faults */
	int	smc_getmap;	/* number of segmap_getmaps */
	int	smc_get_use;	/* # of getmaps that reuse an existing map */
	int	smc_get_reclaim; /* # of getmaps that do a reclaim */
	int	smc_get_reuse;	/* # of getmaps that reuse a slot */
	int	smc_rel_write;	/* # of releases that write (do putpage) */
	int	smc_rel_async;	/* # of releases that do async putpages */
	int	smc_rel_abort;	/* # of releases that abort */
	int	smc_rel_dontneed; /* # of releases with dontneed set */
	int	smc_release;	/* # of releases that don't do putpage */
	int	smc_age;	/* # times aging was run */
	int	smc_age_clean;	/* # times aging didn't find anything to age */
} segmapcnt;
#define BUMPSMAPCNT(x)	((x)++)
#else
#define BUMPSMAPCNT(x)
#endif

/*
 * Return number of map pages in segment.
 */
#define MAP_PAGES(seg)		((seg)->s_size >> MAXBSHIFT)

/*
 * Translate addr into smap number within segment.
 */
#define MAP_PAGE(seg, addr)	(((addr) - (seg)->s_base) >> MAXBSHIFT)

/*
 * Translate addr in seg into struct smap pointer.
 */
#define GET_SMAP(seg, addr)	\
	&(((struct segmap_data *)((seg)->s_data))->smd_sm[MAP_PAGE(seg, addr)])

/*
 * Return base address covered by smap chunk.
 */
#define SMAP_ADDR(seg, smd, smp)	\
	((seg)->s_base + (((smp) - (smd)->smd_sm) << MAXBSHIFT))

/*
 * void
 * segmap_calloc(struct seg *seg)
 *	Phase one segmap initialization.
 *
 * Calling/Exit State:
 *	Called at calloc time.
 *	seg->s_size has been set to an upper bound on the size, in bytes,
 *	which will be allocated to the segmap segment.
 */
void
segmap_calloc(struct seg *seg)
{
	/* Allocate segmap private data. */
	seg->s_data = calloc(sizeof(struct segmap_data));
}

/*
 * void
 * segmap_create(struct seg *seg)
 *	Phase two segmap initialization.
 *
 * Calling/Exit State:
 *	Called after calloc time, now that we have the exact segment size;
 *	this will be <= the size passed to segmap_init().
 */
void
segmap_create(struct seg *seg)
{
	vaddr_t segend;

	/*
	 * Make sure that seg->s_base and seg->s_base + seg->s_size
	 * are on MAXBSIZE aligned pieces of virtual memory.
	 *
	 * Since we assume we are creating a large segment
	 * (it's just segkmap), trimming off the excess at the
	 * beginning and end of the segment is considered safe.
	 */
	segend = ((seg->s_base + seg->s_size) & MAXBMASK);
	seg->s_base = roundup(seg->s_base, MAXBSIZE);
	seg->s_size = segend - seg->s_base;

	/*
	 * Allocate static page tables.
	 */
	hat_statpt_alloc(seg->s_base, seg->s_size);

	seg->s_ops = &segmap_ops;
}

/*
 * void
 * segmap_init(struct seg *seg)
 *	Final segmap initialization.
 *
 * Calling/Exit State:
 *	Called after KMA enabled.
 */
void
segmap_init(struct seg *seg)
{
	struct segmap_data *smd = seg->s_data;
	struct smap *smp;
	uint_t nsmap;

	/* Determine how many smap chunks we'll need. */
	nsmap = MAP_PAGES(seg);

	/* Allocate smap array. */
	smd->smd_sm = kmem_zalloc(nsmap * sizeof(struct smap), KM_NOSLEEP);

	/*
	 * Compute hash table size.
	 */
	smd->smd_hashsz = pageid_compute_hashsz(nsmap);

	/* Allocate hash table. */
	smd->smd_hash = kmem_zalloc(smd->smd_hashsz * sizeof(struct smap *),
				    KM_NOSLEEP);
	if (smd->smd_sm == (struct smap *)NULL ||
	    smd->smd_hash == (struct smap **)NULL) {
		/*
		 *+ Boot-time allocation of data structures for the segmap
		 *+ kernel segment driver failed.  This probably indicates
		 *+ that the system is configured with either too little
		 *+ physical memory or too much segmap virtual memory.
		 */
		cmn_err(CE_PANIC, "segmap_init: kmem_alloc failed");
		/* NOTREACHED */
	}

	/*
	 * Link up and initialize all the slots.
	 */
	for (smp = &smd->smd_sm[nsmap]; smp-- != smd->smd_sm;) {
		segmap_smapadd(smd, smp);
		LOCK_INIT(&smp->sm_lock, VM_SEGMAP_HIER + 1, PLMIN,
			  &segmap_sm_lkinfo, KM_NOSLEEP);
	}

	/*
	 * Initialize locks, etc.
	 */
	SV_INIT(&smd->smd_resv_sv);
	LOCK_INIT(&smd->smd_list_lock, VM_SEGMAP_HIER, PLMIN,
		  &segmap_list_lkinfo, KM_NOSLEEP);
	FSPIN_INIT(&smd->smd_fspin);
	SV_INIT(&smd->smd_sv);

	/*
	 * pre-compute some aging times and max constants
	 */
	segmap_normal_age_time = (segmap_age_time * 2) / 5;
	segmap_fast_age_time = (segmap_normal_age_time * 2) / 5;
	segmap_desperate_age_time = (segmap_fast_age_time * 2) / 5;
	segmap_normal_agings = segmap_agings * 2;
	segmap_fast_agings = segmap_normal_agings * 2;
	segmap_desperate_agings = nsmap; /* all of segmap */

	/*
	 * Hold onto one segmap chunk's worth of M_BOTH memory reservation,
	 * which can be used as a last resort for critical cases.
	 */
	if (!mem_resv(SEGMAP_RESV_POOL_SIZE * PGPERSMAP, M_BOTH)) {
		/*
		 *+ Insufficient memory was available to initialize the
		 *+ segmap subsystem.  Corrective action: reconfigure the
		 *+ system to use less memory or add more physical memory.
		 */
		cmn_err(CE_PANIC, "segmap_init: not enough memory");
		/* NOTREACHED */
	}
	smd->smd_resv_pool = SEGMAP_RESV_POOL_SIZE;
}

/*
 * STATIC void
 * segmap_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segmap_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segmap segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segmap_badop");
	/*NOTREACHED*/
}

/*
 * STATIC faultcode_t
 * segmap_fault(struct seg *seg, vaddr_t addr, uint_t len, enum fault_type type,
 *	        enum seg_rw rw)
 *	Fault handler; called for hardware faults and from segmap_getmap().
 *
 * Calling/Exit State:
 *	Addr and len arguments have been properly aligned and rounded
 *	with respect to page boundaries by the caller (this is true of
 *	all SOP interfaces).
 *
 *	On success, 0 is returned and the requested fault processing has
 *	taken place. On error, non-zero is returned in the form of a
 *	fault error code.
 *
 *	This routine may block, so no spinlocks should be held on entry,
 *	and none will be held on exit.
 */
/* ARGSUSED */
STATIC faultcode_t
segmap_fault(struct seg *seg, vaddr_t addr, uint_t len, enum fault_type type,
	     enum seg_rw rw)
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp;
	off_t sm_off;
	off_t off, endoff;
	off_t pp_off;
	off_t invar;
	page_t *pp, **ppp;
	vnode_t *vp;
	uint_t prot;
	uint_t pg, endpg;
	int err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ASSERT(len != 0);
	ASSERT(((addr + len - 1) & MAXBOFFSET) < MAXBSIZE);

	ASSERT(type == F_INVAL || type == F_PROT);

	smp = GET_SMAP(seg, addr);
	if (smp->sm_refcnt == 0)
		return FC_NOMAP;

	vp = smp->sm_vp;
	ASSERT(vp != (vnode_t *)NULL);

	ASSERT(u.u_segmap_data.sud_smp == smp);
	ASSERT(u.u_segmap_data.sud_pl[0] == (page_t *)NULL);

	BUMPSMAPCNT(segmapcnt.smc_fault);

	/*
	 *
	 * virtual      sm_addr       addr      endaddr       sm_endaddr
	 * addresses       |           |           |               |
	 *                /           /..request../               /
	 *               /           /           /               /
	 *              |           |           |               |
	 * object    sm_off        off        endoff        sm_endoff
	 * offsets
	 *
	 * sm_addr = (addr & MAXBMASK);
	 * sm_endaddr = sm_addr + MAXBSIZE - 1;
	 * endaddr = addr + len - 1;
	 *
	 * Note: compute end offsets as last byte, not last plus one,
	 * to avoid wraparound problems.
	 */
	sm_off = smp->sm_off;

#ifdef DEBUG
	off = sm_off + (addr & MAXBOFFSET);
	endoff = off + len - 1;

	ASSERT(endoff <= sm_off + MAXBOFFSET);

	ASSERT(btop(off) <= btop(sm_off + (off_t)u.u_segmap_data.sud_end - 1));
	ASSERT(btop(endoff) >= btop(sm_off + (off_t)u.u_segmap_data.sud_off));
#endif

	if (SM_WRITING(smp)) {
		/*
		 * If we're writing, we want to tell VOP_GETPAGE() the exact
		 * range to which we're writing.  This way it can (optionally)
		 * implement the "page-create" optimization, avoiding I/O
		 * for the range we're going to fill in anyway.
		 */
		rw = S_OVERWRITE;
		off = sm_off + (off_t)u.u_segmap_data.sud_off;
		endoff = sm_off + (off_t)u.u_segmap_data.sud_end - 1;
	} else {
		ASSERT(rw == S_READ || rw == S_OTHER);
		/*
		 * If we're reading, we have to round the range out to
		 * page boundaries in order to keep VOP_GETPAGE happy.
		 */
		off = sm_off + ((off_t)u.u_segmap_data.sud_off & PAGEMASK);
		endoff = sm_off +
			  (((off_t)u.u_segmap_data.sud_end + PAGEOFFSET) &
			   PAGEMASK) - 1;
	}

	addr = (addr & MAXBMASK) + (off - sm_off);

	err = VOP_GETPAGE(vp, off, (endoff - off) + 1, &prot,
			  u.u_segmap_data.sud_pl, MAXBSIZE,
			  seg, addr, rw, sys_cred);
	if (err) {
		u.u_segmap_data.sud_pl[0] = NULL;
		return FC_MAKE_ERR(err);
	}

	if (!(prot & PROT_WRITE)) {
		/*
		 * The filesystem indicated that arbitrary writing to the
		 * page should not be allowed (since the page contains backing-
		 * store holes).  For each page covered by the VOP_GETPAGE,
		 * set the "must-fault" flag, so subsequent writes do explicit
		 * faults.
		 */
		pg = btop(off - sm_off);
		endpg = btop(endoff - sm_off);
		FSPIN_LOCK(&smd->smd_fspin);
		do {
			SM_SET_PG_MUSTFAULT(smp, pg);
		} while (pg++ < endpg);
		FSPIN_UNLOCK(&smd->smd_fspin);

		/*
		 * We will always allow write permissions.  This is required
		 * in the write case.  It is a performance optimization in
		 * other cases, since reloading translations with different
		 * permissions could cause TLB shootdown.
		 *
		 * We trust our callers to stick to the range and access type
		 * they specified.
		 *
		 * The reason writing to the page is okay is that we called
		 * VOP_GETPAGE with S_OVERWRITE and the exact range.  In this
		 * case, the returned permissions apply to the rest of the page
		 * outside the range we're going to write to this time.
		 */
	} else {
		/*
		 * We can clear the "mustfault" flag for any page in the
		 * range for which we called VOP_GETPAGE, since it gave us
		 * permission to write by setting PROT_WRITE in prot.
		 */
		if (SM_ANY_MUSTFAULT(smp)) {
			pg = btop(off - sm_off);
			endpg = btop(endoff - sm_off);
			FSPIN_LOCK(&smd->smd_fspin);
			do {
				SM_CLR_PG_MUSTFAULT(smp, pg);
			} while (pg++ < endpg);
			FSPIN_UNLOCK(&smd->smd_fspin);
		}
	}

	/*
	 * Remove invariant from the loop. To compute the virtual
	 * address of the page, we use the following computation:
	 *
	 *	addr = sm_addr + (pp->p_offset - sm_off)
	 *
	 * Since sm_addr and sm_off do not vary:
	 *
	 *	addr = (invar = (sm_addr - sm_off)) + pp->p_offset
	 */
	invar = (addr & MAXBMASK) - sm_off;

	/* Round offset down to page boundary. */
	off &= PAGEMASK;

	for (ppp = u.u_segmap_data.sud_pl; (pp = *ppp++) != (page_t *)NULL;) {
		ASSERT(pp->p_vnode == vp);

		pp_off = pp->p_offset;
		ASSERT(pp_off >= sm_off && pp_off <= sm_off + MAXBOFFSET);

		if (pp_off >= off && pp_off <= endoff) {
			/*
			 * Within range requested.
			 */
			ASSERT(SM_WRITING(smp) || !PAGE_IS_WRLOCKED(pp));
			hat_kas_memload(invar + pp_off, pp,
					PROT_READ|PROT_WRITE);
		} else {
			/*
			 * Within segmap buffer, but not in requested range.
			 */
			ASSERT(!PAGE_IS_WRLOCKED(pp));
			if (prot & PROT_WRITE) {
				hat_kas_memload(invar + pp_off, pp,
						PROT_READ|PROT_WRITE);
				/*
				 * Since VOP_GETPAGE gave us this extra
				 * page with PROT_WRITE set, we have
				 * permission to write to it, and so we can
				 * clear its "mustfault" bit.
				 */
				pg = btop(pp_off - sm_off);
				if (SM_PG_MUSTFAULT(smp, pg)) {
					FSPIN_LOCK(&smd->smd_fspin);
					SM_CLR_PG_MUSTFAULT(smp, pg);
					FSPIN_UNLOCK(&smd->smd_fspin);
				}
			}
		}
	}

	return 0;
}

/*
 * STATIC int
 * segmap_kluster(struct seg *seg, vaddr_t addr, int delta)
 *	Check to see if it makes sense to do kluster/read ahead to
 *	addr + delta relative to the mapping at addr.  We assume here
 *	that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * Calling/Exit State:
 *	On success (klustering approved) 0 is returned; on failure -1.
 *
 *	For segmap, we do not allow klustering to go outside of an smap chunk.
 */
STATIC int
segmap_kluster(struct seg *seg, vaddr_t addr, int delta)
{
	return (MAP_PAGE(seg, addr) == MAP_PAGE(seg, addr + delta)) ? 0 : -1;
}

/*
 * void
 * segmap_age(struct seg *seg)
 *	Age segmap mappings.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine looks for smap chunks w/file associations which have
 *	not been referenced since the last time it checked.  It only looks
 *	for chunks which are not currently in use (sm_refcnt == 0).  If it
 *	finds any, it unloads them, so that the references to the pages
 *	go away, potentially allowing the pages to be freed.
 */
void
segmap_age(struct seg *seg)
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp;
	pl_t oldpri;
	clock_t age_time;
	ulong_t max_agings;

	BUMPSMAPCNT(segmapcnt.smc_age);

	/*
	 * OPTIMIZATION: quick exit if the list is empty
	 */
	if (smd->smd_age == (struct smap *)NULL)
		return;

	oldpri = LOCK_PLMIN(&smd->smd_list_lock);

	/*
	 * We start looking from the "age" pointer.  There's no need to
	 * look at chunks we've already aged.
	 */
	if ((smp = smd->smd_age) == (struct smap *)NULL) {
		UNLOCK_PLMIN(&smd->smd_list_lock, oldpri);
		BUMPSMAPCNT(segmapcnt.smc_age_clean);
		return;
	}


	/*
	 * Adjust the aging speed to the demand on memory.
	 * We examine the counts without locking, as the results are just
	 * an approximation, used only for advise.
	 */
	switch (mem_avail_state) {
		case MEM_AVAIL_EXTRA_PLENTY:
		case MEM_AVAIL_PLENTY:
			age_time = segmap_age_time;
			max_agings = segmap_agings;
			break;
		case MEM_AVAIL_NORMAL:
			age_time = segmap_normal_age_time;
			max_agings = segmap_normal_agings;
			break;
		case MEM_AVAIL_FAIR:
			age_time = segmap_fast_age_time;
			max_agings = segmap_fast_agings;
		case MEM_AVAIL_POOR:
		case MEM_AVAIL_DESPERATE:
		default:
			age_time = segmap_desperate_age_time;
			max_agings = segmap_desperate_agings;
			break;
	}

	/*
	 * Traverse the not-yet-aged portion of the free list looking for
	 * file-associated chunks which haven't been referenced recently.
	 */
	while (lbolt - smp->sm_last_time >= age_time && max_agings-- != 0) {
		ASSERT(smp->sm_refcnt == 0);
		ASSERT(smp->sm_vp != (vnode_t *)NULL);
		/*
		 * Unload the translations so the pages can potentially
		 * be freed.  Since no TLB shootdown is necessary until
		 * we actually re-use the addresses, we don't call
		 * hat_shootdown() here, increasing the probability that it
		 * won't actually cause a shootdown by the time we do call it.
		 */
		hat_kas_unload(SMAP_ADDR(seg, smd, smp), MAXBSIZE, 0);
		smp->sm_flags |= SMP_UNLOADED|SMP_TLBI;

		if ((smp = smp->sm_next) == smd->smd_free) {
			smp = (struct smap *)NULL;
			break;
		}
	}
#ifdef DEBUG
	if (smp == smd->smd_age)
		BUMPSMAPCNT(segmapcnt.smc_age_clean);
#endif
	smd->smd_age = smp;

	UNLOCK_PLMIN(&smd->smd_list_lock, oldpri);
}


/*
 * STATIC void
 * segmap_hashin(struct segmap_data *smd, struct smap *smp,
 *			vnode_t *vp, off_t off)
 *	Enter an smap chunk in the hash table for <vp, off>.
 *
 * Calling/Exit State:
 *	On return, the smap chunk, smp, is assigned the vnode/offset identity
 *	<vp, off> and entered into the segmap hash table by that identity.
 *
 *	Called with smd_list_lock held, and returns with it still held.
 */
STATIC void
segmap_hashin(struct segmap_data *smd, struct smap *smp, vnode_t *vp, off_t off)
{
	struct smap **hpp;

	ASSERT(LOCK_OWNED(&smd->smd_list_lock));
	ASSERT((off & MAXBOFFSET) == 0);

	/*
	 * Funniness here - we don't increment the ref count on the vnode
	 * even though we have another pointer to it here.  The reason
	 * for this is that we don't want the fact that a seg_map
	 * entry somewhere refers to a vnode to prevent the vnode
	 * itself from going away.  This is because this reference
	 * to the vnode is a "soft one".  In the case where a mapping
	 * is being used by a rdwr or directory routine there already
	 * has to be a non-zero ref count on the vnode.  In the case
	 * where the vp has been freed and the the smap structure is
	 * on the free list, there are no pages in memory that can
	 * refer to the vnode.  Thus even if we reuse the same
	 * vnode/smap structure for a vnode which has the same
	 * address but represents a different object, we are ok.
	 */
	smp->sm_vp = vp;
	smp->sm_off = off;

	hpp = &smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
	smp->sm_hash = *hpp;
	*hpp = smp;
}

/*
 * STATIC void
 * segmap_hashout(struct segmap_data *smd, struct smap *smp)
 *	Remove an smap chunk from the hash table.
 *
 * Calling/Exit State:
 *	On return, the smap chunk, smp, has been removed from the
 *	segmap hash table.
 *
 *	Called with smd_list_lock held, and returns with it still held.
 */
STATIC void
segmap_hashout(struct segmap_data *smd, struct smap *smp)
{
	struct smap **hpp, *hp;
	vnode_t *vp;

	ASSERT(LOCK_OWNED(&smd->smd_list_lock));
	ASSERT(!(smp->sm_flags & SMP_UNLOADED));
	ASSERT(smp->sm_vp != (vnode_t *)NULL);

	vp = smp->sm_vp;
	hpp = &smd->smd_hash[SMAP_HASHFUNC(smd, vp, smp->sm_off)];
	for (;;) {
		hp = *hpp;
		if (hp == (struct smap *)NULL) {
			/*
			 *+ A segmap chunk was not found in the hash table
			 *+ while attempting to remove it.  This indicates
			 *+ a kernel software problem.
			 */
			cmn_err(CE_PANIC, "segmap_hashout");
			/* NOTREACHED */
		}
		if (hp == smp)
			break;
		hpp = &hp->sm_hash;
	}

	*hpp = smp->sm_hash;
	smp->sm_hash = (struct smap *)NULL;
	smp->sm_vp = (vnode_t *)NULL;
	smp->sm_off = 0;
	SM_CLRALL_MUSTFAULT(smp);
}

/*
 * Special public segmap operations
 */

/*
 * void *
 * segmap_getmap(struct seg *seg, vnode_t *vp, off_t off, size_t len,
 *	         enum seg_rw rw, boolean_t mustfault, int *errorp)
 *	Allocate a virtual window into a chunk of a vnode.
 *
 * Calling/Exit State:
 *	The range [off, off + len), which must not cross a MAXBSIZE boundary,
 *	of the vnode, vp, is mapped into kernel address space.  The address
 *	of this mapping, rounded down to a MAXBSIZE boundary, is returned.
 *
 *	If rw == S_WRITE, the caller must ensure that there are no other
 *	accesses to this MAXBSIZE chunk.
 *
 *	If mustfault is true, segmap_getmap() will force a fault on the
 *	given range, even if the hardware would not have faulted.  The
 *	mustfault flag is ignored if rw != S_WRITE.
 *
 *	If errorp is non-NULL, and a fault error occurs, (*errorp) will be
 *	set to the errno from the fault, and segmap_getmap will return NULL.
 *	If no fault error occurs, (*errorp) will be undefined.
 *
 *	The calling context must call segmap_release() before calling
 *	segmap_getmap() again.
 *
 *	The caller guarantees not to access any part of the returned
 *	mapping outside of the range corresponding to [off, off + len).
 *
 *	This routine may block, so no spinlocks should be held on entry,
 *	and none will be held on exit.
 */
void *
segmap_getmap(struct seg *seg, vnode_t *vp, off_t off, size_t len,
	      enum seg_rw rw, boolean_t mustfault, int *errorp)
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp;
	vaddr_t base_addr;
	faultcode_t fc;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ASSERT(vp != (vnode_t *)NULL);
	ASSERT((off & MAXBMASK) == ((off + len - 1) & MAXBMASK));
	ASSERT(len != 0);

	ASSERT(u.u_segmap_data.sud_smp == (struct smap *)NULL);
	ASSERT(u.u_segmap_data.sud_pl[0] == (page_t *)NULL);

	BUMPSMAPCNT(segmapcnt.smc_getmap);

retry:
	(void) LOCK_PLMIN(&smd->smd_list_lock);

	/* XXX - keep stats for hash function */
	for (smp = smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
	    smp != (struct smap *)NULL; smp = smp->sm_hash)
		if (smp->sm_vp == vp && smp->sm_off == (off & MAXBMASK))
			break;

	if (smp != (struct smap *)NULL) {
		if (smp->sm_refcnt > 0) {
			BUMPSMAPCNT(segmapcnt.smc_get_use);
			smp->sm_refcnt++;		/* another user */
			ASSERT(!SM_WRITING(smp));
		} else {
			if (!mem_resv(PGPERSMAP, M_BOTH)) {
				/*
				 * The system's M_BOTH reservations have
				 * been exhausted. Try to take a reservation
				 * from the private pool. If that is
				 * exhausted as well, then block (waiting for
				 * someone to give up a private reservation).
				 * Then retry.
				 */
				if (smd->smd_resv_pool == 0) {
					SV_WAIT(&smd->smd_resv_sv, PRIMEM - 2,
						&smd->smd_list_lock);
					goto retry;
				}
				--smd->smd_resv_pool;
				smp->sm_flags |= SMP_PRIVATE_RESV;
			}
			BUMPSMAPCNT(segmapcnt.smc_get_reclaim);
			segmap_smapsub(smd, smp);	/* reclaim */
			smp->sm_refcnt = 1;
			ASSERT((smp->sm_flags & (SMP_UNLOADED|SMP_TLBI)) !=
							SMP_UNLOADED);
			if (smp->sm_flags & SMP_TLBI) {
				/*
				 * Since the page used for this identity
				 * may not be the one we had before we unloaded
				 * the translation, we have to shootdown TLBs.
				 */
				hat_shootdown(smp->sm_last_use, HAT_HASCOOKIE);
				smp->sm_flags &= ~(SMP_TLBI|SMP_UNLOADED);
			}
		}

		base_addr = SMAP_ADDR(seg, smd, smp);
	} else {
		/*
		 * smp is NULL (no entry in cache).
		 * Allocate a new slot and set it up.
		 */
		if ((smp = smd->smd_free) == (struct smap *)NULL) {
			SV_WAIT(&smd->smd_sv, PRIMEM - 2, &smd->smd_list_lock);
			goto retry;
		}
		if (!mem_resv(PGPERSMAP, M_BOTH)) {
			/*
			 * The system's M_BOTH reservations have been
			 * exhausted. Try to take a reservation from the
			 * private pool. If that is exhausted as well, then
			 * block (waiting for someone to give up a private
			 * reservation). Then retry.
			 */
			if (smd->smd_resv_pool == 0) {
				SV_WAIT(&smd->smd_resv_sv, PRIMEM - 2,
					&smd->smd_list_lock);
				goto retry;
			}
			--smd->smd_resv_pool;
			smp->sm_flags |= SMP_PRIVATE_RESV;
		}
		segmap_smapsub(smd, smp);
		smp->sm_refcnt = 1;

		base_addr = SMAP_ADDR(seg, smd, smp);

		if (smp->sm_vp != (vnode_t *)NULL) {
			BUMPSMAPCNT(segmapcnt.smc_get_reuse);
			/*
			 * Destroy old vnode association and unload any
			 * hardware translations to the old object.
			 */
			if (!(smp->sm_flags & SMP_UNLOADED)) {
				hat_kas_unload(base_addr, MAXBSIZE, 0);
				smp->sm_flags |= SMP_TLBI;
			} else
				smp->sm_flags &= ~SMP_UNLOADED;
			segmap_hashout(smd, smp);
		}
		if (smp->sm_flags & SMP_TLBI) {
			hat_shootdown(smp->sm_last_use, HAT_HASCOOKIE);
			smp->sm_flags &= ~SMP_TLBI;
		}
		segmap_hashin(smd, smp, vp, off & MAXBMASK);
	}

	ASSERT(smp->sm_vp == vp && smp->sm_off == (off & MAXBMASK));
	ASSERT(!(smp->sm_flags & SMP_UNLOADED));
	ASSERT(!(smp->sm_flags & SMP_TLBI));

	UNLOCK_PLMIN(&smd->smd_list_lock, PLBASE);

#ifdef DEBUG
	u.u_segmap_data.sud_smp = smp;
#endif /* DEBUG */

	u.u_segmap_data.sud_off = off - smp->sm_off;
	u.u_segmap_data.sud_end = off + len - smp->sm_off;

	if (rw == S_WRITE) {
		ASSERT(smp->sm_refcnt == 1);
		/* NOTE: No locking needed below; we have exclusive access */
		smp->sm_flags |= SMP_WRITING;
		if (!mustfault) {
			uint_t pg, endpg;

			endpg = ((off + len - 1) & MAXBOFFSET) >> PAGESHIFT;
			pg = (off & MAXBOFFSET) >> PAGESHIFT;
			while (pg <= endpg) {
				if (SM_PG_MUSTFAULT(smp, pg)) {
					SM_CLR_PG_MUSTFAULT(smp, pg);
					mustfault = B_TRUE;
				}
				pg++;
			}
		}
		if (mustfault) {
			off = u.u_segmap_data.sud_off;
			if ((fc = segmap_fault(seg, base_addr + off, len,
					       F_PROT, S_WRITE)) != 0) {
				/*
				 * If the fault failed, remove any leftover
				 * translations, so we fault later and see the
				 * error then.
				 */
				uint_t pgoff = (off & PAGEOFFSET);
				hat_kas_unload(base_addr + (off - pgoff),
					       ptob(btopr(len + pgoff)), 0);
				hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
				if (errorp)
					goto backout;
			}
		}
	} else if (mustfault) {
		off = u.u_segmap_data.sud_off;
		if ((fc = segmap_fault(seg, base_addr + off, len, F_INVAL,
				       S_OTHER)) != 0 && errorp)
			goto backout;
	}

	return (void *)base_addr;

backout:
	ASSERT(FC_CODE(fc) == FC_OBJERR);
	*errorp = FC_ERRNO(fc);
	segmap_release(seg, (void *)base_addr, 0);
	return NULL;
}


/*
 * int
 * segmap_release(struct seg *seg, void *segmap_ptr, uint_t flags)
 *	Release a mapping acquired by segmap_getmap().
 *
 * Calling/Exit State:
 *	The segmap mapping for address, addr, is released.
 *	The flags argument can be used to initiate I/O on the covered pages;
 *	legal values are:
 *
 *	SM_WRITE	Initiate a write I/O operation for the pages.
 *	SM_ASYNC	Do the write asynchronously (ignored if !SM_WRITE).
 *	SM_INVAL	Invalidate (remove from cache) the pages after
 *			writing them.  Also, remove chunk from segmap cache.
 *	SM_NOCACHE	Remove chunk from segmap cache (but not page cache).
 *	SM_DONTNEED	Hint that the pages are unlikely to be needed soon.
 *	SM_SETMOD	Mark pages modified before unlocking them.
 *
 *	This routine may block, so no spinlocks should be held on entry,
 *	and none will be held on exit.
 */
int
segmap_release(struct seg *seg, void *segmap_ptr, uint_t flags)
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp;
	vaddr_t addr = (vaddr_t)segmap_ptr;
	page_t *pp, **ppp;
	boolean_t do_wakeup = B_FALSE;
	boolean_t do_resv_wakeup = B_FALSE;
	int error;
	vnode_t *cache_vp;
	off_t cache_off;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (addr < seg->s_base || addr >= seg->s_base + seg->s_size ||
	    (addr & MAXBOFFSET) != 0) {
		/*
		 *+ Segmap_release() was called with an address not covered
		 *+ by the segmap segment driver, or not properly aligned.
		 *+ This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "segmap_release addr");
		/* NOTREACHED */
	}

	smp = &smd->smd_sm[MAP_PAGE(seg, addr)];
	ASSERT(smp->sm_refcnt != 0);
	ASSERT(!(smp->sm_flags & SMP_UNLOADED));
	ASSERT(!(smp->sm_flags & SMP_TLBI));

	ASSERT(u.u_segmap_data.sud_smp == smp);

	/*
	 * See if we have any pages to unlock.
	 */
	for (ppp = u.u_segmap_data.sud_pl; (pp = *ppp++) != (page_t *)NULL;) {
		ASSERT(SM_WRITING(smp) || !PAGE_IS_WRLOCKED(pp));
		if (flags & SM_SETMOD)
			page_setmod(pp);
		page_unlock(pp);
	}
	u.u_segmap_data.sud_pl[0] = (page_t *)NULL;

	cache_vp = smp->sm_vp;
	cache_off = smp->sm_off;

	(void) LOCK_PLMIN(&smd->smd_list_lock);

	if (--smp->sm_refcnt == 0) {
		smp->sm_flags &= ~SMP_WRITING;

		smp->sm_last_use = hat_getshootcookie();

		if (flags & (SM_INVAL|SM_NOCACHE)) {
			segmap_hashout(smd, smp);	/* remove map info */
			/*
			 * Unload the translations.  Defer the
			 * shootdown until we reuse this slot.
			 *
			 * We have to do the unload here even for the
			 * SM_INVAL case, even though VOP_PUTPAGE(B_INVAL)
			 * will abort the pages (and thus unload any
			 * translations), since that unloading would use
			 * lazy shootdown, which wouldn't work since we've
			 * done a hashout.
			 */
			hat_kas_unload(SMAP_ADDR(seg, smd, smp), MAXBSIZE, 0);
			smp->sm_flags |= SMP_TLBI;
		} else {
			smp->sm_last_time = lbolt;
			if (flags & SM_DONTNEED) {
				/*
				 * Unload the translations so the pages can
				 * potentially be freed.  Set "dontneed",
				 * so the page will be freed to the front
				 * of the free list.
				 */
				hat_kas_unload(SMAP_ADDR(seg, smd, smp),
					       MAXBSIZE, HAT_DONTNEED);
				smp->sm_flags |= SMP_UNLOADED|SMP_TLBI;
			}
		}
		segmap_smapadd(smd, smp);		/* add to free list */

		/*
		 * Give back memory reservation.
		 */
		if (smp->sm_flags & SMP_PRIVATE_RESV) {
			++smd->smd_resv_pool;
			smp->sm_flags &= ~SMP_PRIVATE_RESV;
			do_resv_wakeup = SV_BLKD(&smd->smd_resv_sv);
		} else
			mem_unresv(PGPERSMAP, M_BOTH);

		/*
		 * Wakeup any sleepers.
		 */
		do_wakeup = SV_BLKD(&smd->smd_sv);
	}
	ASSERT(!SM_WRITING(smp));

	UNLOCK_PLMIN(&smd->smd_list_lock, PLBASE);

#ifdef DEBUG
	if (flags & SM_DONTNEED)
		BUMPSMAPCNT(segmapcnt.smc_rel_dontneed);
#endif

	if (flags & (SM_WRITE|SM_INVAL)) {
		int bflags = 0;

		BUMPSMAPCNT(segmapcnt.smc_rel_write);
		if (flags & SM_ASYNC) {
			bflags |= B_ASYNC;
			BUMPSMAPCNT(segmapcnt.smc_rel_async);
		}
		if (flags & SM_INVAL) {
			bflags |= B_INVAL;
			BUMPSMAPCNT(segmapcnt.smc_rel_abort);
		}
		if (flags & SM_DONTNEED)
			bflags |= B_DONTNEED;
		error = VOP_PUTPAGE(cache_vp, cache_off, MAXBSIZE, bflags,
				    sys_cred);
	} else {
		BUMPSMAPCNT(segmapcnt.smc_release);
		error = 0;
	}

	if (do_wakeup)
		SV_SIGNAL(&smd->smd_sv, 0);
	
	if (do_resv_wakeup)
		SV_SIGNAL(&smd->smd_resv_sv, 0);

#ifdef DEBUG
	u.u_segmap_data.sud_smp = (struct smap *)NULL;
#endif /* DEBUG */

	return error;
}

/*
 * boolean_t
 * segmap_lazy_shootdown(struct seg *seg, vaddr_t addr)
 *	Return an indication to the caller whether shootdown needs to be
 *	performed immediately or whether the segment driver will perform a
 *	lazy shootdown.
 *
 * Calling/Exit State:
 *	Returns B_FALSE if lazy shootdown is not possible and immediate
 *	shootdown needs to be done by the caller;
 *	Returns B_TRUE if the segment driver can perform a lazy shootdown.
 * 
 * Remarks: 
 *	The smd_list_lock is only trylocked because of lock ordering problems;
 *	if trylock fails B_FALSE is returned. For the same reason,
 *	need to lock smd_list_lock at current ipl value.
 */
boolean_t
segmap_lazy_shootdown(struct seg *seg, vaddr_t addr)
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp;
	pl_t savpl;
	boolean_t lazy_shoot;

	savpl = getpl();
	if (TRYLOCK(&smd->smd_list_lock, savpl) == INVPL)
		return B_FALSE;

	smp = &smd->smd_sm[MAP_PAGE(seg, addr)];
	ASSERT(smp->sm_vp != (vnode_t *)NULL);

	lazy_shoot = (smp->sm_refcnt == 0);
	if (lazy_shoot)
		smp->sm_flags |= SMP_TLBI;

	UNLOCK(&smd->smd_list_lock, savpl);

	return lazy_shoot;
}

/*
 * off_t
 * segmap_abort_create(struct seg *seg, void *segmap_ptr, off_t off,
 *		       size_t len)
 *	Abort any created pages in the given [off, off + len) range.
 *
 * Calling/Exit State:
 *	The smap chunk given by segmap_ptr (as returned by segmap_getmap())
 *	must be exclusively held for writing (by segmap_getmap() w/S_WRITE).
 *
 *	Any pages in this chunk with vnode offsets in the given range
 *	which were created with uninitialized data during a previous fault
 *	will be aborted.
 *
 *	This is used to clean up after an error.
 *
 *	Returns the vnode offset of the first aborted page, or -1 if none
 *	were aborted.
 */
/* ARGSUSED */
off_t
segmap_abort_create(struct seg *seg, void *segmap_ptr, off_t off, size_t len)
{
	page_t *pp, **ppp, **ppp2;
	off_t eoff, minoff;
#ifdef DEBUG
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	struct smap *smp = &smd->smd_sm[MAP_PAGE(seg, (vaddr_t)segmap_ptr)];
#endif

	ASSERT(u.u_segmap_data.sud_smp == smp);
	ASSERT(smp->sm_refcnt == 1);
	ASSERT(SM_WRITING(smp));

	eoff = off + len;
	off &= PAGEMASK;
	minoff = (off_t)-1;

	for (ppp = u.u_segmap_data.sud_pl; (pp = *ppp) != (page_t *)NULL;) {
		if (PAGE_IS_WRLOCKED(pp) &&
		    pp->p_offset >= off && pp->p_offset < eoff) {
			page_abort(pp);
			if (minoff == (off_t)-1 || minoff > off)
				minoff = off;
			/* remove page from array by moving the rest down */
			for (ppp2 = ppp; (ppp2[0] = ppp2[1]) != (page_t *)NULL;)
				++ppp2;
		} else
			++ppp;
	}

	return minoff;
}


/*
 * Debugging routines only!
 */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_smap(const struct smap *smp)
 *	Print a segmap smap structure.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
print_smap(const struct smap *smp)
{
	struct segmap_data *smd = (struct segmap_data *)segkmap->s_data;
	uint_t i;

	if (smp < smd->smd_sm || smp >= &smd->smd_sm[MAP_PAGES(segkmap)] ||
	    ((vaddr_t)smp - (vaddr_t)smd->smd_sm) % sizeof(struct smap) != 0) {
		debug_printf("%lx is not an smap structure pointer\n",
			     (ulong_t)smp);
		return;
	}

	debug_printf("segmap vaddr %lx, smap struct @ %lx:\n",
		     SMAP_ADDR(segkmap, smd, smp), (ulong_t)smp);
	debug_printf("  vp %lx  off %lx  refcnt %d ",
		     (ulong_t)smp->sm_vp, (ulong_t)smp->sm_off, smp->sm_refcnt);
	if (smp->sm_flags & SMP_TLBI)
		debug_printf(" TLBI");
	if (smp->sm_flags & SMP_UNLOADED)
		debug_printf(" UNLOADED");
	if (smp->sm_flags & SMP_PRIVATE_RESV)
		debug_printf(" PRIVATE_RESV");
	if (smp->sm_flags & SMP_WRITING)
		debug_printf(" WRITING");
	debug_printf("\n  hash %lx  next %lx  prev %lx\n",
		     (ulong_t)smp->sm_hash,
		     (ulong_t)smp->sm_next, (ulong_t)smp->sm_prev);
	debug_printf("  last_use %lx  last_time %lx",
		     (ulong_t)smp->sm_last_use, (ulong_t)smp->sm_last_time);
	debug_printf("  pgflag");
	for (i = 0; i < SM_PGARRAYSZ; i++)
		debug_printf(" %x", smp->sm_pgflag[i]);
	debug_printf("\n");
}

/*
 * void
 * find_smap_by_id(vnode_t *vp, off_t off)
 *	Find smap chunk for <vp, off>.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
find_smap_by_id(vnode_t *vp, off_t off)
{
	struct segmap_data *smd = (struct segmap_data *)segkmap->s_data;
	struct smap *smp;

	off &= MAXBMASK;

	for (smp = smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
	     smp != (struct smap *)NULL; smp = smp->sm_hash) {
		if (smp->sm_vp == vp && smp->sm_off == off) {
			print_smap(smp);
			break;
		}
	}
}

/*
 * void
 * find_smap_by_addr(vaddr_t addr)
 *	Find smap chunk for segmap virtual address, addr.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
find_smap_by_addr(vaddr_t addr)
{
	if (addr < segkmap->s_base ||
	    addr >= segkmap->s_base + segkmap->s_size) {
		debug_printf("%lx is not a segkmap address\n", addr);
		return;
	}

	print_smap(GET_SMAP(segkmap, addr));
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef DEBUG

/*
 * void
 * print_segmap_stats(void)
 *	Print debug statistics to console.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
print_segmap_stats(void)
{
#define X(v)	debug_printf("%6d  "#v"\n", segmapcnt.v)

	X(smc_fault);
	X(smc_getmap);
	X(smc_get_use);
	X(smc_get_reclaim);
	X(smc_get_reuse);
	X(smc_rel_write);
	X(smc_rel_async);
	X(smc_rel_abort);
	X(smc_rel_dontneed);
	X(smc_release);
	X(smc_age);
	X(smc_age_clean);

#undef X
}

#endif /* DEBUG */
