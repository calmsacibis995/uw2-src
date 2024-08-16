/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bufsubr.c	1.63"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg_kmem.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>	

#ifdef _PAGEIO_HIST
#include <proc/disp.h>
#include <proc/lwp.h>
#endif

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

/*
 * A separate lock and list is used to maintain a private pool of
 * buffer headers used for page I/O for the pageout daemon (if normal
 * allocation fails).  The pageio_lists_lock lock protects this list,
 * and its associated variables, pgouthdrlist, pgoutboutcnt and pgoutbufused.
 */
STATIC lock_t	pageio_lists_lock;
/*+ pageio hash/free lists lock (global) */
STATIC LKINFO_DECL(pageio_lists_lkinfo, "FS:buf:pageio_lists_lock", 0);
STATIC sv_t     pgoutlist_sv;

STATIC buf_t	pgouthdrlist;    /* pageout buffer header list        */

extern buf_t	pgoutbuf[];      /* global storage for buffer headers */
STATIC buf_t	*pgoutblast;
extern int	npgoutbuf;      /* number of buffers configured       */

#ifdef DEBUG
uint_t pgoutboutcnt;	/* # times pageout waited for a buffer header */
uint_t pgoutbufused;	/* # times pageout used a buffer from pgouthdrlist */
#endif

/*
 * Support for pageio buffers.
 *
 * This stuff should be generalized to provide a generalized bp
 * header facility that can be used for things other than pageio.
 */

/*
 * Pageio_out is a list of all the buffers currently checked out
 * for pageio use.
 */
STATIC struct hbuf pageio_out = {
	0,
	(buf_t *)&pageio_out,
	(buf_t *)&pageio_out,
};

/*
 * pgiobrkup_strat points to an array of strategy routines, corresponding
 * one-to-one with bdevsw[].  For each driver which does not have D_NOBRKUP
 * set, its strategy routine in bdevsw[] is replaced by pageio_breakup(),
 * saving the original in pgiobrkup_strat.
 */
STATIC int (**pgiobrkup_strat)();

STATIC int pageio_breakup();

/*
 * A scgth_entry_t is a ba_scgth_t scatter/gather array, for use by
 * buf_breakup BA_SCGTH, plus a link pointer to hook it onto a freelist.
 */
typedef struct scgth_entry {
	struct scgth_entry	*sge_nextfree;	/* freelist link */
	ba_scgth_t	sge_scgth[1];	/* variable length ba_scgth_t array */
} scgth_entry_t;

STATIC sv_t scgth_sv;
STATIC lock_t scgth_mutex;
/*+ scatter/gather pool lock */
STATIC LKINFO_DECL(scgth_lkinfo, "FS:buf:scgth_mutex", 0);

/*
 * buf_breakup and the scgth_xxx routines manage pools of ba_scgth_t arrays.
 * Associated with each of these pools is also a private page of memory which
 * is used when normal memory allocation fails in a critical context (pageout).
 * This private page can be used for either a ba_scgth_t array or a data
 * buffer for bufbrkup_copy().  There are separate pools for each different
 * max_scgth size and for each different DMAability type (as indicated by the
 * physreq_t).
 */
typedef struct brkup_pool {
	scgth_entry_t	*bp_freelist;	/* linked list of free entries */
	physreq_t	bp_physreq;	/* physreq to use for allocations */
	void		*bp_private_page; /* (pageout) private overflow page */
	sleep_t		bp_private_lock;  /* lock for private page ownership */
	uint_t		bp_npages;	/* # pages in scgth pool */
	struct brkup_pool *bp_next;
} brkup_pool_t;
STATIC brkup_pool_t *brkup_pools;	/* list of all pools */

/*+ buf_breakup private page ownership sleeplock */
STATIC LKINFO_DECL(brkup_priv_lkinfo, "FS:buf:bp_private_lock", 0);

/*
 * The brkup_fspin lock is used to protect shared "parent" buffer fields
 * in buf_breakup().
 */
STATIC fspin_t brkup_fspin;

/*
 * Data structures used to manage private virtual addresses for use by bp_mapin
 * when kpg runs out, and a critical process is running (NOMEMWAIT()).
 */
STATIC vaddr_t bpmap_private_addr;
STATIC boolean_t bpmap_privaddr_inuse;
STATIC sv_t bpmap_privaddr_sv;
STATIC lock_t bpmap_privaddr_mutex;
STATIC LKINFO_DECL(bpmap_lkinfo, "FS:buf:bpmap_privaddr_mutex", 0);
STATIC clock_t bpmap_privaddr_warned;	/* last time message printed */

/*
 * MAXBIOSIZE must be a multiple of the page size
 */
#if (MAXBIOSIZE % PAGESIZE) != 0
#error	MAXBIOSIZE must be a multiple of the page size
#endif

#define BPMAP_PRIVADDR_SIZE	btop(MAXBIOSIZE)

STATIC void scgth_init(void);

#ifdef _PAGEIO_HIST

/*
 * Pageio logging facility definitions.
 */
typedef struct pageio_hist_record {
	buf_t		*phr_bp;	/* buf pointer */
	uint_t		phr_flags;	/* flags */
	page_t		*phr_pp;	/* associated page */
	char		*phr_service;	/* service name */
	int		phr_line;	/* line number */
	char		*phr_file;	/* file name */
	lwp_t		*phr_lwp;	/* calling LWP */
	ulong_t		phr_stamp;	/* time stamp */
} pageio_hist_record_t;

#define PAGEIO_LOG_SIZE	200000
pageio_hist_record_t	pageio_logb[PAGEIO_LOG_SIZE];
int			pageio_cursor;

clock_t			*pageio_stamp;
event_t			bufsubr_event;

fspin_t pageio_hist_lock;

#endif	/* _PAGEIO_HIST */

/*
 * void pageio_fix_bswtbl(uint_t maj)
 *
 *	Fix up the bdevsw entry to support non-D_NOBRKUP
 *	drivers (pageio_breakup()).
 *
 * Calling/Exit State:
 *	If the major number corresponds to a driver which
 *	does not have D_NOBRKUP set, its strategy routine
 *	in bdevsw[] is replaced by pageio_breakup(); the
 *	original is saved in pgiobrkup_strat.
 */
void
pageio_fix_bswtbl(uint_t maj)
{
	if (!(*bdevsw[maj].d_flag & D_NOBRKUP) &&
	    bdevsw[maj].d_strategy != nodev &&
	    bdevsw[maj].d_strategy != nulldev) {
		pgiobrkup_strat[maj] = bdevsw[maj].d_strategy;
		bdevsw[maj].d_strategy = pageio_breakup;
	}
}

/*
 * void
 * pageio_init(void)
 *	Initialization for pageio buffer support.
 *
 * Calling/Exit State:
 *	On exit, pageio_setup() may be used.
 */
void
pageio_init(void)
{
	buf_t *bp;
	uint_t i;

	pgouthdrlist.av_forw = bp = pgoutbuf;
	for (; bp < &pgoutbuf[npgoutbuf-1]; bp++) {
		bp->av_forw = bp + 1;
	}
	bp->av_forw = NULL;
	pgoutblast = bp;

	/*
	 * Initialize Synchronization Objects
	 */
	LOCK_INIT(&pageio_lists_lock, FS_BUFPGIOHIER, FS_BUFPGIOPL,
		  &pageio_lists_lkinfo, KM_NOSLEEP);
	SV_INIT(&pgoutlist_sv);

	/*
	 * Initialize support for non-D_NOBRKUP drivers (pageio_breakup()).
	 */
	pgiobrkup_strat = kmem_alloc(bdevcnt * sizeof(int (*)()), KM_NOSLEEP);
	if (pgiobrkup_strat == NULL) {
		/*
		 *+ Boot-time allocation of data structures
		 *+ for support of non-D_NOBRKUP block device
		 *+ drivers failed.  This probably indicates
		 *+ that the system is configured with too
		 *+ little physical memory.
		 */
		cmn_err(CE_PANIC, "pageio_init: not enough memory");
	}

	for (i = bdevcnt; i-- != 0;)
		pageio_fix_bswtbl(i);
#ifndef NO_RDMA
	if (rdma_mode != RDMA_DISABLED)
		rdma_fix_swtbls();
#endif

	FSPIN_INIT(&brkup_fspin);

	SV_INIT(&scgth_sv);
	LOCK_INIT(&scgth_mutex, FS_HIER_BASE, PLMIN, &scgth_lkinfo, KM_NOSLEEP);

#ifdef _PAGEIO_HIST
	FSPIN_INIT(&pageio_hist_lock);
	pageio_stamp = kmem_zalloc((epages - pages) * sizeof (clock_t),
				   KM_NOSLEEP);
	if (pageio_stamp == NULL) {
		/*
		 *+ Boot-time allocation of memory for the PAGEIO_HIST
		 *+ debug facility failed. This failure can only occur
		 *+ in kerels specially build for debug purposes.
		 */
		cmn_err(CE_PANIC,
			"pageio_init: not enough memory for PAGEIO_HIST");
	}
#endif

	/*
	 * Initialize support for the bp_mapin() reserved virtual for
	 * critical use (pageout daemon).
	 */
	bpmap_private_addr = kpg_vm_alloc(BPMAP_PRIVADDR_SIZE, NOSLEEP);
	if (bpmap_private_addr == NULL) {
		/*
		 *+ Boot-time allocation of virtual memory
		 *+ for support of bp_mapin() failed.  This probably indicates
		 *+ that the system is configured with too little
		 *+ kernel virtual memory (SEGKMEM_BYTES/SEGKMEM_PERCENT).
		 */
		cmn_err(CE_PANIC, "pageio_init: not enough virtual memory");
	}
	SV_INIT(&bpmap_privaddr_sv);
	LOCK_INIT(&bpmap_privaddr_mutex, FS_HIER_BASE, PLHI, &bpmap_lkinfo,
		  KM_NOSLEEP);
}

/*
 * buf_t *
 * pageio_setup(page_t *pp, uint_t pgoff, uint_t len, int flags)
 *	Allocate and initialize a buf struct for use with pageio.
 *
 * Calling/Exit State:
 *	Page is locked excl. if B_READ and shared if B_WRITE on entry
 *	and remains locked at exit.
 *
 * Description:
 *	Call kmem_zalloc to allocate the buffer header. If caller is
 *	the pageout daemon, call kmem_zalloc with KM_NOSLEEP.
 *	If system is under memory exhaustion condition and kmem_zalloc
 *	fails, then use the private pools of buffer headers stashed
 *	away.
 */
buf_t *
pageio_setup(page_t *pp, uint_t pgoff, uint_t len, int flags)
{
	buf_t *bp;

	ASSERT(getpl() == PLBASE);
	ASSERT(pgoff < PAGESIZE);

        bp = kmem_zalloc(sizeof(*bp), NOMEMWAIT() ? KM_NOSLEEP : KM_SLEEP);

	(void) LOCK(&pageio_lists_lock, FS_BUFPGIOPL);

        if (bp == NULL) {
		/*
		 * In order to avoid memory deadlock, the pageout daemon
		 * cannot block waiting for memory from the general pool.
		 * Instead, use a private pool if the kmem_alloc fails.
		 */
		while ((bp = pgouthdrlist.av_forw) == NULL) {
#ifdef DEBUG
			pgoutboutcnt++;
#endif
			SV_WAIT(&pgoutlist_sv, PRIMEM + 1, &pageio_lists_lock);
			(void) LOCK(&pageio_lists_lock, FS_BUFPGIOPL);
		}
		pgouthdrlist.av_forw = bp->av_forw;
		bp->av_forw = bp->av_back = NULL;
#ifdef DEBUG
		pgoutbufused++;
#endif
        }

	bp->b_flags = B_KERNBUF | B_BUSY | B_PAGEIO | flags;
	bp->b_bcount = bp->b_bufsize = len;
	bp->b_pages = pp;
	bp->b_un.b_addr = (caddr_t)pgoff;
	bp->b_numpages = btopr(pgoff + len);

	EVENT_INIT(&(bp)->b_iowait);
	SLEEP_INIT(&(bp)->b_avail, 0, &buf_avail_lkinfo, KM_NOSLEEP);
	SLEEP_LOCK_PRIVATE(&bp->b_avail);

	binshash(bp, (buf_t *)&pageio_out);

	UNLOCK(&pageio_lists_lock, PLBASE);

	PAGEIO_LOG(bp, "setup");

	/*
	 * Caller sets dev & blkno and can use bp_mapin
	 * to make pages kernel addressable.
	 */
	return (bp);
}

/*
 * void
 * pageio_done(buf_t *bp)
 *	Free the buf struct after I/O is done.
 *
 * Calling/Exit State:
 *	Page is locked excl. if B_READ and shared if B_WRITE on entry
 *	and remains locked at exit.
 */
void
pageio_done(buf_t *bp)
{
	pl_t s;

	ASSERT((bp->b_flags & (B_PAGEIO|B_REMAPPED)) == B_PAGEIO);

	PAGEIO_LOG(bp, "done");

#ifdef DEBUG
	if (u.u_debugflags & NO_PAGEIO_DONE)
		return;
#endif

	s = LOCK(&pageio_lists_lock, FS_BUFPGIOPL);
	bremhash(bp);

	if (bp >= pgoutbuf && bp <= pgoutblast) {
		/* Return buffer to the pageout private pool */
		bp->av_forw = pgouthdrlist.av_forw;
		pgouthdrlist.av_forw = bp;
		UNLOCK(&pageio_lists_lock, s);
		if (SV_BLKD(&pgoutlist_sv))
			SV_SIGNAL(&pgoutlist_sv, 0);
	} else {
		UNLOCK(&pageio_lists_lock, s);
		SLEEP_UNLOCK(&(bp)->b_avail);
		SLEEP_DEINIT(&(bp)->b_avail);
		kmem_free((caddr_t)bp, sizeof (*bp));
	}
}

/*
 * STATIC vaddr_t
 * bpmap_get_private_addr(void)
 *	Acquire reserved virtual address for bp_mapin().
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */
STATIC vaddr_t
bpmap_get_private_addr(void)
{
	ASSERT(getpl() == PLBASE);

	if (lbolt - bpmap_privaddr_warned >= HZ) {
		/*
		 *+ bp_mapin() was called to assign a virtual address to
		 *+ an I/O buffer while running the pageout daemon,
		 *+ and there was no more kernel virtual memory available.
		 *+ The system will continue, but performance may be poor.
		 *+ Use SEGKMEM_BYTES/SEGKMEM_PERCENT tunables to add more
		 *+ virtual memory.
		 */
		cmn_err(CE_NOTE,
			"Kernel virtual memory for buffer I/O temporarily"
			" exhausted;\n"
			"\tusing reserved pool to allow paging to proceed.");
		bpmap_privaddr_warned = lbolt;
	}
	(void) LOCK(&bpmap_privaddr_mutex, PLHI);
	while (bpmap_privaddr_inuse) {
		SV_WAIT(&bpmap_privaddr_sv, PRIMEM, &bpmap_privaddr_mutex);
		(void) LOCK(&bpmap_privaddr_mutex, PLHI);
	}
	bpmap_privaddr_inuse = B_TRUE;
	UNLOCK(&bpmap_privaddr_mutex, PLBASE);
	return bpmap_private_addr;
}

/*
 * STATIC void
 * bpmap_release_private_addr(void)
 *	Release use of bp_mapin() reserved virtual address acquired by
 *	bpmap_get_private_addr().
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
bpmap_release_private_addr(void)
{
	pl_t oldpri = LOCK(&bpmap_privaddr_mutex, PLHI);
	bpmap_privaddr_inuse = B_FALSE;
	UNLOCK(&bpmap_privaddr_mutex, oldpri);
	SV_SIGNAL(&bpmap_privaddr_sv, 0);
}

/*
 * void
 * bp_mapin(buf_t *bp)
 *	Map a buffer into a kernel virtual address.
 *
 * Calling/Exit State:
 *	If the buffer, bp, is a B_PAGEIO or B_PHYS buffer, and it's not
 *	already mapped in, map the data into a newly allocated kernel
 *	virtual address range.  The buffer's b_addr is updated accordingly.
 */
void
bp_mapin(buf_t *bp)
{
	ulong_t npages;
	vaddr_t kaddr, uvaddr;
	ppid_t ppid;

	ASSERT(bp->b_flags & B_KERNBUF);
	ASSERT((bp->b_flags & (B_PAGEIO|B_PHYS)) != (B_PAGEIO|B_PHYS));

	/*
	 * If not a B_PAGEIO or B_PHYS buffer, or already mapped, just return.
	 */
	if (!(bp->b_flags & (B_PAGEIO|B_PHYS)) ||
	    (bp->b_flags & B_REMAPPED))
		return;

	ASSERT(!(bp->b_flags & B_WASPHYS));

	bp->b_orig_addr = bp->b_un.b_addr;

	npages = btopr(bp->b_bufsize + ((vaddr_t)bp->b_un.b_addr & PAGEOFFSET));

	if (bp->b_flags & B_PAGEIO) {
		kaddr = (vaddr_t)kpg_pl_mapin(npages, bp->b_pages,
					      PROT_READ|PROT_WRITE,
					      (NOMEMWAIT() ? NOSLEEP : SLEEP));
		if (kaddr == NULL) {
			ASSERT(NOMEMWAIT());
			ASSERT(npages <= BPMAP_PRIVADDR_SIZE);
			kaddr = bpmap_get_private_addr();
			segkmem_pl_mapin(kpgseg, kaddr, npages, bp->b_pages,
					 PROT_READ|PROT_WRITE);
			bp->b_flags |= B_PRIVADDR;
		}
		bp->b_un.b_addr = (caddr_t)kaddr +
				   ((vaddr_t)bp->b_un.b_addr & PAGEOFFSET);
		bp->b_flags &= ~B_PAGEIO;
	} else if (bp->b_proc != NULL) {
		ASSERT(bp->b_flags & B_PHYS);
		bp->b_flags = (bp->b_flags & ~B_PHYS) | B_WASPHYS;
		bp->b_orig_proc = bp->b_proc;
		uvaddr = (vaddr_t)bp->b_un.b_addr;
		kaddr = kpg_vm_alloc(npages, SLEEP);
		bp->b_un.b_addr = (caddr_t)kaddr +
				   ((vaddr_t)bp->b_un.b_addr & PAGEOFFSET);
		while (npages-- != 0) {
			ppid = hat_vtoppid(bp->b_proc->p_as, uvaddr);
			segkmem_ppid_mapin(kpgseg, kaddr, 1, ppid,
					   PROT_READ|PROT_WRITE);
			kaddr += PAGESIZE;
			uvaddr += PAGESIZE;
		}
		bp->b_proc = NULL;
	} else {
		ASSERT(!(bp->b_flags & B_PHYS));
		ASSERT(KADDR(bp->b_un.b_addr));
		/* Buffer is already kernel virtual; nothing to do */
		return;
	}

	bp->b_flags |= B_REMAPPED;
	bp->b_addrtype = BA_KVIRT;
}

/*
 * void
 * bp_mapout(buf_t *bp)
 *	Unmap a buffer mapped by bp_mapin().
 *
 * Calling/Exit State:
 *	On return, any mapping allocated by bp_mapin() for the buffer is
 *	unmapped and released.  If there was no such mapping, nothing is done.
 */
void
bp_mapout(buf_t *bp)
{
	uint_t	pagoff;
	ulong_t	npages;

	ASSERT(bp->b_flags & B_KERNBUF);

	if (!(bp->b_flags & B_REMAPPED))
		return;

	ASSERT(!(bp->b_flags & B_PHYS));

	pagoff = ((vaddr_t)bp->b_orig_addr & PAGEOFFSET);
	npages = btopr(bp->b_bufsize + pagoff);

	if (bp->b_flags & B_PRIVADDR) {
		ASSERT(!(bp->b_flags & B_WASPHYS));
		segkmem_mapout(kpgseg, (vaddr_t)bp->b_un.b_addr - pagoff,
			       npages);
		bp->b_flags &= ~B_PRIVADDR;
		bpmap_release_private_addr();
	} else
		kpg_mapout(bp->b_un.b_addr - pagoff, npages);

	bp->b_un.b_addr = bp->b_orig_addr;

	if (bp->b_flags & B_WASPHYS) {
		bp->b_proc = bp->b_orig_proc;
		bp->b_addrtype = BA_UVIRT;
		bp->b_flags = (bp->b_flags & ~B_WASPHYS) | B_PHYS;
	} else {
		bp->b_addrtype = BA_PAGELIST;
		bp->b_flags |= B_PAGEIO;
	}

	bp->b_flags &= ~B_REMAPPED;
}


boolean_t
brkup_prep(physreq_t *preqp, int flags)
{
	brkup_pool_t *poolp;

	ASSERT((flags & ~KM_NOSLEEP) == 0);
	ASSERT(preqp->phys_max_scgth <= PAGESIZE / sizeof(ba_scgth_t) / 2);

	FSPIN_LOCK(&brkup_fspin);
	for (poolp = brkup_pools; poolp != NULL; poolp = poolp->bp_next) {
		if (preqp->phys_dmasize != 0 &&
		    preqp->phys_dmasize != poolp->bp_physreq.phys_dmasize)
			continue;
		if (preqp->phys_max_scgth != 0 &&
		    preqp->phys_max_scgth != poolp->bp_physreq.phys_max_scgth)
			continue;
		FSPIN_UNLOCK(&brkup_fspin);
		preqp->phys_brkup_poolp = poolp;
		return B_TRUE;
	}
	FSPIN_UNLOCK(&brkup_fspin);

	poolp = kmem_alloc(sizeof *poolp, flags);
	if (poolp == NULL)
		return B_FALSE;

	poolp->bp_private_page = kmem_alloc_physreq(PAGESIZE, preqp,
						    flags | KM_PHYSCONTIG);
	if (poolp->bp_private_page == NULL) {
		kmem_free(poolp, sizeof *poolp);
		return B_FALSE;
	}

	preqp->phys_brkup_poolp = poolp;
	poolp->bp_physreq = *preqp;

	poolp->bp_freelist = NULL;
	poolp->bp_npages = 0;

	SLEEP_INIT(&poolp->bp_private_lock, 0, &brkup_priv_lkinfo, flags);

	/*
	 * Add this new pool to the list of pools.  There is a potential
	 * race condition here in which the same type of pool can be created
	 * by multiple agents and both (all) of them added to the list.
	 * This is, however, benign (though a bit wasteful of space), since
	 * we don't have to have everything use the same pool.
	 */
	FSPIN_LOCK(&brkup_fspin);
	poolp->bp_next = brkup_pools;
	brkup_pools = poolp;
	FSPIN_UNLOCK(&brkup_fspin);

	return B_TRUE;
}

/*
 * STATIC ba_scgth_t *
 * scgth_alloc(physreq_t *preqp, boolean_t mustget)
 *	Allocate a scatter/gather list from the pool.
 *
 * Calling/Exit State:
 *	If mustget, this routine will block until it can get the memory.
 *	If !mustget, the routine will not block, returning NULL if the
 *	memory is not available.
 *
 *	count must not be more than half a page worth.
 */
STATIC ba_scgth_t *
scgth_alloc(physreq_t *preqp, boolean_t mustget)
{
	brkup_pool_t *poolp;
	scgth_entry_t *sgep;
	int sleepflag;
	void *mem;
	size_t sz;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	poolp = preqp->phys_brkup_poolp;

	(void)LOCK_PLMIN(&scgth_mutex);
	for (;;) {
		if ((sgep = poolp->bp_freelist) != NULL) {
			poolp->bp_freelist = sgep->sge_nextfree;
			UNLOCK_PLMIN(&scgth_mutex, PLBASE);
			return sgep->sge_scgth;
		}
		if (!mustget) {
			UNLOCK_PLMIN(&scgth_mutex, PLBASE);
			return NULL;
		}
		UNLOCK_PLMIN(&scgth_mutex, PLBASE);
		if (NOMEMWAIT() || poolp->bp_npages != 0)
			sleepflag = KM_NOSLEEP;
		else
			sleepflag = KM_SLEEP;
		mem = kmem_alloc_physreq(PAGESIZE, preqp,
					 sleepflag | KM_PHYSCONTIG);
		if (mem != NULL) {
			/* Mark the page as belonging to this pool */
			*(brkup_pool_t **)mem = poolp;
			/* Chop up the page and link it onto the freelist */
			sz = sizeof(scgth_entry_t) +
			      (preqp->phys_max_scgth - 1) * sizeof(ba_scgth_t);
			sgep = (scgth_entry_t *)mem + 1;
			(void)LOCK_PLMIN(&scgth_mutex);
			while ((char *)sgep + sz < (char *)mem + PAGESIZE) {
				sgep->sge_nextfree = poolp->bp_freelist;
				poolp->bp_freelist = sgep;
			}
			++poolp->bp_npages;
			continue;
		}
		if (NOMEMWAIT()) {
			SLEEP_LOCK(&poolp->bp_private_lock, PRIBUF);
			SLEEP_DISOWN(&poolp->bp_private_lock);
			/* Mark private page as belonging to this pool */
			*(brkup_pool_t **)poolp->bp_private_page = poolp;
			sgep = (scgth_entry_t *)poolp->bp_private_page + 1;
			/* Mark this entry as private, for scgth_free */
			sgep->sge_nextfree = sgep;
			return sgep->sge_scgth;
		}
		(void)LOCK_PLMIN(&scgth_mutex);
		if (poolp->bp_freelist == NULL) {
			SV_WAIT(&scgth_sv, PRIBUF, &scgth_mutex);
			(void)LOCK_PLMIN(&scgth_mutex);
		}
	}
	/* NOTREACHED */
}

/*
 * void
 * scgth_free(ba_scgth_t *scgth)
 *	Free a scatter/gather list.
 *
 * Calling/Exit State:
 *	No locks may be held on entry.
 */
void
scgth_free(ba_scgth_t *scgth)
{
	brkup_pool_t *poolp;
	scgth_entry_t *sgep;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	poolp = *(brkup_pool_t **)((vaddr_t)scgth & PAGEMASK);
	sgep = (scgth_entry_t *)
		((char *)scgth - offsetof(scgth_entry_t, sge_scgth));
	if (sgep->sge_nextfree == sgep) {
		/*
		 * Allocation was from a private overflow page;
		 * release it now.
		 */
		SLEEP_UNLOCK(&poolp->bp_private_lock);
		return;
	}

	(void)LOCK_PLMIN(&scgth_mutex);
	sgep->sge_nextfree = poolp->bp_freelist;
	poolp->bp_freelist = sgep;
	UNLOCK_PLMIN(&scgth_mutex, PLBASE);
	if (SV_BLKD(&scgth_sv))
		SV_BROADCAST(&scgth_sv, 0);
}


#ifdef DEBUG
STATIC void
ASSERT_CONSTRAINTS_MET(buf_t *bp, const bcb_t *bcbp)
{
	dl_t loffset, lgran;

	ASSERT(bp->b_bcount % bcbp->bcb_granularity == 0);
	loffset.dl_lop = bp->b_blkno;
	loffset.dl_hop = 0;
	loffset = lshiftl(loffset, SCTRSHFT);
	loffset.dl_lop |= bp->b_blkoff;
	lgran.dl_lop = bcbp->bcb_granularity;
	lgran.dl_hop = 0;
	lgran = lmod(loffset, lgran);
	ASSERT(lgran.dl_hop == 0);
	ASSERT(lgran.dl_lop == 0);
	ASSERT(bcbp->bcb_max_xfer == 0 ||
	       bp->b_bcount <= bcbp->bcb_max_xfer);
#ifndef NO_RDMA
	ASSERT(RDMA_REQUIREMENT(bcbp->bcb_physreqp) == RDMA_NOTREQUIRED ||
	       !rdma_must_copy(bp, bcbp->bcb_physreqp));
#endif
}
#else /* !DEBUG */
#define ASSERT_CONSTRAINTS_MET(bp, bcbp)	/**/
#endif /* DEBUG */


#define add_blkoff(bp, size) \
		(((bp)->b_blkno += btodt((size) + (bp)->b_blkoff)), \
		 ((bp)->b_blkoff = (((size) + (bp)->b_blkoff) & (NBPSCTR - 1))))

typedef struct {
	const bcb_t	*bci_bcbp;
	caddr_t		bci_copy_addr;
	uint_t		bci_headcount;
	uint_t		bci_tailcount;
	uint_t		bci_datasize;
	uint_t		bci_xfersize;
	buf_t		*bci_parentbp;
	uint_t		bci_offset;
	uint_t		bci_orig_resid;
	void		(*bci_strat)();
	uint_t		bci_flags;
	void		(*bci_nextstate)();
} brkupcopy_info_t;

#define BC_BREAKUP	(1 << 0)
#define BC_PRIVATE_PAGE	(1 << 1)

#define MAXCOPYSZ	(16 * 1024)

STATIC void brkupcopy_head_read();
STATIC void brkupcopy_head_preread();
STATIC void brkupcopy_head_write();
STATIC void brkupcopy_body_read();
STATIC void brkupcopy_body_write();
STATIC void brkupcopy_tail_read();
STATIC void brkupcopy_tail_preread();
STATIC void brkupcopy_tail_write();

STATIC void
brkupcopy_head_read(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_headcount);
	add_blkoff(bp, bcinfop->bci_bcbp->bcb_granularity);
	bcopy(bcinfop->bci_copy_addr + bcinfop->bci_offset,
	      parentbp->b_un.b_addr,
	      bcinfop->bci_datasize);
	parentbp->b_resid -= bcinfop->bci_datasize;
	if (parentbp->b_resid == bcinfop->bci_tailcount) {
		bcinfop->bci_nextstate = brkupcopy_tail_read;
		bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
		bcinfop->bci_datasize = bcinfop->bci_tailcount;
	} else {
		bcinfop->bci_nextstate = brkupcopy_body_read;
		bcinfop->bci_datasize = bp->b_bufsize;
		if (bcinfop->bci_datasize > parentbp->b_resid)
			bcinfop->bci_datasize = parentbp->b_resid;
		bp->b_bcount = bcinfop->bci_datasize;
	}
	bcinfop->bci_offset = 0;
}

STATIC void
brkupcopy_head_preread(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_headcount);
	bcopy(parentbp->b_un.b_addr,
	      bcinfop->bci_copy_addr + bcinfop->bci_offset,
	      bcinfop->bci_datasize);
	bcinfop->bci_nextstate = brkupcopy_head_write;
	bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
	bp->b_flags &= ~B_READ;
}

STATIC void
brkupcopy_head_write(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_headcount);
	add_blkoff(bp, bcinfop->bci_bcbp->bcb_granularity);
	parentbp->b_resid -= bcinfop->bci_datasize;
	bcinfop->bci_datasize = bp->b_bufsize;
	if (bcinfop->bci_datasize > parentbp->b_resid)
		bcinfop->bci_datasize = parentbp->b_resid;
	if (parentbp->b_resid == bcinfop->bci_tailcount) {
		bcinfop->bci_nextstate = brkupcopy_tail_preread;
		ASSERT(bcinfop->bci_datasize == bcinfop->bci_tailcount);
		bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
		bp->b_flags |= B_READ;
	} else {
		bcinfop->bci_nextstate = brkupcopy_body_write;
		bcopy(parentbp->b_un.b_addr +
			parentbp->b_bcount - parentbp->b_resid,
		      bcinfop->bci_copy_addr,
		      bcinfop->bci_datasize);
		bp->b_bcount = bcinfop->bci_datasize;
	}
	bcinfop->bci_offset = 0;
}

STATIC void
brkupcopy_body_read(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	add_blkoff(bp, bcinfop->bci_datasize);
	bcopy(bcinfop->bci_copy_addr,
	      parentbp->b_un.b_addr + parentbp->b_bcount - parentbp->b_resid,
	      bcinfop->bci_datasize);
	parentbp->b_resid -= bcinfop->bci_datasize;
	if (parentbp->b_resid == bcinfop->bci_tailcount) {
		bcinfop->bci_nextstate = brkupcopy_tail_read;
		bcinfop->bci_datasize = bcinfop->bci_tailcount;
		bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
	} else {
		if (parentbp->b_resid < bcinfop->bci_datasize) {
			ASSERT(parentbp->b_resid > bcinfop->bci_tailcount);
			bcinfop->bci_datasize =
				parentbp->b_resid - bcinfop->bci_tailcount;
		}
		bp->b_bcount = bcinfop->bci_datasize;
	}
}

STATIC void
brkupcopy_body_write(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	add_blkoff(bp, bcinfop->bci_datasize);
	parentbp->b_resid -= bcinfop->bci_datasize;
	if (parentbp->b_resid == bcinfop->bci_tailcount) {
		bcinfop->bci_nextstate = brkupcopy_tail_preread;
		bcinfop->bci_datasize = bcinfop->bci_tailcount;
		bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
		bp->b_flags |= B_READ;
	} else {
		if (parentbp->b_resid < bcinfop->bci_datasize) {
			ASSERT(parentbp->b_resid > bcinfop->bci_tailcount);
			bcinfop->bci_datasize =
				parentbp->b_resid - bcinfop->bci_tailcount;
		}
		bcopy(parentbp->b_un.b_addr +
			parentbp->b_bcount - parentbp->b_resid,
		      bcinfop->bci_copy_addr,
		      bcinfop->bci_datasize);
		bp->b_bcount = bcinfop->bci_datasize;
	}
}

/* ARGSUSED */
STATIC void
brkupcopy_tail_read(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_tailcount);
	bcopy(bcinfop->bci_copy_addr,
	      parentbp->b_un.b_addr +
			parentbp->b_bcount - bcinfop->bci_datasize,
	      bcinfop->bci_datasize);
	ASSERT(parentbp->b_resid == bcinfop->bci_datasize);
	parentbp->b_resid = 0;
}

STATIC void
brkupcopy_tail_preread(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_tailcount);
	bcopy(parentbp->b_un.b_addr +
			parentbp->b_bcount - bcinfop->bci_tailcount,
	      bcinfop->bci_copy_addr,
	      bcinfop->bci_datasize);
	bcinfop->bci_nextstate = brkupcopy_tail_write;
	bp->b_bcount = bcinfop->bci_bcbp->bcb_granularity;
	bp->b_flags &= ~B_READ;
}

/* ARGSUSED */
STATIC void
brkupcopy_tail_write(brkupcopy_info_t *bcinfop, buf_t *bp, buf_t *parentbp)
{
	ASSERT(bcinfop->bci_datasize == bcinfop->bci_tailcount);
	ASSERT(parentbp->b_resid == bcinfop->bci_tailcount);
	parentbp->b_resid = 0;
}


STATIC void (*brkupcopy_start_state[2][2][2])() = {
	/* B_READ   headcount == 0  bodycount == 0 */	brkupcopy_tail_read,
	/* B_READ   headcount == 0  bodycount != 0 */	brkupcopy_body_read,
	/* B_READ   headcount != 0  bodycount == 0 */	brkupcopy_head_read,
	/* B_READ   headcount != 0  bodycount != 0 */	brkupcopy_head_read,
	/* B_WRITE  headcount == 0  bodycount == 0 */	brkupcopy_tail_preread,
	/* B_WRITE  headcount == 0  bodycount != 0 */	brkupcopy_body_write,
	/* B_WRITE  headcount != 0  bodycount == 0 */	brkupcopy_head_preread,
	/* B_WRITE  headcount != 0  bodycount != 0 */	brkupcopy_head_preread
};


STATIC void
brkupcopy_iodone(buf_t *bp)
{
	brkupcopy_info_t *bcinfop = bp->b_misc;
	buf_t *parentbp = bcinfop->bci_parentbp;
	brkup_pool_t *poolp;
	int err;

	/*
	 * Make sure we're not at interrupt level, since we may do large
	 * data copies and may call breakup and/or strategy routines.
	 */
	if (servicing_interrupt()) {
		bdelaydone(bp);
		return;
	}

	ASSERT(bp->b_resid <= bcinfop->bci_xfersize);
	ASSERT(bcinfop->bci_offset <= bcinfop->bci_xfersize);
	ASSERT(bcinfop->bci_datasize <= bcinfop->bci_xfersize);

	/*
	 * If we have a residual, clip it to fit the actual data.
	 */
	if (bp->b_resid) {
		uint_t datasize, tail;

		datasize = bcinfop->bci_datasize;
		tail = bcinfop->bci_xfersize - datasize - bcinfop->bci_offset;
		if (bp->b_resid <= tail)
			bp->b_resid = 0;
		else {
			bp->b_resid -= tail;
			if (bp->b_resid > datasize)
				bp->b_resid = datasize;
		}
	}

	/*
	 * Advance to the next state: do any copies necessary, advance
	 * blkno/blkoff and b_resid as appropriate, set up new counts.
	 */
	(*bcinfop->bci_nextstate)(bcinfop, bp, parentbp);

	/*
	 * Check for errors and termination conditions.
	 */
	if ((err = geterror(bp)) != 0 ||
	    parentbp->b_resid == 0 || bp->b_resid) {
		/*
		 * Transfer errors/partials to the "parent".
		 */
		bioerror(parentbp, err);
		parentbp->b_resid += bp->b_resid;

		/*
		 * Free the memory for the copy buffer and the accounting
		 * structure.
		 */
		if (bcinfop->bci_flags & BC_PRIVATE_PAGE) {
			/*
			 * Allocation was from a private overflow page;
			 * release it now.
			 */
			poolp = bcinfop->bci_bcbp->bcb_physreqp->
							phys_brkup_poolp;
			SLEEP_UNLOCK(&poolp->bp_private_lock);
		} else {
			kmem_free(bcinfop->bci_copy_addr, bp->b_bufsize);
		}
		kmem_free(bcinfop, sizeof *bcinfop);

		if (bp->b_flags & B_ASYNC) {
			freerbuf(bp);
			parentbp->b_addrtype = 0;
			biodone(parentbp);
		} else {
			bp->b_iodone = NULL;
			biodone(bp);
		}
		return;
	}

	/*
	 * Start the next piece.
	 */
	bioreset(bp);
	bp->b_un.b_addr = bcinfop->bci_copy_addr;
	bcinfop->bci_xfersize = bp->b_bcount;
	bp->b_resid = bcinfop->bci_orig_resid;	/* just in case */
	if (bcinfop->bci_flags & BC_BREAKUP)
		buf_breakup(bcinfop->bci_strat, bp, bcinfop->bci_bcbp);
	else {
		ASSERT_CONSTRAINTS_MET(bp, bcinfop->bci_bcbp);
		(*bcinfop->bci_strat)(bp);
	}
}

STATIC void
bufbrkup_copy(void (*strat)(buf_t *), buf_t *bp, const bcb_t *bcbp, uint_t off)
{
	uint_t totalcount, headcount, tailcount, bodycount;
	caddr_t copy_addr;
	uint_t copy_size;
	size_t granularity;
	size_t max_xfer;
	brkupcopy_info_t *bcinfop;
	const physreq_t *preqp = bcbp->bcb_physreqp;
	brkup_pool_t *poolp;
	buf_t *cbp;
	uint_t bcbflags, cbpflags;
	int flags = (NOMEMWAIT() ? KM_NOSLEEP : KM_SLEEP);

	totalcount = bp->b_bcount;
	granularity = bcbp->bcb_granularity;
	max_xfer = bcbp->bcb_max_xfer;
	bcbflags = bcbp->bcb_flags;

	ASSERT(off < granularity);

	/*
	 * Compute sizes of unaligned head and tail pieces.
	 */
	if ((headcount = granularity - off) == granularity)
		headcount = 0;
	else if (headcount > totalcount)
		headcount = totalcount;
	bodycount = totalcount - headcount;
	tailcount = (bodycount % granularity);
	bodycount -= tailcount;
	ASSERT((bodycount % granularity) == 0);

	/*
	 * Determine size of copy buffer.
	 */
	copy_size = bodycount;
	if (!(bcbflags & BCB_ONE_PIECE)) {
		if (copy_size > MAXCOPYSZ)
			copy_size = MAXCOPYSZ - (MAXCOPYSZ % granularity);
		if (copy_size == 0)
			copy_size = granularity;
	} else {
		if (copy_size == 0)
			copy_size = granularity;
		/*
		 * If BCB_ONE_PIECE and the job can't be handled in one piece,
		 * fail it.
		 */
		if (((headcount | tailcount) &&
		     (!(bp->b_flags & B_READ) ||
		      (headcount && tailcount) || bodycount)) ||
		    (max_xfer && copy_size > max_xfer) ||
		    (copy_size > PAGESIZE && flags == KM_NOSLEEP)) {
			bioerror(bp, EINVAL);
			bp->b_resid = bp->b_bcount;
			bp->b_addrtype = 0;
			biodone(bp);
			return;
		}
	}
	if (copy_size > max_xfer && max_xfer != 0)
		copy_size = max_xfer - (max_xfer % granularity);
	ASSERT((copy_size % granularity) == 0);

	/*
	 * Allocate and initialize an accounting structure to keep track of
	 * the state of the transfer.
	 */
	bcinfop = kmem_alloc(sizeof *bcinfop, KM_SLEEP);
	bcinfop->bci_bcbp = bcbp;
	bcinfop->bci_headcount = headcount;
	bcinfop->bci_tailcount = tailcount;
	bcinfop->bci_parentbp = bp;
	bcinfop->bci_offset = off;
	bcinfop->bci_strat = strat;
	bcinfop->bci_flags = 0;
	bcinfop->bci_orig_resid = bp->b_resid;

	/*
	 * Allocate a "child" copy buffer and initialize its constant fields.
	 *
	 * Until all callers set PREQ_PHYSCONTIG, we need to handle
	 * BCB_PHYSCONTIG as well.
	 */
	cbp = getrbuf(KM_SLEEP);
	if ((bcbflags & BCB_PHYSCONTIG) ||
	    (preqp->phys_flags & PREQ_PHYSCONTIG)) {
		/*
		 * Multiple-page physcontig allocations are potentially
		 * deadlock-prone.  Try to avoid them.
		 */
		if (copy_size > PAGESIZE &&
		    !(bcbflags & BCB_ONE_PIECE)) {
			copy_size = granularity;
			while (copy_size + granularity <= PAGESIZE)
				copy_size += granularity;
		}

		flags |= KM_PHYSCONTIG;
	}
	copy_addr = kmem_alloc_physreq(copy_size, preqp, flags);
	if (copy_addr == NULL) {
		/*
		 * Get the private page, reserved for critical use.
		 * First, copy_size must be clipped to fit in the page.
		 */
		ASSERT(flags & KM_NOSLEEP);
		if (copy_size > PAGESIZE) {
			ASSERT(!(bcbflags & BCB_ONE_PIECE));
			copy_size = granularity;
			while (copy_size + granularity <= PAGESIZE)
				copy_size += granularity;
		}
		poolp = preqp->phys_brkup_poolp;
		SLEEP_LOCK(&poolp->bp_private_lock, PRIBUF);
		SLEEP_DISOWN(&poolp->bp_private_lock);
		copy_addr = poolp->bp_private_page;
		bcinfop->bci_flags = BC_PRIVATE_PAGE;
	}
	bcinfop->bci_copy_addr = copy_addr;
	cbp->b_un.b_addr = copy_addr;
	cbp->b_addrtype = BA_KVIRT;
	cbp->b_bufsize = copy_size;
	cbp->b_flags |= (bp->b_flags & (B_READ|B_ASYNC));
	cbp->b_proc = bp->b_proc;
	cbp->b_priv = bp->b_priv;
	cbp->b_priv2 = bp->b_priv2;
	cbp->b_edev = bp->b_edev;
	/*
	 * Just in case we were called for a very old driver
	 * (from dma_pageio or pageio_breakup), copy b_odev
	 * from the parent buffer.
	 */
	cbp->b_odev = bp->b_odev;
	cbp->b_resid = bp->b_resid;	/* just in case */
	cbp->b_misc = bcinfop;
	cbp->b_iodone = brkupcopy_iodone;

	/*
	 * If we're not allowed to wait for memory, and there are going to be
	 * multiple pieces, we have to handle the job synchronously to make
	 * sure all allocations are done in the original context.  This ensures
	 * that NOMEMWAIT() is set for any attempted allocations.  On the other
	 * hand, if the driver guarantees it is not going to wait for any
	 * allocations, we can avoid this.
	 */
	if ((flags & KM_NOSLEEP) && !(bcbflags & BCB_NOMEMWAIT)) {
		if (headcount | tailcount) {
			if (!(bp->b_flags & B_READ) ||
			    (headcount && tailcount) || bodycount)
				cbp->b_flags &= ~B_ASYNC;
		}
		if (bodycount > copy_size)
			cbp->b_flags &= ~B_ASYNC;
	}

	/*
	 * We're going to need kernel virtual on both buffers in order
	 * to copy the data, so convert the parent to BA_KVIRT now.
	 */
	bp_mapin(bp);

	bp->b_resid = totalcount;

	/*
	 * Start the first piece.
	 */
	cbp->b_blkno = bp->b_blkno;
	cbp->b_blkoff = bp->b_blkoff;
	cbp->b_bcount = copy_size;
	bcinfop->bci_datasize = copy_size;
	if (headcount || (tailcount && !bodycount)) {
		cbp->b_bcount = granularity;
		bcinfop->bci_datasize = tailcount;
		if (headcount) {
			bcinfop->bci_datasize = headcount;
			ASSERT(cbp->b_blkno >= btodt(off));
			cbp->b_blkno -= btodt(off);
			off &= (NBPSCTR - 1);
			if ((ushort_t)off > cbp->b_blkoff) {
				ASSERT(cbp->b_blkno != 0);
				--cbp->b_blkno;
				cbp->b_blkoff += NBPSCTR - (ushort_t)off;
			} else
				cbp->b_blkoff -= (ushort_t)off;
		}
	}
	ASSERT(cbp->b_bcount <= cbp->b_bufsize);
	bcinfop->bci_xfersize = cbp->b_bcount;

	/*
	 * Compute the initial starting state, based on the transfer direction
	 * and whether there are any head and/or tail pieces.
	 */
	bcinfop->bci_nextstate = brkupcopy_start_state[!(bp->b_flags & B_READ)]
						      [headcount != 0]
						      [bodycount != 0];
	if (bcinfop->bci_nextstate == brkupcopy_body_write) {
		ASSERT(headcount == 0);
		bcopy(bp->b_un.b_addr, copy_addr, copy_size);
	} else if (bcinfop->bci_nextstate == brkupcopy_head_preread ||
		   bcinfop->bci_nextstate == brkupcopy_tail_preread) {
		ASSERT(!(cbp->b_flags & B_READ));
		cbp->b_flags |= B_READ;
	}

	/*
	 * Get a copy of the child's flags, which we need to test later.
	 * We must, however, sample this before calling the strategy routine.
	 */
	cbpflags = cbp->b_flags;

	if (!(bcbp->bcb_addrtypes & BA_KVIRT)) {
		/*
		 * If kernel virtual is not acceptable,
		 * recurse into buf_breakup to get it converted.
		 */
		ASSERT(bp->b_iodone != brkupcopy_iodone);
		bcinfop->bci_flags |= BC_BREAKUP;
		buf_breakup(strat, cbp, bcbp);
	} else {
		ASSERT_CONSTRAINTS_MET(cbp, bcbp);
		(*strat)(cbp);
	}

	/*
	 * For synchronous xfers, wait for the I/O to complete,
	 * then cleanup.
	 */
	if (!(cbpflags & B_ASYNC)) {
		biowait(cbp);
		freerbuf(cbp);
		bp->b_addrtype = 0;
		biodone(bp);
	}
}


/*
 * STATIC void
 * bufbrkup_transfer(buf_t *bp, buf_t *parentbp)
 *	Transfer results from a completed "child" buffer to the "parent".
 *
 * Calling/Exit State:
 *	On entry, I/O has been completed on the buffer, bp.
 *	On exit, any errors and/or residual count are transferred
 *	to the "parent" buffer, parentbp.
 */
STATIC void
bufbrkup_transfer(buf_t *bp, buf_t *parentbp)
{
	int err;
	uint_t resid;

	/*
	 * Transfer the error, if there isn't one already.
	 */
	if ((err = geterror(bp)) != 0 && !parentbp->b_error)
		bioerror(parentbp, err);

	/*
	 * If there's any residual count, transfer it to the parent,
	 * making sure that the one closest to the front of the buffer
	 * takes precedence.
	 */
	if (bp->b_resid != 0) {
		resid = parentbp->b_bcount -
			 dtob(bp->b_blkno - parentbp->b_blkno) -
			  bp->b_blkoff + parentbp->b_blkoff -
			   (bp->b_bufsize - bp->b_resid);
		if (parentbp->b_resid < resid)
			parentbp->b_resid = resid;
	}
}


STATIC void
bufbrkup_iodone(buf_t *bp)
{
	buf_t *parentbp = bp->b_misc;

	/*
	 * The bp_mapout which may be called in freerbuf must be performed
	 * at base level.
	 */
	if ((bp->b_flags & B_REMAPPED) && servicing_interrupt()) {
		bdelaydone(bp);
		return;
	}

	bufbrkup_transfer(bp, parentbp);

	/*
	 * If we're the last "child", complete the "parent".
	 */
	FSPIN_LOCK(&brkup_fspin);
	if (--parentbp->b_childcnt == 0) {
		FSPIN_UNLOCK(&brkup_fspin);
		parentbp->b_addrtype = 0;
		biodone(parentbp);
	} else
		FSPIN_UNLOCK(&brkup_fspin);

	/*
	 * Free up this child buffer.
	 */
	freerbuf(bp);
}


	/* max # concurrent I/Os from a single synchronous job */
#define MAXCONC_S	4

void
buf_breakup(void (*strat)(buf_t *), buf_t *bp, const bcb_t *bcbp)
{
	size_t granularity;
	size_t max_xfer;
	paddr_t boundary, align_mask;
	uint_t totalcount, bufcount, bcount, count;
	uint_t off, blkoff, pgoff;
	uint_t orig_resid;
	daddr_t blkno;
	o_dev_t odev;
	caddr_t vaddr, vaddr2;
	paddr_t paddr, paddr2;
	page_t *pp, *pp2;
	int flags;
	buf_t *parentbp;
	buf_t *cbp[MAXCONC_S];
	uint_t nbuf, i, n, maxconc;
	boolean_t do_phys, do_physcontig;
	proc_t *proc;
	physreq_t *preqp;
	ba_scgth_t *scgth = NULL;
	uint_t max_scgth, nscgth;
#ifndef NO_RDMA
	boolean_t do_rdma = B_FALSE;
#endif

#if !(PAGESIZE >= NBPSCTR)
#error buf_breakup assumes PAGESIZE >= NBPSCTR
#endif

	ASSERT(!servicing_interrupt());
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(bp->b_flags & B_KERNBUF);
	ASSERT(bp->b_bcount > 0);

	/*
	 * TEMPORARY CODE:
	 *
	 * Rather than converting the rest of the system to use b_addrtype
	 * right away, temporarily allow the buffer to come in to buf_breakup
	 * without b_addrtype set.  We convert here from the old flag settings
	 * to the corresponding address type code.
	 *
	 * WARNING: Drivers creating their own buffers and then calling
	 *	    buf_breakup should not take advantage of this; they
	 *	    should be sure to set b_addrtype.  Otherwise, they
	 *	    will not work when this code is removed.
	 */
	if (bp->b_addrtype == 0) {
		bp->b_addrtype = BA_KVIRT;
		if (bp->b_flags & B_PAGEIO)
			bp->b_addrtype = BA_PAGELIST;
		else if (bp->b_flags & B_PHYS)
			bp->b_addrtype = BA_UVIRT;
	}

	/*
	 * For now, at least, only handle the original three types coming in.
	 * In other words, don't allow buf_breakup to be called "recursively".
	 */
	ASSERT(bp->b_addrtype == BA_UVIRT ||
	       bp->b_addrtype == BA_KVIRT ||
	       bp->b_addrtype == BA_PAGELIST);
	/*
	 * ASSERT bcbp->bcb_addrtypes legal combinations:
	 *	BA_KVIRT [+ BA_UVIRT] [+ BA_PAGELIST]
	 *	BA_PHYS [+ BA_PAGELIST]
	 *	BA_PHYS [+ BA_SCGTH]
	 */
	ASSERT(bcbp->bcb_addrtypes & (BA_KVIRT|BA_PHYS));
	ASSERT((bcbp->bcb_addrtypes & (BA_KVIRT|BA_PHYS)) !=
					(BA_KVIRT|BA_PHYS));
	ASSERT((bcbp->bcb_addrtypes & (BA_KVIRT|BA_UVIRT)) != BA_UVIRT);
	ASSERT((bcbp->bcb_addrtypes & (BA_PHYS|BA_SCGTH)) != BA_SCGTH);
	ASSERT((bcbp->bcb_addrtypes & (BA_PAGELIST|BA_SCGTH)) !=
					(BA_PAGELIST|BA_SCGTH));
	ASSERT(bcbp->bcb_physreqp->phys_flags & PREQ_PREPPED);
	/* align must be non-zero and a power of 2 */
	ASSERT(bcbp->bcb_physreqp->phys_align != 0);
	ASSERT((bcbp->bcb_physreqp->phys_align &
		(bcbp->bcb_physreqp->phys_align - 1)) == 0);
	/* granularity must be non-zero */
	ASSERT(bcbp->bcb_granularity != 0);
	/* boundary must be zero or a power of 2 >= granularity */
	ASSERT(bcbp->bcb_physreqp->phys_boundary == 0 ||
	       (bcbp->bcb_physreqp->phys_boundary &
		(bcbp->bcb_physreqp->phys_boundary - 1)) == 0);
	ASSERT(bcbp->bcb_physreqp->phys_boundary == 0 ||
	       bcbp->bcb_physreqp->phys_boundary >= bcbp->bcb_granularity);
	/* max_xfer must be zero or a multiple of granularity */
	ASSERT(bcbp->bcb_max_xfer == 0 ||
	       (bcbp->bcb_max_xfer % bcbp->bcb_granularity) == 0);
	/* max_scgth must be at least 2 if BA_SCGTH accepted */
	ASSERT(!(bcbp->bcb_addrtypes & BA_SCGTH) ||
	       bcbp->bcb_physreqp->phys_max_scgth >= 2);

	maxconc = MAXCONC_S;
	if (bcbp->bcb_flags & BCB_SYNCHRONOUS) {
		if (bp->b_flags & B_ASYNC) {
			bioerror(bp, EINVAL);
			bp->b_resid = bp->b_bcount;
			bp->b_addrtype = 0;
			biodone(bp);
			return;
		}
		maxconc = 1;
	}

	/*
	 * Until all callers set PREQ_PHYSCONTIG, we need to handle
	 * BCB_PHYSCONTIG as well.
	 */
	preqp = bcbp->bcb_physreqp;
	do_phys = do_physcontig = (bcbp->bcb_flags & BCB_PHYSCONTIG) ||
	    			  (preqp->phys_flags & PREQ_PHYSCONTIG);
	boundary = preqp->phys_boundary;
	align_mask = preqp->phys_align - 1;
	if ((bp->b_addrtype & bcbp->bcb_addrtypes) != BA_PAGELIST &&
	    (bcbp->bcb_addrtypes & BA_PHYS))
		do_phys = do_physcontig = B_TRUE;
	else if (align_mask | boundary)
		do_phys = B_TRUE;

	/*
	 * First check for unaligned cases.  These are all handled
	 * by allocating a new (aligned) buffer and copying.
	 */
	if ((granularity = bcbp->bcb_granularity) != NBPSCTR) {
		/*
		 * Handle the general case.
		 *
		 * Check for misaligned starting offsets (b_blkno),
		 * partial sizes, and (if physical contiguity is
		 * required) misaligned addresses.  (The latter is
		 * actually checked in the loop below.)
		 */
		off = 0;
		if (granularity != 1) {
			if ((granularity & (NBPSCTR - 1)) == 0) {
				/*
				 * Avoid the double-long arithmetic if
				 * granularity is a multiple of sector size.
				 */
				off = dtob(bp->b_blkno % btodt(granularity)) +
					bp->b_blkoff;
				if (off != 0)
					goto unaligned_off;
			} else {
				dl_t loffset, lgran;

				loffset.dl_lop = bp->b_blkno;
				loffset.dl_hop = 0;
				loffset = lshiftl(loffset, SCTRSHFT);
				loffset.dl_lop |= bp->b_blkoff;
				lgran.dl_lop = granularity;
				lgran.dl_hop = 0;
				lgran = lmod(loffset, lgran);
				ASSERT(lgran.dl_hop == 0);
				if ((off = lgran.dl_lop) != 0)
					goto unaligned_off;
			}
			if ((bp->b_bcount % granularity) != 0)
				goto unaligned_off;
		}
	} else {
		/*
		 * Optimized check for the extremely common case
		 * of granularity == NBPSCTR.
		 */
		if ((off = bp->b_blkoff) != 0 ||
		    (bp->b_bcount & (NBPSCTR - 1)) != 0) {
unaligned_off:
			if (bcbp->bcb_flags & BCB_EXACT_SIZE) {
				bioerror(bp, EINVAL);
				bp->b_resid = bp->b_bcount;
				bp->b_addrtype = 0;
				biodone(bp);
				return;
			}
unaligned:
			/*
			 * Handle unaligned copy.
			 */
			bufbrkup_copy(strat, bp, bcbp, off);
			return;
		}
	}
#ifndef NO_RDMA
	switch (RDMA_REQUIREMENT(preqp)) {
	case RDMA_REQUIRED:
		do_rdma = rdma_must_copy(bp, preqp);
		if (do_rdma) {
			if (bp->b_addrtype != BA_PAGELIST)
				goto unaligned;
			rdma_substitute_pages(bp, preqp);
		}
		break;
	case RDMA_IMPOSSIBLE:
		bioerror(bp, EINVAL);
		bp->b_resid = bp->b_bcount;
		bp->b_addrtype = 0;
		biodone(bp);
		return;
	default:
		ASSERT(RDMA_REQUIREMENT(preqp) == RDMA_NOTREQUIRED);
		break;
	}
#endif /* NO_RDMA */

	/*
	 * See if we need to create a virtual mapping to the buffer.
	 */
	if ((bp->b_addrtype & bcbp->bcb_addrtypes) == 0 &&
	    (bcbp->bcb_addrtypes & BA_KVIRT))
		bp_mapin(bp);

	/*
	 * Not using scatter/gather (yet).
	 * Set max_scgth to one to indicate one piece per buffer.
	 */
	max_scgth = 1;

	if ((max_xfer = bcbp->bcb_max_xfer) == 0)
		max_xfer = roundup(MAXBIOSIZE, granularity);

	parentbp = bp;
	totalcount = bp->b_bcount;
	pp = bp->b_pages;
	vaddr = (caddr_t)((vaddr_t)bp->b_un.b_addr & PAGEMASK);
	pgoff = ((size_t)bp->b_un.b_addr & PAGEOFFSET);
	proc = bp->b_proc;
	blkno = bp->b_blkno;
	blkoff = bp->b_blkoff;
	odev = bp->b_odev;

	flags = (bp->b_flags & (B_READ|B_ASYNC));

	nscgth = nbuf = i = 0;
	bufcount = 0;

	for (;;) {
		bcount = totalcount;

		if (do_phys) {
			/*
			 * If we're checking for physical alignments and/or
			 * boundaries, we first have to compute the physical
			 * address of the start of this piece of the buffer.
			 * The way we do this depends on the address type of
			 * the (parent) buffer.
			 */
			if (parentbp->b_addrtype == BA_PAGELIST)
				paddr = page_pptophys(pp2 = pp);
			else
				paddr = vtop(vaddr2 = vaddr, parentbp->b_proc);
			paddr += pgoff;
			/*
			 * If the alignment is not correct (this can only
			 * happen the first time through) revert to the
			 * unaligned copy case.
			 */
			if ((paddr & align_mask) != 0) {
				ASSERT(nbuf + nscgth == 0); /* first time */
				goto unaligned;
			}
			/*
			 * Reduce the xfer count for this piece if we would
			 * cross a (DMA) boundary.
			 */
			if (boundary) {
				count = boundary - (paddr & (boundary - 1));
				if (count < bcount)
					bcount = count;
			}
			if (do_physcontig) {
				/*
				 * Reduce the xfer count for this piece
				 * if we would encounter a discontiguity
				 * in the physical addresses.
				 */
				count = PAGESIZE - pgoff;
				while (count < bcount) {
					if (parentbp->b_addrtype ==
					     BA_PAGELIST) {
						pp2 = pp2->p_next;
						paddr2 = page_pptophys(pp2);
					} else {
						vaddr2 += PAGESIZE;
						paddr2 = vtop(vaddr2,
							      parentbp->b_proc);
					}
					if (paddr2 != paddr + count) {
						bcount = count;
						break;
					}
					count += PAGESIZE;
				}
			}
		}
		/*
		 * Reduce the xfer count for this piece if we would
		 * exceed the maximum transfer size.
		 */
		if (max_xfer && bcount > max_xfer)
			bcount = max_xfer;

		totalcount -= bcount;

		/*
		 * Determine whether we can (so far) handle the entire job
		 * as a single transfer.  If not, we must allocate "child"
		 * buffers for each piece, copying relevant info from the
		 * original ("parent") buffer.
		 *
		 * We delay buffer allocations and setup as long as possible,
		 * so that the single-transfer case can be handled using the
		 * original buffer, thus saving the allocation/setup costs.
		 * This is complicated by the ability to use scatter/gather
		 * lists, allowing multiple pieces to be handled with a
		 * single transfer.  In the scatter/gather case, we delay
		 * allocation of the first child buffer across multiple pieces,
		 * because the entire job may be able to fit in a single
		 * scatter/gather list.  In fact, with a high-performance
		 * system, this will be the dominant case.
		 *
		 * Decisions about the number of transfers cannot efficiently
		 * be made ahead of time, since the pieces may be of variable
		 * sizes.
		 */
		if (nbuf == 0) {
		    if (totalcount != 0) {
			if (nscgth == 0) {
				/*
				 * This is the first time, and we're going to
				 * have to split the job up into multiple
				 * pieces. If this would cause a granularity
				 * problem, revert to the unaligned copy case.
				 */
				if (granularity != NBPSCTR) {
					if ((bcount % granularity) != 0)
						goto unaligned;
					/*
					 * If all boundary values are
					 * multiples of granularity, any
					 * subsequent boundaries cannot
					 * cause granularity problems,
					 * since we're aligned at this
					 * boundary.  However, if there
					 * are any non-multiples, we can't
					 * tell for sure whether there
					 * will really be any problems,
					 * so we make a conservative
					 * guess, handling a superset of
					 * the required cases as unaligned
					 * copies.
					 */
					if (do_physcontig &&
					    (PAGESIZE % granularity) != 0)
						goto unaligned;
				} else {
					if ((bcount & (NBPSCTR - 1)) != 0)
						goto unaligned;
				}
				/*
				 * Check for potential future misalignments.
				 */
				if (((max_xfer|PAGESIZE) & align_mask) != 0) {
					if ((max_xfer & align_mask) != 0 &&
					    totalcount > max_xfer)
						goto unaligned;
					if (do_physcontig &&
					    (PAGESIZE & align_mask) != 0)
						goto unaligned;
				}

				/*
				 * If the caller is accepting scatter/gather,
				 * switch to scatter/gather now, instead of
				 * using multiple buffers.
				 */
				if ((bcbp->bcb_addrtypes & BA_SCGTH) &&
				    !scgth) {
					max_scgth = preqp->phys_max_scgth;
					scgth = scgth_alloc(preqp, B_TRUE);
				}
			}
			if (nscgth == max_scgth - 1) {
				/*
				 * If the driver can only handle one piece,
				 * we have to revert to the copy case now.
				 */
				if (bcbp->bcb_flags & BCB_ONE_PIECE) {
					if (scgth)
						scgth_free(scgth);
					goto unaligned;
				}
				/*
				 * Allocate a buffer to be the (first) "child".
				 */
				cbp[0] = bp = getrbuf(KM_SLEEP);
				nbuf = i = 1;
				bp->b_addrtype = parentbp->b_addrtype;
				if (scgth) {
					bp->b_addrtype = BA_SCGTH;
					bp->b_un.b_scgth = scgth;
				} else if (bp->b_addrtype == BA_UVIRT) {
					bp->b_flags |= B_PHYS;
					bp->b_proc = parentbp->b_proc;
				}
				/*
				 * b_childcnt in the "parent" is used to
				 * keep track of the number of pending
				 * "child" buffers.  It is only needed
				 * for B_ASYNC, but it's cheaper to set it
				 * all the time than it is to do the test.
				 *
				 * Start with a child count of -1 so any
				 * children which complete before we've
				 * started up all of the children don't
				 * terminate the job prematurely.
				 */
				parentbp->b_childcnt = -1;
				/*
				 * Set the parent's b_resid to 0, so we can
				 * build up a correct value using
				 * bufbrkup_transfer as each piece completes.
				 * The driver might be expecting to see, in
				 * its strategy routine, the b_resid value as
				 * it was passed into the filter, so we delay
				 * this until we know we're going to use
				 * "child" buffers.
				 */
				orig_resid = parentbp->b_resid;
				parentbp->b_resid = 0;
			}
		    }
		} else if (nscgth == 0) {
			if (flags & B_ASYNC) {
				bp = getrbuf(KM_SLEEP);
				ASSERT(bp != NULL);
			} else {
				ASSERT(nbuf <= maxconc);
				if (nbuf == maxconc ||
				    (bp = getrbuf(KM_NOSLEEP)) == NULL) {
alloc_failed:
					/*
					 * We've filled up all the buffer
					 * slots or failed to allocate a new
					 * buffer; we have to wait for one to
					 * complete so we can reuse it.
					 */
					maxconc = nbuf;
					if (i >= nbuf)
						i = 0;
					bp = cbp[i++];
					biowait(bp);
					bufbrkup_transfer(bp, parentbp);
					bioreset(bp);
					if (scgth)
						scgth = bp->b_un.b_scgth;
					goto gotbuf;
				}
				ASSERT(i <= nbuf && i < MAXCONC_S);
				cbp[i++] = bp;
			}
			bp->b_addrtype = parentbp->b_addrtype;
			if (scgth) {
				bp->b_addrtype = BA_SCGTH;
				bp->b_un.b_scgth = scgth_alloc(preqp, B_FALSE);
				if (bp->b_un.b_scgth == NULL) {
					if (flags & B_ASYNC) {
						/*
						 * If the scgth allocation
						 * fails for an ASYNC buffer,
						 * we still have to use the
						 * buffer we got, so revert to
						 * BA_PHYS type for the rest
						 * of the job.
						 */
						ASSERT(bcbp->bcb_addrtypes &
							BA_PHYS);
						scgth = NULL;
						bp->b_addrtype = BA_PHYS;
					} else {
						/*
						 * If the scgth allocation
						 * fails for a SYNC buffer,
						 * just discard the buffer
						 * and use the one(s) we
						 * already have.  Since they're
						 * already set up as BA_SCGTH
						 * buffers, we have to use
						 * them that way, so can't
						 * revert to BA_PHYS.
						 */
						--i;
						freerbuf(bp);
						goto alloc_failed;
					}
				}
			} else if (bp->b_addrtype == BA_UVIRT) {
				bp->b_flags |= B_PHYS;
				bp->b_proc = parentbp->b_proc;
			}
			++nbuf;
		}
gotbuf:

		ASSERT(bcount % granularity == 0);

		bufcount += bcount;

		/*
		 * If we're doing scatter/gather, store this piece in the
		 * next scatter/gather slot and continue.
		 * Otherwise fill in the appropriate buffer fields,
		 * starting with those that we need to do even when we're
		 * using the parent buffer directly.
		 */
		if (scgth) {
			scgth[nscgth].sg_base = paddr;
			scgth[nscgth].sg_size = bcount;
			if (++nscgth < max_scgth && totalcount != 0)
				goto cont;
			bp->b_addrtype = BA_SCGTH;
			bp->b_un.b_scgth = scgth;
			bp->b_scgth_count = (ushort_t)nscgth;
			/*
			 * The following is a short-term hack, which will be
			 * eliminated when b_odev is dropped.
			 *
			 * Since b_scgth_count is unioned with b_odev, the
			 * assignment of odev into b_odev below would clobber
			 * b_scgth_count.  Rather than take the cost of a test
			 * (of scgth) before the assignment, we set odev here
			 * to the b_scgth_count value.  It's OK for us to
			 * clobber o_dev in this case, since an old driver
			 * couldn't possibly be accepting BA_SCGTH.
			 */
			odev = (o_dev_t)nscgth;
			/*
			 * Reset the scatter/gather count for the next buffer.
			 */
			nscgth = 0;
		} else if ((parentbp->b_addrtype & bcbp->bcb_addrtypes) == 0) {
			ASSERT(parentbp->b_addrtype == BA_PAGELIST);
			ASSERT(bcbp->bcb_addrtypes == BA_PHYS);
			bp->b_addrtype = BA_PHYS;
			ASSERT(bp != parentbp ||
			    (caddr_t)(paddr & PAGEOFFSET) == bp->b_un.b_addr);
			bp->b_un.b_paddr = paddr;
		} else
			bp->b_un.b_addr = vaddr + pgoff;

		if (nbuf != 0) {
			/*
			 * For child buffers only: copy fields from the
			 * parent and fill in fields which vary from child
			 * to child.
			 */
			ASSERT(bp != parentbp);
			if (bp->b_addrtype == BA_PAGELIST) {
				bp->b_pages = pp;
				bp->b_numpages = btop(pgoff + bufcount);
				bp->b_flags |= B_PARTIAL|B_PAGEIO;
			}
			bp->b_flags |= flags;
			bp->b_bcount = bp->b_bufsize = bufcount;
			bp->b_blkno = blkno;
			bp->b_blkoff = (ushort_t)blkoff;
			bp->b_proc = proc;
			bp->b_priv = parentbp->b_priv;
			bp->b_priv2 = parentbp->b_priv2;
			bp->b_edev = parentbp->b_edev;
			/*
			 * Just in case we were called for a very old driver
			 * (from dma_pageio or pageio_breakup), copy b_odev
			 * from the parent buffer.
			 */
			bp->b_odev = odev;
			bp->b_resid = orig_resid;
			if (flags & B_ASYNC) {
				bp->b_misc = parentbp;
				bp->b_iodone = bufbrkup_iodone;
			}
		} else {
			/*
			 * The parent already has all these fields
			 * set up correctly.
			 */
			ASSERT(bp == parentbp);
			ASSERT((bp->b_flags & flags) == flags);
			ASSERT(bp->b_bcount == bufcount);
			ASSERT(bp->b_odev == odev);
			ASSERT(bp->b_blkno == blkno);
			ASSERT(bp->b_blkoff == (ushort_t)blkoff);
			ASSERT(bp->b_edev == parentbp->b_edev);
		}

		ASSERT_CONSTRAINTS_MET(bp, bcbp);
		(*strat)(bp);

		if (totalcount == 0)
			break;

		bufcount = 0;

cont:
		/*
		 * Advance to the next piece.
		 */
		if (parentbp->b_addrtype == BA_PAGELIST) {
			/* Advance pp and pgoff for next time around */
			pgoff += bcount;
			while (pgoff >= PAGESIZE) {
				pp = pp->p_next;
				pgoff -= PAGESIZE;
			}
		} else {
			vaddr += bcount + pgoff;
			pgoff = ((vaddr_t)vaddr & PAGEOFFSET);
			vaddr -= pgoff;
		}

		blkno += btodt(bcount + blkoff);
		blkoff = ((bcount + blkoff) & (NBPSCTR - 1));
	}

	if (flags & B_ASYNC) {
		if (nbuf != 0) {
			/*
			 * Now that we've started all of the children,
			 * add in the child count, plus one more to compensate
			 * for the initial -1 value.
			 */
			FSPIN_LOCK(&brkup_fspin);
			if ((parentbp->b_childcnt += nbuf + 1) == 0) {
				FSPIN_UNLOCK(&brkup_fspin);
				parentbp->b_addrtype = 0;
				biodone(parentbp);
			} else
				FSPIN_UNLOCK(&brkup_fspin);
		}
		return;
	}

	if (nbuf != 0) {
		/*
		 * Wait for all of the (remaining) buffers in order; that way,
		 * if there's more than one error, we use the first one.
		 */
		for (n = nbuf; n-- != 0;) {
			ASSERT(i <= nbuf);
			if (i == nbuf)
				i = 0;
			biowait(cbp[i]);
			bufbrkup_transfer(cbp[i++], parentbp);
		}

		parentbp->b_addrtype = 0;
		biodone(parentbp);

		/*
		 * Free all of the buffers we allocated (if any).
		 */
		do {
			freerbuf(cbp[--nbuf]);
		} while (nbuf != 0);
	}
}


/*
 * bcb_t *
 * bcb_alloc(int flags)
 *	Allocate a bcb_t structure.
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
bcb_t *
bcb_alloc(int flags)
{
	ASSERT((flags & ~KM_NOSLEEP) == 0);

	return kmem_zalloc(sizeof(bcb_t), flags);
}

/*
 * void
 * bcb_free(bcb_t *bcbp)
 *	Free a bcb_t structure, previously returned by bcb_alloc().
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 */
void
bcb_free(bcb_t *bcbp)
{
	kmem_free(bcbp, sizeof(bcb_t));
}


/*
 * STATIC int
 * pageio_breakup(buf_t *bp)
 *	Break a job up into non-B_PAGEIO single-page sub-jobs.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
pageio_breakup(buf_t *bp) 
{
	static physreq_t pageio_preq = {
		/* phys_align */	1,
		/* phys_boundary */	PAGESIZE,
		/* phys_dmasize */	0,
		/* phys_max_scgth */	0,
		/* phys_flags */	PREQ_PHYSCONTIG
	};
	static const bcb_t pageio_bcb = {
		/* bcb_addrtypes */	BA_KVIRT,
		/* bcb_flags */		0,
		/* bcb_max_xfer */	PAGESIZE,
		/* bcb_granularity */	NBPSCTR,
		/* bcb_physreqp */	&pageio_preq
	};

	(void)physreq_prep(&pageio_preq, KM_SLEEP);
	buf_breakup((void (*)())pgiobrkup_strat[getmajor(bp->b_edev)],
		    bp, &pageio_bcb);
	return 0;
}


/*
 * void
 * pgwait(dev_t)
 *	Wait for asynchronous writes to finish.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	Called by bdwait().
 * 	Wait for all asynchronous scheduled i/o to complete
 * 	for a particular device (or any device if NODEV).
 * 	Normally called when unmounting/remounting a file system. 
 *
 * 	Replaces original bdwait() which used basyncnt
 * 	to keep track of asynchronous i/o.
 *
 * Remarks:
 *	This way it solved the unmounting of a persistent file system.
 *	
 */
void
pgwait(dev_t dev)
{
	buf_t	*bp;
	pl_t	s;
	extern void cleanup(void);

ploop:
	s = LOCK(&pageio_lists_lock, FS_BUFPGIOPL);
	bp = (buf_t *)&pageio_out;
	while ((bp = bp->b_forw) != (buf_t *)&pageio_out) {
		if ((bp->b_flags & (B_DONE|B_ASYNC)) == B_ASYNC &&
			(dev == NODEV || dev == bp->b_edev)) {
			UNLOCK(&pageio_lists_lock, s);
			cleanup();
			LBOLT_WAIT(PRIBUF);
			goto ploop;
		}
	}
	UNLOCK(&pageio_lists_lock, s);
}


#ifdef _PAGEIO_HIST

/*
 * Pageio logging facility function.
 */

/*
 * void
 * pageio_log_pp(buf_t *bp, page_t *pp, uint_t flags, char *msg, int line,
 *		 char *file)
 *	Make an entry in the pageio log.
 *
 * Calling/Exit State:
 *	The caller does not hold an fspin_t.
 */
void
pageio_log_pp(buf_t *bp, page_t *pp, uint_t flags, char *msg, int line,
	      char *file)
{
	pageio_hist_record_t *prp;

	FSPIN_LOCK(&pageio_hist_lock);

	prp = &pageio_logb[pageio_cursor];
	if (++pageio_cursor == PAGEIO_LOG_SIZE)
		pageio_cursor = 0;
	prp->phr_bp = bp;
	prp->phr_flags = flags;
	prp->phr_pp = pp;
	prp->phr_service = msg;
	prp->phr_line = line;
	prp->phr_file = file;
	prp->phr_lwp = CURRENT_LWP();
	GET_TIME(&prp->phr_stamp);

	FSPIN_UNLOCK(&pageio_hist_lock);
}

/*
 * void
 * pageio_log(buf_t *bp, char *msg, int line, char *file)
 *	Enter a buffer, and all its pages, into the pageio log.
 *
 * Calling/Exit State:
 *	The caller has stabilized the bp->b_pages list.
 *	The caller does not hold an fspin_t.
 */
void
pageio_log(buf_t *bp, char *msg, int line, char *file)
{
	page_t *pp;

	pp = bp->b_pages;
	do {
		pageio_log_pp(bp, pp, bp->b_flags, msg, line, file);
		if (pp == NULL)
			return;
		pp = pp->p_next;
	} while (pp != bp->b_pages);
}

/*
 * void
 * pageio_log_pages(page_t *plist, uint_t flags, char *msg, int line,
 *		    char *file)
 *	Enter a list of pages, plus associated flags, into the pageio log.
 *
 * Calling/Exit State:
 *	The caller has stabilized plist.
 *	The caller does not hold an fspin_t.
 */
void
pageio_log_pages(page_t *plist, uint_t flags, char *msg, int line, char *file)
{
	page_t *pp;

	pp = plist;
	do {
		pageio_log_pp(NULL, pp, flags, msg, line, file);
		if (pp == NULL)
			return;
		pp = pp->p_next;
	} while (pp != plist);
}

/*
 * void
 * print_pageio_log(buf_t *bp, page_t *pp, lwp_t *lwp, int n)
 *	Print entries from the pageio log.
 *
 * Calling/Exit State:
 *	Intended for use from a kernel debugger.
 *
 * Descriptions:
 *	If all parameters are 0, then print all entries.
 *
 *	bp	If non-NULL, then only print entries matching the
 *		specified buffer.
 *	pp	If non-NULL, then only print entries matching the
 *		specified page.
 *	lwp	If non-NULL, then only print entries matching the
 *		specified LWP.
 *	n	If non-zero, then limit the number of entries printed to n.
 */
void
print_pageio_log(buf_t *bp, page_t *pp, lwp_t *lwp, int n)
{
	pageio_hist_record_t *prp;
	ulong_t last_stamp, diff;
	char digit[12];
	int i;
	char c, *p;
	int last;
	int cursor = pageio_cursor;

	debug_printf("TIME      LWP      bp       pp       flags\n"
		     "----      ---      --       --       -----\n");

	last = cursor - 1;
	if (last < 0)
		last += PAGEIO_LOG_SIZE;
	last_stamp = pageio_logb[last].phr_stamp;

	if (n > 0) {
		do {
			prp = &pageio_logb[last];
			if (prp->phr_service != NULL &&
			    (bp == NULL || bp == prp->phr_bp) &&
			    (pp == NULL || pp == prp->phr_pp) &&
			    (lwp == NULL || lwp == prp->phr_lwp)) {
				if (--n == 0) {
					cursor = last;
					break;
				}
			}
			--last;
			if (last < 0)
				last += PAGEIO_LOG_SIZE;
		} while (last != cursor);
	}

	do {
		prp = &pageio_logb[cursor];
		if (++cursor == PAGEIO_LOG_SIZE)
			cursor = 0;
		if (prp->phr_service == NULL)
			continue;
		if (bp != NULL && bp != prp->phr_bp)
			continue;
		if (pp != NULL && pp != prp->phr_pp)
			continue;
		if (lwp != NULL && lwp != prp->phr_lwp)
			continue;
		diff = last_stamp - prp->phr_stamp;
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
		debug_printf("-%s %lx %lx %lx %lx %s from line %d of file %s\n",
			p + 1, (ulong_t)prp->phr_lwp, (ulong_t)prp->phr_bp,
			(ulong_t)prp->phr_pp, (ulong_t)prp->phr_flags,
			prp->phr_service, prp->phr_line, prp->phr_file);
		if (debug_output_aborted())
			break;
	} while (cursor != pageio_cursor);
}

/*
 * void
 * bufsubr_daemon(void *arp)
 *	Daemon to scan the pageio_stamp array, looking for hung pageouts.
 *
 * Calling/Exit State:
 *	Only invoked as the result of creating the bufsubr daemon.
 */
/* ARGSUSED */
void
bufsubr_daemon(void *arp)
{
	page_t *pp;
	clock_t stamp;

	u.u_lwpp->l_name = "bufsubr daemon";

	for (;;) {
		EVENT_WAIT(&bufsubr_event, PRIBUF);
		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);

		for (pp = pages; pp < epages; pp++) {
			stamp = pageio_stamp[pp - pages];
			if (stamp != 0 && lbolt - stamp > 200 * HZ)
				/*
				 *+ I/O was initiated on a dirty page by the
				 *+ pageout daemon, but it never completed.
				 */
				cmn_err(CE_PANIC, "hung pageout (pp = %lx)",
					pp);
		}
	}
}

#endif	/* _PAGEIO_HIST */
