/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/kma.c	1.91"
#ident	"$Header: $"

/*
 * Dynamic kernel memory allocator.
 * Uses "quick fit" algorithm.
 */

#define	_KMEM_C	1

#include <mem/anon.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/rff.h>
#include <mem/seg_kmem.h>
#include <proc/bind.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef _KMEM_HIST
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/user.h>
#endif /* _KMEM_HIST */

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

#define PROT_KMA	(PROT_ALL & ~PROT_USER)

/*
 * KMA_PARANOID enables consistency checks on kmem_free().
 * It is always enabled if DEBUG is on.
 */
#ifdef DEBUG
#define KMA_PARANOID
#endif
#ifdef KMA_PARANOID
			/*+ KMA_PARANOID consistency check failure */
#define FREE_FAIL(why)	cmn_err(CE_PANIC, \
			  "KMA consistency check failed: kmem_free(%lx, %x):" \
			  "\n       %s.\n", (ulong_t)addr, size, (why))
			/*+ KMA_PARANOID consistency check failure */
#define FREE_CORRUPT(addr)	cmn_err(CE_PANIC, \
				  "KMA consistency check failed: free buffer" \
				  " %lx corrupted.\n", (ulong_t)(addr))
			/*+ KMA_PARANOID consistency check failure */
#define INFO_CORRUPT(infop,freep) cmn_err(CE_PANIC, \
				  "KMA consistency check failed: free kminfo" \
				  " %lx corrupted;\n\tbuffer was %lx.\n", \
				  (ulong_t)(infop), (ulong_t)(freep))

#define SLINK_UNUSED	((void *)&kma_shrinktime)
#define KMA_MAGIC	0x53FFFF35
#define KMA_TMAGIC(bp)	(0xc7a00000 + (((uint_t)(bp) >> 6) & 0x000fffff))

#define SET_TAIL_MAGIC(bufp, psize, bsize) { \
	uint_t n = (bsize) - (psize); \
	kminfo_t *infop; \
	uint_t magic; \
 \
	if (n == 0) { \
		infop = &kminfo[IDX((vaddr_t)(bufp))]; \
		FSPIN_LOCK(&kma_bitmap_lock); \
		BITMASKN_SET1(infop->ki_bmap, BITNO(bufp)); \
		FSPIN_UNLOCK(&kma_bitmap_lock); \
	} else { \
		magic = KMA_TMAGIC(bufp); \
		if ((n % sizeof(ulong_t)) == 0) { \
			*(ulong_t *)(void *)&((char *)(bufp))[psize] = magic; \
		} else do { \
			((uchar_t *)bufp)[(bsize) - n] = (uchar_t)magic; \
			magic >>= NBBY; \
			--n; \
		} while (magic != 0 && n != 0); \
	} \
}

#define CHECK_TAIL_MAGIC(bufp, psize, bsize) { \
	uint_t n = (bsize) - (psize); \
	uint_t bitno = BITNO(bufp); \
	kminfo_t *infop = &kminfo[IDX((vaddr_t)(bufp))]; \
	ulong_t magic; \
 \
	if (n == 0) { \
		FSPIN_LOCK(&kma_bitmap_lock); \
		if (!BITMASKN_TEST1(infop->ki_bmap, bitno)) { \
			FSPIN_UNLOCK(&kma_bitmap_lock); \
			FREE_FAIL("wrong size freed or already free"); \
		} \
		BITMASKN_CLR1(infop->ki_bmap, BITNO(bufp)); \
		FSPIN_UNLOCK(&kma_bitmap_lock); \
	} else {\
		FSPIN_LOCK(&kma_bitmap_lock); \
		if (BITMASKN_TEST1(infop->ki_bmap, bitno)) { \
			FSPIN_UNLOCK(&kma_bitmap_lock); \
			FREE_FAIL("size allocated does not equal size freed"); \
		} \
		FSPIN_UNLOCK(&kma_bitmap_lock); \
		magic = KMA_TMAGIC(bufp); \
		if ((n % sizeof(ulong_t)) == 0) { \
			if (*(ulong_t *)(void *)&((char *)(bufp))[psize] != magic) \
				FREE_FAIL("buffer written past end or already free"); \
			*(ulong_t *)(void *)&((char *)(bufp))[psize] = 0; \
		} else do { \
			if (((uchar_t *)bufp)[(bsize) - n] != (uchar_t)magic) \
				FREE_FAIL("buffer written past end or already free"); \
			((uchar_t *)bufp)[(bsize) - n] = 0; \
			magic >>= NBBY; \
			--n; \
		} while (magic != 0 && n != 0); \
	} \
}

#define CHECK_FREEB(freep, enable) { \
		kminfo_t *infop; \
		kmfree_t *nextp; \
		int page_no; \
		kmlist_t *lstp; \
\
		if ((freep)->kf_magic != KMA_MAGIC) { \
			enable \
			FREE_CORRUPT(freep); \
		} \
		if ((freep)->kf_slink != SLINK_UNUSED) { \
			enable \
			FREE_CORRUPT(freep); \
		} \
		infop = &kminfo[IDX((vaddr_t)(freep))]; \
		page_no = infop->ki_page_no; \
		infop -= page_no; \
		if (infop->ki_blist_idx >= maxpool) { \
			enable \
			INFO_CORRUPT(infop, freep); \
		} \
		lstp = &blist[infop->ki_blist_idx]; \
		if (infop->ki_page_no != 0 || \
		    page_no > (int)lstp->km_npage || \
		    infop->ki_nalloc > lstp->km_nbpc) { \
			enable \
			INFO_CORRUPT(infop, freep); \
		} \
		if ((freep)->kf_infop != infop) { \
			enable \
			FREE_CORRUPT(freep); \
		} \
		for (page_no = 1; page_no < (int)lstp->km_npage; ++page_no) { \
			++infop; \
			if (infop->ki_nalloc != NALLOC_CONT || \
			    infop->ki_page_no != page_no) { \
				enable \
				INFO_CORRUPT(infop, freep); \
			} \
		} \
		nextp = (freep)->kf_next; \
		if (nextp != NULL && \
		    ((vaddr_t)nextp < kpg_vbase || \
		     (vaddr_t)nextp >= kpg_vbase + ptob(kpg_vsize))) { \
			enable \
			FREE_CORRUPT(freep); \
		} \
	}

#else /* !KMA_PARANOID */

#define CHECK_FREEB(freep, enable)	/**/

#endif /* KMA_PARANOID */

/*
 * The accounting for KMA buffers is managed with four structures:
 * kmlist_t, kminfo_t, kmfree_t, and kmlocal_t.
 *
 * There is one kmlist_t for each size freelist, one kminfo_t per page of
 * virtual address space, and one kmfree_t for each free buffer (overlayed onto
 * the buffer itself).  There is one kmlocal_t per freelist per engine,
 * which is used to keep track of the local per-engine free pool.
 */
 
/*
 * Per-allocated-chunk accounting structure.
 * Note that once a kminfo_t is initialized and made visible, the ki_blist_idx
 * and ki_page_no fields are read-only, and stay that way until it is
 * deallocated.
 *
 * Ki_nalloc can store a number of special values when
 * blist[ki_blist_idx] == SPECLIST (i.e. when ki_blist_idx == 0):
 *
 *	NALLOC_PGOUT		Memory allocated from pageout private pool
 *				buffers.
 *
 *	NALLOC_OVERSIZE		Memory allocated in granularity of pages.
 *
 *	NALLOC_CONT		Used in the KMA_PARANOID case to indicate
 *				a continuation page.
 *
 * Usage notes:
 *
 *	When the corresponding virtual address is not in use by KMA,
 *	all fields of the kminfo structure are 0. For a multipage chunk,
 *	only the first kminfo holds data in the ki_blist_idx and ki_nalloc
 *	fields. The other chunks have zeros in these field (except for
 *	for ki_nalloc, which is set NALLOC_CONT in the KMA_PARANOID case).
 *
 *	The ki_page_no field is used only by chunks allocated for the
 *	pools.
 */ 
typedef struct kminfo {
	ushort_t	ki_nalloc;	/* # buffers in chunk currently
					   allocated (given out to KMA users) */
	uchar_t		ki_blist_idx;	/* index into the blist */
	uchar_t		ki_page_no;	/* page number in chunk */
#ifdef KMA_PARANOID
	uint_t		ki_bmap[PAGESIZE/(MINBUFSZ * NBITPW)];
#endif
} kminfo_t;

/*
 * Free buffer structure.
 * WARNING: sizeof(kmfree_t) must be <= MINBUFSZ.
 */
typedef struct kmfree {
#ifdef KMA_PARANOID
	ulong_t		kf_magic;
#endif
	struct kmfree	*kf_next;	/* next free buffer for freelist */
	struct kminfo	*kf_infop;	/* ptr to kminfo_t for this buffer */
	/* The following field is temporarily used by kma_shrinkpools(). */
	struct kmfree	*kf_slink;	/* link ptr to next empty chunk */
} kmfree_t;

/* Per-freelist (global) accounting structure. */
typedef struct kmlist {
	kmfree_t	*km_free;	/* list of free buffers */
	kmlocal_t	*km_localp;	/* ptr to local per-engine info */
	lock_t		km_lock;	/* mutex for kmlist_t fields
					   (except km_npage, km_bsize, km_nbpc,
					   km_cachesz, and km_metindex)
					   which are read-only once initted) */
	uchar_t		km_grow_flags;	/* indicates need of growth */
	uchar_t		km_npage;	/* # pages per chunk */
#ifndef NO_RDMA
	uchar_t		km_pflags;      /* page flags */
#endif /* NO_RDMA */
	struct metp_kmem *km_metp;	/* poiner to kma metrics */
	ushort_t	km_bsize;	/* # bytes per buffer */
	ushort_t	km_nbpc;	/* # buffers per chunk */
	ushort_t	km_cachesz;	/* des. # buffers in local free list */
	sv_t		km_wait;	/* to wait for more memory */
} kmlist_t;

#ifndef NO_RDMA
#define KM_PFLAGS(listp)	((listp)->km_pflags)
#else
#define KM_PFLAGS(listp)	0
#endif

/*
 * Buffer size offsets for indexing into the kmlistp (see below) for
 * DMAable, preferentially non-DMAable, preferentially DMAable, and
 * contiguous preferentially non-DMAable requests, respectively.
 */
#define KM_DMA_BSIZE		0
#define KM_NDMA_BSIZE		(MAXBUFSZ + MINBUFSZ)
#define KM_PDMA_BSIZE		(2 * KM_NDMA_BSIZE)
#define KM_CNDMA_BSIZE		(3 * KM_NDMA_BSIZE)

/*
 * Restricted DMA Modes:
 *
 *   KMA has 6 distinct modes of operation:
 *
 * 	1) No restricted DMA support compiled into the kernel.
 *
 *		This mode occurs when the kernel is compiled with
 *		-DNO_RDMA. In this mode, all memory is considered to be
 *		DMAable, so that KMA does not keep seperate pools
 *		of DMAable and non-DMAable memory. However, the KM_REQ_DMA
 *		flag still has meaning, it indicates that the allocated
 *		memory must be physically contiguous. Similarly, KM_DMA
 *		prefers physical contiguity.
 *
 *	2) rdma_mode == RDMA_DISABLED
 *
 *		Operationally, RDMA_DISABLED is just like KMA when
 *		compiled with -DNO_RDMA. However, the restricted DMA
 *		support is present in the kernel.
 *
 *	3) rdma_mode == RDMA_MICRO
 *
 *		For KMA, mode RDMA_MICRO is identical to RDMA_DISABLED.
 *		Thus, the comments in the remainder of this file will not
 *		distinguish between RDMA_DISABLED and RDMA_MICRO.
 *
 *	4) rdma_mode == RDMA_SMALL (non-mini-kernel case)
 *
 *		In this mode, KMA maintains three sets of pools: (i) pools
 *		for preferentially non-DMAable memory [default], (ii) pools
 *		for preferentially DMAable memory [specified by KM_DMA],
 *		and (iii) pools of DMAable memory [specified via KM_REQ_DMA
 *		or through the kmem_alloc_physreq() interface]. These
 *		pools nominally allocate from the (i) STD_PAGE,
 *		(ii) PAD_PAGE, and (iii) DMA_PAGE pools, respectively.
 *
 *	5) rdma_mode == RDMA_SMALL (mini-kernel case)
 *
 *		In this mode, KMA maintains the same three pools (as in
 *		standard RDMA_SMALL). However, since the smap translation
 *		has not been performed on drivers built under UW1.0 and
 *		UW1.1, it is necessary that kmem_alloc() and kmem_zalloc()
 *		will, with no special flags, allocate from the DMAable
 *		memory pools. Thus, the tables below are adjusted so that
 *		kmem_alloc() and kmem_zalloc() will, by default, allocate
 *		from the DMAable pools.
 *
 *	6) rdma_mode == RDMA_LARGE
 *
 *		In this mode, KMA maintains two sets of pools: (i) pools
 *		for preferentially non-DMAable memory [default], and
 *		(ii) pools for DMAable memory [specified via KM_REQ_DMA
 *		or through the kmem_alloc_physreq() interface]. These
 *		pools nominally allocate from the (i) STD_PAGE,
 *		(ii) DMA_PAGE pools, respectively.
 */

/*
 * Table to translate kmem_alloc() flags into an index into the kmlistp
 * table, as a method to select between the non-DMAable, preferred DMAable,
 * and DMAable memory pools.
 *
 * This table covers all the valid combinations of flags for kmem_alloc() and
 * kmem_zalloc().
 */
STATIC uint_t km_flag_index[] = {
	KM_NDMA_BSIZE,		/* KM_SLEEP				   */
	KM_NDMA_BSIZE,		/* KM_NOSLEEP				   */
	KM_PDMA_BSIZE,		/* KM_SLEEP | KM_DMA			   */
	KM_PDMA_BSIZE,		/* KM_NOSLEEP | KM_DMA			   */
	KM_DMA_BSIZE,		/* KM_SLEEP | KM_REQ_DMA		   */
	KM_DMA_BSIZE,		/* KM_NOSLEEP | KM_REQ_DMA		   */
	0,			/* unused				   */
	0,			/* unused				   */
	KM_CNDMA_BSIZE,		/* KM_SLEEP | KM_PHYSCONTIG		   */
	KM_CNDMA_BSIZE,		/* KM_NOSLEEP | KM_PHYSCONTIG		   */
	KM_PDMA_BSIZE,		/* KM_SLEEP | KM_DMA | KM_PHYSCONTIG	   */
	KM_PDMA_BSIZE,		/* KM_NOSLEEP | KM_DMA | KM_PHYSCONTIG	   */
	KM_DMA_BSIZE,		/* KM_SLEEP | KM_REQ_DMA | KM_PHYSCONTIG   */
	KM_DMA_BSIZE,		/* KM_NOSLEEP | KM_REQ_DMA | KM_PHYSCONTIG */
};

#define KM_FLAG_INDEX(flag)	(km_flag_index[flag])

#ifndef NO_RDMA

/*
 * Table to translate kmem_alloc() flags to page flags. This table is
 * correct for restricted DMA models RDMA_SMALL and RDMA_LARGE. Note that the
 * P_DMA and P_NODMA flags are ignored in model RDMA_DISABLED. Adjustments
 * are needed in the RDMA_SMALL mini-kernel case.
 *
 * This table covers all the valid combinations of flags for kmem_alloc(),
 * kmem_zalloc().
 */
STATIC uint_t km_flag_xlat[] = {
	SLEEP | P_NODMA,	/* KM_SLEEP				   */
				/* set to SLEEP | P_DMA for mini-kernel	   */
	NOSLEEP | P_NODMA,	/* KM_NOSLEEP				   */
				/* set to NOSLEEP|P_DMA for mini-kernel	   */
	SLEEP,			/* KM_SLEEP | KM_DMA			   */
	NOSLEEP,		/* KM_NOSLEEP | KM_DMA			   */
	SLEEP | P_DMA,		/* KM_SLEEP | KM_REQ_DMA		   */
	NOSLEEP | P_DMA,	/* KM_NOSLEEP | KM_REQ_DMA		   */
	0,			/* unused				   */
	0,			/* unused				   */
	SLEEP | P_NODMA,	/* KM_SLEEP | KM_PHYSCONTIG		   */
				/* set to SLEEP | P_DMA for mini-kernel	   */
	NOSLEEP | P_NODMA,	/* KM_NOSLEEP | KM_PHYSCONTIG		   */
				/* set to NOSLEEP|P_DMA for mini-kernel	   */
	SLEEP,			/* KM_SLEEP | KM_DMA | KM_PHYSCONTIG	   */
	NOSLEEP,		/* KM_NOSLEEP | KM_DMA | KM_PHYSCONTIG	   */
	SLEEP | P_DMA,		/* KM_SLEEP | KM_REQ_DMA | KM_PHYSCONTIG   */
	NOSLEEP | P_DMA,	/* KM_NOSLEEP | KM_REQ_DMA | KM_PHYSCONTIG */
};

/*
 * Table to translate kmem_alloc()flags into the appropriate mresvtyp_t.
 * This table is correct for restricted DMA models RDMA_SMALL and RDMA_LARGE.
 * The entries of this table are all converted to M_KERNEL_ALLOC for model
 * RDMA_DISABLED. Adjustments are also needed in the RDMA_SMALL mini-kernel
 * case.
 *
 * This table covers all the valid combinations of flags for kmem_alloc().
 */
STATIC uint_t km_flag_mtype[] = {
	M_KERNEL_ALLOC,		/* KM_SLEEP				   */
				/* set to M_DMA for mini-kernel		   */
	M_KERNEL_ALLOC,		/* KM_NOSLEEP				   */
				/* set to M_DMA for mini-kernel		   */
	M_KERNEL_ALLOC,		/* KM_SLEEP | KM_DMA			   */
	M_KERNEL_ALLOC,		/* KM_NOSLEEP | KM_DMA			   */
	M_DMA,			/* KM_SLEEP | KM_REQ_DMA		   */
				/* set to M_KERNEL_ALLOC in RMDA_DISABLED  */
	M_DMA,			/* KM_NOSLEEP | KM_REQ_DMA		   */
				/* set to M_KERNEL_ALLOC in RMDA_DISABLED  */
	0,			/* unused				   */
	0,			/* unused				   */
	M_KERNEL_ALLOC,		/* KM_SLEEP | KM_PHYSCONTIG		   */
				/* set to M_DMA for mini-kernel		   */
	M_KERNEL_ALLOC,		/* KM_NOSLEEP | KM_PHYSCONTIG		   */
				/* set to M_DMA for mini-kernel		   */
	M_KERNEL_ALLOC,		/* KM_SLEEP | KM_DMA | KM_PHYSCONTIG	   */
	M_KERNEL_ALLOC,		/* KM_NOSLEEP | KM_DMA | KM_PHYSCONTIG	   */
	M_DMA,			/* KM_SLEEP | KM_REQ_DMA | KM_PHYSCONTIG   */
				/* set to M_KERNEL_ALLOC in RMDA_DISABLED  */
	M_DMA,			/* KM_NOSLEEP | KM_REQ_DMA | KM_PHYSCONTIG */
				/* set to M_KERNEL_ALLOC in RMDA_DISABLED  */
};

#define KM_FLAG_MTYPE(flag)	(km_flag_mtype[flag])
#define KM_FLAG_XLAT(flag)	(km_flag_xlat[flag])

#define KM_FLAG_TABLE_SIZE	\
	(sizeof(km_flag_xlat) / sizeof(km_flag_xlat[0]))

#else /* NO_RDMA */

#define KM_FLAG_MTYPE(flag)	M_KERNEL_ALLOC
#define KM_FLAG_XLAT(flag)	((flag) & KM_NOSLEEP)

#endif /* NO_RDMA */

/*
 * km_grow_flags associated with each freelist.
 */
#define KMA_GROW_PENDING	(1 << 0)
#define KMA_NOSLEEP_FAILED	(1 << 1)

#define LINK(listp, bp) { \
		(bp)->kf_next = (listp)->km_free; \
		(listp)->km_free = (bp); \
	}

#define NEXT(bp, listp) \
		(bp) = (void *)((char *)(bp) + (listp)->km_bsize);

	/* IDX(): compute index into kminfo[] for a given virtual addr */
#define IDX(addr)		btop((addr) - kpg_vbase)
	/* IDX_TO_ADDR(): compute virtual addr for a given kminfo[] idx */
#define IDX_TO_ADDR(idx)	(kpg_vbase + ptob(idx))
	/* INFOP_TO_ADDR(): compute virtual addr for a given kminfo[] ptr */
#define INFOP_TO_ADDR(infop)	IDX_TO_ADDR((infop) - kminfo)
	/* LISTIDX(): compute index into kmlistp[] for a given byte size */
#define LISTIDX(size)		((uint_t)((size) + MINBUFSZ - 1) >> MINSZSHIFT)
#define LISTIDXF(size, flags)	LISTIDX((size) + KM_FLAG_INDEX(flags))
#define BITNO(addr)		(((uint_t)(addr) & PAGEOFFSET) / MINBUFSZ)

/*
 * The blist[] array contains all of the kmlist_t freelist structures,
 * for both the fixed binary sizes and any additional sizes created as a
 * result of kmem_advise().  In the non-restricted DMA case, the arrangement
 * is as follows:
 *
 *	1		special list (SPECLIST), followed by
 *
 *	NFIXEDBUF 	fixed size freelist structures, followed by
 *
 *	NVARBUF		variable size freelist structures.
 *
 * In the restricted DMA case (rmda_model RDMA_SMALL or RDMA_LARGE) the
 * arrangment is as follows:
 *
 *	1		special list (SPECLIST), followed by
 *
 *	NFIXEDBUF	fixed size preferentially DMAable freelist structures,
 *			followed by
 *
 *	NFIXEDBUF	fixed size DMAable freelist structures, followed by
 *
 *	NFIXEDBUF	fixed size preferentially non-DMAable freelist
 *			structures, followed by
 *
 *	NVARBUF		variable sized preferentially non-DMAable freelist
 *			structures.
 *
 * Each ``NFIXEDBUF'' segment of the blist contains freelist structures
 * for buffers of size MINBUFSZ to MAXBUFSZ, in order of increasing blist
 * index.
 *
 * The first slot is used to optimize kmem_alloc() and kmem_free() paths when
 * handling atypical cases; these are:
 *	requests of size 0 (alloc)
 *	memory from pageout daemon private pool (free)
 *	page granular requests (free)
 */
#define NSPECIALBUF	1
#define KM_NPOOLS	((KMEM_POOL_TYPES * NFIXEDBUF) + NVARBUF)
#define SPECLIST_IDX	0
#define SPECLIST	(&blist[SPECLIST_IDX])

STATIC kmlist_t	blist[KM_NPOOLS + NSPECIALBUF];
STATIC kmlist_t *km_grow_start;

STATIC uint_t nvarbuf;
#ifdef NO_RDMA
STATIC const uint_t npools = 1;
STATIC const uint_t minpool = 1;
STATIC const uint_t dmapool = 1;
STATIC const uint_t ndmapool = 1;
STATIC const uint_t varpool = 1 + NFIXEDBUF;
STATIC uint_t maxpool = 1 + NFIXEDBUF;
#else /* !NO_RDMA */
STATIC uint_t npools = KMEM_POOL_TYPES;
STATIC uint_t minpool = 1 + NFIXEDBUF;
STATIC const uint_t pdmapool = 1;
STATIC uint_t dmapool = 1 + NFIXEDBUF;
STATIC uint_t ndmapool = 1 + (2 * NFIXEDBUF);
STATIC uint_t varpool = 1 + (KMEM_POOL_TYPES * NFIXEDBUF);
STATIC uint_t maxpool = 1 + (KMEM_POOL_TYPES * NFIXEDBUF);
#endif /* !NO_RDMA */

STATIC kminfo_t *kminfo;

/*
 * kmlistp array
 *
 * Array of kmlist_t pointers, one per MINBUFSZ bytes of size, for sizes up
 * to MAXBUFSZ, the maximum size buffer kept in the KMA pools; this array
 * (kmlistp) is used to find the appropriate kmlist_t freelist--which covers
 * a range of sizes--from a desired size, with a simple index operation.
 *
 * Actually, the kmlistp stores a sequence of four such lists.
 *
 * In order to prevent the allocation of discontiguous memory to those who
 * require contiguous memory, such requests (when satisfied by a pool) are
 * satisfied from fixed size (power of 2) pools. Other requests for memory
 * can come from either fixed size or advised size pools.
 *
 * When restricted DMA is not in effect, all four lists point into the
 * same (only) set of pools. However, only the non-DMAable list which
 * allows physical discontinuity references the variable sized pools.
 *
 * In the RDMA_SMALL case (no mini-kernel), the four lists point into
 * three disjoint set of pools. In the RDMA_LARGE case, the preferred DMA,
 * contiguous DMAable, and non-DMAable lists all point at the same set of
 * pools (non-DMAable). However, only the non-DMAable list references the
 * variable sized list. In the RDMA_SMALL mini-kernel case, what would
 * normally be the non-DMAable list becomes a copy of the DMAable list.
 *
 * 0:
 *	Points to the SPECLIST as a technique for efficiently handling
 *	requests of size zero (required by the DDI).
 *
 * 1 -> LISTIDX(MAXBUFSZ):
 *
 *	For each index, kmlistp[index] points to the kmlist_t freelist for
 *	allocating buffers of size (index << MINSZSHIFT) bytes. In the
 *	restricted DMA case, these point to the freelists for DMAable
 *	memory. In all cases, these entries always point to fixed size
 *	pools.
 *
 * LISTIDX(KM_NDMA_BSIZE):
 *
 *	Points to the SPECLIST as a technique for efficiently handling
 *	requests of size zero for non-DMAable memory.
 *
 * LISTIDX(KM_NDMA_BSIZE) + 1 -> LISTIDX(KM_NDMA_BSIZE + MAXBUFSZ):
 *
 *	For each index, kmlistp[index] points to the kmlist_t freelist for
 *	allocating non-DMAable buffers of size
 *	((index << MINSZSHIFT) - KM_NDMA_BSIZE).
 *
 * LISTIDX(KM_PDMA_BSIZE):
 *
 *	Points to the SPECLIST as a technique for efficiently handling
 *	requests of size zero for preferentially DMAable memory.
 *
 * LISTIDX(KM_PDMA_BSIZE) + 1 -> LISTIDX(KM_PDMA_BSIZE + MAXBUFSZ):
 *
 *	For each index, kmlistp[index] points to the kmlist_t freelist for
 *	allocating preferentially DMAable buffers of size
 *	((index << MINSZSHIFT) - KM_PDMA_BSIZE).
 *
 * LISTIDX(KM_CNDMA_BSIZE):
 *
 *	Points to the SPECLIST as a technique for efficiently handling
 *	requests of size zero for contiguous preferentially non-DMAable memory.
 *
 * LISTIDX(KM_CNDMA_BSIZE) + 1 -> LISTIDX(KM_CNDMA_BSIZE + MAXBUFSZ):
 *
 *	For each index, kmlistp[index] points to the kmlist_t freelist for
 *	allocating contiguous preferentially non-DMAable buffers of size
 *	((index << MINSZSHIFT) - KM_CNDMA_BSIZE).
 */
#define KM_NBASIC_LISTIDX	(LISTIDX(MAXBUFSZ) + 1)
#define KM_N_LISTIDX            ((KMEM_POOL_TYPES + 1) * KM_NBASIC_LISTIDX)
STATIC kmlist_t *kmlistp[KM_N_LISTIDX];

/* Static kmlocal_t for the special list. */
STATIC kmlocal_t kmlocal_speclist;

/* Special ki_nalloc values for the special list */

#define NALLOC_OVERSIZE	USHRT_MAX	/* oversized buffers */
#define NALLOC_PGOUT	(USHRT_MAX - 1)	/* pageout private pool buffers */
#ifdef KMA_PARANOID
#define NALLOC_CONT	(USHRT_MAX - 2)	/* for continuation pages, but only  */
					/* under KMA_PARANOID */
#else
#define NALLOC_CONT	0
#endif

	/*+ KMA freelist lock */
STATIC LKINFO_DECL(kmlist_lkinfo, "MP:KMA:km_lock", 0);

	/*+ mutex for KMA giveback handshaking */
STATIC LKINFO_DECL(kmgiveback_lkinfo, "MP:KMA:kma_giveback_lock", 0);

lock_t kma_giveback_lock;
clock_t kma_lastgive;	/* Time of last forced giveback from kma_force_shrink */
sv_t kma_giveback_sv;
STATIC sv_t kma_completion_sv;
STATIC int kma_spawn_error;	/* Error, if any, during daemon spawning */

/* kma_giveback_action controls what the daemon LWPs do when they wake up */
enum giveback_action {
	GB_IDLE,	/* no action in progress */
	GB_GIVEBACK,	/* normal giveback */
	GB_SHRINK,	/* aggressive giveback */
	GB_EXIT,	/* unbind from the engine and exit */
	GB_SPAWN,	/* spawn a new daemon LWP */
	GB_COMPLETE	/* complete a previous GB_SPAWN action */
};
STATIC enum giveback_action kma_giveback_action;

STATIC uint_t kma_giveback_seq;	/* Incremented for each completed action */

STATIC engine_t * volatile kma_new_eng;

STATIC emask_t kma_live_daemons;
STATIC emask_t kma_targets;

STATIC void kma_giveback_daemon(void *);
STATIC void *kmem_alloc_oversize(size_t, const physreq_t *, int);

extern long kma_giveback_time;

#ifdef DEBUG
uint_t kma_shrinkcount;		/* Statistic: # times KMA pools were shrunk */
#endif

	/* Flag to indicate that kmem_alloc can be used. */
boolean_t kma_initialized = B_FALSE;
	/* Flag to disable kmem_advise after startup. */
STATIC boolean_t kmadv_disabled = B_FALSE;
	/* Flag to indicate the system is up enough to have multiple threads */
STATIC boolean_t kma_system_up_and_running = B_FALSE;

uint_t kma_waitcnt;		/* Count of waiters on all freelists */
uint_t kma_shrinktime = KMA_MAXSHRINKTIME;	/* Next time to shrink pools */
fspin_t kma_lock;		/* Mutex for kma_shrinktime and kma_waitcnt */
#ifdef KMA_PARANOID
fspin_t kma_bitmap_lock;
#endif /* KMA_PARANOID */

#define INCR_WAITCNT() { \
		FSPIN_LOCK(&kma_lock); \
		++kma_waitcnt; \
		FSPIN_UNLOCK(&kma_lock); \
	}
#define DECR_WAITCNT() { \
		FSPIN_LOCK(&kma_lock); \
		--kma_waitcnt; \
		FSPIN_UNLOCK(&kma_lock); \
	}

static vaddr_t kpg_vbase;	/* Base of kpg virtual space */
static ulong_t kpg_vsize;	/* Size of kpg virtual space (in pages) */

STATIC rff_t kma_rff;	/* Private pageout daemon pool; mutexed by kma_lock */
extern size_t kma_pageout_pool_size;	/* Size of private pool (in bytes)
					   for pageout daemon */
STATIC vaddr_t kma_pageout_pool;	/* Address of pageout private pool */

extern void (*kmadv_funcs[])();

#ifdef _KMEM_STATS

/*
 * KMA STATISTICS
 *
 *	kma statistical information is organized into two tables.
 *	The SIZES table holds the number of bytes "owned" (allocated
 *	but not freed) for each size.  The INVOCATIONS table holds the
 *	number of invocations of kma_alloc or kma_free for each size,
 *	for each invocation point, where an invocation point is a
 *	<line number, file name> pair.  Each entry in the SIZES table
 *	is linked to the chain of corresponding INVOCATIONS entries.
 *
 *	The information is gathered by defining kma_alloc, kma_zalloc,
 *	and kma_free to the pre-processor macros (kmem_instr_alloc,
 *	kmem_instr_zalloc, and kmem_instr_free respectively) - see
 *	kmem.h.
 */

/*
 * flag to dynamically turn off statistics collection at run time
 */
int kmem_stats_enabled = 1;

#define KMA_SIZES	3271	/* # of entries in the SIZES table */
#define	KMA_INVOCS	5689	/* # of entries in the INVOCATIONS table */

/*
 * mutex for all KMA STATISTICS data
 */
STATIC fspin_t kma_instr_lock;

/*
 * SIZES table - organized as an open hash table
 */

struct kma_szinfo {
	/*
	 *  information in the hash key
	 */
	uint_t			kmsi_size;	/* size of the allocation */

	/*
	 * information not in the hash key
	 */
	uint_t			kmsi_owned;	/* bytes of memory owned */
	struct kma_invinfo	*kmsi_invp;	/* invocation chain */
};

struct kma_szinfo		kma_size_table[KMA_SIZES];
struct kma_szinfo		*kma_sort_table[KMA_SIZES];
int				kma_nsize;	   /* # of entries */
boolean_t			kma_size_overflow; /* overflow? */

/*
 * INVOCATIONS table - also organized as an open hash table
 */
struct kma_invinfo {
	/*
	 * information in the hash key
	 */
	uint_t			kmvi_size;	/* size of the allocation */
	ushort_t		kmvi_type;	/* alloc, fail, or free */
	ushort_t		kmvi_line;	/* source file line number */
	char			*kmvi_file;	/* name of the source file */

	/*
	 * information not in the hash key
	 */
	uint_t			kmvi_nops;	/* number of operations */
	struct kma_invinfo	*kmvi_chain;	/* invocation chain */
};

struct kma_invinfo		kma_inv_table[KMA_INVOCS];
int				kma_ninv;	  /* # of entries */
boolean_t			kma_inv_overflow; /* invocation? */

/*
 * values for kmvi_type field
 */
#define KMA_INV_ALLOC		0		/* successfull allocation */
#define KMA_INV_FAILED		1		/* failed allocation */
#define KMA_INV_FREE		2		/* free */

#endif /* _KMEM_STATS */

#ifdef _KMEM_HIST

/*
 * KMA History Facility
 *
 *	KMA History information is gathered primarily for the purpose
 *	of finding undisciplined users of kmem_alloc/kmem_free.
 *	The last KMA_HIST_MAX calls to kmem_alloc/kmem_zalloc, and kmem_free
 *	are recorded in the kma_hist_buffer.
 *	
 *	The information is gathered by defining kma_alloc, kma_zalloc,
 *	and kma_free to the pre-processor macros (kmem_instr_alloc,
 *	kmem_instr_zalloc, and kmem_instr_free respectively) - see
 *	kmem.h.
 */

#define KMA_HIST_MAX   10000

/*
 * The history table is composed of kma_log_record structures.
 */
struct kma_log_record {
	char		*klr_svc_name;
	int		klr_line;
	char		*klr_file;
	void		*klr_addr;
	size_t		klr_size;
	lwp_t		*klr_lwp;
	ulong_t		klr_stamp;
} kma_hist_buffer[KMA_HIST_MAX];

/*
 * Index of the oldest entry in the kma_hist_buffer
 */
int			kma_hist_cursor;

/*
 * mutex for the kma_hist_buffer and kma_hist_cursor
 */
STATIC fspin_t kma_hist_lock;

#endif /* _KMEM_HIST */

/*
 * void
 * kma_calloc(void)
 *	Called at calloc time to do callocs needed for KMA.
 *
 * Calling/Exit State:
 *	No parameters.
 *	Must be called after segkmem_init().
 */
void
kma_calloc(void)
{
	/*
	 * We need to allocate an array based on the size of kpg_alloc()
	 * virtual space.  We get this from kpg_vaddr_limits(), but at this
	 * point it is just an upper bound, since further calloc()s may
	 * decrease the available virtual memory size.  We live with the
	 * possible wasted array elements.  Note that even though kpg_vbase
	 * is not valid at this point, we set it again in kma_init() before
	 * we use it.
	 */
	kpg_vaddr_limits(&kpg_vbase, &kpg_vsize);
	kminfo = (kminfo_t *)calloc(btop(kpg_vsize) * sizeof(kminfo_t));

	/*
	 * Allocate space for a small pool of overflow memory for the
	 * pageout daemon.  This will be used by kmem_alloc if no memory
	 * is available through the regular mechanisms, rather than blocking
	 * in the pageout context.
	 */
	if (kma_pageout_pool_size < PAGESIZE)
		kma_pageout_pool_size = PAGESIZE;
	kma_pageout_pool = (vaddr_t)calloc_physio(kma_pageout_pool_size);
}

/*
 * void
 * kma_init(void)
 *	Called after calloc time to do remaining KMA initialization.
 *
 * Calling/Exit State:
 *	No parameters.
 *	Must be called after segkmem_create().
 */
void
kma_init(void)
{
	uint_t i, idx, n, last_n;
	void (**funcpp)();
	vaddr_t pool;
	kmlist_t *listp;
	kminfo_t *kminfop;
	static int npage[] = {NPAGE16, NPAGE32, NPAGE64, NPAGE128,
			      NPAGE256, NPAGE512, NPAGE1024, NPAGE2048,
			      NPAGE4096, NPAGE8192};
#ifdef NO_RDMA
	static ulong_t scale[] = {1};
	const int j = 1;
#else /* !NO_RDMA */
	int j;
	static ulong_t scale[] = {1, 1, 1};
	static uint_t pflags[] = {0, P_DMA, P_NODMA};
#endif /* NO_RDMA */

#ifndef lint
	/* Make sure kmfree_t structure isn't too big. */
	ASSERT(sizeof(kmfree_t) <= MINBUFSZ);
#endif

	/*
	 * Now that virtual address allotments are fixed,
	 * we can get the exact base and size of the kpg segment.
	 */
	kpg_vaddr_limits(&kpg_vbase, &kpg_vsize);
	kpg_vsize = btop(kpg_vsize);

	FSPIN_INIT(&kma_lock);

#ifdef _KMEM_STATS
	FSPIN_INIT(&kma_instr_lock);
#endif /* _KMEM_STATS */
#ifdef _KMEM_HIST
	FSPIN_INIT(&kma_hist_lock);
#endif /* _KMEM_HIST */
#ifdef KMA_PARANOID
	FSPIN_INIT(&kma_bitmap_lock);
#endif /* KMA_PARANOID */

	ASSERT(NFIXEDBUF == sizeof(npage)/sizeof(npage[0]));

#ifndef NO_RDMA
	/*
	 * Mode RDMA_DISABLED is distinguished from RDMA_SMALL and
	 * RDMA_LARGE by the two properties:
	 *	1) there is only one set of KMA pools, and
	 *	2) all pages are STD_PAGEs.
	 * Adjust the pools and the kmem_alloc() flag translation tables
	 * appropriately.
	 */
	if (rdma_mode == RDMA_DISABLED || rdma_mode == RDMA_MICRO) {
		km_flag_mtype[KM_SLEEP|KM_REQ_DMA] = M_KERNEL_ALLOC;
		km_flag_mtype[KM_NOSLEEP|KM_REQ_DMA] = M_KERNEL_ALLOC;
		km_flag_mtype[KM_SLEEP|KM_REQ_DMA|KM_PHYSCONTIG] =
							M_KERNEL_ALLOC;
		km_flag_mtype[KM_NOSLEEP|KM_REQ_DMA|KM_PHYSCONTIG] =
							M_KERNEL_ALLOC;
		npools = 1;
		minpool = dmapool = ndmapool = 1;
		maxpool = varpool = 1 + NFIXEDBUF;
	}
#endif /* NO_RDMA */
	km_grow_start = &blist[maxpool - 1];
	scale[npools - 1] = 1 + max_freemem() / KMA_GRADIENT;

#if SPECLIST_IDX != 0
#error kma_init implementation assumes SPECLIST_IDX equals 0
#endif

	/*
	 * The following loops assume the NFIXEDBUF buffers are the first
	 * buffers in blist[] and are the powers of 2 from MINBUFSZ to
	 * MAXBUFSZ, in order.
	 */

	listp = &blist[1];
	for (i = 1; i < varpool; ++i) {
#ifdef NO_RDMA
		idx = i - 1;
#else /* !NO_RDMA */
		idx = (i - 1) % NFIXEDBUF;
		j = (i - 1) / NFIXEDBUF;
#endif /* NO_RDMA */
		LOCK_INIT(&listp->km_lock, VM_KMA_HIER, VM_KMA_IPL,
			  &kmlist_lkinfo, KM_NOSLEEP);
		SV_INIT(&listp->km_wait);
		listp->km_bsize = (MINBUFSZ << idx);
		listp->km_npage = min(scale[j] * npage[idx],
					KMA_POOL_ALLOC_MAX);
		ASSERT(((ulong_t)ptob(listp->km_npage) >> (idx + MINSZSHIFT)) <
		       (1L << (NBBY * sizeof(ushort_t))));
		listp->km_nbpc =
			((uint_t)ptob(listp->km_npage) >> (idx + MINSZSHIFT));
		ASSERT((int)listp->km_nbpc > 1);
		listp->km_cachesz = ((int)listp->km_nbpc - 1) / 2;
		MET_KMA_INIT(i, idx, listp->km_bsize);
#ifndef NO_RDMA
		listp->km_pflags = pflags[j];
#endif /* NO_RDMA */
		++listp;
	}

	/*
	 * Initialize the local lists.
	 *
	 *	This is a bit overzealous unless we are actually going to
	 *	transition to RDMA_SMALL, since many of the blist elements
	 *	will never be used. However, this overzealousness does no
	 *	harm.
	 */
	for (i = 0; i < KM_NPOOLS; ++i)
		blist[i + 1].km_localp = &l.kmlocal[i];

	/*
	 * Initialize the kmlistp array.
	 */
	n = (1 << ((i = NFIXEDBUF) - 1));
	while (i-- != 0) {
		last_n = (n >> 1);
		while (n > last_n) {
			/*
			 * For model RDMA_LARGE, KM_DMA (preferentially
			 * DMAable) requests are satisfied by the non-DMAable
			 * pool. If restricted DMA is not in effect, then
			 * KM_DMA requests are satisfied from the one (and
			 * only) pool.
			 */
			kmlistp[LISTIDX(KM_NDMA_BSIZE) + n] =
					&blist[i + ndmapool];
			
			/*
			 * Special rule for kmlistp lists which may be used for
			 * physically contiguous allocations: do not use the
			 * pools for requests larger than PAGESIZE.
			 */
			if (n > PAGESIZE/MINBUFSZ) {
				kmlistp[LISTIDX(KM_PDMA_BSIZE) + n] =
					kmlistp[LISTIDX(KM_DMA_BSIZE) + n] =
					kmlistp[LISTIDX(KM_CNDMA_BSIZE) + n] =
						SPECLIST;
			} else {
				kmlistp[LISTIDX(KM_PDMA_BSIZE) + n] =
					&blist[i + ndmapool];
				kmlistp[LISTIDX(KM_DMA_BSIZE) + n] =
					&blist[i + dmapool];
				kmlistp[LISTIDX(KM_CNDMA_BSIZE) + n] =
					&blist[i + ndmapool];
			}
			--n;
		}
	}

	/*
	 * Initialize the special blist, which is used to optimize size 0
	 * allocations, oversized frees, and pageout private pool frees.
	 */
	LOCK_INIT(&SPECLIST->km_lock, VM_KMA_HIER, VM_KMA_IPL,
		  &kmlist_lkinfo, KM_NOSLEEP);
	SPECLIST->km_localp = &kmlocal_speclist;
	kmlistp[LISTIDX(KM_DMA_BSIZE)] = SPECLIST;
	kmlistp[LISTIDX(KM_NDMA_BSIZE)] = SPECLIST;
	kmlistp[LISTIDX(KM_PDMA_BSIZE)] = SPECLIST;
	kmlistp[LISTIDX(KM_CNDMA_BSIZE)] = SPECLIST;
	MET_KMA_INIT(SPECLIST_IDX, MET_KMOVSZ, 0);

	/*
	 * Map the pageout private pool space into KPG virtual, so it will be
	 * covered by kminfo structures.
	 */
	n = btopr(kma_pageout_pool + kma_pageout_pool_size) -
		   btop(kma_pageout_pool);
	pool = kpg_vm_alloc(n, NOSLEEP);
	if (pool == (vaddr_t)NULL) {
		/*
		 *+ Boot-time allocation of virtual memory for the pageout
		 *+ daemon private memory pool failed.  This probably indicates
		 *+ that the system is configured with either too little
		 *+ segkmem virtual or too big of a pageout KMA pool.
		 */
		cmn_err(CE_PANIC, "Not enough memory for pageout KMA pool");
		/* NOTREACHED */
	}
	kminfop = &kminfo[IDX(pool) + n];
	pool += ptob(n);
	kma_pageout_pool += ptob(n);
	do {
		pool -= PAGESIZE;
		kma_pageout_pool -= PAGESIZE;
		segkmem_ppid_mapin(kpgseg, pool, 1,
				   kvtoppid((caddr_t)kma_pageout_pool),
				   PROT_KMA);
		/*
		 * Set the info for all of the private pool pages so that they
		 * refer to the special blist.
		 */
		(--kminfop)->ki_nalloc = NALLOC_PGOUT;
	} while (--n != 0);
	kma_pageout_pool = pool + (kma_pageout_pool & PAGEOFFSET);

	/*
	 * Initialize a rotating first-fit allocator for the private pool.
	 */
	rff_init(&kma_rff);
	rff_add_chunk(&kma_rff,
		      (void *)kma_pageout_pool, kma_pageout_pool_size);

	/*
	 * Indicate that kmem_alloc is now useable.
	 */

	kma_initialized = B_TRUE;

	/* Call out to all _kmadv functions so they can call kmem_advise(). */
	for (funcpp = kmadv_funcs; *funcpp;)
		(*(*funcpp++))();

	/* Disable kmem_advise() since it's not multi-threaded. */
	kmadv_disabled = B_TRUE;

	/* Initializations needed for kma_giveback_daemon(). */
	LOCK_INIT(&kma_giveback_lock, VM_KMA_GIVEBACK_HIER, VM_KMA_GIVEBACK_IPL,
		  &kmgiveback_lkinfo, KM_NOSLEEP);
	SV_INIT(&kma_giveback_sv);
	SV_INIT(&kma_completion_sv);
	EMASK_CLRALL(&kma_live_daemons);
	EMASK_CLRALL(&kma_targets);
	kma_giveback_action = GB_IDLE;
}

/*
 * void
 * kma_postroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
kma_postroot(void)
{
	/* Spawn the giveback daemon LWP for this engine. */
	if (spawn_lwp(NP_SYSPROC|NP_FAILOK, NULL, LWP_DETACHED, NULL,
		      kma_giveback_daemon, l.eng) != 0) {
		/*
		 *+ Could not create the first LWP for the KMA
		 *+ giveback daemon.  This is probably due to lack of memory.
		 */
		cmn_err(CE_PANIC, "Failed to create kernel LWP for KMA");
		/* NOTREACHED */
	}

	/*
	 * Start the periodic timer for the giveback daemon.
	 */
	if (itimeout(kma_do_giveback, (void *)NULL,
		     kma_giveback_time | TO_PERIODIC, PLBASE) == 0) {
		/*
		 *+ Could not set up a timeout for KMA to trigger the
		 *+ giveback daemon.  This is probably due to insufficient
		 *+ memory.
		 */
		cmn_err(CE_PANIC, "Can't get timeout for KMA giveback daemon");
		/* NOTREACHED */
	}

	kma_system_up_and_running = B_TRUE;
}

#ifndef NO_RDMA
/*
 * void
 * kma_switch_small(boolean_t is_mini)
 *	Activate the preferentially DMAable pools, thus transitioning KMA
 *	from model RDMA_LARGE to RDMA_SMALL.
 *
 * Calling/Exit State:
 *	Normally called from rdma_convert(), which is invoked by main()
 *	just before the vfs_mountroot(), but only when the system is
 *	actually transitioning to model RDMA_SMALL.
 *
 *	In the case of the mini-kernel, called from mcompat_init().
 *
 * Remarks:
 *	If this is a mini-kernel, then we must provide a kmem_alloc()
 *	interface compatibile with 4.2 drivers, even though Driver.o files
 *	may not have been processed for smap based symbol translation. This
 *	means that the a kmem_alloc() request without a KM_REQ_DMA flag
 *	must be processed as though just a flag was present.
 */
void
kma_switch_small(boolean_t is_mini)
{
	uint_t i, n, last_n;
	
	/*
	 * The following depend on atomic access to integers.
	 */
	minpool = 1;
	
	/*
	 * Re-initialze the preferred DMAable list within the kmlistp array
	 * to point to the preferentially DMAable pools.
	 */
	n = (1 << ((i = NFIXEDBUF) - 1));
	while (i-- != 0) {
		last_n = (n >> 1);
		while (n > last_n) {
			kmlistp[LISTIDX(KM_PDMA_BSIZE) + n] =
				&blist[i + pdmapool];
			--n;
		}
	}

	if (is_mini) {
		/*
		 * For the mini-kenrel, no flags to kmem_alloc() is
		 * equivalent to KM_REQ_DMA.
		 */
		for (i = 0; i < LISTIDX(MAXBUFSZ + MINBUFSZ); ++i)
			kmlistp[LISTIDX(KM_NDMA_BSIZE) + n] =
				kmlistp[LISTIDX(KM_CNDMA_BSIZE) + n] =
					kmlistp[LISTIDX(KM_DMA_BSIZE) + n];

		/*
		 * The following depends upon the fact that the mini-kernel
		 * is UNIPROC. Also, even an MP is in effect UNIPROC at this
		 * point, and will remain that way until main() begins to spawn.
		 */
		DISABLE();
		km_flag_xlat[KM_SLEEP] = km_flag_xlat[KM_REQ_DMA|KM_SLEEP];
		km_flag_xlat[KM_NOSLEEP] = km_flag_xlat[KM_REQ_DMA|KM_NOSLEEP];
		km_flag_mtype[KM_SLEEP] = km_flag_mtype[KM_REQ_DMA|KM_SLEEP];
		km_flag_mtype[KM_NOSLEEP] =
			km_flag_mtype[KM_REQ_DMA|KM_NOSLEEP];
		km_flag_xlat[KM_SLEEP|KM_PHYSCONTIG] =
			km_flag_xlat[KM_REQ_DMA|KM_SLEEP];
		km_flag_xlat[KM_NOSLEEP|KM_PHYSCONTIG] =
			km_flag_xlat[KM_REQ_DMA|KM_NOSLEEP];
		km_flag_mtype[KM_SLEEP|KM_PHYSCONTIG] =
			km_flag_mtype[KM_REQ_DMA|KM_SLEEP];
		km_flag_mtype[KM_NOSLEEP|KM_PHYSCONTIG] =
			km_flag_mtype[KM_REQ_DMA|KM_NOSLEEP];
		ENABLE();
	}
}
#endif /* NO_RDMA */

/*
 * void *
 * kmem_alloc(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size.
 *
 * Calling/Exit State:
 *	The size argument gives the size in bytes of the desired chunk
 *	of memory; the actual size allocated will be at least this big.
 *	The flags argument may specify either KM_SLEEP or KM_NOSLEEP;
 *	KM_NOSLEEP indicates that the caller does not want to block if
 *	memory is not available.
 *
 *	In addition, the following flags are accepted.
 *
 *		KM_DMA		Indicates that the caller prefers DMAable
 *				memory.
 *
 *		KM_REQ_DMA	Indicates that the caller requires DMAable
 *				memory. 
 *
 *		KM_PHYSCONTIG	Indicates that the caller requires
 *				physically contiguous memory.
 *
 *	Specifying both KM_REQ_DMA and KM_DMA is erroneous.
 *	Specifying both KM_PHYSCONTIG and KM_DMA is erroneous.
 *
 *	Returns a (virtual) pointer to the allocated memory; if called
 *	with KM_NOSLEEP and memory is not immediately available, NULL
 *	is returned.
 *
 *	KMA locks are used internally, but no locks are required on
 *	entry or held on exit.
 *
 * Remarks:
 *	This function is called very frequently. Changes to this function
 *	should be carefully measured for their performance effects.
 *
 *	WARNING: KM_DMA, KM_REQ_DMA, and KM_PHYSCONTIG are not defined
 *		 in the DDI.
 */
void *
kmem_alloc(size_t size, int flags)
{
	kmlist_t *listp;
	kmlocal_t *localp;
	kmfree_t **lfreep;
	kminfo_t *infop;
	kmfree_t *freep;
	void *bufp;
	void *memp;
	boolean_t need_wakeup;
	pl_t oldpri;
	int i, page_no;

	ASSERT(kma_initialized);
	ASSERT((flags & KM_NOSLEEP) || KS_HOLD0LOCKS());
	ASSERT((flags & KM_NOSLEEP) || !servicing_interrupt());
	ASSERT((flags & ~KM_VALID_FLAGS) == 0);
	ASSERT((flags & (KM_DMA|KM_REQ_DMA)) != (KM_DMA|KM_REQ_DMA));
	ASSERT((flags & (KM_DMA|KM_PHYSCONTIG)) != (KM_DMA|KM_PHYSCONTIG));

#ifdef DEBUG
	/*
	 * Under DEBUG, the FAIL_KMEM_ALLOC flag can be used by tests
	 * to simulate memory exhaustion.
	 */
	if ((u.u_debugflags & FAIL_KMEM_ALLOC) && (flags & KM_NOSLEEP) &&
	    !servicing_interrupt())
		return NULL;
#endif

	/*
	 * Oversized requests are handled seperately.
	 */
	if (size > MAXBUFSZ)
		return kmem_alloc_oversize(size, NULL, flags);

	listp = kmlistp[LISTIDXF(size, flags)];
	lfreep = &(localp = listp->km_localp)->lfree;

	/*
	 * First try to get something from the local list.
	 */
	DISABLE();
	if ((bufp = (void *)(*lfreep)) != NULL) {
		ASSERT(listp != SPECLIST);
got_local:
#ifdef KMA_PARANOID
		if (localp->lnfree == 0) {
			ENABLE();
			FREE_CORRUPT(bufp);
		}
		CHECK_FREEB((kmfree_t *)bufp, ENABLE(););
#endif /* KMA_PARANOID */
		*lfreep = ((kmfree_t *)bufp)->kf_next;
		localp->lnfree--;
		ENABLE();
#ifdef KMA_PARANOID
		((kmfree_t *)bufp)->kf_magic = 0;
		SET_TAIL_MAGIC(bufp, size, listp->km_bsize);
#endif /* KMA_PARANOID */
		MET_KMEM_ALLOC(listp, listp->km_bsize);
		MET_KMEM_REQ(listp, size);
		return bufp;
	}
	ENABLE();

	/*
	 * Now try to get something from the global free list.
	 * For this, we need the km_lock.
	 */
global_startover:
	oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);
tryagain:
	if ((bufp = (void *)listp->km_free) != NULL) {
		ASSERT(listp != SPECLIST);
		CHECK_FREEB((kmfree_t *)bufp, /**/);
		listp->km_free = ((kmfree_t *)bufp)->kf_next;
		((kmfree_t *)bufp)->kf_infop->ki_nalloc++;
		need_wakeup = B_FALSE;
got_one:
		/*
		 * While we're here, grab a bunch of buffers and
		 * move them to the local list, unless there are
		 * already some buffers on the local list.
		 */
		if ((freep = listp->km_free) != NULL && localp->lnfree == 0) {
			CHECK_FREEB(freep, /**/);
			if ((i = listp->km_cachesz) > 0) {
				localp->lnfree = i;
				*lfreep = freep;
				freep->kf_infop->ki_nalloc++;
				while (--i != 0 && freep->kf_next != NULL) {
					freep = freep->kf_next;
					CHECK_FREEB(freep, /**/);
					freep->kf_infop->ki_nalloc++;
				}
				ASSERT(localp->lnfree > i);
				localp->lnfree -= i;
				listp->km_free = freep->kf_next;
				freep->kf_next = NULL;
			}
		}
		UNLOCK(&listp->km_lock, oldpri);
		if (need_wakeup)
			SV_BROADCAST(&listp->km_wait, 0);
#ifdef KMA_PARANOID
		((kmfree_t *)bufp)->kf_magic = 0;
		SET_TAIL_MAGIC(bufp, size, listp->km_bsize);
#endif /* KMA_PARANOID */
		MET_KMEM_ALLOC(listp, listp->km_bsize);
		MET_KMEM_REQ(listp, size);
		return bufp;
	}

	/*
         * Ensure that only one LWP is trying to acquire memory for this
	 * freelist at one time.  This is serialized with the 
	 * KMA_GROW_PENDING state,
	 *
	 * We don't just use the km_lock we already hold so that frees and
	 * other allocs which could pick up those freed buffers can proceed
	 * in parallel with us.
	 */

	if (listp->km_grow_flags & KMA_GROW_PENDING) {
		ASSERT(listp != SPECLIST);
		UNLOCK(&listp->km_lock, oldpri);
		/*
		 * Wait until something changes, while not holding the km_lock.
		 * The test is stale, but we test both conditions again
		 * after reacquiring the lock.
		 */
		while (((kmlist_t volatile *)listp)->km_free == NULL &&
			(((kmlist_t volatile *)listp)->km_grow_flags & 
							KMA_GROW_PENDING))
			;
		goto global_startover;
	}

	if (listp == SPECLIST) {
		UNLOCK(&listp->km_lock, oldpri);
		if (size == 0) {
			/*
			 * DDI/DKI requires size 0 to return NULL. We were
			 * able to delay the check until this point, since we
			 * set up an always-empty buffer list for size 0.
			 */
			return NULL;
		}

		/*
		 * What we have here is an oversized allocation request for
		 * DMAable or physically contiguous memory, where the size is
		 * between PAGESIZE and MAXBUFSZ. We send it on to
		 * kmem_alloc_oversize().
		 */
		ASSERT(size > PAGESIZE && size <= MAXBUFSZ);
		return kmem_alloc_oversize(size, NULL, flags);
	}

	/*
	 * We leave interrupts disabled here since we don't want to be context
	 * switched out while in the KMA_GROW_PENDING state (we don't want
	 * the waiters to be spinning for too long) and since we can't
	 * tolerate an interrupt coming in and getting into kmem_alloc for
	 * the same pool size.
	 */
	ASSERT(listp != SPECLIST);
	listp->km_grow_flags |= KMA_GROW_PENDING;
	UNLOCK(&listp->km_lock, VM_KMA_IPL);

	/*
	 * NOTE: Much of the following code is similar to code in
	 * kma_growpools() which acquires more memory and puts it onto a
	 * freelist; it was not made into a common subroutine due to the
	 * performance needs of kmem_alloc().
	 */
	if ((memp = kpg_alloc(listp->km_npage, PROT_KMA,
			      NOSLEEP | KM_PFLAGS(listp))) != NULL) {
		infop = &kminfo[IDX((vaddr_t)memp)];
		ASSERT(infop->ki_nalloc == 0);
		ASSERT(infop->ki_blist_idx == 0);
		ASSERT(infop->ki_page_no == 0);
		infop->ki_nalloc = 1;	/* the one being allocated for
					 * the caller */
		infop->ki_blist_idx = listp - blist;
		bufp = memp;
		NEXT(memp, listp);

		(void) LOCK(&listp->km_lock, VM_KMA_IPL);

		if (listp->km_grow_flags & KMA_NOSLEEP_FAILED) {
			ASSERT(kma_waitcnt != 0);
			DECR_WAITCNT();
		}

		ASSERT(listp->km_grow_flags & KMA_GROW_PENDING);
		listp->km_grow_flags = 0;

		for (i = (int)listp->km_nbpc; i-- > 1;) {
			((kmfree_t *)memp)->kf_infop = infop;
#ifdef KMA_PARANOID
			((kmfree_t *)memp)->kf_magic = KMA_MAGIC;
			((kmfree_t *)memp)->kf_slink = SLINK_UNUSED;
#endif /* KMA_PARANOID */
			LINK(listp, (kmfree_t *)memp);
			NEXT(memp, listp);
		}
		for (page_no = 1; page_no < (int)listp->km_npage; ++page_no) {
			++infop;
			ASSERT(infop->ki_nalloc == 0);
			ASSERT(infop->ki_blist_idx == 0);
			ASSERT(infop->ki_page_no == 0);
			infop->ki_page_no = page_no;
#ifdef KMA_PARANOID
			infop->ki_nalloc = NALLOC_CONT;
#endif /* KMA_PARANOID */
		}

		/*
		 * Even if we give away half our buffers to the local list,
		 * plus one to satisfy this request, there will still be at
		 * least one buffer left, so we wakeup any waiters.
		 */
		ASSERT((int)listp->km_nbpc - (int)listp->km_cachesz - 1 >= 1);
		need_wakeup = SV_BLKD(&listp->km_wait);
		MET_KMEM_MEM(listp, listp->km_npage);
		goto got_one;
	}

	(void) LOCK(&listp->km_lock, VM_KMA_IPL);

	ASSERT(listp->km_grow_flags & KMA_GROW_PENDING);
	listp->km_grow_flags &= ~KMA_GROW_PENDING;

	if (listp->km_free)
		goto tryagain;

	/*
	 * We're out of memory.  Fail if NOSLEEP, or try again later.
	 */

	if (flags & KM_NOSLEEP) {
		if (!(listp->km_grow_flags & KMA_NOSLEEP_FAILED)) {
			listp->km_grow_flags = KMA_NOSLEEP_FAILED;
			INCR_WAITCNT();
		}
		UNLOCK(&listp->km_lock, oldpri);
		MET_KMEM_FAIL(listp);
		return NULL;
	}

	/*
	 * Normal memory allocation failed.  If we're running in the
	 * pageout daemon, try its private pool before giving up.
	 */
	ASSERT(!servicing_interrupt());
	if (u.u_flags & U_CRITICAL_MEM) {
		FSPIN_LOCK(&kma_lock);
		bufp = rff_alloc(&kma_rff, size);
		FSPIN_UNLOCK(&kma_lock);
		if (bufp != NULL) {
			UNLOCK(&listp->km_lock, oldpri);
			ASSERT(kminfo[IDX((vaddr_t)bufp)].ki_blist_idx ==
				SPECLIST_IDX);
			return bufp;
		}
	}

	/*
	 * If the system has not yet been fully initialized,
	 * there's no hope of getting more memory, even if we wait.
	 * (There's no one else to free up the memory.)
	 * Just panic in this case, to let the user know what's going on.
	 */
	if (!kma_system_up_and_running) {
		/*
		 *+ Attempted allocation of memory during system initialization
		 *+ failed.  This indicates that the system is improperly
		 *+ configured, or something is trying to allocate an
		 *+ excessive amount of memory.  Check to see if sufficient
		 *+ segkmem virtual is configured, and that there are no
		 *+ excessive tunables.
		 */
		cmn_err(CE_PANIC, "Out of KMA memory during startup");
		/* NOTREACHED */
	}

	INCR_WAITCNT();
	SV_WAIT(&listp->km_wait, PRIMEM, &listp->km_lock);
	DECR_WAITCNT();

	/* Now that we've blocked, check the local list again. */
	DISABLE();
	if ((bufp = (void *)(*lfreep)) != NULL)
		goto got_local;
	ENABLE();
	(void) LOCK(&listp->km_lock, VM_KMA_IPL);
	goto tryagain;
	/* NOTREACHED */
}

/*
 * void
 * kmem_free(void *addr, size_t size)
 *	Release memory allocated via kmem_alloc(), kmem_zalloc(),
 *	or kmem_alloc_physreq().
 *
 * Calling/Exit State:
 *	The addr argument points to the memory to be freed.
 *	The size argument is the size passed to the corresponding
 *	kmem_alloc(), kmem_zalloc(), or kmem_alloc_physreq() call.
 *
 *	KMA locks are used internally, but no locks are required on
 *	entry or held on exit.
 *
 * Remarks:
 *	This function is called very frequently. Changes to this function
 *	should be carefully measured for their performance effects.
 */
void
kmem_free(void *addr, size_t size)
{
	kmlist_t *listp;
	kmlocal_t *localp;
	kmfree_t **lfreep;
	kminfo_t *infop;
	pl_t oldpri;
#ifdef DEBUG
	int page_no;
#endif

	ASSERT(kma_initialized);

	if (size == 0) {	/* DDI/DKI requires handling size of 0. */
#ifdef KMA_PARANOID
		if (addr != NULL)
			FREE_FAIL("address not NULL when size is 0");
#endif /* KMA_PARANOID */
		return;
	}

#ifdef KMA_PARANOID
	if ((vaddr_t)addr < kpg_vbase ||
	    (vaddr_t)addr >= kpg_vbase + ptob(kpg_vsize))
		FREE_FAIL("address not a valid KMA address");
#endif /* KMA_PARANOID */

	infop = &kminfo[IDX((vaddr_t)addr)];
#ifdef DEBUG
	page_no = infop->ki_page_no;
#endif
	infop -= infop->ki_page_no;
	ASSERT(infop->ki_page_no == 0);
	listp = &blist[infop->ki_blist_idx];
	ASSERT(page_no <= (int)listp->km_npage);
	lfreep = &(localp = listp->km_localp)->lfree;
#ifdef KMA_PARANOID
	if (infop->ki_nalloc == 0)
		FREE_FAIL("nothing allocated at address");
	if (listp != SPECLIST) {
		if (listp != kmlistp[LISTIDXF(size, 0)] &&
		      listp != kmlistp[LISTIDXF(size, KM_DMA)] &&
		      listp != kmlistp[LISTIDXF(size, KM_REQ_DMA)] &&
		      listp != kmlistp[LISTIDXF(size, KM_PHYSCONTIG)])
			FREE_FAIL("size freed differs from size allocated");
		if ((((vaddr_t)addr - INFOP_TO_ADDR(infop)) %
				listp->km_bsize) != 0)
			FREE_FAIL("address not at beginning of a buffer");
		CHECK_TAIL_MAGIC(addr, size, listp->km_bsize);
	}
#endif /* KMA_PARANOID */

	/*
	 * Just put the buffer onto local free list unless there's
	 * already a bunch there.  This is a stale check, but completely
	 * benign in both directions.
	 */
	if (localp->lnfree < listp->km_nbpc) {
		ASSERT(listp != SPECLIST);
		DISABLE();
#ifdef KMA_PARANOID
		if (infop->ki_nalloc == 0) {
			ENABLE();
			FREE_FAIL("all buffers in chunk already free");
		}
		if (*lfreep != NULL)
			CHECK_FREEB(*lfreep, ENABLE(););
		((kmfree_t *)addr)->kf_magic = KMA_MAGIC;
		((kmfree_t *)addr)->kf_slink = SLINK_UNUSED;
#endif /* KMA_PARANOID */
		((kmfree_t *)addr)->kf_infop = infop;
		((kmfree_t *)addr)->kf_next = *lfreep;
		*lfreep = ((kmfree_t *)addr);
		localp->lnfree++;
		ENABLE();
		MET_KMEM_ALLOC(listp, -listp->km_bsize);
		MET_KMEM_REQ(listp, -size);
		return;
	}

	/*
	 * Check for special memory.  We were able to delay the check until
	 * this point, by marking this memory with the special list, SPECLIST.
	 */
	if (listp == SPECLIST) {
		ASSERT(infop->ki_blist_idx == SPECLIST_IDX);
		if (infop->ki_nalloc == NALLOC_PGOUT) {
			/*
			 * This memory came from the pageout private pool,
			 * which needs to be freed specially.
			 */
			FSPIN_LOCK(&kma_lock);
			rff_free(&kma_rff, addr, size);
			FSPIN_UNLOCK(&kma_lock);
			return;
		}
		/* Oversized buffers just go directly back to VM. */
#ifdef KMA_PARANOID
		/* Covered kminfo slots should be a single oversize buffer. */
		if (infop->ki_nalloc != NALLOC_OVERSIZE ||
		    ((vaddr_t)addr & PAGEOFFSET) != 0) {
			FREE_FAIL("address not at beginning of a buffer");
		}
		while (++infop != &kminfo[kpg_vsize] &&
		       infop->ki_nalloc == NALLOC_CONT)
			;
		if (infop - &kminfo[IDX((vaddr_t)addr)] != btopr(size)) {
			FREE_FAIL(
			    "size freed is different from size allocated");
		}
		infop = &kminfo[IDX((vaddr_t)addr)];
		do {
			infop->ki_nalloc = 0;
			ASSERT(infop->ki_blist_idx == SPECLIST_IDX);
			ASSERT(infop->ki_page_no == SPECLIST_IDX);
		} while ((++infop)->ki_nalloc == NALLOC_CONT);
#else /* !KMA_PARANOID */
		ASSERT(infop->ki_nalloc == NALLOC_CONT);
		ASSERT(infop->ki_page_no == 0);
		infop->ki_nalloc = 0;
#endif /* KMA_PARANOID */
		kpg_free(addr, btopr(size));
		MET_KMEM_ALLOC(SPECLIST, -((size + PAGEOFFSET) & PAGEMASK));
		MET_KMEM_REQ(SPECLIST, -size);
		return;
	}

	/*
	 * Otherwise, put the buffer onto the global free list.
	 * For this, we need the km_lock.
	 */
	oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);

#ifdef KMA_PARANOID
	if (infop->ki_nalloc == 0)
		FREE_FAIL("all buffers in chunk already free");
	if (listp->km_free != NULL)
		CHECK_FREEB(listp->km_free, /**/);
	((kmfree_t *)addr)->kf_magic = KMA_MAGIC;
	((kmfree_t *)addr)->kf_slink = SLINK_UNUSED;
#endif /* KMA_PARANOID */

	infop->ki_nalloc--;
	((kmfree_t *)addr)->kf_infop = infop;
	LINK(listp, (kmfree_t *)addr);

	if (SV_BLKD(&listp->km_wait)) {
		UNLOCK(&listp->km_lock, oldpri);
		SV_SIGNAL(&listp->km_wait, 0);
	} else
		UNLOCK(&listp->km_lock, oldpri);

	MET_KMEM_ALLOC(listp, -listp->km_bsize);
	MET_KMEM_REQ(listp, -size);
}

/*
 * void *
 * kmem_zalloc(size_t size, int flags)
 *	Allocate and zero-out memory.
 *
 * Calling/Exit State:
 *	Same as kmem_alloc(), except that the memory is zeroed.
 */
void *
kmem_zalloc(size_t size, int flags)
{
	void *mem;

	mem = kmem_alloc(size, flags);
	if (mem)
		bzero(mem, size);
	return mem;
}

/*
 * void *
 * kmem_alloc_physreq(size_t size, physreq_t *physreq, int flags)
 *	Allocate memory with physical properties constrained by
 *	the caller.
 *
 * Calling/Exit State:
 *	The size argument gives the size in bytes of the desired chunk
 *	of memory; the actual size allocated will be at least this big.
 *
 *	The allocated memory will start at a physical address which is a
 *	multiple of phys_align, and will not cross a boundary which is a
 *	multiple of boundary. Both phys_align and boundary must be powers
 *	of two (except that boundary may be zero).
 *
 *	The allocated memory will be physically contiguous if the
 *	KM_PHYSCONTIG flag is specified, if either of PREQ_PHYSCONTIG or
 *	boundary are specified, or if aligment is greater than pagesize.
 *
 *	The KM_SLEEP, KM_NOSLEEP, KM_REQ_DMA, and KM_PHYSCONTIG flags have
 *	the same meanings as for kmem_alloc(). The KM_DMA flag is not
 *	accepted by this function.
 *
 *	Returns a (virtual) pointer to the allocated memory; if called
 *	with KM_NOSLEEP and memory is not immediately available, NULL
 *	is returned.
 *
 *	No locks are required on entry or held on exit.
 *
 * Remarks:
 *	WARNING: KM_REQ_DMA, and KM_PHYSCONTIG are not defined
 *		 in the DDI.
 */

void *
kmem_alloc_physreq(size_t size, const physreq_t *physreq, int flags)
{
	ASSERT(size != 0);
	ASSERT(physreq->phys_flags & PREQ_PREPPED);
	ASSERT(physreq->phys_align != 0);
	ASSERT(physreq->phys_boundary == 0 || size <= physreq->phys_boundary);
	ASSERT((flags & KM_NOSLEEP) || getpl() == PLBASE);
	ASSERT((flags & KM_NOSLEEP) || KS_HOLD0LOCKS());
	ASSERT((flags & ~(KM_VALID_FLAGS - KM_DMA)) == 0);

#ifdef DEBUG
	/*
	 * Under DEBUG, the FAIL_KMEM_ALLOC flag can be used by tests
	 * to simulate memory exhaustion.
	 */
	if ((u.u_debugflags & FAIL_KMEM_ALLOC) && (flags & KM_NOSLEEP) &&
	    !servicing_interrupt())
		return NULL;
#endif

#ifndef NO_RDMA
	switch (RDMA_REQUIREMENT(physreq)) {
	case RDMA_REQUIRED:
		flags |= KM_REQ_DMA;
		break;
	case RDMA_IMPOSSIBLE:
		return NULL;
	}
#endif /* NO_RDMA */

	/*
	 * Try to use kmem_alloc if possible. Some constraints:
	 *
	 *	(1) If the request requires more than one page, then
	 *	    discontiguous memory might result, or worse an
	 *	    infinite recursion might result.
	 *
	 *	(2) If this is the pageout daemon, then the rff allocator
	 *	    can only provide sizeof(ulong_t) alignment.
	 *
	 *	(3) When allocating from the fixed size KMA pools, the
	 *	    alignment constraint must not be greater than the
	 *	    allocation size.
	 */
	if (size > PAGESIZE)
		return kmem_alloc_oversize(size, physreq, flags);
	if (u.u_flags & U_CRITICAL_MEM) {
		if (physreq->phys_align > sizeof(ulong_t))
			return kmem_alloc_oversize(size, physreq, flags);
	} else {
#ifdef KMA_PARANOID
		/*
		 * In the KMA_PARANOID case, we cannot change the allocation
		 * size without causing a PANIC in kmem_free().
		 */
		if (physreq->phys_align > size)
			return kmem_alloc_oversize(size, physreq, flags);
#else /* !KMA_PARANOID */
		if (physreq->phys_align > PAGESIZE)
			return kmem_alloc_oversize(size, physreq, flags);
		if (physreq->phys_align > size)
			size =  physreq->phys_align;
#endif /* KMA_PARANOID */
	}

	/*
	 * We must avoid the non-power of 2 pools in order to
	 * guarantee alignment. This will also assure physical
	 * contiguity, should it be needed.
	 */
	flags |= KM_PHYSCONTIG;
	return kmem_alloc(size, flags);
}

/*
 * void *
 * kmem_zalloc_physreq(size_t size, const physreq_t *physreq, int flags)
 *	Allocate memory with physical properties constrained by
 *	the caller, and zero-out the contents.
 *
 * Calling/Exit State:
 *	Same as kmem_alloc_physreq(), except that the memory is zeroed.
 */
void *
kmem_zalloc_physreq(size_t size, const physreq_t *physreq, int flags)
{
	void *mem;

	mem = kmem_alloc_physreq(size, physreq, flags);
	if (mem)
		bzero(mem, size);
	return mem;
}

/*
 * STATIC void *
 * kmem_alloc_oversize(size_t size, physreq_t *physreq, int flags)
 *	Internal KMA interface to satisfy a KMA request by obtaining
 *	pages directly from VM, and then mapping them into kpg.
 *
 * Calling/Exit State:
 *	The size argument gives the size in bytes of the desired chunk
 *	of memory; the actual size allocated will be at least this big.
 *
 *	The allocated memory will be physically contiguous if one of the
 *	following holds,
 *		(i) the KM_PHYSCONTIG flag is specified,
 * 	       (ii) physreq is non-NULL and PREQ_PHYSCONTIG or
 * 	            boundary are specified,
 *	      (iii) physreq is non-NULL and aligment is greater than PAGESIZE.
 *
 *	If physreq is non-NULL, the allocated memory will start at a
 *	physical address which is a multiple of phys_align, and will not
 *	cross a boundary which is a multiple of boundary. Both phys_align
 *	and boundary must be powers of two (except that boundary may be
 *	zero).
 *
 *	The KM_SLEEP, KM_NOSLEEP, KM_DMA, KM_REQ_DMA and KM_PHYSCONTIG
 *	flags have the same meanings as for kmem_alloc().
 *
 *	Returns a (virtual) pointer to the allocated memory; if called
 *	with KM_NOSLEEP and memory is not immediately available, NULL
 *	is returned.
 *
 *	No locks are required on entry or held on exit.
 */

STATIC void *
kmem_alloc_oversize(size_t size, const physreq_t *physreq, int flags)
{
	uint_t npages = btopr(size);
	page_t *pp;
	paddr_t align;
	mresvtyp_t mtype;
	vaddr_t memp;
	kminfo_t *infop;
	int pflags;
#if defined(DEBUG) || defined(KMA_PARANOID)
	int i;
#endif /*  DEBUG || KMA_PARANOID */

	if (npages == 0)
		return NULL;

	mtype = KM_FLAG_MTYPE(flags);
	pflags = KM_FLAG_XLAT(flags);

	if (!mem_resv(npages, mtype)) {
		if (flags & NOSLEEP) {
			MET_KMEM_FAIL(SPECLIST);
			return NULL;
		}
		mem_resv_wait(npages, mtype, B_FALSE);
	}

	memp = kpg_vm_alloc(npages, flags);
	if (memp == NULL) {
		mem_unresv(npages, mtype);
		MET_KMEM_FAIL(SPECLIST);
		return NULL;
	}

	/*
	 * For consistency, page_get_aligned should eventually be converted
	 * to take a physreq_t argument, but for now we pull out the pieces
	 * of the physreq and construct arguments in the form page_get_aligned
	 * wants.
	 */
	if (flags & KM_PHYSCONTIG) {
		if (physreq != NULL) {
			if ((align = physreq->phys_align) < PAGESIZE)
				align = PAGESIZE;
			pp = page_get_aligned(size, align, (paddr_t)0,
					      physreq->phys_boundary & PAGEMASK,
					      pflags);
		} else {
			pp = page_get_aligned(size, PAGESIZE, 0, 0, pflags);
		}
	} else if (physreq != NULL) {
		if ((physreq->phys_flags & PREQ_PHYSCONTIG) ||
		    physreq->phys_boundary != 0 ||
		    physreq->phys_align > PAGESIZE) {
			if ((align = physreq->phys_align) < PAGESIZE)
				align = PAGESIZE;
			pp = page_get_aligned(size, align, (paddr_t)0,
					      physreq->phys_boundary & PAGEMASK,
					      pflags);
		} else {
			pp = page_get(size, pflags);
		}
	} else {
		pp = page_get(size, pflags);
	}

	if (pp == NULL) {
		ASSERT(flags & NOSLEEP);
		mem_unresv(npages, mtype);
		kpg_vm_free(memp, npages);
		MET_KMEM_FAIL(SPECLIST);
		return NULL;
	}

	segkmem_pl_mapin(kpgseg, memp, npages, pp, PROT_KMA);

#ifdef DEBUG
	infop = &kminfo[IDX(memp)];
	for (i = btopr(size); i-- != 0;) {
		ASSERT(infop->ki_blist_idx == SPECLIST_IDX);
		ASSERT(infop->ki_page_no == 0);
		ASSERT((infop++)->ki_nalloc == 0);
	}
#endif /* DEBUG */
	infop = &kminfo[IDX(memp)];
	infop->ki_nalloc = NALLOC_OVERSIZE;
#ifdef KMA_PARANOID
	for (i = btopr(size); --i != 0;)
		(++infop)->ki_nalloc = NALLOC_CONT;
#endif /* KMA_PARANOID */
	MET_KMEM_ALLOC(SPECLIST,
		       (size + PAGEOFFSET) & PAGEMASK);
	MET_KMEM_REQ(SPECLIST, size);

	return  (void *)memp;
}

/*
 * void
 * kmem_advise(size_t size)
 *	Provide a hint about a commonly-used allocation size, so KMA
 *	can optimize memory usage.
 *
 * Calling/Exit State:
 *	Called with the size in bytes of an allocation unit expected
 *	to be frequently used.
 *
 *	Called when booting, only from master CPU.
 *	Hence, single-threaded and no interrupts.
 *
 * Remarks:
 *	The optimization applies only to the non-DMAable pools. The
 *	DMAable pools are restricted to power of 2 sizes so that
 *	DMAable memory will be physically contiguous.
 */
void
kmem_advise(size_t size)
{
	int i, idx, lidx;
	size_t chunksize;
	size_t lbound, ubound;
#ifdef DEBUG
	int uidx;
#endif
	size_t lim;

	ASSERT(kma_initialized);

	if (kmadv_disabled)
		return;

	/* Round up to a MINBUFSZ boundary */
	size = (size + MINBUFSZ - 1) & ~(MINBUFSZ - 1);

	/* Early bail-out for out of range sizes. */
	if (size >= MAXBUFSZ || size <= MINVBUFSZ)
		return;

	/*
	 * Compute the chunk size we would use for this buffer size.
	 * This is done by requiring at least eight buffers per chunk.
	 * The number 8 is arbitrary, but we want to make sure to use
	 * enough to minimize the wastage at the end of the chunk; since
	 * these buffers are not powers of 2, they won't pack evenly.
	 */
	chunksize = ptob(btopr(size * 8));

	/*
	 * Round up the size as much as possible without changing the
	 * number of buffers per chunk; this further reduces the wastage
	 * (though we have no guarantee that anyone will use the extra
	 * bytes in the buffers).
	 */
	size = (chunksize / (chunksize / size)) & ~(MINBUFSZ - 1);

	/* Find lower and upper neighbors */
	lbound = MINVBUFSZ;
	lidx = MINVIDX;
	ubound = MAXBUFSZ;
#ifdef DEBUG
	uidx = 0;
#endif
	/*
	 * If restricted DMA support is in effect, we limit the search for
	 * neighbors to the non-DMAable pools. If restricted DMA support is
	 * not in effect, the following will search all pools.
	 */
	for (i = ndmapool; i < maxpool; i++) {
		if (blist[i].km_bsize <= size && blist[i].km_bsize > lbound) {
			lbound = blist[i].km_bsize;
			lidx = i;
		} else if (blist[i].km_bsize > size &&
			   blist[i].km_bsize <= ubound) {
			ubound = blist[i].km_bsize;
#ifdef DEBUG
			uidx = i;
#endif
		}
	}
	ASSERT(lidx != 0 && uidx != 0);

	/* If we match an existing size, there's nothing to do. */
	if (size == ubound || size == lbound)
		return;

	/*
	 * If we're "close" to the size above us, it's not worth creating
	 * another freelist, due to the potential fragmentation.
	 * For "close", we arbitrarily pick the max of 25% or MINVBUFSZ.
	 */
	lim = (ubound - lbound) / 4;
	if (lim < MINVBUFSZ)
		lim = MINVBUFSZ;
	if (ubound - size <= lim)
		return;

#ifdef KMA_PARANOID
	/*
	 * Since buffers of size 'size' would have been allocated from
	 * the buffer list at our upper neighbor, make sure they haven't
	 * allocated any buffers yet.  This is done for the benefit of
	 * the KMA_PARANOID check in kmem_free(), which verifies
	 * that the size freed agrees with the size allocated.
	 */
	for (i = 0; i < kpg_vsize; i++) {
		idx = kminfo[i].ki_blist_idx;
		if (idx >= ndmapool && blist[idx].km_bsize == ubound) {
			/*
			 *+ A kmem_advise() request, which provides a hint for
			 *+ tuning kernel memory allocation for a commonly-used
			 *+ size, conflicted with a previous allocation.
			 */
			cmn_err(CE_NOTE, "kmem_advise() request ignored due to"
					 "conflicting allocation");
			return;
		}
	}
#endif /* KMA_PARANOID */

	/*
	 * If we're "close" to the size below us, try to "steal" the freelist
	 * below us and change it to the size we need.
	 * For "close", we arbitrarily pick the max of 25% or MINVBUFSZ.
	 */
	if (size - lbound <= lim) {
		/*
		 * If our lower neighbor has no allocations and is not one of
		 * the fixed power-of-2 buffers, we can steal it.
		 * Note that the km_free test is just a quick check for a
		 * special case; the complete check is done by the for loop.
		 */
		if (lidx >= varpool && !blist[lidx].km_free) {
			for (i = 0; i < kpg_vsize; i++) {
				if (lidx == kminfo[i].ki_blist_idx)
					break;
			}
			if (i >= kpg_vsize)
				goto init_new_freelist;	/* steal it */
		}
	}

	/*
	 * Allocate and initialize a buffer freelist.
	 */

	if (nvarbuf == NVARBUF)	/* no more freelists available */
		return;

	lidx = maxpool++;
	++nvarbuf;
	ASSERT(blist[lidx].km_bsize == 0);
	ASSERT(blist[lidx].km_free == NULL);
	LOCK_INIT(&blist[lidx].km_lock, VM_KMA_HIER, VM_KMA_IPL,
		  &kmlist_lkinfo, KM_NOSLEEP);
	SV_INIT(&blist[lidx].km_wait);

init_new_freelist:
	blist[lidx].km_bsize = (ushort_t)size;
	blist[lidx].km_npage = btop(chunksize);
	blist[lidx].km_nbpc = chunksize / size;
	ASSERT((int)blist[lidx].km_nbpc > 1);
	blist[lidx].km_cachesz = ((int)blist[lidx].km_nbpc - 1) / 2;
	MET_KMA_INIT(lidx, lidx - ndmapool, blist[lidx].km_bsize);

	/* Update kmlistp array */
	for (; size > lbound; size--) {
		kmlistp[LISTIDXF(size, 0)] = &blist[lidx];
	}
}

/*
 * int
 * kmem_avail(size_t size, int flags)
 *	Estimates whether a certain size memory chunk could be allocated.
 *
 * Calling/Exit State:
 *	Returns non-zero if the requested size, in bytes, could be allocated.
 *	This information will potentially be stale and is only a hint.
 *	As such, no locking is needed.
 *
 * Remarks:
 *	This routine is provided only to support STREAMS bufcall()
 *	functionality.
 */
int
kmem_avail(size_t size, int flags)
{
	kmlist_t *listp;
#ifndef NO_RDMA
	int idx;
#endif /* NO_RDMA */

	ASSERT(kma_initialized);

	if (size > MAXBUFSZ)
		return mem_check(btopr(size));
	listp = kmlistp[LISTIDXF(size, flags)];
	if (listp->km_free != NULL || listp->km_localp->lfree != NULL)
		return 1;
#ifndef NO_RDMA
	idx = listp - blist;
	if (dmapool <= idx && idx < ndmapool)
		return mem_dma_check((uint_t)listp->km_npage);
#endif /* NO_RDMA */

	return mem_check((uint_t)listp->km_npage);
}

/*
 * STATIC void
 * kma_shrinkpools(void)
 *	Return memory to VM subsystem, if possible.
 *	Called once every 30 minutes or whenever memory is exhausted.
 *
 * Calling/Exit State:
 *	Called only from kma_refreshpools() and therefore single-threaded
 *	w.r.t. itself.
 */
STATIC void
kma_shrinkpools(void)
{
	pl_t oldpri;
	kminfo_t *infop, *finfop;
	kmlist_t *listp;
	kmlocal_t *localp;
	kmfree_t *freep, **freepp;
	kmfree_t *empties, **empty_pp;
	uint_t npage, page_no;
	void *addr;

	ASSERT(kma_initialized);
	ASSERT(!servicing_interrupt());

	/*
	 * Look for memory to free.
	 */
	listp = &blist[maxpool];
	while (listp-- != &blist[minpool]) {
		ASSERT(listp->km_bsize != 0);
		oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);
		/*
		 * First move all free buffers from the local list for this
		 * engine to the global list.  Normally, this will have been
		 * done by the giveback daemon in shrink mode, but we do it
		 * here as well.  This catches any which were freed since
		 * giveback ran, and also catches the case of one engine,
		 * for which we don't bother to run giveback before shrinking.
		 */
		if ((freep = (localp = listp->km_localp)->lfree) != NULL) {
			CHECK_FREEB(freep, /**/);
			/*
			 * Find the tail of the local list,
			 * and update allocation counts while we're at it.
			 */
			freep->kf_infop->ki_nalloc--;
#ifdef DEBUG
			localp->lnfree--;
#endif
			while (freep->kf_next != NULL) {
				freep = freep->kf_next;
				CHECK_FREEB(freep, /**/);
				freep->kf_infop->ki_nalloc--;
#ifdef DEBUG
				localp->lnfree--;
#endif
			}
			ASSERT(localp->lnfree == 0);
			localp->lnfree = 0;
			/* splice onto the global list */
			freep->kf_next = listp->km_free;
			listp->km_free = localp->lfree;
			localp->lfree = NULL;
		}
		if (psm_intrpend(oldpri)) {
			UNLOCK(&listp->km_lock, oldpri);
			oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);
		}
		/*
		 * Make a list of all completely empty chunks.
		 * Unlink all but the first one from the freelist.
		 * PERF: Would it be better to free all of them?
		 */
		empties = NULL;
		freepp = &listp->km_free;
		while ((freep = *freepp) != NULL) {
			CHECK_FREEB(freep, /**/);
			/* Skip buffer if anything is allocated in its chunk. */
			if ((infop = freep->kf_infop)->ki_nalloc != 0) {
				freepp = &freep->kf_next;
				continue;
			}
			/* Add to empty chunk list if not already there. */
			for (empty_pp = &empties;;) {
				if (*empty_pp == NULL) {
					(*empty_pp = freep)->kf_slink = NULL;
					break;
				}
				if ((*empty_pp)->kf_infop == infop)
					break;
				empty_pp = &(*empty_pp)->kf_slink;
			}
			/* If part of the first chunk, leave it on freelist. */
			if (empty_pp == &empties) {
				freepp = &freep->kf_next;
				continue;
			}
			/* Unlink buffer from freelist. */
			*freepp = freep->kf_next;
		}
		if (empties) {
			/* Remove first chunk, since we left it on freelist. */
#ifdef KMA_PARANOID
			freep = empties;
#endif /* KMA_PARANOID */
			empties = empties->kf_slink;
#ifdef KMA_PARANOID
			freep->kf_slink = SLINK_UNUSED;
#endif /* KMA_PARANOID */
		}
		UNLOCK(&listp->km_lock, oldpri);
		/*
		 * Free all of the chunks extracted from the freelist.
		 */
		npage = listp->km_npage;
		while (empties) {
			infop = empties->kf_infop;
			empties = empties->kf_slink;
			ASSERT(infop->ki_nalloc == 0);
			ASSERT(infop->ki_blist_idx == listp - blist);
			ASSERT(infop->ki_page_no == 0);
			infop->ki_blist_idx = SPECLIST_IDX;
			addr = (void *)INFOP_TO_ADDR(infop);
			for (page_no = 1; page_no <  npage; ++page_no) {
				++infop;
				ASSERT(infop->ki_nalloc == NALLOC_CONT);
#ifdef KMA_PARANOID
				infop->ki_nalloc = 0;
#endif /* KMA_PARANOID */
				ASSERT(infop->ki_blist_idx == 0);
				ASSERT(infop->ki_page_no == page_no);
				infop->ki_page_no = 0;
			}
			kpg_free(addr, npage);
			MET_KMEM_MEM(listp, -npage);
		}
	}
}

/*
 * STATIC void
 * kma_growpools(void)
 *	Look through the freelists, and if anyone is waiting for more memory,
 *	try to get more and if successful wake them up.
 *
 * Calling/Exit State:
 *	Called only from kma_refreshpools() and therefore single-threaded
 *	w.r.t. itself.
 */
STATIC void
kma_growpools(void)
{
	kmlist_t *listp, *start;
	kminfo_t *infop;
	void *memp;
	int i, page_no;
	pl_t oldpri;

	/*
	 * Early bail-out if there are no waiters.
	 * OK to check without the lock, since we're just giving a shutter
	 * effect anyway.
	 */
	if (kma_waitcnt == 0)
		return;

	/*
	 * For each freelist which has an LWP waiting on it,
	 * try to get more memory; if successful wakeup all the waiters.
	 *
	 * NOTE: Much of the following code is similar to code in kmem_alloc()
	 * which acquires more memory and puts it onto a freelist; it was not
	 * made into a common subroutine due to the performance needs of
	 * kmem_alloc().
	 */
	listp = start = km_grow_start;
	do {
		ASSERT(listp->km_bsize != 0);
		oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);
		if ((!(listp->km_grow_flags & KMA_GROW_PENDING)) && 
			((SV_BLKD(&listp->km_wait)) || 
			(listp->km_grow_flags & KMA_NOSLEEP_FAILED))) {

			listp->km_grow_flags |= KMA_GROW_PENDING;
			UNLOCK(&listp->km_lock, VM_KMA_IPL);

			memp = kpg_alloc(listp->km_npage, PROT_KMA,
					NOSLEEP | KM_PFLAGS(listp));
			if (memp == NULL) {
				(void) LOCK(&listp->km_lock, VM_KMA_IPL);
				ASSERT(listp->km_grow_flags & KMA_GROW_PENDING);
				listp->km_grow_flags &= ~KMA_GROW_PENDING;
				UNLOCK(&listp->km_lock, oldpri);
				/*
				 * Make sure that next time we try to grow the
				 * pools, we start over again here.
				 */
				km_grow_start = listp;
				goto cont;
			}
			infop = &kminfo[IDX((vaddr_t)memp)];
#ifdef DEBUG
			for (i = listp->km_npage; i-- != 0;) {
				ASSERT(infop->ki_nalloc == 0);
				ASSERT(infop->ki_page_no == 0);
				ASSERT(infop->ki_blist_idx == SPECLIST_IDX);
				++infop;
			}
			infop -= listp->km_npage;
#endif /* DEBUG */
			ASSERT(infop->ki_nalloc == 0);
			infop->ki_blist_idx = listp - blist;
			infop->ki_page_no = 0;

			(void) LOCK(&listp->km_lock, VM_KMA_IPL);

			if (listp->km_grow_flags & KMA_NOSLEEP_FAILED) {
				ASSERT(kma_waitcnt != 0);
				DECR_WAITCNT();
			}

			ASSERT(listp->km_grow_flags & KMA_GROW_PENDING);
			listp->km_grow_flags = 0;

			for (i = (int)listp->km_nbpc; i-- > 0;) {
				((kmfree_t *)memp)->kf_infop = infop;
#ifdef KMA_PARANOID
				((kmfree_t *)memp)->kf_magic = KMA_MAGIC;
				((kmfree_t *)memp)->kf_slink = SLINK_UNUSED;
#endif /* KMA_PARANOID */
				LINK(listp, (kmfree_t *)memp);
				NEXT(memp, listp);
			}
			for (page_no = 1;
			     page_no < (int)listp->km_npage; ++page_no) {
				++infop;
				infop->ki_page_no = page_no;
#ifdef KMA_PARANOID
				infop->ki_nalloc = NALLOC_CONT;
#endif /* KMA_PARANOID */
			}

			UNLOCK(&listp->km_lock, oldpri);

			if (SV_BLKD(&listp->km_wait))
				SV_BROADCAST(&listp->km_wait, 0);

			MET_KMEM_MEM(listp, listp->km_npage);
		} else
			UNLOCK(&listp->km_lock, oldpri);
cont:
		if (listp == &blist[minpool])
			listp = &blist[maxpool];
	} while (--listp != start);
}

/*
 * void
 * kma_refreshpools(void)
 *	Refresh KMA memory pools.
 *
 * Calling/Exit State:
 *	Called once a second from a daemon process.
 *	No parameters.
 */
void
kma_refreshpools(void)
{
	uint_t seq;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	FSPIN_LOCK(&kma_lock);
	if (kma_shrinktime-- != 0) {
		FSPIN_UNLOCK(&kma_lock);
		kma_growpools();
		return;
	}

	/* 30 minutes until next shrink */
	kma_shrinktime = KMA_MAXSHRINKTIME;

	FSPIN_UNLOCK(&kma_lock);

	/*
	 * Begin a shrink-pools operation.
	 *
	 * First get the giveback daemon to give back all per-engine free
	 * buffers to the global pool.
	 *
	 * We don't need to do this if there's only one online engine,
	 * since kma_shrinkpools() will take care of it.  Note: the test of
	 * nonline is stale, but is benign.
	 */
	if (nonline > 1) {
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		while (kma_giveback_action != GB_IDLE) {
			SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		}
		kma_giveback_action = GB_SHRINK;
		kma_targets = kma_live_daemons;
		seq = kma_giveback_seq;
		UNLOCK(&kma_giveback_lock, PLBASE);
		SV_BROADCAST(&kma_giveback_sv, 0);
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		while (kma_giveback_seq == seq) {
			SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		}
		UNLOCK(&kma_giveback_lock, PLBASE);
	}

	/*
	 * Now shrink the global pools.
	 */
	kma_shrinkpools();

	/*
	 * Now grow pools if there are any waiters.
	 */
	kma_growpools();
}

/*
 * void
 * kma_do_giveback(void)
 *	Kick off a normal giveback.
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on exit.
 */
void
kma_do_giveback(void)
{
	pl_t oldpri;

	oldpri = LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	if (kma_giveback_action != GB_IDLE) {
		UNLOCK(&kma_giveback_lock, oldpri);
		return;
	}
	kma_giveback_action = GB_GIVEBACK;
	kma_targets = kma_live_daemons;
	UNLOCK(&kma_giveback_lock, oldpri);
	SV_BROADCAST(&kma_giveback_sv, 0);
}

/*
 * STATIC void
 * kma_giveback_all(void)
 *	Give back all memory from the local freelist to the global freelist.
 *
 * Calling/Exit State:
 *	No locks required on entry and none held on return.
 *	Does not block.
 */
STATIC void
kma_giveback_all(void)
{
	kmlist_t *listp;
	kmlocal_t *localp;
	kmfree_t **lfreep;
	kmfree_t *freep;
	pl_t oldpri;

	ASSERT(!servicing_interrupt());

	/*
	 * Loop through each buffer list and try to give back buffers
	 * to the corresponding global free list.  We try to leave up to n
	 * buffers on the local list, where n is half of the number of buffers
	 * per chunk; we will not, however, leave behind any which are part
	 * of an otherwise empty (nothing allocated) chunk.  Buffers which
	 * belong to empty chunks are always moved to the end of the global
	 * free list.
	 */
	listp = &blist[maxpool];
	while (listp-- != &blist[minpool]) {
		ASSERT(listp->km_bsize != 0);
		/*
		 * First check the easy case without holding the km_lock.
		 */
		lfreep = &(localp = listp->km_localp)->lfree;
		if ((freep = *lfreep) == NULL)
			continue;
		oldpri = LOCK(&listp->km_lock, VM_KMA_IPL);
		/*
		 * Re-check under the protection of the lock.
		 */
		if ((freep = *lfreep) == NULL) {
			UNLOCK(&listp->km_lock, oldpri);
			continue;
		}
		CHECK_FREEB(freep, /**/);
		/*
		 * Find the tail of the local list,
		 * and update allocation counts while we're at it.
		 */
		freep->kf_infop->ki_nalloc--;
#ifdef DEBUG
		localp->lnfree--;
#endif
		while (freep->kf_next != NULL) {
			freep = freep->kf_next;
			CHECK_FREEB(freep, /**/);
			freep->kf_infop->ki_nalloc--;
#ifdef DEBUG
			localp->lnfree--;
#endif
		}
		ASSERT(localp->lnfree == 0);
		localp->lnfree = 0;
		/* splice onto the global list */
		freep->kf_next = listp->km_free;
		listp->km_free = *lfreep;
		*lfreep = NULL;
		UNLOCK(&listp->km_lock, oldpri);
	}
}

/*
 * STATIC void
 * kma_giveback_daemon(void *argp)
 *	Daemon which periodically gives back memory from local to global pools.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none will be held on return.
 *
 * Remarks:
 *	Bound LWPs are needed in order to access the per-engine local
 *	buffer free lists for each engine.  Each instance of this daemon
 *	is bound to a specific engine which is coming online.  The engine
 *	to bind to is passed in through argp.
 */
STATIC void
kma_giveback_daemon(void *argp)
{
	int eng_num, new_eng;
	boolean_t do_exit = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	u.u_lwpp->l_name = "kma_giveback";

	(void) kbind((engine_t *)argp);

	eng_num = l.eng_num;
	EMASK_SET1(&kma_live_daemons, eng_num);

	(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	for (;;) {
		/* Wait until we're a target of some action. */
		while (!EMASK_TEST1(&kma_targets, eng_num)) {
			SV_WAIT(&kma_giveback_sv, PRIMEM, &kma_giveback_lock);
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		}

		/* Perform the desired action. */
		ASSERT(kma_giveback_action != GB_IDLE);
		switch (kma_giveback_action) {
		case GB_GIVEBACK:
			/*
			 * For now, treat GB_GIVEBACK like GB_SHRINK.
			 * Eventually, this could be optimized to leave
			 * some buffers on the local list.
			 */
			/* FALLTHROUGH */
		case GB_SHRINK:
			UNLOCK(&kma_giveback_lock, PLBASE);
			kma_giveback_all();
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
			break;

		case GB_EXIT:
			UNLOCK(&kma_giveback_lock, PLBASE);
			kunbind((engine_t *)NULL);
			do_exit = B_TRUE;
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
			EMASK_CLR1(&kma_live_daemons, eng_num);
			break;

		case GB_SPAWN:
			/* Request to spawn a new daemon LWP. */
			ASSERT(kma_new_eng != NULL);
			/* hand off completion to the new LWP */
			ASSERT(EMASK_TEST1(&kma_targets, eng_num));
			EMASK_CLR1(&kma_targets, eng_num);
			new_eng = PROCESSOR_UNMAP(kma_new_eng);
			ASSERT(!EMASK_TEST1(&kma_targets, new_eng));
			EMASK_SET1(&kma_targets, new_eng);
			kma_giveback_action = GB_COMPLETE;
			UNLOCK(&kma_giveback_lock, PLBASE);
			kma_spawn_error = spawn_lwp(NP_SYSPROC|NP_FAILOK,
						    NULL, LWP_DETACHED, NULL,
						    kma_giveback_daemon,
						    kma_new_eng);
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
			if (kma_spawn_error == 0)
				continue;
			/* spawn failed; we have to handle completion now */
			ASSERT(EMASK_TEST1(&kma_targets, new_eng));
			EMASK_CLR1(&kma_targets, new_eng);
#ifdef DEBUG
			ASSERT(!EMASK_TEST1(&kma_targets, eng_num));
			EMASK_SET1(&kma_targets, eng_num);
#endif
			break;

		case GB_COMPLETE:
			/* no action */
			break;
		}

		/* Indicate that we're done. */
		ASSERT(kma_giveback_action != GB_IDLE);
		ASSERT(EMASK_TEST1(&kma_targets, eng_num));
		EMASK_CLR1(&kma_targets, eng_num);
		if (!EMASK_TESTALL(&kma_targets)) {
			/* We're the last one; complete the action. */
			++kma_giveback_seq;
			kma_giveback_action = GB_IDLE;
			UNLOCK(&kma_giveback_lock, PLBASE);
			if (SV_BLKD(&kma_completion_sv))
				SV_BROADCAST(&kma_completion_sv, 0);
			if (do_exit) {
				lwp_exit();
				/* NOTREACHED */
			}
			(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
		}

		ASSERT(!do_exit);
	}
}

/*
 * int
 * kma_online(engine_t *eng)
 *	Inform the KMA giveback daemon of a new engine going online.
 *
 * Calling/Exit State:
 *	The onoff_mutex sleep lock is held on entry and remains held;
 *	this is necessary to serialize access to kma_spawn_error.
 *	This routine blocks until an LWP has been bound to the new engine.
 */
int
kma_online(engine_t *eng)
{
	uint_t seq;
	int eng_num;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Spawn another giveback daemon LWP to be bound to the new engine.
	 */
	(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	while (kma_giveback_action != GB_IDLE) {
		SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	}
	kma_giveback_action = GB_SPAWN;
	/* pick one of the existing daemons to do the spawn */
	eng_num = EMASK_FFS(&kma_live_daemons);
	EMASK_INIT(&kma_targets, eng_num);
	kma_new_eng = eng;
	seq = kma_giveback_seq;
	UNLOCK(&kma_giveback_lock, PLBASE);
	SV_BROADCAST(&kma_giveback_sv, 0);
	(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	while (kma_giveback_seq == seq) {
		SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	}
	UNLOCK(&kma_giveback_lock, PLBASE);
	return kma_spawn_error;
}

/*
 * void
 * kma_offline(engine_t *eng)
 *	Inform the KMA giveback daemon that an engine is going offline.
 *
 * Calling/Exit State:
 *	No locks are required on entry or held on exit.
 *	This routine blocks until the corresponding LWP has been unbound.
 */
void
kma_offline(engine_t *eng)
{
	uint_t seq;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	while (kma_giveback_action != GB_IDLE) {
		SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	}
	kma_giveback_action = GB_EXIT;
	ASSERT(EMASK_TEST1(&kma_live_daemons, PROCESSOR_UNMAP(eng)));
	EMASK_INIT(&kma_targets, PROCESSOR_UNMAP(eng));
	seq = kma_giveback_seq;
	UNLOCK(&kma_giveback_lock, PLBASE);
	SV_BROADCAST(&kma_giveback_sv, 0);
	(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	while (kma_giveback_seq == seq) {
		SV_WAIT(&kma_completion_sv, PRIMEM, &kma_giveback_lock);
		(void) LOCK(&kma_giveback_lock, VM_KMA_GIVEBACK_IPL);
	}
	UNLOCK(&kma_giveback_lock, PLBASE);
}

/*
 * void
 * kma_offline_self(void)
 *	This engine is going offline.
 *
 * Calling/Exit State:
 *	Called just before an engine finishes taking itself offline.
 *	No locks are required on entry or held on exit.
 */
void
kma_offline_self(void)
{
	/*
	 * Since we're going offline, make sure we aren't holding any
	 * local buffers.
	 */
	kma_giveback_all();
}

#ifdef _KMEM_STATS

/*
 * void
 * print_kma_stats(void)
 *	Routine to print out kma statistics.
 *
 * Calling/Exit State:
 *	No parameters.
 *
 * Remarks:
 *	The kma_instr_lock is held during the sort, but not during the
 *	printout.  Consequently, the printout might print stale information.
 *	However, since information is never deleted from the tables,
 *	the pointers should always lead to valid addresses.
 *
 *	This function is intended for use from a kernel debugger.
 */
void
print_kma_stats(void)
{
	boolean_t did_flip;
	struct kma_szinfo **sp0, **sp1, **endp;
	struct kma_szinfo *sizep;
	struct kma_invinfo *invp;
	char *type;
	uint_t i, j;
	long mem = 0;
	long balloc = 0;
	long ralloc = 0;
	ushort fail = 0;
	struct plocalmet *plp;

	debug_printf("\nmetp_kmem:    eng\tmpk_mem  mpk_balloc"
		      "  mpk_ralloc  mpk_fail\n");
	for (j = 0; j < MET_KMEM_NCLASS; j++) {
		if (j == MET_KMOVSZ)
			debug_printf("     OVSZ:\n");
		else
			debug_printf("   %6d:\n", m.mets_kmemsizes.msk_sizes[j]);
		for (i = 0; i < Nengine; i++) {
			plp = ENGINE_PLOCALMET_PTR(i);
		debug_printf("              %2d\t %6d      %6d"
			      "      %6d    %6d\n",
				i, plp->metp_kmem[j].mpk_mem,
				plp->metp_kmem[j].mpk_balloc,
				plp->metp_kmem[j].mpk_ralloc,
				plp->metp_kmem[j].mpk_fail);
			mem += plp->metp_kmem[j].mpk_mem;
			balloc += plp->metp_kmem[j].mpk_balloc;
			ralloc += plp->metp_kmem[j].mpk_ralloc;
			fail += plp->metp_kmem[j].mpk_fail;
		}
		debug_printf("                 \t ------      ------"
			      "      ------    ------\n"
			     "                 \t %6d      %6d"
			      "      %6d    %6d\n\n",
			mem, balloc, ralloc, fail);
		mem = 0;
		balloc = 0;
		ralloc = 0;
		fail = 0;
		if (debug_output_aborted())
			return;
	}

	FSPIN_LOCK(&kma_instr_lock);

	/*
	 * first sort the size table in decreasing order of memory ownership
	 */
	endp = &kma_sort_table[kma_nsize];
	do {
		did_flip = B_FALSE;
		sp0 = &kma_sort_table[0];
		sp1 = &kma_sort_table[1];
		while (sp1 < endp) {
			if ((*sp0)->kmsi_owned < (*sp1)->kmsi_owned) {
				sizep = *sp0;
				*sp0 = *sp1;
				*sp1 = sizep;
				did_flip = B_TRUE;
			}
			++sp0;
			++sp1;
		}
		--endp;
	} while (did_flip);

	FSPIN_UNLOCK(&kma_instr_lock);

	/*
	 * now print out table contents
	 */
	debug_printf("%d of %d entries used in the size table\n",
		     kma_nsize, KMA_SIZES);
	if (kma_size_overflow)
		debug_printf("WARNING: sizes table has overflowed\n");
	debug_printf("%d of %d entries used in the invocations table\n",
		     kma_ninv, KMA_INVOCS);
	if (kma_inv_overflow)
		debug_printf("WARNING: invocations table has overflowed\n");

	sp0 = &kma_sort_table[0];
	endp = &kma_sort_table[kma_nsize];
	while (sp0 < endp) {
		if (debug_output_aborted())
			return;
		sizep = *sp0;
		debug_printf("%d bytes owned, %d alloc size\n",
			     sizep->kmsi_owned, sizep->kmsi_size);
		invp = sizep->kmsi_invp;
		while (invp != (struct kma_invinfo *)NULL) {
			switch (invp->kmvi_type) {
			case KMA_INV_ALLOC:
				type = "allocations";
				break;
			case KMA_INV_FREE:
				type = "frees";
				break;
			default:
				type = "allocation failures";
				break;
			}
			debug_printf("    %d %s from line %d of file %s\n",
				     invp->kmvi_nops, type, invp->kmvi_line,
				     invp->kmvi_file);
			if (debug_output_aborted())
				return;
			invp = invp->kmvi_chain;
		}
		++sp0;
	}
}

/*
 * STATIC void
 * kma_stat_instr(size_t size, int flag, int line, char *file)
 *	Routine to collect kma statistics.
 *
 * Calling/Exit State:
 *	size give the size being allocated or freed
 *	flag is one of:
 *		KMA_INV_ALLOC	storage is being allocated
 *		KMA_INV_FAILED	allocation attempt failed
 *		KMA_INV_FREE	storage is being freed
 *	line and file identify the caller
 *
 *	Information is added to the sizes/invocations tables.  New entries
 *	are made, if required.
 *
 * Remarks:
 *	The kma_instr_lock is held during the sort, but not during the
 *	printout.  Consequently, the printout might print stale information.
 */
STATIC void
kma_stat_instr(size_t size, int flag, int line, char *file)
{
	struct kma_szinfo *sizep, *org;
	struct kma_invinfo *invp, *iorg;

	FSPIN_LOCK(&kma_instr_lock);

	/*
	 * size table lookup
	 */
	org = sizep = &kma_size_table[size % KMA_SIZES];
	while (size != sizep->kmsi_size) {
		if (sizep->kmsi_size == 0) {
			/*
			 * not found - so allocate a new entry
			 */
			sizep->kmsi_size = size;
			kma_sort_table[kma_nsize] = sizep;
			++kma_nsize;
			break;
		}
		if (++sizep == &kma_size_table[KMA_SIZES]) {
			sizep = &kma_size_table[0];
		}
		if (sizep == org) {
			kma_size_overflow = B_TRUE;
			FSPIN_UNLOCK(&kma_instr_lock);
			return;
		}
	}

	/*
	 * update ownerhip information
	 */
	switch (flag) {
	case KMA_INV_ALLOC:
		sizep->kmsi_owned += size;
		break;
	case KMA_INV_FREE:
		sizep->kmsi_owned -= size;
		break;
	default:
		break;
	}

	/*
	 * invocation table lookup
	 */
	iorg = invp = &kma_inv_table[(size + line + (int)file) % KMA_INVOCS];
	while (size != invp->kmvi_size || flag != invp->kmvi_type ||
	       line != invp->kmvi_line || file != invp->kmvi_file) {
		if (invp->kmvi_size == 0) {
			/*
			 * not found - so allocate a new entry
			 */
			invp->kmvi_size = size;
			invp->kmvi_type = (ushort_t)flag;
			invp->kmvi_line = (ushort_t)line;
			invp->kmvi_file = file;

			/*
			 * thread this invocation entry into the size table
			 *
			 *	Link the invoc table entry on the size chain
			 *	after it points to the existing chain, so
			 *	that a racing kma_print_stats does not fail.
			 */
			invp->kmvi_chain = sizep->kmsi_invp;
			sizep->kmsi_invp = invp;
			++kma_ninv;
			break;
		}
		if (++invp == &kma_inv_table[KMA_INVOCS]) {
			invp = &kma_inv_table[0];
		}
		if (invp == iorg) {
			kma_inv_overflow = B_TRUE;
			FSPIN_UNLOCK(&kma_instr_lock);
			return;
		}
	}

	/*
	 * update operations count
	 */
	++invp->kmvi_nops;

	FSPIN_UNLOCK(&kma_instr_lock);
}

#endif /* _KMEM_STATS */

#ifdef _KMEM_HIST

/*
 * STATIC void
 * kma_log_hist(char *name, int line, char *file, void *addr, size_t size)
 *	Routine to collect kma history information.
 *
 * Calling/Exit State:
 *	name identifies the service being used (kmem_alloc, kmem_zalloc, or
 *		kmem_free)
 *	size give the size being allocated or freed
 *	line and file identify the caller
 *
 *	A kma_log_record is written to the oldest entry in the kma_hist_buffer.
 */
STATIC void
kma_log_hist(char *name, int line, char *file, void *addr, size_t size)
{
	struct kma_log_record *khp;

	FSPIN_LOCK(&kma_hist_lock);
	khp = &kma_hist_buffer[kma_hist_cursor];
	if (++kma_hist_cursor == KMA_HIST_MAX)
		kma_hist_cursor = 0;
	khp->klr_svc_name = name;
	khp->klr_line = line;
	khp->klr_file = file;
	khp->klr_addr = addr;
	khp->klr_size = size;
	khp->klr_lwp = CURRENT_LWP();
	KMA_HIST_STAMP(khp->klr_stamp);
	FSPIN_UNLOCK(&kma_hist_lock);
}

#define KMA_PRINT_HIST_USAGE \
	"Usage: kma_print_hist(char *ctrl, ...)\n" \
	"       ctrl string includes zero or more cmd chars (either case)\n" \
	"       for each cmd char, there is an additional argument\n" \
	"       any cmd char except N may be preceded by '!' to reverse\n" \
	"       the sense of the comparison\n" \
	"           L: LWP == arg\n" \
	"           A: addr == arg\n" \
	"           S: size == arg (rounded to KMA buffer size)\n" \
	"           2: size == arg (rounded to next power of 2)\n" \
	"           C: addr is in same chunk as arg\n" \
	"           F: filename == arg\n" \
	"           N: consider only youngest arg history entries\n"
/*
 * void
 * print_kma_hist(const char *ctrl, ...)
 *	Routine to print KMA history information to the console.
 *
 * Calling/Exit State:
 *	The ctrl string, if non-blank, specifies constraints on which
 *	history entries to print.  The default is to print everything.
 *	Each letter in the ctrl string creates an additional constraint.
 *	Corresponding to each letter is a successive additional argument
 *	to kma_print_hist(), which is the value for which the constraint
 *	applies.  Case is ignored (i.e. 'A' is the same as 'a').  Each
 *	letter (except 'N') may be preceded by '!' to reverse the sense
 *	of the comparison.
 *
 *	Valid control letters are described in KMA_PRINT_HIST_USAGE, above.
 *
 * Remarks:
 *	This function is intended for use from a kernel debugger.
 */
void
print_kma_hist(const char *ctrl, ...)
{
	const char *ctrlp;
	VA_LIST argp;
	boolean_t first_time = B_TRUE;
	boolean_t n_set = B_FALSE;
	uint_t n = KMA_HIST_MAX;
	struct kma_log_record *khp;
	boolean_t match, match_type;
	lwp_t *lwp;
	void *addr;
	size_t size;
	kminfo_t *infop, *khinfop;
	char *file;
	int k, last, i;
	ulong_t last_stamp, diff;
	char digit[9];
	char c, *p;

	/* In case of a bad control string pointer, treat as help request */
	if (ctrl == NULL || !KADDR(ctrl))
		ctrl = "?";

	last = kma_hist_cursor - 1;
	if (last < 0)
		last += KMA_HIST_MAX;
	last_stamp = kma_hist_buffer[last].klr_stamp;

restart:
	k = kma_hist_cursor - n;
	if (k < 0)
		k += KMA_HIST_MAX;
	khp = &kma_hist_buffer[k];

	for (;;) {
		if (khp->klr_svc_name == NULL)
			goto next;

		match = match_type = B_TRUE;
		VA_START(argp, ctrl);
		for (ctrlp = ctrl; (c = *ctrlp) != '\0'; ++ctrlp) {
			if (c == '!' && ctrlp[1] != '\0') {
				match_type = B_FALSE;
				continue;
			}
			if (c >= 'a' && c <= 'z')
				c += 'A' - 'a';
			switch (c) {
			case 'L':
				lwp = VA_ARG(argp, lwp_t *);
				if ((lwp == khp->klr_lwp) != match_type)
					match = B_FALSE;
				break;
			case 'A':
				addr = VA_ARG(argp, void *);
				if ((addr == khp->klr_addr) != match_type)
					match = B_FALSE;
				break;
			case '2':
				size = VA_ARG(argp, size_t);
				if ((kmlistp[LISTIDX(size)] ==
					  kmlistp[LISTIDX(khp->klr_size)])
							!= match_type) {
					match = B_FALSE;
				}
				break;
			case 'S':
				size = VA_ARG(argp, size_t);
				if ((kmlistp[LISTIDXF(size, 0)] ==
				     kmlistp[LISTIDXF(khp->klr_size, 0)])
						!= match_type) {
					match = B_FALSE;
				}
				break;
			case 'C':
				addr = VA_ARG(argp, void *);
				if ((vaddr_t)addr < kpg_vbase ||
				    (vaddr_t)addr >= kpg_vbase +
						      ptob(kpg_vsize)) {
					match = B_FALSE;
					break;
				}
				infop = &kminfo[IDX((vaddr_t)addr)];
				infop -= infop->ki_page_no;
				khinfop = &kminfo[IDX((vaddr_t)khp->klr_addr)];
				khinfop -= khinfop->ki_page_no;
				if ((infop == khinfop) != match_type)
					match = B_FALSE;
				break;
			case 'F':
				file = VA_ARG(argp, char *);
				if ((strcmp(file, khp->klr_file) == 0)
								!= match_type)
					match = B_FALSE;
				break;
			case 'N':
				if (n_set) {
					(void)VA_ARG(argp, uint_t);
					break;
				}
				if (match_type != B_FALSE) {
					n = VA_ARG(argp, uint_t);
					if (n == 0 || n > KMA_HIST_MAX)
						n = KMA_HIST_MAX;
					n_set = B_TRUE;
					goto restart;
				}
				/* FALLTHRU */
			default:
				debug_printf("%s", KMA_PRINT_HIST_USAGE);
				return;
			}
			match_type = B_TRUE;
		}

		if (first_time) {
			debug_printf("TIME    LWP      addr    size\n"
				     "----    ---      ----    ----\n");
			first_time = B_FALSE;
		}

		if (!match)
			goto next;

		diff = last_stamp - khp->klr_stamp;
		p = &digit[8];
		*p-- = '\0';
		diff /= 100;
		for (i = 1; i <= 6 || diff != 0; ++i) {
			if (i == 5)
				*p-- = '.';
			else {
				*p-- = (diff % 10) + '0';
				diff /= 10;
			}
		}
		debug_printf("-%s %lx %lx %lx %s from line %d of file %s\n",
			     p + 1,
			     (ulong_t)khp->klr_lwp, (ulong_t)khp->klr_addr,
			     (ulong_t)khp->klr_size, khp->klr_svc_name,
			     khp->klr_line, khp->klr_file);
next:
		if (--n == 0 || debug_output_aborted())
			break;
		if (++khp == &kma_hist_buffer[KMA_HIST_MAX])
			khp = &kma_hist_buffer[0];
	}
}

#endif /* _KMEM_HIST */

#if defined (_KMEM_STATS) || defined(_KMEM_HIST)

/*
 * void *
 * kmem_instr_alloc(size_t size, int flags, int line, char *file);
 *	Allocate virtual and physical memory of arbitrary size.
 *
 * Calling/Exit State:
 *	Same as kmem_alloc(), except that statistics and/or history
 *	information is also gathered.
 */
void *
kmem_instr_alloc(size_t size, int flags, int line, char *file)
{
	void *mem;

	mem = kmem_alloc(size, flags);
#ifdef _KMEM_STATS
	if (kmem_stats_enabled) {
		kma_stat_instr(size, mem ? KMA_INV_ALLOC : KMA_INV_FAILED,
			       line, file);
	}
#endif /* _KMEM_STATS */
#ifdef _KMEM_HIST
	kma_log_hist("allocated", line, file, mem, size);
#endif /* _KMEM_HIST */
	return mem;
}

/*
 * void *
 * kmem_instr_zalloc(size_t size, int flags, int line, char *file);
 *	Allocate virtual and physical memory of arbitrary size, with
 *      the contents zeroed.
 *
 * Calling/Exit State:
 *	Same as kmem_zalloc(), except that statistics and/or history
 *	information is also gathered.
 */
void *
kmem_instr_zalloc(size_t size, int flags, int line, char *file)
{
	void *mem;

	mem = kmem_alloc(size, flags);
#ifdef _KMEM_STATS
	if (kmem_stats_enabled) {
		kma_stat_instr(size, mem ? KMA_INV_ALLOC : KMA_INV_FAILED,
			       line, file);
	}
#endif /* _KMEM_STATS */
	if (mem) {
		bzero(mem, size);
	}
#ifdef _KMEM_HIST
	kma_log_hist("z allocated", line, file, mem, size);
#endif /* _KMEM_HIST */
	return mem;
}

/*
 * void *
 * kmem_i_alloc_physreq(size_t size, const physreq_t *physreq, int flags,
 *			       int line, char *file);
 *	Allocate memory with physical properties constrained by
 *	the caller.
 *
 * Calling/Exit State:
 *	Same as kmem_alloc_physreq(), except that statistics and/or history
 *	information is also gathered.
 */
void *
kmem_i_alloc_physreq(size_t size, const physreq_t *physreq, int flags,
			    int line, char *file)
{
	void *mem;

	mem = kmem_alloc_physreq(size, physreq, flags);
#ifdef _KMEM_STATS
	if (kmem_stats_enabled) {
		kma_stat_instr(size, mem ? KMA_INV_ALLOC : KMA_INV_FAILED,
			       line, file);
	}
#endif /* _KMEM_STATS */
#ifdef _KMEM_HIST
	kma_log_hist("p allocated", line, file, mem, size);
#endif /* _KMEM_HIST */

	return mem;
}

/*
 * void *
 * kmem_i_zalloc_physreq(size_t size, const physreq_t *physreq, int flags,
 *			 int line, char *file)
 *	Allocate memory with physical properties constrained by
 *	the caller, and zero-out the contents.
 *
 * Calling/Exit State:
 *	Same as kmem_zalloc_physreq(), except that statistics and/or history
 *	information is also gathered.
 */
void *
kmem_i_zalloc_physreq(size_t size, const physreq_t *physreq, int flags,
			     int line, char *file)
{
	void *mem;

	mem = kmem_alloc_physreq(size, physreq, flags);
#ifdef _KMEM_STATS
	if (kmem_stats_enabled) {
		kma_stat_instr(size, mem ? KMA_INV_ALLOC : KMA_INV_FAILED,
			       line, file);
	}
#endif /* _KMEM_STATS */
	if (mem) {
		bzero(mem, size);
	}
#ifdef _KMEM_HIST
	kma_log_hist("zp allocated", line, file, mem, size);
#endif /* _KMEM_HIST */
	return mem;
}

/*
 * void
 * kmem_instr_free(void * addr, size_t size, int line, char *file)
 *	Free memory previously allocated by kmem_instr_alloc.
 *
 * Calling/Exit State:
 *	Same as kmem_free(), except that statistics and/or history
 *	information is also gathered.
 */
void
kmem_instr_free(void * addr, size_t size, int line, char *file)
{
	kmem_free(addr, size);
#ifdef _KMEM_STATS
	if (kmem_stats_enabled) {
		kma_stat_instr(size, KMA_INV_FREE, line, file);
	}
#endif /* _KMEM_STATS */
#ifdef _KMEM_HIST
	kma_log_hist("freed", line, file, addr, size);
#endif /* _KMEM_HIST */
}

#endif defined (_KMEM_STATS) || defined(_KMEM_HIST)
