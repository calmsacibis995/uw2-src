/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:mem/vm_hat.c	1.39.6.18"
#ident	"$Header: $"

/*
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed should not be visible
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.	 Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and each page structure contains an opaque pointer
 * which is used by the hat code to hold a list of active translations to
 * that page.
 *
 *
 *	It is assumed by this code:
 *
 *		- no load/unload requests for a range of addresses will
 *			span kernel/user boundaries.
 *		- all addresses to be mapped are on page boundaries.
 *		- all segment sizes, s_size, are multiples of the page
 *		  size.
 *
 * NOMINAL SPIN LOCK ORDER:
 *	p_uselock in a page_t
 *	hat_resourcelock in a hat_t
 *	vm_pageidlock
 *	vm_pagefreelck
 *	hat_mcpoollock	hat_ptpoollock (same hierarchy)
 *
 *	The only routine that will be called from interrupt level is vtop.
 */

#include <acc/mac/mac.h>
#include <util/types.h>
#include <util/param.h>
#include <util/emask.h>
#include <mem/immu.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/tuneable.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/anon.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <mem/mem_hier.h>
#include <proc/cred.h>
#include <util/plocal.h>
#include <svc/clock.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <proc/disp.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

#if (VM_HAT_RESOURCE_HIER > VM_HAT_PAGE_HIER_MAX)
#error hat resource lock hierarchy exceeding max. level
#endif

#if (VM_HAT_MCPOOL_HIER > VM_HAT_LOCAL_HIER_MAX)
#error mcpool lock hierarchy exceeding max. level
#endif

#if (VM_HAT_PTPOOL_HIER > VM_HAT_LOCAL_HIER_MAX)
#error ptpool lock hierarchy exceeding max. level
#endif

#if (VM_HAT_TLBS_HIER > VM_HAT_LOCAL_HIER_MAX)
#error tlb lock hierarchy exceeding max. level
#endif

#ifdef DEBUG
/*
 * Some debugging is always available if DEBUG is defined.
 * But some debugging is too expensive and needs further control.
 * Special debugging variables are provided to control the more
 * costly or intrusive debugging audits.
 */
int hatdb_mcfreeck = 1;	/*
			 * If this is nonzero, any mapping chunk
			 * that is being freed is audited for all zeros.
			 * This is an imperfect audit since zero is also
			 * a valid value of an active mapping (NULL
			 * terminated linked list).
			 * The audit is performed with the hat_resourcelock
			 * held for the as that was using the chunk.
			 */

int hatdb_ptfreeck = 1;	/*
			 * If this is nonzero, any page table that is
			 * being freed is audited for all zeros.
			 * The mapping chunk pointers are also audited
			 * for all zeros.
			 * The audit is performed with the hat_resourcelock
			 * held for the as that was using the page table.
			 */
/*
 * DEBUGGING MACROS:
 * These checks are encoded as macros to isolate the details
 * to one place for easier maintenance.
 * For instance, DYNPT_KAS_SEG_CK() audits the segment to insure it
 * really uses the dynamic page table format if it is in the kas.
 * That list of relevant segments may change as the design evolves,
 * so it is buried in a macro.
 */
#define DYNPT_KAS_SEG_CK(SEG)	ASSERT(!((SEG)->s_as == &kas && (SEG) != segkmap))

#else
#define DYNPT_KAS_SEG_CK(SEG)
#endif

extern struct seg *segkmap;	/* kernel generic mapping segment */

/*
 * Variables to describe kernel visible mappings.
 * Macros for working with them.
 */
STATIC pte_t *kas_visible_ptes;
STATIC hatmap_t *kas_visible_maps;	/* starting page table in visible 
					 * range */
STATIC hatmap_t *kas_visible_maps_end;	/* ending page table in visible
					 * range */
STATIC ulong_t kas_vis_map_to_pte_delta; /* the delta to get at a pte from the
					  * mapping pointer. This is a fixed 
					  * delta. */

/* The following test works because kas_visible_maps are from calloc space which
 * is guaranteed to be lower than seg_kmem space, from which mapping pointers
 * for user dynamic mappings are allocated.
 */
#define IS_KVIS_MAP(mapp) ((mapp) < kas_visible_maps_end)

hat_t *kas_hatp = &kas.a_hat;

/* vprot array used in the hat_vtop_prot macro */
uint_t vprot_array[PROT_ALL+1];

/* tlb flush asm call */
asm	void flushtlb(void)
{
	movl	%cr3, %eax
	movl	%eax, %cr3
}
#pragma asm partial_optimization flushtlb

/* function prototypes */
STATIC hatpt_t *hat_findfirstpt(vaddr_t *, hat_t *);
STATIC enum hat_cont_type hat_dupmc(struct seg *, pte_t *, hatmc_t **,
			 hatpt_t **,vaddr_t *, vaddr_t , int, int, uint_t);
hatpt_t *hat_findpt(hat_t *, pte_t *);
STATIC hat_stats_t *hat_findstats(hat_t *, pte_t *);
pte_t * hat_vtopte_l(struct hat *, vaddr_t, hatpt_t **);
STATIC void hat_free_modstats(struct as *as);

/*
 * This lkinfo structure is used for all hat_resourcelock instances.
 */
STATIC LKINFO_DECL(hatresource_lkinfo, "hat resource lock of an as", 0);

/*
 ************************************************************************
 * hat resource allocation amd free pool management code:
 *
 * This section of code contains all of the routines for managing
 * PT and MC allocation and free pools.
 *
 * The code is designed to work with a daemon to help with pool management.
 * Resources are always freed into the corresponding pool.  The daemon
 * trims the pool if it grows too big.
 * Allocation always tries the pool first.  If that fails, and flags permit,
 * an attempt is made to create a new instance of the desired resource.
 * If that fails (resource exhaustion via kpg_alloc (for pages) or
 * kmem_zalloc (for companion structures)), and the flag allows sleeping,
 * a reserve is placed and the code sleeps, waiting for the daemon to
 * replenish the desired pool and wake the waiters.
 * The daemon is expected to run about once a second.
 * Pool maintenance is a non-sleeping chore.
 * Waiting for a second or so is OK here since resource exhaustion will
 * be dominated by page pool exhaustion.  That, in turn, causes swapping
 * which creates worse delays while "solving" the problem.
 */

/*
 * THE FOLLOWING SHOULD BECOME TUNEABLES, BUT I AM CHEATING FOR NOW.
 */
long hat_minptfree = 12;
long hat_maxptfree = 36;
long hat_minmcfree = 4*(HAT_MCPP - 2);
long hat_maxmcfree = 12*(HAT_MCPP - 2);

STATIC lock_t hat_ptpoollock;
STATIC lock_t hat_mcpoollock;
STATIC LKINFO_DECL(hat_ptfree_lkinfo, "mem: hat_ptpoollock: hat pt free list lock", 0);
STATIC LKINFO_DECL(hat_mcfree_lkinfo, "mem: vm_mcpoollock: hat mc free list lock", 0);
STATIC pl_t ptpool_pl;
STATIC pl_t mcpool_pl;

STATIC sv_t hat_ptwait;
STATIC sv_t hat_mcwait;

/*
 * Pool locking macros.
 */
#if (!VM_INTR_IPL_IS_PLMIN)

#define PTPOOL_LOCK()	(ptpool_pl = LOCK(&hat_ptpoollock, VM_HAT_PTPOOL_IPL))
#define PTPOOL_UNLOCK()	UNLOCK(&hat_ptpoollock, ptpool_pl)
#define MCPOOL_LOCK()	(mcpool_pl = LOCK(&hat_mcpoollock, VM_HAT_MCPOOL_IPL))
#define MCPOOL_UNLOCK()	UNLOCK(&hat_mcpoollock, mcpool_pl)
#define TLBS_LOCK()	LOCK(&TLBSlock, VM_HAT_TLBS_IPL)
#define TLBS_UNLOCK(oldpl)	UNLOCK(&TLBSlock, oldpl)

#else	/* VM_INTR_IPL_IS_PLMIN */
#define PTPOOL_LOCK()	(ptpool_pl = LOCK_PLMIN(&hat_ptpoollock))
#define PTPOOL_UNLOCK()	UNLOCK_PLMIN(&hat_ptpoollock, ptpool_pl)
#define MCPOOL_LOCK()	(mcpool_pl = LOCK_PLMIN(&hat_mcpoollock))
#define MCPOOL_UNLOCK()	(UNLOCK_PLMIN(&hat_mcpoollock, mcpool_pl))
#define TLBS_LOCK()	LOCK_PLMIN(&TLBSlock)
#define TLBS_UNLOCK(oldpl)	UNLOCK_PLMIN(&TLBSlock, oldpl)

#endif /* !VM_INTR_IPL_IS_PLMIN */

STATIC hatpt_t *hatpt_freelist;
STATIC long nptfree;
#ifdef DEBUG
STATIC long nptneeded;		/* unwoken up reservers */
#endif
STATIC long nptreserved;	/* untaken reserved page tables */

STATIC long nmcfree;
STATIC page_t mcfreelist;	/* list of pages with some free mapping chunks
				   - this is a doubly linked list */
STATIC long nmcpgfree;
STATIC page_t *mcpgfreelist;	/* list of pages with all mapping chunks free 
				   - this is a singly linked list */
#ifdef DEBUG
STATIC long nmcneeded;		/* unwoken up reservers */
#endif
STATIC long nmcreserved;	/* untaken reserved chunks */

#ifdef DEBUG
/*
 * void checkptinit(hatpt_t *ptap)
 *	Debugging subroutine for checking page table initialization.
 *
 * Calling/Exit State:
 *	Page table body and the mapping chunk pointer array should be zero.
 * Remarks:
 *      Should not be called with PSE type ptap.
 */
STATIC void
checkptinit(hatpt_t *ptap)
{
	pte_t *ptep;
	hatmcp_t *mcp;
	int i;

	ASSERT(ptap);
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	if (hatdb_ptfreeck == 0)
		return;
	ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte;
	for (i = 0; i < HAT_EPMC*HAT_MCPP; ptep++, i++) {
		ASSERT(ptep->pg_pte == 0);
	}
	mcp = ptap->hatpt_mcp;
	for (i = 0; i < HAT_MCPP; mcp++, i++) {
		ASSERT(mcp->hat_mcp == (hatmc_t *)NULL);
	}
}

/*
 * void checkmcinit(hatmc_t *mcp)
 *	Debugging check to see that all chunk entries are zeroed.
 *
 * Calling/Exit State:
 *	mapp is beginning of a chunk and entries are zero.
 */
STATIC void
checkmcinit(hatmc_t *mcp)
{
	hatmap_t *mapp = mcp->hat_map;
	int i;

	if (hatdb_mcfreeck == 0)
		return;
	for (i = 0; i < HAT_EPMC; i++) {
		ASSERT(mapp[i].hat_mapv == 0);
	}
	ASSERT(*mapptowasrefp(mapp) == 0);
}

#else

#define checkptinit(XXX)
#define checkmcinit(XXX)

#endif	/*DEBUG */

/*
 * hatpt_t *allocptfrompool()
 *	Attempts to allocate a page table from the pool.
 *	If the pool is empty, it returns NULL.
 *
 * Calling/Exit State:
 *	No pool locks are held by caller.
 *	Pt pool lock used to access pool, but is dropped before return.
 */
STATIC hatpt_t *
allocptfrompool(void)
{
	hatpt_t *ptap;

	PTPOOL_LOCK();
	ASSERT(nptfree >= 0);
	ASSERT(nptreserved >= 0);
	if (nptfree <= nptreserved) {
		PTPOOL_UNLOCK();
		return((hatpt_t *)NULL);
	}
	ASSERT(hatpt_freelist != (hatpt_t *)NULL);
	ASSERT(nptfree > 0);
	ptap = hatpt_freelist;
	hatpt_freelist = ptap->hatpt_forw;
	nptfree--;
	ASSERT(nptfree >= 0);
	PTPOOL_UNLOCK();

	checkptinit(ptap);
	return(ptap);
}

/*
 * hatpt_t *allocnewpt()
 *	allocates a pt using kmem_zalloc and kpg_alloc.
 *	Field that are fixed at allocation time are initialized
 *	in the hatpt_t structure.
 *	It never sleeps.
 *
 * Calling/Exit State:
 *	The caller should have no spin locks that interfere
 *	(hierarchy-wise) with kmem_zalloc and kpg_alloc.
 *	The lock for the pt pool is used when bumping the count.
 */
STATIC hatpt_t *
allocnewpt(void)
{
	hatpt_t *ptap;
	page_t *pp;
	int i;
	hatmc_t *mcp;

	/*
	 * Right now MMU_PAGESIZE == PAGESIZE.
	 * If that changes, much hat code must change to loop
	 * over multiple physical pages per logical page.
	 */
#ifdef DEBUG
	/*LINTED*/
	ASSERT(MMU_PAGESIZE == PAGESIZE);
#endif
trytoalloc:
	ptap = (hatpt_t *)kmem_zalloc(sizeof (hatpt_t), KM_NOSLEEP);
	if (ptap == (hatpt_t *)NULL)
		return((hatpt_t *)NULL);
	ptap->hatpt_ptva = (hatpgt_t *)
			kpg_alloc((ulong_t)1, PROT_READ|PROT_WRITE,
				  P_NODMA|NOSLEEP);
	if (ptap->hatpt_ptva == (hatpgt_t *)NULL) {
		/*
		 * See if a page can be gotten from the mc pool.
		 * Look first to avoid lock operation.
		 * PERF :  Is hat_maxmcfree the right check? Probably should be
		 * (min + max) / 2.
		 */
		if (nmcfree - nmcreserved <= hat_maxmcfree ||
				mcpgfreelist == (page_t *)NULL) {
			kmem_free(ptap, sizeof (hatpt_t));
			return((hatpt_t *)NULL);
		}
		MCPOOL_LOCK();
		if (nmcfree - nmcreserved <= hat_maxmcfree ||
				mcpgfreelist == (page_t *)NULL) {
			MCPOOL_UNLOCK();
			kmem_free(ptap, sizeof (hatpt_t));
			return((hatpt_t *)NULL);
		}
		/*
		 * Steal one.
		 */
		pp = mcpgfreelist;
		mcpgfreelist = pp->p_next;
		nmcpgfree--;
		nmcfree -= HAT_MCPP - 2;
		ASSERT(nmcpgfree >= 0);
		ASSERT(nmcfree >= 0);
		MCPOOL_UNLOCK();
		ASSERT(PHAT2(pp)->pgmcfree);
		mcp = (hatmc_t *)(PHAT2(pp)->phat_mcpgp);
		ASSERT(mcp);
		ptap->hatpt_ptva = (hatpgt_t *)mcp;
		for (i = 0; i < HAT_MCPP; mcp++, i++)
			mcp->hat_mcnext = (hatmc_t *)NULL;
		checkptinit(ptap);
	} else {
		struct_zero(ptap->hatpt_ptva, PAGESIZE);
		pp = kvtopp((vaddr_t)ptap->hatpt_ptva);
	}

	ASSERT(pp != NULL);
	PHAT2(pp)->phat_ptap = ptap;
	ptap->hatpt_pde.pg_pte = (uint_t)mkpte(PTE_RW|PG_V, page_pptonum(pp));

	/* stale check o.k. here w/o holding the ptpool lock */
	if (nptreserved <= nptfree)
	  return (ptap);
	else {
	  PTPOOL_LOCK();
	  ASSERT(nptreserved >= 0);
	  ASSERT(nptfree >= 0);
	  ASSERT(nptneeded >= 0);
	  ASSERT(nptfree + nptneeded >= nptreserved);
	  ptap->hatpt_forw = hatpt_freelist;
	  hatpt_freelist = ptap;

	  if (nptfree++ < nptreserved) {
		ASSERT(nptneeded-- != 0);
		ASSERT(nptfree + nptneeded == nptreserved);
		SV_SIGNAL(&hat_ptwait, 0);
	  }

	  PTPOOL_UNLOCK();
	  goto trytoalloc;
	}
	/* NOTREACHED */
}

/*
 * hatpt_t *waitforpt()
 *	Waits for a pt to become available (either freed or allocated by
 *	the pool refresh daemon or other allocating lwps) and then allocates it.
 *
 * Calling/Exit State:
 *	This code sleeps, so the caller can hold no spin locks.
 *
 */
STATIC hatpt_t *
waitforpt(void)
{
	hatpt_t *ptap;

	PTPOOL_LOCK();
	ASSERT(nptfree >= 0);
	ASSERT(nptneeded >= 0);
	ASSERT(nptreserved >= 0);
	ASSERT(nptfree + nptneeded >= nptreserved);
	if (nptfree <= nptreserved) {
		nptreserved++;
		ASSERT(nptfree + ++nptneeded == nptreserved);
		SV_WAIT(&hat_ptwait, PRIMEM, &hat_ptpoollock);
		PTPOOL_LOCK();
		ASSERT(nptfree > 0);
		nptreserved--;
		ASSERT(nptreserved >= 0);
	}
	ASSERT(nptfree > 0);
	ASSERT(hatpt_freelist != (hatpt_t *)NULL);
	ptap = hatpt_freelist;
	hatpt_freelist = ptap->hatpt_forw;
	nptfree--;
	ASSERT(nptfree >= 0);
	PTPOOL_UNLOCK();

	checkptinit(ptap);
	return(ptap);
}

/*
 * hatpt_t *hat_ptalloc(uint_t flag)
 *	Allocate a page table with its hatpt_t structure,
 *	set up pte for page table in the hatpt_t, zero the page table,
 *	and return a pointer to its hatpt_t structure.
 *	There is an internal pt free pool. Access to it is controlled
 *	by the hat_ptpoollock spin lock.
 *
 * Calling/Exit State:
 *	The calling state depends on the flag passed in.
 *	The flag can have the following values:
 *	HAT_POOLONLY	-> use local free pt pool only.
 *		Any collection of locks (except TLBS) can be held.
 *		The free pool locks are after the global page layer locks
 *		(pgidlk and pgfreelk) to allow this. Pool locks are only
 *		ever held by the low level pool code.
 *	HAT_NOSLEEP	-> return immediately if no memory
 *		Try the local pt free pool first. if that is exhausted
 *		allocate a new pt via kpg_alloc and kmem_zalloc,
 *		but pass them NOSLEEP in the appropriate form.
 *		The pool lock is dropped while calling outside of hat.
 *		No locks are held by the calling hat code. The outside
 *		code that calls into the hat may only hold locks earlier
 *		in the hierarchy than the earliest of PAGE_USELOCK and
 *		locks used in seg_kmem via kpg_alloc and locks in KMA.
 *	HAT_REFRESH	-> do a HAT_NOSLEEP-style allocation and if that
 *		is successful, refresh the pools if they need it.
 *		This is used by the bulk loaders (hat_map and hat_dup)
 *		to get maximum mileage out of having to drop all of
 *		the locks.
 *	HAT_CANWAIT	-> wait if no memory currently available
 *		Do a NOSLEEP allocation. If that fails, bump a reserve
 *		count and sleep in the ptalloc code. The pool daemon
 *		runs about every second and tries to refresh the pools.
 *		To the extent that it succeeds, it wakes up waiters.
 *		This is flag is used only in hat_pteload, which does
 *		only a single PTE.
 *
 */
STATIC hatpt_t *
hat_ptalloc(uint_t flag)
{
	hatpt_t *ptap;

	ptap = allocptfrompool();
	if (ptap) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(ptap);
	}
	if (flag == HAT_POOLONLY)
		return((hatpt_t *) NULL);
	ptap = allocnewpt();
	if (ptap) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(ptap);
	}
	if (flag & (HAT_NOSLEEP|HAT_REFRESH)) {
		ASSERT(flag == HAT_NOSLEEP || flag == HAT_REFRESH);
		return((hatpt_t *) NULL);
	}
	ASSERT(flag == HAT_CANWAIT);
	ptap = waitforpt();
	ASSERT(ptap != (hatpt_t *)NULL);
	return(ptap);
}

/*
 * void hat_ptfree(hatpt_t *ptap)
 *	return page table to the free pt pool.
 *
 * Calling/Exit State:
 *	Any combination of locks through MV_PAGELOCK in the hierarchy
 *	can be held by the caller. This code uses only the spin lock
 *	for the free pt pool. It lets the daemon return excess pages
 *	from this pool back to kmem_free and kpg_free.
 *
 * Remarks:
 *      If called to free a PSE ptap, then it calls pse_hat_ptfree.
 */
STATIC void
hat_ptfree(hatpt_t *ptap)
{

	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	checkptinit(ptap);
	PTPOOL_LOCK();
	ptap->hatpt_forw = hatpt_freelist;
	hatpt_freelist = ptap;
	ASSERT(nptfree >= 0);
	ASSERT(nptneeded >= 0);
	ASSERT(nptreserved >= 0);
	ASSERT(nptfree + nptneeded >= nptreserved);
	if (nptfree++ < nptreserved) {
		ASSERT(nptneeded-- != 0);
		ASSERT(nptfree + nptneeded == nptreserved);
		SV_SIGNAL(&hat_ptwait, 0);
	}
	PTPOOL_UNLOCK();
}

/*
 * hatmc_t *allocmcfrompool_l()
 *	Attempts to allocate a mapping chunk from the pool.
 *	If the pool is empty, it returns NULL.
 *
 * Calling/Exit State:
 *	The mc pool lock is held by the caller.
 */
STATIC hatmc_t *
allocmcfrompool_l(void)
{
	page_t *pp;
	hatmc_t *mcp;
	struct phat2 *phat2p;

	if ((pp = mcfreelist.p_next) != &mcfreelist) {
retry:
		phat2p = PHAT2(pp);
		mcp = phat2p->pgmcfree;
		ASSERT(mcp != (hatmc_t *)NULL);
		phat2p->pgmcfree = mcp->hat_mcnext;
		mcp->hat_mcnext = (hatmc_t *)NULL;
		ASSERT(phat2p->npgmcfree > 0);
		if (--phat2p->npgmcfree == 0) {
			ASSERT(phat2p->pgmcfree == (hatmc_t *)NULL);
			/* put page in nevernever land */
			pp->p_prev->p_next = pp->p_next;
			pp->p_next->p_prev = pp->p_prev;
		}
		nmcfree--;
		return(mcp);
	}
	if ((pp = mcpgfreelist) != (page_t *)NULL) {
		/*
		 *put it on partial page list.
		 */
		mcpgfreelist = pp->p_next;
		mcfreelist.p_next = mcfreelist.p_prev = pp;
		pp->p_next = pp->p_prev = &mcfreelist;
		ASSERT(PHAT2(pp)->npgmcfree == HAT_MCPP - 2);
		ASSERT(PHAT2(pp)->pgmcfree != (hatmc_t *)NULL);
		ASSERT(nmcpgfree > 0);
		ASSERT(nmcfree >= HAT_MCPP - 2);
		nmcpgfree--;
		goto retry;
	}
	return((hatmc_t *)NULL);
}

/*
 * hatmc_t *allocmcfrompool()
 *	Attempts to allocate a mapping chunk from the pool.
 *	If the pool is empty, it returns NULL.
 *
 * Calling/Exit State:
 *	No pool locks are held by caller.
 *	mc lock used to access pool, but is dropped before return.
 */
STATIC hatmc_t *
allocmcfrompool(void)
{
	hatmc_t *mcp;

	MCPOOL_LOCK();
	ASSERT(nmcfree >= 0);
	ASSERT(nmcneeded >= 0);
	ASSERT(nmcreserved >= 0);
	ASSERT(nmcfree + nmcneeded >= nmcreserved);
	if (nmcfree <= nmcreserved) {
		ASSERT(nmcfree + nmcneeded == nmcreserved);
		MCPOOL_UNLOCK();
		return((hatmc_t *)NULL);
	}
	mcp = allocmcfrompool_l();
	MCPOOL_UNLOCK();
	return(mcp);
}

/*
 * hatmc_t *allocnewmc()
 *	Allocate new mc page from kpg_alloc and allocate one mc from it.
 *	This is a NOSLEEP attempt and can fail.
 *
 * Calling/Exit State:
 *	No spin locks that interfere hierarchy-wise with kpg_alloc
 *	may be held by caller.
 *	If the kpg_alloc call is successful, mc pool lock is used
 *	for the rest of the allocation.
 */
STATIC hatmc_t *
allocnewmc(void)
{
	hatmcpg_t *pgaddr;
	page_t *pp;
	hatmc_t *mcp, *prevmcp;
	struct phat2 *phat2p;
	int i;
	hatpt_t *ptap;

trytoalloc:
	pgaddr = (hatmcpg_t *)kpg_alloc((ulong_t)1, PROT_READ|PROT_WRITE,
					P_NODMA|NOSLEEP);
	if (pgaddr == (hatmcpg_t *)NULL) {
		/*
		 * How is the pt pool doing?
		 */
		if (nptfree - nptreserved <= hat_maxptfree)
			return((hatmc_t *)NULL);
		PTPOOL_LOCK();
		if (nptfree - nptreserved <= hat_maxptfree) {
			PTPOOL_UNLOCK();
			return((hatmc_t *)NULL);
		}
		ptap = hatpt_freelist;
		ASSERT(ptap);
		hatpt_freelist = ptap->hatpt_forw;
		nptfree--;
		PTPOOL_UNLOCK();
		checkptinit(ptap);
		pgaddr = (hatmcpg_t *)ptap->hatpt_ptva;
		kmem_free(ptap, sizeof (hatpt_t));
	} else {
		struct_zero(pgaddr, PAGESIZE);
	}

	pp = kvtopp((vaddr_t)pgaddr);
	ASSERT(pp != NULL);
	pgaddr->hat_mcpga.hat_mcpgpp = pp;

	phat2p = PHAT2(pp);
	phat2p->phat_mcpgp = pgaddr;
	mcp = phat2p->phat_mcpgp->hat_mc + 2;
	prevmcp = (hatmc_t *)NULL;
	for (i = 2; i < HAT_MCPP; mcp++, i++) {
		mcp->hat_mcnext = prevmcp;
		prevmcp = mcp;
	}
	phat2p->pgmcfree = prevmcp;
	phat2p->npgmcfree = HAT_MCPP - 2;
	MCPOOL_LOCK();
	ASSERT(nmcfree >= 0);
	ASSERT(nmcpgfree >= 0);
	ASSERT(nmcneeded >= 0);
	ASSERT(nmcreserved >= 0);
	ASSERT(nmcfree + nmcneeded >= nmcreserved);
	pp->p_next = mcpgfreelist;
	mcpgfreelist = pp;
	ASSERT(mcp);
	nmcpgfree++;
	if (nmcfree < nmcreserved) {
		ASSERT(nmcfree + nmcneeded == nmcreserved);
		i = nmcreserved - nmcfree;
		if (i > HAT_MCPP - 2)
			i = HAT_MCPP - 2;
		while (i--) {
#ifdef DEBUG
			nmcneeded--;
#endif
			SV_SIGNAL(&hat_mcwait, 0);
		}
	}
	nmcfree += HAT_MCPP - 2;
	if (nmcfree <= nmcreserved) {
		MCPOOL_UNLOCK();
		goto trytoalloc;
	}
	mcp = allocmcfrompool_l();
	MCPOOL_UNLOCK();
	ASSERT(mcp);
	return(mcp);
}

/*
 * hatmc_t *waitformc()
 *	Waits for a mc to become available (either freed or allocated by
 *	the pool refresh daemon or by another allocating lwp) and then allocates it.
 *
 * Calling/Exit State:
 *	This code sleeps, so the caller can hold no spin locks.
 *
 */
STATIC hatmc_t *
waitformc(void)
{
	hatmc_t *mcp;

	MCPOOL_LOCK();
	ASSERT(nmcfree >= 0);
	ASSERT(nmcpgfree >= 0);
	ASSERT(nmcneeded >= 0);
	ASSERT(nmcreserved >= 0);
	ASSERT(nmcfree + nmcneeded >= nmcreserved);
	if (nmcfree <= nmcreserved) {
		nmcreserved++;
#ifdef DEBUG
		nmcneeded++;
#endif
		ASSERT(nmcfree + nmcneeded == nmcreserved);
		SV_WAIT(&hat_mcwait, PRIMEM, &hat_mcpoollock);
		MCPOOL_LOCK();
		ASSERT(nmcfree > 0);
		nmcreserved--;
		ASSERT(nmcreserved >= 0);
	}
	ASSERT(nmcfree > 0);
	mcp = allocmcfrompool_l();
	ASSERT(mcp);
	MCPOOL_UNLOCK();

	return(mcp);
}

/*
 * void hat_mcfree(hatmap_t *mapp)
 *	mapp passed can be anywhere in a mapping chunk.
 *	Need to adjust to the beginning of the chunk.
 *	The chunk should be zeroed (running zeroing is better for
 *	sparse use) already.
 *
 * Calling/Exit State:
 *	No pool locks are held by the caller, the mc pool lock is used.
 *	Only free to the pool (let the daemon worry about trimming the pool)
 *	in order to minimize lock constraints on the caller.
 */
STATIC void
hat_mcfree(hatmap_t *mapp)
{
	hatmc_t    *mcp;
	hatmcpg_t  *pgp;
	page_t	   *pp;
	int	   mcnum;
	struct phat2 *phat2p;

	mcp = (hatmc_t *)((ulong_t)mapp & HATMC_ADDR);
	pgp = (hatmcpg_t *) ((ulong_t)mcp & PAGEMASK);

	mcnum = HATMAPMCNO(mcp);
	pgp->hat_mcpga.hatptcp[mcnum].hat_pgtcp = (hatpgtc_t *)NULL; 
	ASSERT(*mapptowasrefp(mapp) == 0);

	/* 
	 * find this page's page structure
	 */
	pp = pgp->hat_mcpga.hat_mcpgpp;
	ASSERT(pp);

	phat2p = PHAT2(pp);
	MCPOOL_LOCK();
	ASSERT(phat2p->npgmcfree >= 0 && phat2p->npgmcfree < HAT_MCPP - 2);
	checkmcinit(mcp);
	if (phat2p->npgmcfree) {
		/*
		 * It is on mcfreelist. Remove it now.
		 * Later, the code will put it either at the bottom of
		 * the mcfreelist or on the mcpgfreelist.
		 * The page is put on the bottom of the mcfreelist
		 * to attempt to allow better coalescing of free chunks
		 * to reduce mc page fragmentation for trimming the pool.
		 */
		pp->p_prev->p_next = pp->p_next;
		pp->p_next->p_prev = pp->p_prev;
	}
	mcp->hat_mcnext = phat2p->pgmcfree;
	phat2p->pgmcfree = mcp;
	if (++phat2p->npgmcfree == HAT_MCPP - 2) {
		ASSERT(nmcfree >= 0);
		ASSERT(nmcpgfree >= 0);
		nmcpgfree++;
		pp->p_next = mcpgfreelist;
		mcpgfreelist = pp;
	} else {
		/*
		 * Put at bottom of list to help fragmentation.
		 */
		pp->p_next = &mcfreelist;
		pp->p_prev = mcfreelist.p_prev;
		mcfreelist.p_prev->p_next = pp;
		mcfreelist.p_prev = pp;
		ASSERT(nmcfree >= 0);
	}
	if (nmcfree++ < nmcreserved) {
#ifdef DEBUG
		nmcneeded--;
#endif
		ASSERT(nmcfree + nmcneeded == nmcreserved);
		SV_SIGNAL(&hat_mcwait, 0);
	}
	MCPOOL_UNLOCK();
}

/*
 * void hat_refreshpools()
 *	If pools are too small, try to grow them (NOSLEEP, of course)
 *	and wake up waiters if successful.
 *	If pools are too large, free resources to trim them if possible.
 *	It is is always possible for the pt pool, but mc page fragmentation
 *	limits action for the mc pool.
 *	This is called from the pool daemon and internally.
 *
 * Calling/Exit State:
 *	The caller can hold no spin locks that interfere with kmem_zalloc
 *	or kpg_alloc calls.
 *	The caller cannot hold any of the pool locks.
 */
void
hat_refreshpools(void)
{
	hatpt_t *ptap, *nptap;
	hatmc_t *mcp;
	int i, tofree;
	page_t *pp, *npp;

	/*
	 * The allocation must happen without the locks held
	 * because of lock hierarchy.
	 * So there is no reason to grab the lock to check,
	 * since the operation is inherently (but benignly) stale.
	 */
	if (nptfree - nptreserved < hat_minptfree) {
		do {
			ptap = allocnewpt();
			if (ptap == (hatpt_t *)NULL)
				return;
			hat_ptfree(ptap);
		} while (nptfree - nptreserved < hat_minptfree);
	} else if (nptfree - nptreserved > hat_maxptfree) {
		PTPOOL_LOCK();
		tofree = nptfree - nptreserved - hat_maxptfree;
		if (tofree <= 0) {
			PTPOOL_UNLOCK();
			goto checkmcpool;
		}
		/*
		 * Take all off now since lock must be dropped
		 * to give them back.
		 */
		ptap = nptap = hatpt_freelist;
		for (i = tofree; i--; nptap = nptap->hatpt_forw) {
			ASSERT(nptap);
		}
		ASSERT(nptap);
		hatpt_freelist = nptap;
		nptfree -= tofree;
		PTPOOL_UNLOCK();
		while (tofree--) {
			nptap = ptap->hatpt_forw;
			kpg_free(ptap->hatpt_ptva, (ulong_t)1);
			kmem_free(ptap, sizeof (hatpt_t));
			ptap = nptap;
		}
	}
checkmcpool:
	if (nmcfree - nmcreserved < hat_minmcfree) {
		do {
			mcp = allocnewmc();
			if (mcp == (hatmc_t *)NULL)
				return;
			hat_mcfree(mcp->hat_map);
		} while (nmcfree - nmcreserved < hat_minmcfree);
	} else if (nmcfree - nmcreserved > hat_maxmcfree && nmcpgfree) {
		MCPOOL_LOCK();
		ASSERT(nmcfree >= 0);
		ASSERT(nmcreserved >= 0);
		ASSERT(nmcpgfree >= 0);
		tofree = nmcfree - nmcreserved - hat_maxmcfree;
		if (tofree <= 0 || nmcpgfree == 0) {
			MCPOOL_UNLOCK();
			return;
		}
		tofree = (tofree + (HAT_MCPP - 2) - 1)/(HAT_MCPP - 2);
		if (tofree > nmcpgfree)
			tofree = nmcpgfree;
		ASSERT(tofree > 0);
		pp = npp = mcpgfreelist;
		for (i = tofree; i--; npp = npp->p_next) {
			ASSERT(npp);
		}
		mcpgfreelist = npp;
		nmcpgfree -= tofree;
		nmcfree -= tofree *(HAT_MCPP - 2);
		MCPOOL_UNLOCK();
		while (tofree--) {
			npp = pp->p_next;
			kpg_free(PHAT2(pp)->phat_mcpgp, (ulong_t)1);
			pp = npp;
		}
	}
}

/*
 * hatmc_t *hat_mcalloc(uint_t flag)
 *	Allocate a mapping chunk, insuring that it is zeroed.
 *	There is an internal mc free pool. Access to it is controlled
 *	by the hat_mcpoollock spin lock.
 *
 * Calling/Exit State:
 *	The calling state depends on the flag passed in.
 *	The flag can have the following values:
 *	HAT_POOLONLY	-> use local free mc pool only.
 *		Any collection of locks (except TLBS) can be held.
 *		The free pool locks are after VM_PAGELOCK to allow this.
 *		Pool locks are only ever held by the low level pool code.
 *	HAT_NOSLEEP	-> return immediately if no memory
 *		Try the local mc free pool first. if that is exhausted
 *		allocate a new mc page via kpg_alloc and zero it,
 *		but pass them NOSLEEP in the appropriate form.
 *		The pool lock is dropped while calling outside of hat.
 *		No locks are held by the calling hat code. The outside
 *		code that calls into the hat may only hold locks earlier
 *		in the hierarchy than the earliest of PAGE_USELOCK and
 *		locks used in seg_kmem via kpg_alloc.
 *	HAT_REFRESH	-> do a HAT_NOSLEEP-style allocation and if that
 *		is successful, refresh the pools if they need it.
 *		This is used by the bulk loaders (hat_map and hat_dup)
 *		to get maximum mileage out of having to drop all of
 *		the locks.
 *	HAT_CANWAIT	-> wait if no memory currently available
 *		Do a NOSLEEP allocation. If that fails, bump a reserve
 *		count and sleep in the mcalloc code. The pool daemon
 *		runs about every second and tries to refresh the pools.
 *		To the extent that it succeeds, it wakes up waiters.
 *		This is flag is used only in hat_pteload, which does
 *		only a single PTE.
 *
 */
STATIC hatmc_t *
hat_mcalloc(uint_t flag)
{
	hatmc_t *mcp;

	mcp = allocmcfrompool();
	if (mcp) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(mcp);
	}
	if (flag == HAT_POOLONLY)
		return((hatmc_t *) NULL);
	mcp = allocnewmc();
	if (mcp) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(mcp);
	}
	if (flag & (HAT_NOSLEEP|HAT_REFRESH)) {
		ASSERT(flag == HAT_NOSLEEP || flag == HAT_REFRESH);
		return((hatmc_t *) NULL);
	}
	ASSERT(flag == HAT_CANWAIT);
	mcp = waitformc();
	ASSERT(mcp != (hatmc_t *)NULL);
	return(mcp);
}

/*
 * hatmap_t *hat_mcinit(hatmcp_t *mcp, hatpgtc_t *pgtcp, hatmc_t *new_mc)
 *	Set up all of the linkage for the new mapping chunk and
 *	return a pointer to the first mapping entry of the chunk.
 *
 * Calling/Exit State:
 *	The caller must own the hat_resourcelock of the corresponding as.
 */
STATIC hatmap_t *
hat_mcinit(hatmcp_t *mcp, hatpgtc_t *pgtcp, hatmc_t *new_mc)
{
	hatmcpg_t *mcpgp;
	int index;

	new_mc->hat_mcnext = (hatmc_t *)NULL;	/* zero free link for check */
	checkmcinit(new_mc);

	mcpgp = (hatmcpg_t *)((ulong_t)new_mc & PAGEMASK);
	index = new_mc - mcpgp->hat_mc;
	mcpgp->hat_mcpga.hatptcp[index].hat_pgtcp = pgtcp;
	mcp->hat_mcp = new_mc;
	return(new_mc->hat_map);
}

/*
 * hatmap_t *hat_mcallocinit(hatmc_t *mcp, hatpgtc_t *pgtcp)
 *	Allocate a mapping chunk and initialize the linkage.
 *	The hat_resourcelock must be held by the caller, so
 *	only a HAT_POOLONLY allocation is possible.
 *	See hat_mcalloc for allocation (particularly flag) rules.
 *	See hat_mcinit for initialization rules.
 *
 * Calling/Exit State:
 *	See the hat_mcalloc and hat_mcinit.
 */
STATIC hatmap_t *
hat_mcallocinit(hatmcp_t *mcp, hatpgtc_t *pgtcp)
{
	hatmc_t *new_mc;

	new_mc = hat_mcalloc(HAT_POOLONLY);
	if (new_mc == (hatmc_t *)NULL)
		return((hatmap_t *)NULL);
	return(hat_mcinit(mcp, pgtcp, new_mc));
}

/*
 *  Declarations for TLB shootdown.
 */

STATIC LKINFO_DECL(TLBSlock_lkinfo, "TLBS lock", 0);
STATIC lock_t TLBSlock;		/* serialize all tlb shootdowns */
STATIC pl_t TLBSoldpl;

volatile TLBScookie_t TLBcpucookies[MAXNUMCPU];	/* used by resume and use_private */

/* the following variables are maintained by hat_online/hat_offline. */
STATIC int maxonlinecpu = 0;		/* to limit searches in TLB code */
STATIC int minonlinecpu = MAXNUMCPU;	/* to limit searches in TLB code */

STATIC volatile TLBScookie_t TLBSgoodguess;	/* guess of timestamp last TLBS occured */
STATIC emask_t TLBShootdownbits;	/* for lazy shootdown */

#define TLBS_CPU_SIGNAL(EMASKP, FUNCP, ARG) \
		xcall(EMASKP, NULL, FUNCP, (void *)(ARG))

/*
 * void TLBSflushtlb()
 *	Set local CPU's TLBScookie and flush its TLB.
 *
 * Calling/Exit State:
 *	Go fast and loose with this one.
 *	As long as the memory fetch and memory store are each
 *	atomic (though separate) operations, only benign races
 *	occur (an old value gets stored after a new value).
 *	If an old value replaces a new value, a TLB shootdown
 *	may occur unnecessarily.  The window is VERY small
 *	(one C statement), and even if hit, it might not cause
 *	a Shootdown.  So, besides being problematic to design,
 *	adding locking here would be pure overhead.
 *	A fast spinlock cannot be used since TLB Shootdown
 *	can happen in the middle of a fast spinlock.
 *
 *	N.B.:  this code is inlined in assembly language in use_private
 *	       and resume.  These routines cannot call TLBSflushtlb without
 *	       having the return from TLBSflushtlb dirty the TLB relative
 *	       to the kernel stack (ublock).
 */
void
TLBSflushtlb(void)
{
	TLBcpucookies[l.eng_num] = lbolt;
	flushtlb();
}

/*
 * void hat_online()
 *	Initialize plocal, kas, and TLBS as needed to put this
 *	engine online.
 *
 * Calling/Exit State:
 *	Called late in bringing an engine online.
 *	It is called by that engine.
 *	Calling hat_online() signifies that the engine is ready
 *	to have itself counted in TLB/hat activity.
 *	CR3 must have been loaded.
 *	For the first one, hat_lockinit must have been called already.
 *	It uses both the kas hat resource lock and the TLBSlock to
 *	Protect the fields it has to change.  This lets other code
 *	skip the kas hat resource lock (might even be able to skip
 *	it here) and still get accurate online bit maps.
 */
void
hat_online(void)
{
	TLBSoldpl = TLBS_LOCK();
	if (maxonlinecpu < l.eng_num)
		maxonlinecpu = l.eng_num;
	if (minonlinecpu > l.eng_num)
		minonlinecpu = l.eng_num;
	kas_hatp->hat_activecpucnt++;
	EMASK_SETS(&kas_hatp->hat_activecpubits, &l.eng_mask);
	TLBSflushtlb();
	TLBS_UNLOCK(TLBSoldpl);
}

/*
 * void hat_offline()
 *	Called by an engine when it is going offline.
 *
 * Calling/Exit State:
 *	The engine will do no more TLB/hat-related operations.
 */
void
hat_offline(void)
{
	int i;

	TLBSoldpl = TLBS_LOCK();
	kas_hatp->hat_activecpucnt--;
	EMASK_CLRS(&kas_hatp->hat_activecpubits, &l.eng_mask);
	if (minonlinecpu == l.eng_num) {
		i = l.eng_num + 1;
		while (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i)) {
			i++;
		}
		minonlinecpu = i;
	}
	if (maxonlinecpu == l.eng_num) {
		i = l.eng_num - 1;
		while (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i)) {
			i--;
		}
		maxonlinecpu = i;
	}
	TLBS_UNLOCK(TLBSoldpl);
}

/*
 * void TLBSasync(ulong_t dummy)
 *	Stub with argument to call TLBSflushtlb (which has none).
 *	Used as the action subroutine for the engine mechanism.
 *
 * Calling/Exit State:
 *	Called via the engine signal mechanism during lazy shootdown.
 */
/* ARGSUSED */
STATIC void
TLBSasync(ulong_t dummy)
{
	TLBSflushtlb();
}

/*
 *  int hat_uas_shootdown_l(hat_t *hatp)
 *	Perform a batched shootdown for a given user address space.
 *
 * Calling/Exit State:
 *	The HAT lock is held on entry and remains held on exit.
 *	Returns to the caller an indication if the local tlb flush
 *	is required.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 *
 */
int
hat_uas_shootdown_l(hat_t *hatp)
{
	int flush;
 
	ASSERT(HATRL_OWNED(hatp));
	ASSERT(hatp != kas_hatp);

	if (hatp->hat_activecpucnt == 0)
		return(0);

	flush = EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask);

	if (flush != 0 && hatp->hat_activecpucnt == 1) 
		return flush;

	(void)TLBS_LOCK();
	TLBS_CPU_SIGNAL(&hatp->hat_activecpubits, TLBSasync, 0);	
	TLBS_UNLOCK(VM_HAT_RESOURCE_IPL);

	return flush;
}

/*
 * void hat_uas_shootdown(struct as *as)
 *	Perform a batched shootdown for a given user address space.
 *
 * Calling/Exit State:
 *	The HAT lock is acquired and released by this function. The hat lock
 *	is acquired to stabilize the activecpu bit vector. This function
 *	is a wrapper around hat_uas_shootdown_l().
 */
void
hat_uas_shootdown(struct as *as)
{
	hat_t *hatp = &as->a_hat;

	HATRL_LOCK_SVPL(hatp);

	if (hat_uas_shootdown_l(hatp))
		TLBSflushtlb();

	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void hat_shootdown(TLBScookie_t cookie, uint_t flag)
 *	For the lazy TLB shootdown aficionados (kernel segment drivers),
 *	if flag is HAT_NOCOOKIE, assume all engines need a flush.
 *	Otherwise, flush all engines that have not been flushed prior
 *	to the supplied cookie value.
 *
 * Calling/Exit State:
 *	No spinlocks that preclude grabbing the TLBSlock can be held
 *	by the caller.
 *	Avoid using the kas hat resource lock to minimize constraints
 *	on the callers and for performance.
 *	The races for info protected by the kas lock are all benign
 *	(trust me).
 */
void
hat_shootdown(TLBScookie_t cookie, uint_t flag)
{
	int i;
	int cnt;
	TLBScookie_t newguess, curcookie;

	if (flag == HAT_NOCOOKIE) {
		TLBSoldpl = TLBS_LOCK();
		TLBSgoodguess = lbolt;
		TLBS_CPU_SIGNAL(&kas_hatp->hat_activecpubits, TLBSasync, 0);
		TLBS_UNLOCK(TLBSoldpl);
		return;
	}
	if (cookie < TLBSgoodguess)
		return;
	cnt = 0;
	newguess = (TLBScookie_t)lbolt;

	/*
	 * While we search, prepare for the worst, so we
	 * need exclusive use of the TLBShootdownbits.
	 */
	TLBSoldpl = LOCK(&TLBSlock, VM_HAT_TLBS_IPL);
	EMASK_CLRALL(&TLBShootdownbits);
	for (i = minonlinecpu; i <= maxonlinecpu; i++) {
		if (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i))
			continue;
		curcookie = TLBcpucookies[i];
		if (curcookie > cookie) {
			if (curcookie < newguess)
				newguess = curcookie;
			continue;
		}
		if (i == l.eng_num) 
			TLBSflushtlb();
		else {
			cnt++;
			EMASK_SET1(&TLBShootdownbits, i);
		}
	}
	TLBSgoodguess = newguess;
	if (cnt)
		TLBS_CPU_SIGNAL(&TLBShootdownbits, TLBSasync, 0);

	UNLOCK(&TLBSlock, TLBSoldpl);
}

/*
 ************************************************************************
 * hat initialization (called once at startup)
 */

/*
 * hat_kas_mapping_init(ulong_t size)
 *	Allocate the mapping pointers for visible kernel mappings.
 *	The PTEs (static PTs) for all visible kernel mappings must
 *	be virtually contiguous.  REMEMBER this if another segment
 *	driver winds up needing visible mappings.
 *	size - the upper bound size in bytes of the kernel virtual
 *	space that will support visible mappings.
 *
 * Calling/Exit State:
 *	Callocs must be legal when this is called.
 */
void
hat_kas_mapping_init(ulong_t size)
{
	kas_visible_maps = (hatmap_t *)calloc(btop(size) * sizeof(hatmap_t));
	kas_visible_maps_end = kas_visible_maps + btop(size);
}

/*
 * hat_vis_ptaddr_init(vaddr_t addr)
 *	Tell the hat the start virtual address of visible mapping space.
 *	This space should have its (staticpt) page tables mapped
 *	with the standard virtually contiguous scheme  for the kas.
 *	This is used to initialize the rest of the visible mapping
 *	variables.
 *
 * Calling/Exit State:
 *	hat_kas_mapping_init() must have already been called.
 *	This must be called before any attempt to use visible kernel
 *	mappings.
 */
void
hat_vis_ptaddr_init(vaddr_t addr)
{
	ASSERT(kas_visible_maps);
	kas_visible_ptes = kvtol2ptep(addr);
	kas_vis_map_to_pte_delta = (vaddr_t)kas_visible_ptes
			- (vaddr_t)kas_visible_maps;
}

/*
 * hat_lockinit()
 *	Initialize all hat lock-related structures.
 *
 * Calling/Exit State:
 *	This is called once during system initialization from hat_init().
 *	There is only one engine online.
 *	The page pool and KMA are available.
 */
STATIC void
hat_lockinit(void)
{
	LOCK_INIT(&kas.a_hat.hat_resourcelock, VM_HAT_RESOURCE_HIER,
			VM_HAT_RESOURCE_IPL, &hatresource_lkinfo, 0);
	LOCK_INIT(&hat_ptpoollock, VM_HAT_PTPOOL_HIER, VM_HAT_PTPOOL_IPL,
		&hat_ptfree_lkinfo, 0);
	LOCK_INIT(&hat_mcpoollock, VM_HAT_MCPOOL_HIER, VM_HAT_MCPOOL_IPL,
		&hat_mcfree_lkinfo, 0);
	LOCK_INIT(&TLBSlock, VM_HAT_TLBS_HIER, VM_HAT_TLBS_IPL,
		&TLBSlock_lkinfo, 0);

	SV_INIT(&hat_ptwait);
	SV_INIT(&hat_mcwait);
}

/*
 * void
 * hat_vprotinit(void)
 *
 * hat_vprotinit initializes the prot array.
 * A FEW WORDS ON PROTECTIONS ON THE 80x86:
 *
 * The 32-bit virtual addresses we all know and love are
 * mapped identically to the linear address space (ignoring
 * floating point emulation), so that 0 virtual is 0 linear.
 * The mapping occurs via segment descriptors.
 * There are separate segments for user text (0 thru 0xBFFFFFFF),
 * user data (0 thru 0xFFFFFFFF), kernel text (0 thru 0xFFFFFFFF),
 * and kernel data (0 thru 0xFFFFFFFF).
 * The segment descriptors are the only place that executability
 * can be specified and the segments must be fixed for 32-bit
 * pointers to work as expected.
 * Thus the HAT layer only manipulates the linear address space,
 * which consists of page tables and the page directory table.
 * PROT_EXEC is treated as PROT_READ, since the 80x86 does not provide
 * execute permission control at the page level.
 * Besides the present bit, there are two bits for protection.
 * One bit controls user access, and the other controls writability.
 * The way the hardware works in practice, the kernel can write
 * all valid pages, some combinations do not really exist, but
 * we can pretend.
 * So there are four permission bit states: KR, KW, UR, UW.
 * Of these, KR and KW are effectively the same.
 * The vprot_array contains values for the permission bits (PG_RW|PG_US)
 * and the valid bit (PG_V), with all other bits zero
 * (to be used along with ~(PTE_PROTMASK|PG_V) to modify the valid PTEs).
 * The array is indexed by the generic protection code (PROT_xxx).
 *
 * Calling/Exit State:
 *	None.
 */
void
hat_vprotinit(void)
{
	uint_t p;

	/*
	 * The reason we return PG_US instead of 0 for the
	 * PROT_NONE case, is to get around a potential
	 * problem if this protection is used with page
	 * frame number 0, since we might end up with a 0
	 * pte which is really mapped. Since the kernel
	 * never uses PROT_NONE, the only way this can
	 * happen is if a user maps physical address 0
	 * through /dev/*mem and then uses mprotect()
	 * to set its protection to PROT_NONE.
	 */
	for (p = 0; p <= PROT_ALL; p++) {
		if ((p & ~PROT_USER) == PROT_NONE)
			vprot_array[p] = PG_US;
		else
			vprot_array[p] = PG_V;
		if (p & PROT_USER)
			vprot_array[p] = vprot_array[p & ~PROT_USER] | PG_US;
		if (p & PROT_WRITE)
			vprot_array[p] |= PG_RW;
	}
}

/*
 * void hat_init()
 *	This is called once during startup.
 *	Initializes the dynamic hat information.
 *	The page pool is available.
 *
 * Calling/Exit State:
 *	Single CPU environment.
 *	No dynamic hat calls prior to this point.
 */
void
hat_init(void)
{
	/*
	 * Initialize hatpt and mapping chunk page freelists.
	 * Initialize hat resource locks.
	 */

#ifdef DEBUG
	/*LINTED*/
	ASSERT(HAT_MCPP * HAT_EPMC == NPGPT);
	/*LINTED*/
	ASSERT(PAGESIZE == MMU_PAGESIZE);
#endif
	hatpt_freelist = (hatpt_t *)NULL;
	mcfreelist.p_prev = mcfreelist.p_next = (page_t *)&mcfreelist;
	hat_lockinit();
	hat_refreshpools();
}

/*
 * void
 * hat_kmadv(void)
 *	Call kmem_advise() for HAT data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
hat_kmadv(void)
{
	kmem_advise(sizeof(hatpt_t));
}

/*
 ************************************************************************
 * Static hat utility functions
 */

/*
 * link_ptap(struct as *as, hatpt_t *prevptap, hatpt_t *ptap)
 *	insert ptap into ordered chain of "as" after prevptap.
 *
 * Calling/Exit State:
 *	The hat resource spinlock is owned by the caller and is not dropped.
 *
 * Remarks:
 *      Global so that it can be called from pse_hat.c
 */
void
link_ptap(struct as *as, hatpt_t *prevptap, hatpt_t *ptap)
{
	hat_t *hatp = &as->a_hat;

	ASSERT(ptap->hatpt_pdtep);
	ASSERT(ptap != prevptap);

	ptap->hatpt_as = as;
	hatp->hat_ptlast = ptap;

	if (hatp->hat_pts == (hatpt_t *)NULL) {
		ASSERT(prevptap == (hatpt_t *)NULL);
		ptap->hatpt_forw = ptap->hatpt_back = ptap;
		hatp->hat_pts = ptap;
	} else {
		ASSERT(prevptap);

		/*
		 * Although prevptap is "before" ptap,
		 * the list is circular and prevptap may be the upper end.
		 */

		ptap->hatpt_back = prevptap;
		ptap->hatpt_forw = prevptap->hatpt_forw;
		prevptap->hatpt_forw->hatpt_back = ptap;
		prevptap->hatpt_forw = ptap;
		if (prevptap->hatpt_pdtep > ptap->hatpt_pdtep) {
			ASSERT(hatp->hat_pts->hatpt_pdtep > ptap->hatpt_pdtep);
			hatp->hat_pts = ptap;
		}
	}
}

/*
 * void unlink_ptap(struct as *as, hatpt_t *ptap)
 *	Unlink hatpt structure from the specified address space.
 *
 * Calling/Exit State:
 *	Caller owns the hat_resourcelock.
 *
 * Remarks:
 *      Global so that it can be called from pse_hat.c
 */
void
unlink_ptap(struct as *as, hatpt_t *ptap)
{
	struct hat	*hatp = &(as->a_hat);

	if (ptap->hatpt_forw == ptap) {
		hatp->hat_pts = hatp->hat_ptlast = (hatpt_t *)NULL;
	} else {
		ptap->hatpt_back->hatpt_forw = ptap->hatpt_forw;
		ptap->hatpt_forw->hatpt_back = ptap->hatpt_back;
		if (hatp->hat_pts == ptap) 
			hatp->hat_pts = ptap->hatpt_forw;
		if (hatp->hat_ptlast == ptap) 
			hatp->hat_ptlast = hatp->hat_pts;
	}
}

/*
 * hatpt_t *pteptoptap(pte_t *ptep)
 *	Get the hatpt structure associated with a pte.
 *	Needed when access is through the mapping.
 *
 * Calling/Exit State:
 *	Caller must insure that the pt cannot be deallocated.
 *	The callers do that by holding the p_uselock of the page
 *	having the mapping in question.
 *	That method is sufficient and it is necessary since
 *	the hat_resourcelock cannot be held until the hatpt structure
 *	is known (it provides the *as for the mapping).
 */
STATIC hatpt_t *
pteptoptap(pte_t *ptep)
{
	page_t *pp;

	ASSERT(!PG_ISPSE(ptep));
	pp = kvtopp((vaddr_t)ptep);
	ASSERT(pp != NULL);
	return(PHAT2(pp)->phat_ptap);
}

/*
 * boolean_t hat_remptel(hatmap_t *mapp, page_t *pp, uint_t dontneed)
 *	Internal hat subroutine to remove a PTE from the mapping
 *	linked list for its page.  This must be a visible mapping.
 *	When a mapping is found, the translation's modify bit
 *	is ORed into the page structure and the page is freed
 *	if this is the last mapping.
 *
 * Calling/Exit State:
 *	The caller must own the hat_resourcelock.
 *	The page's p_uselock must also be held.
 *	Because of the lock order of these two locks, the caller must
 *	do the LOCKing.
 *	The page's p_uselock is unlocked in this function either through
 *	page_free if page is not in use, or explicitly.
 *	So the caller must have puselockpl initialized properly to
 *	allow this as part of being prepared for this drop order.
 *	Returns B_TRUE if the page is freed, else returns B_FALSE. 
 *	The caller is responsible for syncing the pte's mod bit into
 *	the page structure.
 */
STATIC boolean_t
hat_remptel(hatmap_t *mapp, page_t *pp, uint_t dontneed)
{
	hatmap_t *nextmap;

	nextmap = (hatmap_t *)&pp->p_hat_mapping;

	while (nextmap->hat_mnext != mapp) {
		ASSERT(nextmap->hat_mnext != (hatmap_t *)NULL);
		nextmap = nextmap->hat_mnext;
	}

	nextmap->hat_mnext = mapp->hat_mnext;
	mapp->hat_mapv = 0;

	if (!PAGE_IN_USE(pp)) {
		pp->p_hat_refcnt = 0;
		pp->p_hat_agecnt = 0;
		page_free(pp, dontneed);
		return B_TRUE;
	}

	PAGE_USEUNLOCK(pp);

	return B_FALSE;
}

/*
 * void hat_zerol1ptes(hat_t *hatp, pte_t *locall1ptep, uint_t killcnt)
 *	Zaps the level 1 ptes for all the active engines.
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Remarks:
 *      Global so that it can be called from pse_hat.c
 */
void
hat_zerol1ptes(hat_t *hatp, pte_t *locall1ptep, uint_t killcnt)
{
	int i, k;
	int index;

	ASSERT(HATRL_OWNED(hatp));
	ASSERT(killcnt != 0);

	if (hatp->hat_activecpucnt == 0)
		return;

	index = locall1ptep - kpd0;	
	/*
	 * races on minonlinecpu and maxonlinecpu are benign.
	 */
	for (i = minonlinecpu; i <= maxonlinecpu; i++) {
	    if (i == l.eng_num)
		continue;
	    if (EMASK_TEST1(&hatp->hat_activecpubits, i)) {
		for (k = 0; k < killcnt; k++)
		    engine[i].e_local->pp_kl1pt[0][index + k].pg_pte = 0;
	    }
	}
}

/*
 * void hat_reload_1ptes(hat_t *hatp, pte_t *locall1ptep, uint_t cnt)
 *	Reloads the level 1 ptes zapped earlier; copies the l1pte from the
 *	current engine to  other engines' page directories.
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Remarks:
 *      Global so that it can be called from pse_hat.c
 */
void
hat_reload_l1ptes(hat_t *hatp, pte_t *locall1ptep, uint_t cnt)
{
	int i, k;
	int index;

	ASSERT(HATRL_OWNED(hatp));
	ASSERT(cnt != 0);

	if (hatp->hat_activecpucnt == 0)
		return;

	index = locall1ptep - kpd0;	
	/*
	 * races on minonlinecpu and maxonlinecpu are benign.
	 */
	for (i = minonlinecpu; i <= maxonlinecpu; i++) {
	    if (i == l.eng_num)
		continue;
	    if (EMASK_TEST1(&hatp->hat_activecpubits, i)) {
		for (k = 0; k < cnt; k++)
		    engine[i].e_local->pp_kl1pt[0][index + k].pg_pte = 
			(locall1ptep + k)->pg_pte;
	    }
	}
}


/* 
 * hat_pteload(struct seg seg, vaddr_t addr, page_t pp, uint_t pf, u_int prot,
 *		uint_t flag, uint_t pteflags)
 *	does the work for hat_memload() and hat_pteload().
 *
 * Calling/Exit State:
 *	There may or may not be a pp, depending on who calls it.
 *	The hat_memload() "Calling/Exit State" comments apply here.
 *	This is a STATIC hat private function. This function may block,
 *	so no spin locks should be held on entry.
 *
 *	This function sets the protections to the "max" of the protections
 *	in the pte and those in the argument ``prot''. Thus, protections
 *	are never downgraded.
 *
 *	/proc calls this function out of context.
 *
 * Description:
 *	The code has been somewhat reorganized and the mapping chunk
 *	allocation primitives have been revised form the uniprocessor
 *	version of the code. The key reason is to optimize lock holding
 *	and lock dropping cases for resource allocation situations.
 *	The page table and mapping chunk allocation cases now have
 *	two levels. First, an attempt is made to allocate without
 *	sleeping to keep from dropping spinlocks. The code eliminates
 *	artificial holds on resources. Artificial holds on resources
 *	would be poorly bounded because of LWPs and the hat code should
 *	not impose an artificial limit on the number of LWPs. The code
 *	actually becomes cleaner as a result of doing all of this
 *	as more code became common code that had been replicated and
 *	the extra code for artificially holding the resources was removed.
 *	Now only special bulk processing that has a guaranteed
 *	single-threaded environment is allowed to artificially hold
 *	resources. Faults are inherently parallel, and hat_pteload()
 *	does the translation work for faults.
 *
 *	In addition, once it is found that a page table is needed,
 *	The code also preallocates a mapping chunk in order to
 *	insure that there is at most only one case of needing to
 *	drop the locks and to insure that there is no interruption
 *	of lock holding once the code starts the setup of the new
 *	page table. This is part of avoiding artificial holds of page
 *	tables. The requirement of a pre-existing readlock on a pp
 *	eliminates the artificial hold of the page.
 *	The existence of the readlock is already checked by an ASSERT
 *	in hat_memload(), so no additional checks are performed here.
 *	The new "preallocation" methods necessitated the revision of
 *	of the mapping chunk allocation/initialization service routines
 *	to allow allocation prior to establishing page table context
 *	needed to initialize the chunks accounting.
 */
STATIC void
hat_pteload(struct seg *seg, vaddr_t addr, page_t *pp, uint_t pf,
		uint_t prot, uint_t flag, uint_t pteflags)
{
	hat_t *hatp;
	hatpt_t *ptap, *eptap, *new_ptap;
	pte_t *ptep, *vpdte;
	hatmap_t *mapp;
	hatmc_t *new_mc;
	hatptcp_t *ptcp;
	int mcndx, mcnum;
	hatpgt_t *pt;
	hatmcp_t *mcp;
	uint_t mode;
	struct as *cur_as = seg->s_as;
#ifdef DEBUG
	boolean_t a_new_ptap = B_FALSE;
#endif

	/* addr must be page aligned */
	ASSERT((addr & POFFMASK) == 0);

	hatp = &cur_as->a_hat;
	mode = hat_vtop_prot(prot) | pteflags;

	vpdte = kpd0 + ptnum(addr);

	/*
	 * kas use of this mechanism is not valid, since seg_map
	 * has special code it is the only visible mapper is kas.
	 */
	ASSERT(cur_as != &kas);

	new_ptap = (hatpt_t *)NULL;
	new_mc = (hatmc_t *)NULL;

	/*
	 * The hat_resourcelock is needed to add a translation.
	 * If pp non-NULL, the page USELOCK is also needed.
	 * These spinlocks must be grabbed in the order: USELOCK, hat lock.
	 * This means both will be held before it is known whether
	 * new resources must be allocated. Some fancy stepping
	 * will ensue if we need new resources.
	 * The worst case occurs only when there is a memory bind
	 * and sleeps are required to allocate resources.
	 * When that happens, the resources are "preallocated"
	 * and then processing starts over at this point.
	 *
	 * If loading a read-only translation, then must wait if any LWP 
	 * other than the current context has acquired the HAT_RDONLY_LOCK, 
	 * until that LWP has released it. The current context does not 
	 * actually need to acquire the HAT_RDONLY_LOCK, since it will be 
	 * holding the controlling hat_resourcelock during the loading.
	 */
tryagain:
	ASSERT(KS_HOLD0LOCKS());
	if (pp) {
		PAGE_USELOCK(pp);
		HATRL_LOCK_SVPL(hatp);
		ASSERT(CURRENT_LWP() != NULL);
		if ((mode & PG_RW) == 0) {
			/* Loading a non-writeable translation. */
			/* Check whether read-only translations */
			/* can be loaded.			*/
			while (!CAN_LOAD_RDONLY(hatp)) {
				/* Drop spin locks and wait */
				UNLOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL);
				SV_WAIT(&hatp->hat_rdonly_sv, PRIMEM, 
					&hatp->hat_resourcelock);
				PAGE_USELOCK(pp);
				HATRL_LOCK_SVPL(hatp);
			}
		}
	} else {
		HATRL_LOCK_SVPL(hatp);
		ASSERT(CURRENT_LWP() != NULL);
		if ((mode & PG_RW) == 0) {
			/* Loading a non-writeable translation. */
			/* Check whether read-only translations */
			/* can be loaded.			*/
			while (!CAN_LOAD_RDONLY(hatp)) {
				/* Drop spin locks and wait */
				SV_WAIT(&hatp->hat_rdonly_sv, PRIMEM,
					&hatp->hat_resourcelock);
				HATRL_LOCK_SVPL(hatp);
			}
		}
	}

	/* 
	 * First: find or allocate the page table.
	 * Note that hat_ptalloc() allocates both the page table
	 * and the hatpt_t and sets up hatpt_pde.
	 * PERF : revisit for changing hat_pts to hat_ptlast ?
	 */

	if ((eptap = ptap = hatp->hat_pts) != (hatpt_t *)NULL) {
		do {
			ASSERT(ptap);
			if (ptap->hatpt_pdtep == vpdte) {
				hatp->hat_ptlast = ptap;
				ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
#ifndef UNIPROC
				/*
				 * Load the L1 entry for MP because of the
				 * shootdown optimization.
				 */
				if (cur_as == u.u_procp->p_as)
					vpdte->pg_pte = ptap->hatpt_pde.pg_pte;
#endif
				goto found_ptap;
			}
			if (ptap->hatpt_pdtep > vpdte)
				break;
			ptap = ptap->hatpt_forw;
		} while (ptap != eptap);
		eptap = ptap->hatpt_back;	/* fix for link_ptap use */
	}

	if (new_ptap == (hatpt_t *)NULL) {
		new_ptap = hat_ptalloc(HAT_POOLONLY);
		if (new_ptap)
			goto trymc;
		/*
		 * Back out of locks and allocate a pt.
		 * Allocate a chunk, also.
		 * Then back to square 1.
		 */
		HATRL_UNLOCK_SVDPL(hatp);
		if (pp)
			PAGE_USEUNLOCK(pp);
		new_ptap = hat_ptalloc(HAT_CANWAIT);
		ASSERT(new_ptap);
		if (new_mc == (hatmc_t *)NULL)
			new_mc = hat_mcalloc(HAT_CANWAIT);
		ASSERT(new_mc);
		goto tryagain;
	}
trymc:
	if (new_mc == (hatmc_t *)NULL) {
		new_mc = hat_mcalloc(HAT_POOLONLY);
		if (new_mc == (hatmc_t *)NULL) {
			HATRL_UNLOCK_SVDPL(hatp);
			if (pp)
				PAGE_USEUNLOCK(pp);
			new_mc = hat_mcalloc(HAT_CANWAIT);
			ASSERT(new_mc);
			goto tryagain;
		}
	}
	ASSERT(new_ptap);
	ASSERT(new_mc);
	/*
	 * Since the code has already preallocated a mapping chunk,
	 * the locks will remain held until processing has completed.
	 * So, we can add the page table here at the most natural
	 * place without further concern.
	 */
	ptap = new_ptap;
	new_ptap = (hatpt_t *)NULL;
#ifdef DEBUG
	a_new_ptap = B_TRUE;
#endif
	ptap->hatpt_pdtep = vpdte;

	if (cur_as == u.u_procp->p_as) {
		ASSERT(vpdte->pg_pte == 0);
		vpdte->pg_pte = ptap->hatpt_pde.pg_pte;
	}
	link_ptap(cur_as, eptap, ptap);

found_ptap:
	ASSERT(ptap);
	ASSERT(hatp->hat_ptlast == ptap);

	pt = ptap->hatpt_ptva;

	/* Now set mapping chunk pointer. (mcnum, mcndx also)  */
	SETMAPP(addr, mcnum, mcp, mcndx, ptap);
	ptep = pt->hat_pgtc[mcnum].hat_pte + mcndx;

	if ((mapp = (hatmap_t *)mcp->hat_mcp) == 0) {
		/* Need a new chunk */
		if (new_mc) {
			mapp = hat_mcinit(mcp, pt->hat_pgtc + mcnum,
				new_mc);
			new_mc = (hatmc_t *)NULL;
		} else {
			/*
			 * This case occurs when there already was
			 * a page table with mappings for other chunks,
			 * but this is the first mapping for its chunk.
			 * Thus, we might need to sleep to get one.
			 */
			ASSERT(!a_new_ptap);
			mapp = (hatmap_t *)hat_mcallocinit(mcp,
				pt->hat_pgtc + mcnum);
			if (mapp == (hatmap_t *)NULL) {
				/*
				 * We will have to sleep. But the ptap
				 * is there honestly, so everything is cool.
				 */
				HATRL_UNLOCK_SVDPL(hatp);
				if (pp)
					PAGE_USEUNLOCK(pp);
				new_mc = hat_mcalloc(HAT_CANWAIT);
				ASSERT(new_mc);
				goto tryagain;
			}
		}
		ASSERT(ptep->pg_pte == 0);
	}
	ASSERT(((vaddr_t)mapp & ~HATMC_ADDR) == 0);
	mapp += mcndx;
	ptcp = mapptoptcp(mapp);

	if (ptep->pg_pte != 0) {
		ASSERT(ptcp->ptp.hat_mcaec);
		ASSERT(ptap->hatpt_aec);
		if ((uint_t)ptep->pgm.pg_pfn == pf) {
			/*
			 * When redoing a PTE, we never downgrade permissions.
			 * Therefore, shootdown is never necessary.
			 */
			if ((ptep->pg_pte & mode) != mode) {
				/*
				 * Or in new mode bits without changing any
				 * other bits.
				 */
				ASSERT((mode & ~(PTE_PROTMASK|PG_V)) == 0);
				atomic_or(&ptep->pg_pte, mode);
				ASSERT(ptep->pg_pte != 0);
			}
			if (flag & HAT_LOCK) {
				if ((ptep->pg_pte & PG_LOCK) == 0) {
					INCR_LOCK_COUNT(ptap, cur_as);
					ptep->pg_pte |= PG_LOCK;
				}
			}
		} else {
			/* 
			 *+ trying to change existing mapping 
			 */
			cmn_err(CE_PANIC,
			  "hat_pteload - changing existing mapping:pte = %x\n",
					ptep->pg_pte);
			/* NOTREACHED */
		}

		ASSERT(pp == NULL || pteptopp(ptep) == pp);
		ASSERT(!a_new_ptap);
		goto cleanupstuff;
	}

	ptep->pg_pte = (uint_t)mkpte(mode, pf);
	ASSERT(ptep->pg_pte != 0);

	if (flag & HAT_LOCK) {
		if ((ptep->pg_pte & PG_LOCK) == 0) {
			INCR_LOCK_COUNT(ptap, cur_as);
			ptep->pg_pte |= PG_LOCK;
		}
	}

	/* add to p_hat_mapping list */

	if (pp) {
		mapp->hat_mnext = pp->p_hat_mapping;
		pp->p_hat_mapping = mapp;

		BITMASK1_SET1(mapptowasrefp(mapp), mcndx);
		pp->p_hat_refcnt++;

		ptep->pgm.pg_chidx = pp->p_chidx;	/* for pteptopp() */

		ASSERT(pteptopp(mapptoptep(mapp)) == pp);
#ifdef DEBUG
	    if (mapp->hat_mnext != NULL) {
		if (IS_KVIS_MAP(mapp->hat_mnext))
		    ASSERT(pteptopp(KVIS_MAPTOPTE(mapp->hat_mnext)) == pp);
		else 
		    ASSERT(pteptopp(mapptoptep(mapp->hat_mnext)) == pp);
	    }
#endif
		BUMP_RSS(cur_as, 1);
	} else {
		mapp->hat_mapv = MAPV_HIDDEN;
		ptap->hatpt_hec++;
	}
	BUMP_AEC(ptcp, ptap, 1);

cleanupstuff:
	HATRL_UNLOCK_SVDPL(hatp);
	if (pp)
		PAGE_USEUNLOCK(pp);
	if (new_ptap)
		hat_ptfree(new_ptap);
	if (new_mc)
		hat_mcfree(new_mc->hat_map);

	if (IS_TRIM_NEEDED(cur_as))
		POST_AGING_EVENT_SELF();
}

/*
 * boolean_t
 * hat_unloadpte(pte_t *ptep, hatpt_t *ptap, hatptcp_t *ptcp, hatmap_t *mapp,
 *		 hat_t hatp, uint_t numpt, pl_t opl, uint_t flags,
 *		 boolean_t doTLBS)
 *	Unload a virtual address tranlation.
 *
 * Calling/Exit State:
 *	The hat resource lock is held by the caller and is returned
 *	held.  However, it may be dropped in the course of processing because
 *	the processing needs page USELOCKs and the order is wrong.
 *	Returns true if freed the page (used for stat collection).
 *	Argument numpt is relevant only if doTLBS is B_TRUE. 
 *
 *	This function does not block.
 */
STATIC boolean_t
hat_unloadpte(pte_t *ptep, hatpt_t *ptap, hatptcp_t *ptcp, hatmap_t *mapp, 
	  hat_t *hatp, uint_t numpt, pl_t opl, uint_t flags, boolean_t doTLBS)
{
	page_t *pp;
	boolean_t ret = B_FALSE;

tryagain:
	if (mapp->hat_mapv != MAPV_HIDDEN) {
		pp = pteptopp(ptep);
		ASSERT(pp != NULL);
		/*
		 * Need the USELOCK to go on.
		 */
		if (psm_intrpend(opl) || PAGE_TRYUSELOCK(pp) == INVPL) {
			/*
			 * Increment the counters so that the
			 * page table and the mapping chunk
			 * doen't go away.
			 */
			ptap->hatpt_aec++;
			ptcp->ptp.hat_mcaec++;
			TLBSflushtlb();
			HATRL_UNLOCK(hatp, opl);
			LOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL);
			HATRL_LOCK(hatp);
			ptcp->ptp.hat_mcaec--;
			ptap->hatpt_aec--;

			if (doTLBS) {
				hat_zerol1ptes(hatp, ptap->hatpt_pdtep, numpt);
				hat_uas_shootdown_l(hatp);
			}
			/* 
			 * somebody could have done a
			 * hat_pageunload() in the meantime
			 * or reloaded another page for the
			 * translation (/proc).
			 */
			if (ptep->pg_pte == 0) { 
				ASSERT(mapp->hat_mapv == 0); 
				l.puselockpl = VM_HAT_RESOURCE_IPL;
				PAGE_USEUNLOCK(pp);
				return ret;
			}
			if (pp != pteptopp(ptep)) {
				l.puselockpl = VM_HAT_RESOURCE_IPL;
				PAGE_USEUNLOCK(pp);
				goto tryagain;
			}
		}
		l.puselockpl = VM_HAT_RESOURCE_IPL;
		if (ptep->pgm.pg_mod)
			PAGE_SETMOD(pp);

		/*
		 * Must remove pte from its p_hat_mapping list
		 */
		if (hat_remptel(mapp, pp, (flags & HAT_DONTNEED)))
			ret = B_TRUE;

		BUMP_RSS(ptap->hatpt_as, -1);
	} else {
		/* hidden mapping */
		mapp->hat_mapv = 0;
		ptap->hatpt_hec--;
	}

	ASSERT((flags & HAT_UNLOCK) || ((ptep->pg_pte & PG_LOCK) == 0));

	if (flags & HAT_UNLOCK) {
		if (ptep->pg_pte & PG_LOCK)
			DECR_LOCK_COUNT(ptap, ptap->hatpt_as);
	}

	ptep->pg_pte = 0;

	BUMP_AEC(ptcp, ptap, -1);
	return ret;
}

/*
 * unloadpt(hat_t *hatp, hatpt_t *ptap, vaddr_t addr, ulong_t len, 
 *		uint_t numpt, pl_t opl, uint_t flags, boolean_t doTLBS)
 *	Unload a range of entries from a page table.
 *
 * Calling/Exit State:
 *	The hat resource lock is held by the caller and is returned held.
 *	However, it may be dropped in the course of processing because
 *	the processing needs page USELOCKs and the order is wrong.
 *	We cannot PREEMPT() here as in the original design since hat_unload
 *	hides the page tables under a temporary address space and we can
 *	deadlock if we are hitting resource limits and the swapper cannot 
 *	get to these pages. The TLB is pre-flushed (or does not need flushing)
 *	but if the hat resource lock must be dropped, reflush the level 1
 *	entry if doTLBS is set. This guarantees TLB state on exit.
 *	It might be possible to do better, but this does very well.
 *	The caller of the hat insures that there is exclusive rights
 *	to the page table pages, but not necessarily exclusive rights
 *	to the neighboring pages. The caller has artificially bumped the
 *	active entry count. This means unloadpt never frees the pt. The 
 *	caller must do it.
 *
 * Remarks:
 *      Should not be called to handle PSE mappings.
 */
STATIC void
unloadpt(hat_t *hatp, hatpt_t *ptap, vaddr_t addr, ulong_t len, uint_t numpt,
		pl_t opl, uint_t flags, boolean_t doTLBS)
{
#ifdef DEBUG
	struct as	*as = ptap->hatpt_as;
#endif
	pte_t		*ptep;
	hatmap_t	*mapp;
	hatptcp_t	*ptcp;
	int		mcnum, mcndx;
	hatmcp_t	*mcp;
	vaddr_t		endaddr = addr + len;
	page_t		*pp;
	uint_t		*wasrefp;

	ASSERT(ptnum(addr) == ptnum(endaddr - 1));
	ASSERT(ptap->hatpt_aec >= 1);
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	/* Now set mapping chunk pointer. (mcnum, mcndx also)  */
	SETMAPP(addr, mcnum, mcp, mcndx, ptap);
	for (; mcnum < HAT_MCPP; mcp++, mcnum++, mcndx = 0) {
		if (mcp->hat_mcp == 0) {
			addr = (((vaddr_t)addr + HATMCSIZE) & HATVMC_ADDR);
			if (addr >= endaddr) goto done;
			continue;
		}
		SETPTRS(mcp, mcndx, mapp, ptcp, ptep, ptap);
		for (; mcndx < HAT_EPMC; addr += PAGESIZE, mapp++, ptep++, mcndx++) {
			if (addr >= endaddr) goto done;
			if (ptep->pg_pte == 0) {
				ASSERT(mapp->hat_mapv == 0); 
				continue;
			}

			wasrefp = mapptowasrefp(mapp);
			if (BITMASK1_TEST1(wasrefp, mcndx)) {
			    BITMASK1_CLR1(wasrefp, mcndx);
			    pp = pteptopp(ptep);
			    ASSERT(pp);
			    pp->p_hat_refcnt--;
			}
			(void)hat_unloadpte(ptep, ptap, ptcp, mapp, hatp,
						numpt, opl, flags, doTLBS);
			ASSERT(as == ptap->hatpt_as);
			if (ptcp->ptp.hat_mcaec == 0) {
				FREE_MC(ptap, ptcp, mcp, mapp, addr);
				if (ptap->hatpt_aec == 1) {
					ASSERT(ptap->hatpt_locks == 0);
					return;
				}
				if (addr >= endaddr)
					goto done;
				break;
			}	/* mcaec = 0 */
			ASSERT(ptap->hatpt_aec >= 1);
		}    /* mcndx	loop */
		ASSERT(ptap->hatpt_aec >= 1);
	}   /* mcnum loop */
done:
	ASSERT(ptap->hatpt_aec != 0);
}

/*
 ************************************************************************
 * External dynamic pt hat interface routines
 */

/*
 * void hat_alloc(struct as *as)
 *	The general definition of hat_alloc is to allocate
 *	and initialize the basic hat structure(s) of an address space.
 *	Called from as_alloc() when address space is being set up.
 *	In this hat implementation, the hat structure is in the as
 *	structure and is assumed to be zeroed by as_alloc when
 *	as_alloc zeroes the as structure.
 *	So, only special MP initialization is needed.
 *
 * Calling/Exit State:
 *	There is only single engine access to the as at this point.
 *	On exit, the as is ready to use.
 */
void
hat_alloc(struct as *as)
{
	/*
	 * no page tables allocated as yet.
	 * as structure zeroed by as_alloc().
	 */
	struct hat *hatp = &as->a_hat;

	ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
	LOCK_INIT(&hatp->hat_resourcelock, VM_HAT_RESOURCE_HIER, VM_HAT_RESOURCE_IPL,
		&hatresource_lkinfo, KM_SLEEP);
	/*
	 * A hat_asload wil be done to install this as.
	 * The active engine info cannot be set until then.
	 * Until the hat_asload, access to the destination 
	 * as must be single engine. This is different from
	 * when we context switch as here we always have a context.
	 * The hat access mechanisms that are valid during the
	 * interim are hat_dup (for fork) and hat_exec (for exec).
	 */
	ASSERT(hatp->hat_rdonly_owner == NULL);
	SV_INIT(&hatp->hat_rdonly_sv);
}

/*
 * void hat_free(struct as *as)
 *	Free all of the translation resources for the specified address space.
 *	Then deinit the lock.
 *	Called from as_free when the address space is being destroyed.
 *
 * Calling/Exit State:
 *	If it is called in context, hat_asunload for this as should
 *	have been done before by the calling context. The context is single
 *	threaded at this point.
 *
 * Remarks:
 *	At this point in time, all hat resources would have been freed by
 *	the segment drivers via seg_unmap(). So we just assert that there
 *	are no page tables left in the address space.
 */
/* ARGSUSED */
void
hat_free(struct as *as)
{
	ASSERT(as != (struct as *)NULL);
	ASSERT(as != &kas);
	ASSERT(as->a_hat.hat_pts == (hatpt_t *)NULL);
	ASSERT(as->a_hat.hat_ptlast == (hatpt_t *)NULL);
	ASSERT(as->a_rss == 0);

	/*
	 * The following is temporary until close-on-exec problem is 
	 * fixed to  close the file in the context of the old AS.
	 */
	if (as->a_hat.hat_modstatsp != NULL)
		hat_free_modstats(as);

	LOCK_DEINIT(&as->a_hat.hat_resourcelock);
}

/*
 * void hat_swapout(struct as *as)
 *	Deallocate any hat structures that should disappear after swapout.
 *	It is called after hat_agerange has been called to remove
 *	unlocked translations.
 *	In this implementation, there is nothing to do.
 *
 * Calling/Exit State:
 *	Address space is seized and all unlock translations are gone.
 */
/*ARGSUSED*/
void
hat_swapout(struct as *as)
{
}

/*
 * hat_swapin(struct as *as)
 *	Set up any translation structures, for the specified address space,
 *	that are needed or preferred when the process is being swapped in.
 *
 * For the 80x86, we
 * don't do anything here. The page tables will be allocated by hat_memload.
 *
 * Calling/Exit State:
 *	Process is being swapped in, so none of its LWPs can run yet.
 */
/*ARGSUSED*/
void
hat_swapin(struct as *as)
{
}

/*
 * hat_pageunload(page_t *pp)
 *	Unload all of the hardware translations that map page pp
 *	visibly, i.e., by appearing on the p_hat_mapping list.
 *
 * Calling/Exit State:
 *	The function is called with the USELOCK spinlock
 *	of the page structure held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and
 *	is still held on return. This lock provides
 *	permission to access and change the p_hat_mapping list,
 *	which is purged as a natural part of removing
 *	the translations.
 *
 * Description:
 *	This function must also grab the hat_resourcelock spinlock
 *	for the as of each translation in the p_hat_mapping chain.
 *	The hat lock is later in the locking order explicitly
 *	to allow the required semantics of never dropping the USELOCK.
 *	To find the as for the translation, the hat resources must
 *	be accessed (but not changed yet). The page USELOCK, by
 *	protecting the mapping guarantees that read access is OK
 *	for all resources related to the translation. To change
 *	any of the hat information for a translation (including
 *	all contents of a PTE) both the page USELOCK and the hat lock
 *	must be held.
 */
void
hat_pageunload(page_t *pp)
{
	pte_t *ptep, *kill_l1pte;
	hatmap_t *mapp, *nmapp;
	hatptcp_t *ptcp;
	hatpt_t *ptap;
	int mcnx, indx;
	struct as *as;
	int flush = 0;
	struct hat *ptes_hat;
	uint_t killcnt, pteval;
	vaddr_t addr;
	struct seg *seg;
	hat_stats_t *modstatsp;

#ifdef DEBUG
	int modset = 0;
#endif
	ASSERT(pp >= pages && pp < epages);
	ASSERT(PAGE_USELOCK_OWNED(pp));

	mapp = pp->p_hat_mapping;
	pp->p_hat_mapping = (hatmap_t *)NULL;

	while (mapp != NULL) {
		nmapp = mapp->hat_mnext;
		if (IS_KVIS_MAP(mapp)) {
			/*
			 * It is a visible kernel mapping.
			 * kernel page tables don't move around
			 * so no special tricks needed.
			 * Even the hat resource lock can be skipped
			 * (no linked list to protect and the PAGE_USELOCK
			 * serializes what needs to be serialized).
			 */
			ptep = KVIS_MAPTOPTE(mapp);
			mapp->hat_mapv = 0;
			ASSERT(ptep->pg_pte != 0);
			ASSERT(pteptopp(ptep) == pp);

			addr = pteptokv(ptep);
			ASSERT(KADDR(addr) && !KADDR_PER_ENG(addr));

			seg = as_segat(&kas, addr);  
			ASSERT(seg);
#ifdef DEBUG
			modset = ptep->pg_pte & PG_M;
#endif
			pteval = atomic_fnc(&ptep->pg_pte);
			ASSERT(pteval != 0);
			if (pteval & PG_M)
				pp->p_mod = 1;

			if (!SOP_LAZY_SHOOTDOWN(seg, addr))
				hat_shootdown(lbolt, HAT_NOCOOKIE);

			mapp = nmapp;
			continue;
		}
		killcnt = 0;
		kill_l1pte = NULL;
		mcnx = HATMAPMCNDX(mapp);
		ptcp = mapptoptcp(mapp);
		ASSERT(ptcp->ptp.hat_mcaec);
		ptep = (pte_t *)(ptcp->hat_ptpv & HATPTC_ADDR) + mcnx;
				/* "& HATPT_ADDR" to get rid of hat_mcaec */ 
		ptap = pteptoptap(ptep);

		ASSERT(ptap);
recycledas:
		as = ptap->hatpt_as;
		ptes_hat = &as->a_hat;

		/*
		 * a virtual address could be calculated here,
		 * but the simpler full flush scheme is being used
		 * for the initial implementation. This does not
		 * require a virtual address. It also works for
		 * newer CPU chips (486, ...) than the 386, making
		 * the hat code intel generic.
		 * Although a mechanism is available on the 386 to
		 * flush individual virtual addresses, it is 386-specific.
		 * A different mechanism is provided on the 486.
		 * NOTE: there is a good side to the full flush.
		 * All flushes count for TLB Shootdown avoidance.
		 */

		HATRL_LOCK_SVPL(ptes_hat);

		/*
		 * Now look for a wrinkle in time, i.e., see if the
		 * address space changed. This can happen because
		 * of special tricks played by the bulk unloaders
		 * (such as hat_unload()) to minimize the hat resource
		 * lock hold time.
		 * However, the PTE is still there because we hold the USELOCK!
		 */
		if (ptap->hatpt_as != as) {
			HATRL_UNLOCK_SVDPL(ptes_hat);
			/*
			 * NOTE: we should only have to go back at most once,
			 * since if it moves from the real to a temporary as 
			 * for unloading, the unloading can't complete while
			 * we hold the PAGE_USELOCK.
			 */
			goto recycledas; 
		}
		ASSERT(ptep->pg_pte != 0);
		ASSERT(pteptopp(ptep) == pp);
#ifdef DEBUG
		modset = ptep->pg_pte & PG_M;
#endif
		pteval = atomic_fnc(&ptep->pg_pte);

		mapp->hat_mapv = 0;
		BITMASK1_CLR1(mapptowasrefp(mapp), mcnx);

		if (pteval & PG_M) {
		    PAGE_SETMOD(pp);
		    if (ptes_hat->hat_modstatsp != NULL) {
			modstatsp = hat_findstats(ptes_hat, ptap->hatpt_pdtep);
			if (modstatsp != NULL && 
			    modstatsp->stats_pdtep == ptap->hatpt_pdtep) {

			    indx = HATMAPMCNO(mapp) * HAT_EPMC + mcnx;
			    BITMASKN_SET1(modstatsp->stats_modinfo, indx);
			}
		    }
		}
		if (pteval & PG_LOCK)
			DECR_LOCK_COUNT(ptap, as);

		BUMP_RSS(as, -1);
		BUMP_AEC(ptcp, ptap, -1);

		/*
		 * the hat resource spin lock must protect a_rss
		 */

		if (ptcp->ptp.hat_mcaec == 0) {
#ifdef DEBUG
		    if (hatdb_mcfreeck) {
			ptep -= mcnx;
			mapp -= mcnx;
			for (mcnx = 0; mcnx < HAT_EPMC;mcnx++, ptep++, mapp++){
				ASSERT(ptep->pg_pte == 0);
				ASSERT(mapp->hat_mapv == 0);
			}
			--mapp;	/* must be in chunk to free it */
		    }
#endif
		    ptap->hatpt_mcp[HATMAPMCNO(ptcp->hat_ptpv)].hat_mcp = 0;
		    hat_mcfree(mapp);
		}

		if (ptap->hatpt_aec == 0) {
			ASSERT(ptap->hatpt_hec == 0);
#ifdef DEBUG
			if (hatdb_ptfreeck) {
				uint_t	idx;

				ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte;
				for (idx = 0; idx < NPGPT; idx++, ptep++)
				    ASSERT(ptep->pg_pte == 0);
				for (idx = 0; idx < HAT_MCPP; idx++)
				    ASSERT(ptap->hatpt_mcp[idx].hat_mcp == 0);
			}
#endif
			ASSERT(ptap->hatpt_locks == 0);
			kill_l1pte = ptap->hatpt_pdtep;
			killcnt = 1;
			if (ptap->hatpt_as == u.u_procp->p_as)
			    ptap->hatpt_pdtep->pg_pte = (uint)0;
			hat_zerol1ptes(ptes_hat, kill_l1pte, killcnt);
			unlink_ptap(as, ptap);
			hat_ptfree(ptap);
		}
		flush |= hat_uas_shootdown_l(ptes_hat);

		HATRL_UNLOCK_SVDPL(ptes_hat);

		mapp = nmapp;
	}

	ASSERT(!modset || pp->p_mod);

	pp->p_hat_refcnt = 0;
	pp->p_hat_agecnt = 0;

	if (flush)
		TLBSflushtlb();
}

#ifndef UNIPROC
/*
 * int hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw)
 *	If level 1 pte is unloaded, reload it (and any others found
 *	unloaded during the search).
 *	If the pt is found and was unloaded, check to see if there
 *	is a valid pte at the right place.  If it is a write fault,
 *	check for a writability, as well.  Don't need to be perfect
 *	here, JUST CLOSE!
 *
 * Calling/Exit State:
 *	Called from early in fault processing to do quick reloads
 *	without full processing.  Must grab the hat resource lock.
 *	If the pt is found and was not loaded, it (and maybe some
 *	other unloaded pts) is reloaded.
 *	If, in addition, there is a good pte in the right place,
 *	return 1, signifying that no further processing is needed.
 *	Otherwise, return 0.
 *
 * Remarks:
 *	This routine does not have to deal with PSE mappings.
 *	hat_fastfault is called only if the kernel takes a fault
 *	on a valid user address and there are no catch fault flags
 *	in effect.  Specifically, if the page is softlocked or
 *	memory locked, drivers can access this page without
 *	calling CATCH_FAULTS(), which could result in a fault
 *	because of the shootdown optimization of unloading the L1
 *	entries (only in MP kernels).  For PSE segments, L1 entries
 *	are unloaded only when the segment itself is being unmapped.
 *	There should never be a case in which a PSE segment has been
 *	softlocked or memory locked and unloaded.
 */
int
hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw)
{
	hat_t *hatp;
	hatpt_t *ptap, *eptap;
	pte_t *vpdtep;
	pte_t *ptep;
	uint_t mode;

	vpdtep = kpd0 + ptnum(addr);
	if (vpdtep->pg_pte & PG_V) {
		return(0);
	}
	hatp = &as->a_hat;
	mode = ((rw == S_WRITE) ? (PG_V | PG_RW) : PG_V);

	HATRL_LOCK_SVPL(hatp);
	ptap = hatp->hat_ptlast;
	if (ptap == (hatpt_t *)NULL) {
		HATRL_UNLOCK_SVDPL(hatp);
		return(0);
	}
	eptap = ptap;
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	if (ptap->hatpt_pdtep == vpdtep) {
		*vpdtep = ptap->hatpt_pde;
		ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte + pnum(addr);
		if ((ptep->pg_pte & mode) == mode) {
			HATRL_UNLOCK_SVDPL(hatp);
			return(1);
		}
		HATRL_UNLOCK_SVDPL(hatp);
		return(0);
	}
	if (ptap->hatpt_pdtep > vpdtep) {
		do {
			/*
			 * While we are scanning, fill in other L1 ptes
			 * as we pass, to avoid later faults. Test for
			 * non-zero before setting them, even though
			 * it would be safe, to avoid cache thrashing in
			 * an MP environment.
			 */
			if (ptap->hatpt_pdtep->pg_pte == 0)
				*ptap->hatpt_pdtep = ptap->hatpt_pde;
			ptap = ptap->hatpt_back;
			if (ptap == eptap || ptap->hatpt_pdtep < vpdtep) {
				HATRL_UNLOCK_SVDPL(hatp);
				return(0);
			}
		} while (ptap->hatpt_pdtep != vpdtep);
		if (ptap->hatpt_pdtep->pg_pte != 0) {
			HATRL_UNLOCK_SVDPL(hatp);
			return(0);
		}
		ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
		*ptap->hatpt_pdtep = ptap->hatpt_pde;
		ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte + pnum(addr);
		if ((ptep->pg_pte & mode) == mode) {
			HATRL_UNLOCK_SVDPL(hatp);
			return(1);
		}
		HATRL_UNLOCK_SVDPL(hatp);
		return(0);
	}
	do {
		/*
		 * look for target of opportunity, but check first
		 * as that is easier on caches in MP environment.
		 */
		if (ptap->hatpt_pdtep->pg_pte == 0)
			*ptap->hatpt_pdtep = ptap->hatpt_pde;
		ptap = ptap->hatpt_forw;
		if (ptap == eptap || ptap->hatpt_pdtep > vpdtep) {
			HATRL_UNLOCK_SVDPL(hatp);
			return(0);
		}
	} while (ptap->hatpt_pdtep != vpdtep);
	if (ptap->hatpt_pdtep->pg_pte != 0) {
		HATRL_UNLOCK_SVDPL(hatp);
		return(0);
	}

        ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	*ptap->hatpt_pdtep = ptap->hatpt_pde;
	ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte + pnum(addr);
	if ((ptep->pg_pte & mode) == mode) {
		HATRL_UNLOCK_SVDPL(hatp);
		return(1);
	}
	HATRL_UNLOCK_SVDPL(hatp);
	return(0);
}
#endif /* UNIPROC */

/*
 * hat_pagesyncmod(page_t *pp)
 *	Sync all visible modify bits back to the page structure,
 *	while clearing them in the PTEs.
 *
 * Calling/Exit State:
 *	The function is called with the USELOCK spinlock
 *	of the page structure held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and
 *	is still held on return. This lock provides
 *	permission to access the p_hat_mapping list, and part of the
 *	permission to change the PTEs (clearing mod bits).
 *	This function is key in establishing a commit point for
 *	writing an active, but dirty, page to secondary media.
 *	Such a commit point is needed to insure that later modifications
 *	(after commencement of I/O) are not lost.
 *
 * Description:
 *	The USELOCK on the page guarantees stable hat resources
 *	for all visible translations for the page, and so allows
 *	looking at a PTE to see if it has a modify bit set.
 *	If a modify bit is found set, this function must also grab
 *	the hat_resourcelock spinlock for the as of that translation
 *	in order to have permission to clear that modify bit.
 *	The hat lock is later in the locking order explicitly 
 *	to allow accomplishing this sort of task without dropping
 *	the page USELOCK.
 */
void
hat_pagesyncmod(page_t *pp)
{
    pte_t *ptep;
    hatmap_t *mapp;
    hatpt_t *ptap;
    int flush = 0;
    struct hat *ptes_hat;
    struct as *as;
    hat_stats_t *modstatsp;
    int indx;
    struct seg *seg;
    vaddr_t addr;

#ifdef DEBUG
	int modset = 0;
#endif

    ASSERT(pp >= pages && pp < epages);
    ASSERT(PAGE_USELOCK_OWNED(pp));

    mapp = pp->p_hat_mapping;

    while (mapp != (hatmap_t *)NULL) {
	if (IS_KVIS_MAP(mapp)) {
	    /*
	     * it is a visible kernel mapping.
	     */
	    ptep = KVIS_MAPTOPTE(mapp);

	    ASSERT(pteptopp(ptep) == pp);
	
	    /* 
	     * It's okay to clear the mod bit here. If somebody sets the
	     * mod bit at a later stage, it will get taken of later. It
	     * won't be missed.
	     */	
	    if (ptep->pg_pte & PG_M) {
#ifdef DEBUG
		modset++;
#endif
		pp->p_mod = 1;
		atomic_and(&ptep->pg_pte, ~PG_M);

		addr = pteptokv(ptep);
		ASSERT(KADDR(addr) && !KADDR_PER_ENG(addr));

		seg = as_segat(&kas, addr);  
		ASSERT(seg);

		if (!SOP_LAZY_SHOOTDOWN(seg, addr))
		    hat_shootdown(lbolt, HAT_NOCOOKIE);
	    } /* if (pg_pte & PG_M) */

	    mapp = mapp->hat_mnext;
	    continue;
	}	/* IS_KVIS? */

	ptep = mapptoptep(mapp);
	ASSERT(pteptopp(ptep) == pp);
	/*
	 * pteptoptap() is encased as a single construct to isolate
	 * knowledge of the details of the page table virtual
	 * mappings from the mainline hat code.
	 */
	ptap = pteptoptap(ptep);
	ASSERT(ptap);
recycledas:
	as = ptap->hatpt_as;
	ptes_hat = &as->a_hat;
	/*
	 * a virtual address could be calculated here,
	 * but the simpler full flush scheme is being used
	 * for the initial implementation. This does not
	 * require a virtual address. It also works for
	 * newer CPU chips (486, ...) than the 386, making
	 * the hat code intel generic.
	 */

	HATRL_LOCK_SVPL(ptes_hat);

	/*
	 * Now look for a wrinkle in time, i.e., see if the
	 * address space changed. This can happen because
	 * of special tricks played by the bulk unloaders
	 * (such as hat_unload()) to minimize the hat resource
	 * lock hold time.
	 * However, the PTE is still there because we hold the USELOCK!
	 */
	if (ptap->hatpt_as != as) {
	    HATRL_UNLOCK_SVDPL(ptes_hat);
	    goto recycledas;
	}

	    if (ptep->pg_pte & PG_M) {
#ifdef DEBUG
		modset++;
#endif
		atomic_and(&ptep->pg_pte, ~PG_M);
		ASSERT(pteptopp(ptep) == pp);
		PAGE_SETMOD(pp);
		if (ptes_hat->hat_modstatsp != NULL) {
		    modstatsp = hat_findstats(ptes_hat, ptap->hatpt_pdtep);
		    if (modstatsp != NULL && 
			modstatsp->stats_pdtep == ptap->hatpt_pdtep) {

			indx = HATMAPMCNO(mapp) * HAT_EPMC + HATMAPMCNDX(mapp);
			BITMASKN_SET1(modstatsp->stats_modinfo, indx);
		    }
		}
		flush |= hat_uas_shootdown_l(ptes_hat);
	    }

	HATRL_UNLOCK_SVDPL(ptes_hat);
	mapp = mapp->hat_mnext;
    }
    ASSERT(!modset || pp->p_mod);	
    if (flush)
	TLBSflushtlb();
}

/*
 * hat_pagegetmod(page_t *pp)
 *	Look for a modify bit either in the page structure or a PTE.
 *	If found in a PTE, set the p_mod field.
 *	Never change a PTE.
 *	This routine is important for performance since it allows
 *	the determination of whether a page is dirty without causing
 *	TLB Shootdown or even needing the hat_resourcelock for
 *	any of the translations.
 *	This is important for minimizing the cost of fsflush.
 *
 * Calling/Exit State:
 *	The function is called with the USELOCK spinlock
 *	of the page structure held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and
 *	is still held on return. This lock provides
 *	permission to access the p_hat_mapping list, and guarantees
 *	stable hat resources (though not PTE bits).
 *	Stable PTE bits can be guaranteed only by unloading the page.
 *	But the instability is one way: ref and mod bits can be set
 *	at any time but are never cleared by MMUs.
 */
void
hat_pagegetmod(page_t *pp)
{
	pte_t *ptep;
	hatmap_t *mapp;

	ASSERT(pp >= pages && pp < epages);
	ASSERT(PAGE_USELOCK_OWNED(pp));

	if (pp->p_mod)
		return;
	mapp = pp->p_hat_mapping;

	while (mapp != (hatmap_t *)NULL) {
		if (IS_KVIS_MAP(mapp)) {
			/*
			 * it is a visible kernel mapping.
			 */
			ptep = KVIS_MAPTOPTE(mapp);
		}
		else
			ptep = mapptoptep(mapp);

		ASSERT(pteptopp(ptep) == pp);

		/*
		 * Since a mapping cannot be removed without first
		 * having the page uselock, no additional locks are needed.
		 */
		if (ptep->pg_pte & PG_M) {
			pp->p_mod = 1;
			return;
		}
		mapp = mapp->hat_mnext;
	}
}

/*
 * alloc_tmp_as()
 *	allocate a dummy as to use to hide page tables in.
 *	initialize the hat lock.
 *	Can sleep in kmem_zalloc.
 *
 * Calling/Exit State:
 *	No spin locks held.
 */
STATIC struct as *
alloc_tmp_as()
{
	struct as *as;
	
	as = (struct as *)kmem_zalloc(sizeof (struct as), KM_NOSLEEP);
	if (as != NULL)
		LOCK_INIT(&as->a_hat.hat_resourcelock, VM_HAT_RESOURCE_HIER,
			VM_HAT_RESOURCE_IPL, &hatresource_lkinfo, KM_SLEEP);
	return(as);
}

/*
 * void free_tmp_as(struct as *as)
 *	deinit the hat lock
 *	and free the dummy as.
 *
 * Calling/Exit State:
 *	Nothing special.
 */
STATIC void
free_tmp_as(struct as *as)
{
	LOCK_DEINIT(&as->a_hat.hat_resourcelock);
	kmem_free(as, sizeof *as);
}

/*
 * hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags)
 *	Unload a range of visible kernel mappings.
 *
 * Calling/Exit State:
 *	The PAGE_USELOCKs will be grabbed and released.
 *	The caller assures there is no access and must do lazy
 *	TLB Shootdown processing.
 *	Valid values for the flags argument are HAT_DONTNEED which is
 *	passed onto page_free() and HAT_CLEARMOD which clears the
 *	mod bit in the page struct as well as in the translation.
 */
void
hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags)
{
	pte_t *ptep, ptesnap;
	hatmap_t *mapp;
	page_t *pp;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((nbytes & POFFMASK) == 0);

	ptep = kvtol2ptep(addr);
	mapp = KVIS_PTETOMAP(ptep);

	ASSERT(IS_KVIS_MAP(mapp));

	for (; nbytes != 0; ptep++, mapp++, nbytes -= PAGESIZE) {
		if ((ptesnap = *ptep).pg_pte == 0) {
			ASSERT(mapp->hat_mapv == 0);
			continue;
		}
		pp = pteptopp(&ptesnap);
		ASSERT(pp != NULL);
		PAGE_USELOCK(pp);
		/* 
		 * The page could have been aborted in the mean time
		 * and our translation unloaded by hat_pageunload.
		 * If so, skip it.
		 */
		if (ptep->pg_pte == 0) {
			PAGE_USEUNLOCK(pp);
			ASSERT(mapp->hat_mapv == 0);
			continue;
		}			
		ASSERT(pteptopp(ptep) == pp);

		/*
		 * Must remove pte from its p_hat_mapping list.
		 * Clear the mod bits if neccessary.
		 */
		if (flags & HAT_CLRMOD) {
			pp->p_mod = 0;
			ptep->pg_pte &= ~PG_M;
		} else if (ptep->pgm.pg_mod) {
			pp->p_mod = 1;
		}

		(void)hat_remptel(mapp, pp, (flags & HAT_DONTNEED));
		ptep->pg_pte = 0;
	}
}

/*
 * hat_unload(struct seg seg, vaddr_t addr, ulong_t len, u_int flags)
 *	Unload all of the mappings in the range [addr,addr+len] .
 *
 * Calling/Exit State:
 *	This routine does not block. The locks acquired by this routine are
 *      the hat resource lock, the USELOCK spinlocks for any of the pages,
 *	and the global vm_pgfreelk spinlock (gota free some pages).
 *	User address spaces:
 *	The state on completion is that the level 1 PTEs that cover the
 *	range have been unloaded (minimizes TLBS hold time and there
 *	is a fast fault path to reload them), any translations in
 *	the range have been unloaded, any hat resources that are no longer
 *	needed have been freed, and any notinuse page has been freed.
 *	Any locks that were held by hat_unload are released.
 *	Kernel address space:
 *	New visible kernel mapping interface obseletes this.
 *
 * Discussion:
 *	To minimize hold time on the hat resource lock special tricks
 *	are used. All page tables fully covered by the range and not
 *	specially locked are removed from the active list and kept on
 *	a private address space (needed to keep hat_page* code happy).
 *	There will be a private pool of address spaces, though with global
 *	virtual addresses (needs to be accessed from other CPUs,
 *	hat_pageunload or even this call if a PREEMPT occurs).
 *	These are needed only occasionally for unusually large requests.
 *	NO MORE ARTIFICIALLY LOCKED page tables because of the new
 *	visible kernel mapping interface.
 *
 *	The partially covered page tables must be processed in place
 *	with the original hat resource lock held.
 *	Then the processing switches to the local as and its resource lock.
 *
 * Remarks:
 *      Segments mapped PSE mappings should not be calling
 *      this routine; such segments call pse_hat_unload instead.
 *
 */
void
hat_unload(struct seg *seg, vaddr_t addr, ulong_t len, uint_t flags)
{
	struct as   *as = seg->s_as;
	struct hat  *hatp, *tmp_hatp;
	struct as   *tmp_as;
	vaddr_t	    endaddr, ptendaddr;
	pte_t	    *a1, *a2, *vpdte;
	hatpt_t	    *ptap, *eptap, *mvptap, *tmp_ptap;
	hatpt_t	    *savptap = (hatpt_t *)NULL;
	pl_t	    origpl, trypl;
	uint_t	    ptcnt, ptlen;
	int	    flush;

	/* addr must be page aligned */
	ASSERT(((u_long) addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);

	endaddr = addr + len;
	ASSERT(endaddr >= addr);
	vpdte = kpd0 + ptnum(addr);

	ASSERT(as != &kas);

	/*
	 * Do we need to use the hide the page tables trick?
	 * Skip fragment PTs at either end.
	 */
	a1 = kpd0 + btoptblr(addr);
	a2 = kpd0 + btoptbl(endaddr);
	if (a2 > a1) {
	    tmp_as = alloc_tmp_as();
	}
	else
	    tmp_as = (struct as *)NULL;

	hatp = &as->a_hat;

	origpl = HATRL_LOCK(hatp);
	ptap = hatp->hat_pts;
	if (ptap == (hatpt_t *)NULL) {
		ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
		HATRL_UNLOCK(hatp, origpl);
		if (tmp_as != NULL)
			free_tmp_as(tmp_as);
		return;
	}

	/*
	 * flush the range of level 1 PTEs.
	 * If we are lucky in the trylock roulette,
	 * this is all that is needed, but at least it
	 * covers any page tables we hide away.
	 */
	ptcnt = ptnum(endaddr-1) - ptnum(addr) + 1;

	hat_zerol1ptes(hatp, vpdte, ptcnt);
	flush = hat_uas_shootdown_l(hatp);

	/*
	 * Now look for PTs to hide. This trick is being done for two reasons: 
	 * One is other fault operations cannot go on if we hold the hat 
	 * resource lock too long (such as hat_pageunload and hat_pagesyncmod)
	 * and the other reason is to save some TLB shootdowns if possible.
	 */
	if (tmp_as) {
		tmp_hatp = &tmp_as->a_hat;
		ASSERT(tmp_hatp->hat_pts == (hatpt_t *)NULL);
		ASSERT(tmp_hatp->hat_ptlast == (hatpt_t *)NULL);
		trypl = HATRL_TRYLOCK(tmp_hatp);
		ASSERT(trypl != (pl_t)INVPL);
		tmp_ptap = (hatpt_t *)NULL;
		eptap = ptap;
		ASSERT(eptap == hatp->hat_pts);
		do {
			ASSERT(ptap && ptap->hatpt_aec);
			ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
			if (ptap->hatpt_pdtep >= a2)
				break;
			if (ptap->hatpt_pdtep < a1
				|| ptap->hatpt_aec > 3*NPGPT) {
				ptap = ptap->hatpt_forw;
				if (ptap == hatp->hat_pts)
					break;
				continue;
			}
			ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
			mvptap = ptap;
			ptap = ptap->hatpt_forw;
			as->a_lockedrss -= mvptap->hatpt_locks;
			as->a_rss -= mvptap->hatpt_aec - mvptap->hatpt_hec;
			tmp_as->a_lockedrss += mvptap->hatpt_locks;
			tmp_as->a_rss += mvptap->hatpt_aec - mvptap->hatpt_hec;
			if (u.u_procp->p_as == as)
				mvptap->hatpt_pdtep->pg_pte = (uint)0;
			ASSERT(mvptap->hatpt_as != tmp_as);
			unlink_ptap(mvptap->hatpt_as, mvptap);
			link_ptap(tmp_as, tmp_ptap, mvptap);
			tmp_ptap = mvptap;
		} while (ptap != eptap && hatp->hat_pts != (hatpt_t *)NULL);

		HATRL_UNLOCK(tmp_hatp, trypl);
		if ((ptap = hatp->hat_pts) == (hatpt_t *)NULL)
			goto doabsconded;
	}
	/*
	 * We still have potential partial page tables at the beginning
	 * and end of the range.
	 * Check for them.
	 */
	a2 = kpd0 + btoptbl(endaddr - 1);
	/*
	 * save current hatpt pointer in eptap here
	 * so the do loop will stop properly
	 */
	eptap = ptap;
	do {
		ASSERT(ptap && ptap->hatpt_aec);
		ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
		if (savptap) {
			eptap = savptap;
			savptap = (hatpt_t *)NULL;
		}

		if (vpdte > ptap->hatpt_pdtep) {
			ptap = ptap->hatpt_forw;
			ASSERT(ptap && ptap->hatpt_aec);
			ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
			if (ptap == hatp->hat_pts)
				break;
			continue;
		}
		if (a2 < ptap->hatpt_pdtep)
			break;

		ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
		a1 = ptap->hatpt_pdtep;
		/* 
		 * addr will be less than the current page table's starting
		 * addr if we have done the "optimization" on an intermediate
		 * page table.
		 */
		if (addr < ((a1 - kpd0) * VPTSIZE))
			addr = ((a1 - kpd0) * VPTSIZE);
		ptendaddr = (vaddr_t) (((a1 - kpd0) * VPTSIZE) - 1 + VPTSIZE);
		if (endaddr > ptendaddr)  /* crossing page tables ? */
			ptlen = ptendaddr - addr + 1;
		else
			ptlen = endaddr - addr;

		ptap->hatpt_aec++;	
		ptcnt = ptnum(endaddr-1) - ptnum(addr) + 1;
		unloadpt(hatp, ptap, addr, ptlen, ptcnt, origpl, flags,
			B_TRUE);
		addr += ptlen;
		/*
		 * We need to reset eptap here. This is because the hat lock
		 * can be dropped in unlodpt operation. What could happen
		 * is that page tables may have been aged or swapped in the
		 * mean time and we could have eptap pointing to bogus
		 * pointer. Thus the loop would never end.
		 */
		eptap = hatp->hat_pts;
		if (--ptap->hatpt_aec == 0) {
			ASSERT(ptap->hatpt_hec == 0);
			savptap = ptap->hatpt_forw;
			FREE_PT(ptap, savptap, as, addr)
			ptap = savptap;
		} else {
			hat_reload_l1ptes(hatp, ptap->hatpt_pdtep, 1);
			ptap = ptap->hatpt_forw;
		}
	} while (ptap != eptap);
doabsconded:
	if (flush)
	    TLBSflushtlb();

	HATRL_UNLOCK(hatp, origpl);

	if (tmp_as == (struct as *)NULL)
		return;
	if (tmp_ptap == 0) {
		free_tmp_as(tmp_as);
		return;
	}
	HATRL_LOCK(tmp_hatp);
	while ((ptap = tmp_hatp->hat_pts) != (hatpt_t *)NULL) {
		ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
		addr = (vaddr_t) ((ptap->hatpt_pdtep - kpd0) * VPTSIZE);
		ptap->hatpt_aec++;
		unloadpt(tmp_hatp, ptap, addr, VPTSIZE, 0, origpl, flags,
			B_FALSE);
		ptap->hatpt_aec--;
		ASSERT(ptap->hatpt_aec == 0);
		ASSERT(ptap->hatpt_hec == 0);
		unlink_ptap(tmp_as, ptap);
		hat_ptfree(ptap);
	}
	HATRL_UNLOCK(tmp_hatp, origpl);
	free_tmp_as(tmp_as);
}

/*
 * hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
 *		bolean_t doshoot)
 *	Change the protections for the virtual address range
 *	[addr,addr+len] to the protection prot.
 *
 * Calling/Exit State:
 *	The return value is useful only when argument doshoot is B_FALSE.
 *	It return B_TRUE, if the caller needs to perform a shootdown.
 *	It returns B_FALSE, if no shootdown is neccessary. 
 *	No hat or page locks are held by the caller.
 *	No hat or page locks are held on return.
 *	This is never used on the kas.
 *	This changes only active PTEs, no mappings are added,
 *	and none are removed. So only the hat resource lock is needed.
 *	TLB Shootdown is performed based on the doshoot flag. If the flag
 *	is B_FALSE, then the caller gurantees that there are no accesses
 *	to this range and the caller is reponsible to perfrom the shootdown.
 *	Since vtop operations must search the hatpt list anyway, the trick of 
 *	unloading level 1 entries is used to minimize the TLBS
 *	synchronization time (the time between TLBSsetup and TLBSteardown).
 * Remarks:
 *      Segments mapped via PSE mappings should not be calling
 *      this routine; such segments call pse_hat_chgprot instead.
 */
boolean_t
hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
		boolean_t doshoot)
{
    struct hat *hatp = &seg->s_as->a_hat;
    vaddr_t endaddr;
    hatpt_t *ptap, *eptap;
    hatmap_t *mapp;
    int mcndx, mcnum;
    hatpgt_t *pt;
    hatmcp_t *mcp;
    pte_t *vpdte, *evpdte, *ptep, *start_l1;
    uint_t pprot, pmask, ptcnt = 0;
    boolean_t shotdown = B_FALSE;
    pte_array_t  *ptesbufp = l.ptesbuf;
    int num_ptes = 0;
#ifdef DEBUG
    hatpt_t *dummy_ptap;
    hatptcp_t *ptcp;
#endif

    /* addr must be page aligned */
    ASSERT(((u_long) addr & POFFMASK) == 0);
    ASSERT(seg->s_as != &kas);

    if (prot == (unsigned)~PROT_WRITE) {
	pmask = (unsigned)~PG_RW;
	pprot = 0;
    } else {
	pmask = (unsigned)~(PTE_PROTMASK|PG_V);
	pprot = hat_vtop_prot(prot);
    }

    endaddr = addr + len;
    ASSERT(endaddr >= addr);
    vpdte = kpd0 + ptnum(addr);
    evpdte = kpd0 + ptnum(endaddr-1);

    HATRL_LOCK_SVPL(hatp);

    /*
     * Wait until anything depending on protections not becoming
     * restrictive finishes before making the protections restrictive.
     */
    ASSERT(CURRENT_LWP() != NULL);
    while (((prot & PROT_WRITE) == 0) && !CAN_LOAD_RDONLY(hatp)) {
	SV_WAIT(&hatp->hat_rdonly_sv, PRIMEM, &hatp->hat_resourcelock);
	HATRL_LOCK_SVPL(hatp);
    }

    ptap = eptap = hatp->hat_pts;
    if (ptap == (hatpt_t *)NULL) {
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
	HATRL_UNLOCK_SVDPL(hatp);
	return B_FALSE;
    }

    ASSERT(ptap->hatpt_aec);
    ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);

    /* look for first pt */
    while (ptap->hatpt_pdtep < vpdte) {
	if ((ptap = ptap->hatpt_forw) == eptap) {
	    HATRL_UNLOCK_SVDPL(hatp);
	    return B_FALSE;
	}
	ASSERT(ptap && ptap->hatpt_aec);
	ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
    }
    if (ptap->hatpt_pdtep > evpdte) {
	HATRL_UNLOCK_SVDPL(hatp);
	return B_FALSE;
    }

    do {
	ASSERT(ptap->hatpt_pdtep >= kpd0);
	if (vpdte++ < ptap->hatpt_pdtep) {
		addr = (vaddr_t)((ptap->hatpt_pdtep - kpd0) * VPTSIZE);
		vpdte = ptap->hatpt_pdtep + 1;
	}
	if (addr >= endaddr)
		goto done;

	ASSERT(!PG_ISPSE(ptap->hatpt_pdtep));

	pt = ptap->hatpt_ptva;
	mcnum = HATMCNO(addr);
	mcndx = HATMCNDX(addr);

	mcp = ptap->hatpt_mcp + mcnum;

	for (; mcnum < HAT_MCPP; mcp++, mcndx = 0, mcnum++) {
	    if (mcp->hat_mcp == 0) {
		addr = (vaddr_t)(((uint)addr + HATMCSIZE) & HATVMC_ADDR);
		if (addr >= endaddr)
		    goto done;
		continue;
	    }
	    mapp = mcptomapp(mcp);
	    ASSERT(((ulong_t)mapp & ~HATMC_ADDR) == 0);
#ifdef DEBUG
	    ptcp = mapptoptcp(mapp);
	    ASSERT(ptcp->ptp.hat_mcaec);
	    ASSERT((uint)ptap->hatpt_aec >= ptcp->ptp.hat_mcaec);
#endif
	    ptep = pt->hat_pgtc[mcnum].hat_pte;
#ifdef DEBUG
	    ASSERT(ptcptoptep(ptcp) == ptep);
#endif
	    mapp += mcndx;
	    ptep += mcndx;
	    for (;mcndx < HAT_EPMC; addr += PAGESIZE, mapp++,ptep++,mcndx++) {
		if (addr >= endaddr)
		    goto done;
		if (ptep->pg_pte == 0) {
		    ASSERT(mapp->hat_mapv == 0);
		    continue;
		}
		ASSERT(hat_vtopte_l(hatp, addr, &dummy_ptap) == ptep);

		if (!shotdown) {
		    if (HAT_REDUCING_PERM(ptep->pg_pte, pprot)) {
			if (doshoot) {
			    ASSERT(num_ptes <= MAXL2_PTES);
			    if (num_ptes == MAXL2_PTES) {
				start_l1 = vpdte - 1;
				ptcnt = evpdte - (vpdte - 1) + 1;

				hat_zerol1ptes(hatp, start_l1, ptcnt);
				if (hat_uas_shootdown_l(hatp))
					TLBSflushtlb();
				shotdown = B_TRUE;
			    } else {
			        ptesbufp->pte_mapp = ptep;
				ptesbufp->pteval = 
					atomic_fnc_bit((uint_t *)ptep,
							ORD_PG_V);
				ptesbufp->pteval |= ptep->pg_pte;
				ptesbufp->pteval &= pmask;
				ptesbufp->pteval |= pprot;
				num_ptes++;
				ptesbufp++;
			    }
			} else  /* doshoot B_FALSE */
			    shotdown = B_TRUE;
		    } else { /* reducing permissions ? */
			ptep->pg_pte = pprot | (ptep->pg_pte & pmask);
			continue;
		    }
		} /* end-if !shotdown */
		if (shotdown)
		    ptep->pg_pte = pprot | (ptep->pg_pte & pmask);
	    }  /* mcndx loop */
	}    /* mcnum loop */
	ptap = ptap->hatpt_forw;
    } while (ptap != eptap);
done:
    if (num_ptes) {
	ASSERT(doshoot);
	if (!shotdown) {
	    if (hat_uas_shootdown_l(hatp))
		TLBSflushtlb();
	}
	ptesbufp = l.ptesbuf;
	while (num_ptes--) {
	    ASSERT(ptesbufp->pteval != 0);
	    ((pte_t *)ptesbufp->pte_mapp)->pg_pte = ptesbufp->pteval;
	    ptesbufp++;
	}	    
	if (ptcnt)  {   /* i.e. we unloaded l1 ptes */
	    ASSERT(shotdown);
	    hat_reload_l1ptes(hatp, start_l1, ptcnt);
	}
	shotdown = B_TRUE;
    }
    HATRL_UNLOCK_SVDPL(hatp);
    return shotdown;
}

/*
 * hat_kas_memload(vaddr_t addr, page_t *pp, uint_t prot)
 *	Load a visible (on p_hat_mapping chain) kernel translation.
 *
 * Calling/Exit State:
 *	Assumes that combination of any locks by caller combined with
 *	the PAGE_USELOCK provide all synchronization that is needed.
 *	All PTEs for visible kernel mappings must be virtually
 *	contiguous.
 *	REMEMBER that fact if a second kernel segment driver needs
 *	visible mappings (seg_map is only one for now).
 */
void
hat_kas_memload(vaddr_t addr, page_t *pp, uint_t prot)
{
	pte_t *ptep;
	hatmap_t *mapp;
	uint_t mode;
	uint_t pf, pteval;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT(pp);

	mode = hat_vtop_prot(prot);
	pf = page_pptonum(pp);
	ptep = kvtol2ptep(addr);

	mapp = KVIS_PTETOMAP(ptep);

	ASSERT(IS_KVIS_MAP(mapp));

	PAGE_USELOCK(pp);
	if (ptep->pg_pte != 0) {
		if ((uint)ptep->pgm.pg_pfn == pf) {
			/*
			 * Redoing an existing PTE is a TLB shootdown case.
			 * If mode is the same, then we don't do anything.
			 */
			if ((ptep->pg_pte & mode) == mode) {
				PAGE_USEUNLOCK(pp);
				return;
			}
			ASSERT((mode & ~(PTE_PROTMASK | PG_V)) == 0);

			pteval = atomic_fnc(&ptep->pg_pte);
			hat_shootdown(lbolt, HAT_NOCOOKIE);

			ptep->pg_pte = mode | (pteval & ~(PTE_PROTMASK|PG_V));

			ASSERT(ptep->pg_pte != 0);
			ASSERT(pteptopp(ptep) == pp);
			PAGE_USEUNLOCK(pp);
			return;
		} else {
			/*
			 *+ trying to change existing mapping.
			 */
			cmn_err(CE_PANIC,
				"hat_kas_memload - changing mapping:pte= %x\n",
				ptep->pg_pte);
		}
	}
	ptep->pg_pte = (uint_t)mkpte(mode, pf);
	ASSERT(ptep->pg_pte != 0);

	/* add to p_hat_mapping list */
	mapp->hat_mnext = pp->p_hat_mapping;
	pp->p_hat_mapping = mapp;
	ptep->pgm.pg_chidx = pp->p_chidx;	/* for pteptopp() */

	PAGE_USEUNLOCK(pp);
}

/*
 * hat_memload(struct seg *seg, vaddr_t addr, page_t *pp, uint_t prot,
 *		uint_t lock)
 *
 *	Set up addr to map to page pp with protection prot.
 *	Lock the translation if lock set.
 *
 * Calling/Exit State:
 *	The function is called with the page read locked but the
 *	USELOCK is not held. There might already be a translation
 *	at the address. The valid case is that the old page is pp.
 *	The new page abort philosophy eliminates the other (historical) case.
 *	If the page is already mapped, fix up the locking and protections.
 *	The page is returned still read locked and USELOCK is not held.
 *	The new translation appears in the p_hat_mapping list.
 *
 * Description:
 *	All of the work is in hat_pteload()
 */
void
hat_memload(struct seg *seg, vaddr_t addr, page_t *pp, uint_t prot, uint_t flag)
{
	ASSERT(pp >= pages && pp < epages);

	ASSERT(PAGE_IS_LOCKED(pp));
	hat_pteload(seg, addr, pp, page_pptonum(pp), prot, flag, 0);

#ifdef CC_PARTIAL
	/*
	 * The following section of code is added to treat a
	 * potential covert channel threat in the use of the page
	 * cache to transmit covert information.
	 *
	 * Initially, p_lid is cleared in page_get().
	 * When a page is physically (i/o) read in (sfs_getpageio(),
	 * spec_getpageio()), the level of the calling process
	 * faulting the page in is registered in p_lid.
	 * Anonymous pages are skipped. Shared memory is not an
	 * issue because shared memory operations are only
	 * allowed at a single level (unprivileged use).
	 * If at this time the caller's level is not the same as
	 * the level at which the page was faulted in, count the
	 * event.  Note that once counted, this page need not
	 * be counted again.  The p_lid field is zeroed to
	 * indicate this fact.
	 */
	if (pp->p_lid
	&&  MAC_ACCESS(MACEQUAL, pp->p_lid, CRED()->cr_lid)) {
		pp->p_lid = (lid_t)0;
		CC_COUNT(CC_CACHE_PAGE, CCBITS_CACHE_PAGE);
	}
#endif /* CC_PARTIAL */
}

/*
 * hat_devload(struct seg, vaddr_t addr, uint_t ppid, u_int prot, u_int lock)
 *	Load a hidden mapping for the page frame specified by physical
 *	page ID ppid with protections prot. Lock the translation if
 *	lock is set. The page frame might happen to have a page structure,
 *	but that never gets involved with hidden mappings (mappings that
 *	do not appear on the p_hat_mapping chain).
 *	Since this a hidden mapping, there is no special entrance lock criteria.
 *	If a mapping already exists, it must match ppid in call.
 *	hat_pteload() does it all.
 *
 * Calling/Exit State:
 *	There are no locks held at the entry to this function since this page is
 *	not in the mapping chain. So unlike hat_memload the p_uselock is not held.
 *
 *
 * Remarks:
 *      Segments mapped via PSE mappings should not be calling
 *      this routine; such segments load mappings via pse_hat_devload.
 */
void
hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot, uint_t flag)
{
	uint_t pteflags = 0;

	/*
	 * Disable caching for anything outside of the mainstore memory.
	 */
	if (!mainstore_memory(mmu_ptob(ppid)))
		pteflags |= PG_CD;

	hat_pteload(seg, addr, NULL, ppid, prot, flag, pteflags);
}


/*
 * hat_map(struct seg *seg, vnode_t *vp, off_t vp_base, uint_t prot,
 *		uint_t flags)
 *	Allocate any hat resources needed for a new segment.
 *
 *	This routine is invoked by the seg_create() routines 
 *	in the segment drivers.
 *
 *	For vnode type of segments, we load up translations 
 *	to any pages of the segment that are already in core.
 *	But this is done only in NOSLEEP mode. Also, it does not
 *	try too hard for locks on pages. Memory exhaustion
 *	causes it to stop trying.
 *
 * Calling/Exit State:
 *	No spin locks that would interfere with kmem_alloc or kpg_alloc
 *	can be held by the caller. This routine can block in PREEMPT().
 *	Both the global page layer locks, the VM_PAGEIDWRLOCK() and 
 *	the VM_FREEPAGELOCK() are held in this function. The 
 *	VM_PAGEIDWRLOCK() is held to guard the vnode hash chain we are
 *	loading.  Also, we don't actually have to hold the
 *	VM_FREEPAGELOCK() here. We can let page_reclaim() acquire it.
 *	But we are holding it here to avoid lock roundtrips.  
 */
uint_t
hat_map(struct seg *seg, vnode_t *vp, off_t vp_base, uint_t prot,
		uint_t flags)
{
	vaddr_t	addr;
	page_t	*pp, *epp, *mpp;
	hatpt_t *ptap, *savptap;
	pte_t	*ptep, *vpdte;
	hat_t	*hatp;
	struct	as *as = seg->s_as;
	int	seg_size, mcndx, mcnum;
	off_t	vp_off;		/* offset of page in file */
	off_t	vp_end;		/* end of VM segment in file */
	long	addr_off;	/* loop invariant; see comments below. */
	hatmap_t *mapp;
	hatptcp_t *ptcp;
	hatpgt_t *pt;
	hatmc_t *mc;
	hatmcp_t *mcp;
	uint_t mode;
	boolean_t isnewptap;
#ifdef DEBUG
	vaddr_t  eaddr;
#endif

	if (vp == NULL || ((flags & HAT_PRELOAD) == 0) ||
			(as != u.u_procp->p_as))
		return(0);

	ASSERT(getpl() == PLBASE);

	/*
	 * Set up as many of the hardware address translations as we can.
	 * If the segment is a vnode-type segment and it has a non-NULL vnode, 
	 * we walk down the list of incore pages associated with the vnode.
	 * For each page on that list, if the page maps into the range managed
	 * by the segment we calculate the corresponding virtual address and
	 * set up the hardware translation tables to point to the page.
	 *
	 * Note: the file system that owns the vnode is not informed about the
	 * new references we have made to the page.
	 */

	/*
	 * Calculate the protections we need for the pages.
	 * (i.e. whether to set fault on write bit or not).
	 */
	if (!((mode = hat_vtop_prot(prot)) & PG_V))
		return(0);		/* don't bother */

	addr = seg->s_base;
	seg_size = seg->s_size;
#ifdef DEBUG
	eaddr = addr + seg_size - 1;
#endif
	ASSERT(((u_long)addr & POFFMASK) == 0);	/* page aligned */

	hatp = &(as->a_hat);

	/*
	 * Remove invariant from the loop. To compute the virtual
	 * address of the page, we use the following computation:
	 *
	 *	addr = seg->s_base + (vp_off - vp_base)
	 *
	 * Since seg->s_base and vp_base do not vary for this segment:
	 *
	 *	addr = vp_off + (addr_off = (seg->s_base - vp_base))
	 *
	 * Note: currently addr = seg->s_base;
	 */
	addr_off = addr - vp_base;
	vp_end = vp_base + seg_size;

	/*
	 * Walk down the list of pages associated with the vnode,
	 * setting up the translation tables if the page maps into
	 * addresses in this segment.
	 *
	 * This is not an easy task because the pages on the list
	 * can disappear as we traverse it. Holding the VM_PGIDLK protects
	 * against this, but sometimes the lock must be dropped.
	 * It is dropped only to allocate page tables or mapping chunks
	 * in NOSLEEP mode. To avoid too much dropping, an attempt
	 * is first made to allocate from the hat's local free caches
	 * of these resources (the page layer locks can be kept while doing
	 * that). To allow that, the hat free cache locks occur later in the
	 * lock hierarchy.
	 *
	 * hat_map() never tries too hard to get a page. Only TRYLOCKs
	 * are used. This eliminates some rather complex lock dropping
	 * code (and a place holder, dummy marker page) caused by the
	 * PAGE_USELOCK being earlier in the hierarchy than the other locks.
	 *
	 * Use a dummy page as a marker of our start point to terminate our
	 * search of the page list. Code that scans this list must skip
	 * strange pages. We need another marker page to insert in the
	 * middle of the list when we call PAGE_RECLAIM_L. If the page cannot
	 * cannot be reclaimed from the free list, the page's identity is
	 * destroyed by PAGE_RECLAIM_L. We move to the next page on the list
	 * by a macro (ADVANCE_FROM_MARKER_PAGE) that removes the marker page
	 * page from the list.
	 */

	/*
	 * First, prime the hat free pools.
	 */
	hat_refreshpools();
	if (vp->v_pages == NULL)
		return 0;

	CREATE_2_MARKER_PAGES(mpp, epp, hat_map);

	VM_PAGEIDWRLOCK();

	if ((pp = vp->v_pages) == NULL) {
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES(mpp, epp);
		return 0;
	}

	INSERT_END_MARKER_PAGE(epp, vp);

	HATRL_LOCK_SVPL(hatp);
	VM_PAGEFREELOCK();

	do {
	    /*
	     * If there are any pending interrupts, then insert the
             * marker page in front of the next page to process, and drop
	     * the locks we're holding
	     */
	    if (psm_intrpend(PLBASE)) {
		INSERT_MARKER_PAGE(pp->p_vpprev, mpp);
		VM_PAGEFREEUNLOCK();
		HATRL_UNLOCK_SVDPL(hatp);
		VM_PAGEIDUNLOCK();
		VM_PAGEIDWRLOCK();
		HATRL_LOCK_SVPL(hatp);
		VM_PAGEFREELOCK();
		ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);
		if (pp == epp)
		    break;
	    }

	    /*
	     * See if the vp offset is within the range mapped
	     * by the segment.  Also skip any marker pages.
	     *
	     * If page is being paged in, ignore it. (There may be 
	     * races if we try to use it.)
	     *
	     * NOTE: We do these checks here, before possibly taking
	     * the page off the freelist, to avoid unecessarily
	     * reclaiming the page. As a consequence, we must check
	     * the vnodes.
	     */
	    vp_off = pp->p_offset;
	    if (pp->p_vnode != vp || (vp_off < vp_base) || 
		(vp_off >= vp_end) || (pp->p_invalid)) {
		    pp = pp->p_vpnext;
		    continue;
	    }
	    if (TRYLOCK(&pp->p_uselock, VM_PAGE_IPL) == INVPL) {
		pp = pp->p_vpnext;
		continue;
	    }

	    l.puselockpl = l.vmpageidlockpl;  /* out of order */
	    l.vmpageidlockpl = VM_PAGE_IPL;

	    /*
	     * Although the earlier check of p_invalid was
	     * sufficient reason to skip the TRYLOCK, it must
	     * be checked again with the USELOCK held.
	     */
	    if (pp->p_invalid) {
		l.vmpageidlockpl = l.puselockpl;
		l.puselockpl = VM_PAGE_IPL;
		PAGE_USEUNLOCK(pp);
		pp = pp->p_vpnext;
		continue;
	    }

	    if (pp->p_free != 0 && 
			!page_reclaim_l(pp, WRITE_LOCKED, P_LOOKUP)) {
		/*
		 * The page was free and the attempt to allocate
		 * ran into freemem limits, so we must give up
		 * on it. We still continue since other pages may
		 * already be in use and we might have PTs and
		 * chunks.
		 */
		l.vmpageidlockpl = l.puselockpl;
		l.puselockpl = VM_PAGE_IPL;
		PAGE_USEUNLOCK(pp);
		pp = pp->p_vpnext;
		continue;
	    }
	    /*
	     * At this point, we have a page (not on any free list)
	     * with an effective reader lock (i.e., count not bumped,
	     * but status say it could be and that cannot change
	     * while the USELOCK is held.
	     */

	    ASSERT(pp->p_vnode == vp && pp->p_offset == vp_off);

	    addr = (vaddr_t) (addr_off + vp_off);
	    ASSERT((vp_off & POFFMASK) == 0);
	    ASSERT((addr & POFFMASK) == 0);
	    ASSERT(addr <= eaddr);

	    /*
	     * Compute the address of page directory entry for the 
	     * page. Search the hatpt list to find a page table
	     * that has a matching pde pointer. If it is 
	     * not found, it means the page table not present.
	     * Allocate and set up the page table. Then get 
	     * the page table entry for the page.
	     */
	    vpdte = kpd0 + ptnum(addr);

	    isnewptap = B_FALSE;

	    ptap = hat_findpt(hatp, vpdte);
	    /*
	     * Now ptap is either NULL (no entries in chatp yet),
	     * it is the desired ptap (cptap->hatpt_pdtep == vpdtep),
	     * or it is the prevptap for link_ptap() and the desired entry
	     * is absent.
	     */
	    if ((ptap == (hatpt_t *)NULL) || (ptap->hatpt_pdtep != vpdte)) {
		savptap = ptap;
		/* we have to allocate a page table */
		ptap = hat_ptalloc(HAT_POOLONLY);
		if (ptap == (hatpt_t *)NULL) {
			/*
			 * That is as hard as we are willing to try.
			 * Give up the whole show.
			 * Going on is likely to run into more of same.
			 */
			if (!PAGE_IN_USE(pp)) {
			    page_free_l(pp, 0);
			}

			l.vmpageidlockpl = l.puselockpl;
			l.puselockpl = VM_PAGE_IPL;
			PAGE_USEUNLOCK(pp);
			break;
		}
		isnewptap = B_TRUE;
	    }

	    ASSERT(ptap);
	    ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	    pt = (hatpgt_t *) ptap->hatpt_ptva;

	    /* Now set mapping chunk pointer. (mcnum, mcndx also)  */
	    SETMAPP(addr, mcnum, mcp, mcndx, ptap);
	    if ((mapp = (hatmap_t *)mcp->hat_mcp) == 0) {
		/* Need a new chunk */
		mc = hat_mcalloc(HAT_POOLONLY);
		if (mc != (hatmc_t *)NULL)
			mapp = hat_mcinit(mcp, pt->hat_pgtc + mcnum, mc);
		else {
		    /* 
		     * We have to drop the locks in this scenario and go back
		     * to ptsearch. So we just set new_ptap to the already
		     * allocated ptap.
		     */		
		    if (isnewptap)
			hat_ptfree(ptap);

		    if (!PAGE_IN_USE(pp)) {
			page_free_l(pp, 0);
		    }
		    l.vmpageidlockpl = l.puselockpl;
		    l.puselockpl = VM_PAGE_IPL;
		    PAGE_USEUNLOCK(pp);
		    break;
		}  /* else */
	    }  /* if mapp == 0 */

	    ASSERT(mapp != (hatmap_t *)NULL);
	    ASSERT(((ulong_t)mapp & ~HATMC_ADDR) == 0);

	    if (isnewptap) {
		/*
		 * Now the ptap can be added.
		 */
		ptap->hatpt_pdtep = vpdte; 
		link_ptap(as, savptap, ptap);
		/*
		 * If we are here, we are in current context.
		 * Load the l1 entry for the newly created
		 * page table.
		 */
		ASSERT(vpdte->pg_pte == 0);
		vpdte->pg_pte = ptap->hatpt_pde.pg_pte;
	    }
	    hatp->hat_ptlast = ptap;

	    mapp += mcndx;
	    ptcp = mapptoptcp(mapp);
	    ptep = pt->hat_pgtc[mcnum].hat_pte + mcndx;

	    ASSERT(ptep->pg_pte == 0); 
	    ASSERT(mapp->hat_mapv == 0);
	    ASSERT(pp->p_vnode == vp && pp->p_offset == vp_off);

	    ptep->pg_pte = (uint_t)mkpte(mode, page_pptonum(pp));
	    ptep->pgm.pg_chidx = pp->p_chidx;	/* for pteptopp() */

	    MET_PREATCH(1);

	    /*
	     * Finally, add this reference to the p_hat_mapping list.
	     * If page is on the freelist and we mapped it into
	     * the process address space, take it off the freelist.
	     * Then get the next page on the vnode page list.
	     */
	    mapp->hat_mnext = pp->p_hat_mapping;
	    pp->p_hat_mapping = mapp;
	    ASSERT(pteptopp(mapptoptep(mapp)) == pp);
#ifdef DEBUG
	    if (mapp->hat_mnext != 0) {
		if (IS_KVIS_MAP(mapp->hat_mnext))
		    ASSERT(pteptopp(KVIS_MAPTOPTE(mapp->hat_mnext)) == pp);
		else 
		    ASSERT(pteptopp(mapptoptep(mapp->hat_mnext)) == pp);
	    }
#endif
	    BUMP_RSS(as, 1);
	    BUMP_AEC(ptcp, ptap, 1);

	    if (flags & HAT_LOCK) {
		INCR_LOCK_COUNT(ptap, as);
		ptep->pg_pte |= PG_LOCK;
	    }

	    l.vmpageidlockpl = l.puselockpl;
	    l.puselockpl = VM_PAGE_IPL;	
	    PAGE_USEUNLOCK(pp);

	    pp = pp->p_vpnext;

	} while (pp != epp);

	REMOVE_MARKER_PAGE(epp, vp);

	VM_PAGEFREEUNLOCK();
	HATRL_UNLOCK_SVDPL(hatp);
	VM_PAGEIDUNLOCK();
	ASSERT(getpl() == PLBASE);
	DESTROY_2_MARKER_PAGES(mpp, epp);

	return 0;
}

/*
 * void hat_unlock(struct seg *seg, vaddr_t addr)
 *	Unlock translation at addr. 
 *	Translation might either not exist (page_abort) or must be locked.
 *	Undo lock bit and counts.
 *
 * Calling/Exit State:
 *	Will grab the hat_resourcelock and release it.
 *
 * Remarks:
 *      Should not be called by segments using PSE mappings.
 */
void
hat_unlock(struct seg *seg, vaddr_t addr)
{
	struct hat *hatp;
	struct as *as = seg->s_as;
	hatpt_t *ptap, *eptap;
	pte_t *vpdte, *ptep;

	hatp = &as->a_hat;
	/* addr is page aligned */
	ASSERT(((int)addr & POFFMASK) == 0);

	ASSERT(as != &kas);

	vpdte = kpd0 + ptnum(addr);
	/* find the hatpt struct */

	HATRL_LOCK_SVPL(hatp);
	ptap = hatp->hat_ptlast;
	if (ptap == (hatpt_t *)NULL) {
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));

	for (eptap = ptap; ptap->hatpt_pdtep != vpdte;) {
		ptap = ptap->hatpt_forw;
		if (ptap == eptap) {
			HATRL_UNLOCK_SVDPL(hatp);
			return;
		}
	}

	hatp->hat_ptlast = ptap;

	ptep = ptap->hatpt_ptva->hat_pgtc[0].hat_pte + pnum(addr);

	if (ptep->pg_pte == 0) {
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}
	ASSERT(ptep->pg_pte & PG_LOCK);

	/*
	 * Assume no TLB Shootdown needed since uniprocessor code
	 * said no TLBSflushtlb was needed.
	 */
	ptep->pg_pte &= ~PG_LOCK;
	DECR_LOCK_COUNT(ptap, as);
	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 *  ppid_t kvtoppid(caddr_t)
 *	Return the physical page ID corresponding to the virtual 
 *	address vaddr. This is a required interface defined by DKI.
 *	For the 80x86, we use the page frame number as the physical page ID.
 *
 * Calling/Exit State:
 *	Kernel page table lookups need no locks.
 *
 * Remarks:
 *      Works properly for PSE mappings.
 */
ppid_t
kvtoppid(caddr_t vaddr)
{
        pte_t *ptep;

        ptep = kvtol1ptep((vaddr_t)vaddr);
        /*
         * Handle most frequest case first: VALID but not PSE.
         */
        if ((ptep->pg_pte & (PG_V|PG_PSE)) == PG_V) {
                if (PG_ISVALID(ptep = kvtol2ptep((vaddr_t)vaddr)))
                        return (ppid_t)ptep->pgm.pg_pfn;
                else
                        return NOPAGE;
	      } else if (PG_ISVALID(ptep))
                return (ppid_t)ptep->pgm.pg_pfn + pnum(vaddr);
        else
                return NOPAGE;
}

/*
 * uint_t phystoppid(paddr_t)
 *	Return the physical page ID corresponding to the physical
 *	address paddr. This is a required interface defined by DKI.
 *	For the 80x86, we use the page frame number as the physical page ID.
 *
 * Calling/Exit State:
 *	Conversion calculation only.
 */
ppid_t
phystoppid(paddr_t paddr)
{
	return(mmu_btop(paddr));
}

/*
 * pte_t *hat_vtopte_l(struct hat *hatp, vaddr_t vaddr, hatpt_t **ptapp)
 *	Find the pte and ptap for a particular user virtual address.
 *
 * Calling/Exit State:
 *	The HAT resource lock is held on entry and still held on return.
 *
 *	On exit, the return value and (*ptapp) are set as follows:
 *
 *		If there is a valid, non-PSE PTE for the virtual
 *		address, the return value is a pointer to the pte,
 *		and *ptapp is set to the hatpt_t structure for the
 *		page table.
 *
 *		If there is a valid PSE PDE for the virtual address,
 *		the return value is NULL, and *ptapp is the hatpt_t
 *		structure for the pde.
 *
 *		If there is no valid PTE for the virtual address, then
 *		the return value and *ptapp are both NULL.
 *
 * Remarks:
 *	This is global only so that it can be accessed from the i386
 *	specific begin_user_write code, and also from kdb db_uvtop
 *	code.
 *
 *	For PSE mappings, returns NULL, but also sets *ptapp to the
 *	proper hatpt_t pointer.  Therefore, callers uninterested in
 *	PSE mappings can ignore the NULL return value; other callers
 *	can find PSE mappings by checking *ptapp in the face of a NULL
 *	return value.
 */
pte_t *
hat_vtopte_l(struct hat *hatp, vaddr_t vaddr, hatpt_t **ptapp)
{
	pte_t *ptep, *pdtep;
	hatpt_t *ptap, *eptap;
	hatpgt_t *pt;

	ASSERT(!KADDR(vaddr));

	/*
	 * The only way to find the virtual addresses of user page tables
	 * is to search the hatpt list.
	 */
	
	pdtep = kpd0 + ptnum(vaddr);
	ptap = eptap = hatp->hat_pts;
	if (ptap == (hatpt_t *)NULL) {
		ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
		*ptapp = NULL;
		return (pte_t *)NULL;
	} else {
		do {
			ASSERT(ptap && ptap->hatpt_aec);
			ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);
			if (pdtep > ptap->hatpt_pdtep) {
				ptap = ptap->hatpt_forw;
				continue;
			}
			else
				break;
		} while (ptap != eptap);

		if (pdtep == ptap->hatpt_pdtep) { 
			if (PG_ISPSE(&ptap->hatpt_pde)) {
				*ptapp = ptap;
				return (pte_t *)NULL;
			} else {
				pt = ptap->hatpt_ptva;
				ptep = pt->hat_pgtc[0].hat_pte + pnum(vaddr);
			}
		}
		else {
			*ptapp = NULL;
			return (pte_t *)NULL;
		}
	}
	if (ptep->pg_pte == 0) {
		*ptapp = NULL;
		return (pte_t *)NULL;
	}
	*ptapp = ptap;
	return ptep;
}

/*
 * uint_t hat_xlat_info(struct as *as, vaddr_t addr)
 *	Returns whether or not a translation is loaded at the given address,
 *	which must be a user address in the user address space, as.
 *
 * Calling/Exit State:
 *	The return value is stale unless the caller stabilizes the
 *	access to this address.
 */
uint_t
hat_xlat_info(struct as *as, vaddr_t addr)
{
	hat_t *hatp;
	hatpt_t *dummy;
	uint_t flags = 0;
	pte_t *ptep;
	/* xlat_flags indexed by PTE bits PG_US|PG_RW|PG_V */
	static int xlat_flags[] = {
		/* !U !W !V */	HAT_XLAT_EXISTS,
		/* !U !W  V */	HAT_XLAT_EXISTS,
		/* !U  W !V */	HAT_XLAT_EXISTS,
		/* !U  W  V */	HAT_XLAT_EXISTS,
		/*  U !W !V */	HAT_XLAT_EXISTS,
		/*  U !W  V */	HAT_XLAT_EXISTS | HAT_XLAT_READ,
		/*  U  W !V */	HAT_XLAT_EXISTS,
		/*  U  W  V */	HAT_XLAT_EXISTS | HAT_XLAT_READ | HAT_XLAT_WRITE
	};

	ASSERT(!KADDR(addr));
	ASSERT(as != (struct as *)NULL);

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	ptep = hat_vtopte_l(hatp, addr, &dummy);
	if (ptep != (pte_t *)NULL)
		flags = xlat_flags[ptep->pg_pte & (PG_US|PG_RW|PG_V)];
	HATRL_UNLOCK_SVDPL(hatp);

	return flags;
}

/*
 * ppid_t hat_vtoppid(struct as *as, vaddr_t vaddr)
 *	Routine to translate virtual addresses to physical page IDs.
 *
 * Calling/Exit State:
 *	If an invalid user address is received, hat_vtoppid returns NOPAGE.
 *	It is illegal to pass an invalid (unmapped) kernel address, and if
 *	done, the system may panic.
 *	hat_vtoppid will grab hat_resourcelock and release it if user address.
 *
 *	The return value is stale unless the caller stabilizes the conditions;
 *	this is typically done by having the page SOFTLOCKed or by virtue of
 *	using the kernel address space.
 *
 * Remarks:
 *	Handles PSE translations.
 */
ppid_t
hat_vtoppid(struct as *as, vaddr_t vaddr)
{
	struct hat *hatp;
	pte_t *ptep;
	ppid_t retval;
	hatpt_t *ptap;
	pl_t savpl;

	if (KADDR(vaddr))
		return (ppid_t)mmu_btop(kvtophys(vaddr));

	/*
	 * It is a user address and the only way to find the virtual
	 * addresses of user page tables is to search the hatpt list.
	 */

	ASSERT(as != (struct as *)NULL);
	hatp = &as->a_hat;
	/*
	 * The following ugliness due to  the fact that wd driver
	 * calls this function and the wd lock is at PLHI whereas
	 * the HAT lock could be lower than that.
	 */
#ifdef _LOCKTEST	/* KLUDGE for now */
	do {
		savpl = TRYLOCK(&hatp->hat_resourcelock, PLHI);
	} while(savpl == (pl_t)INVPL);
	hatp->hat_lockpl = savpl;
#else
	savpl = LOCK(&hatp->hat_resourcelock, PLHI);
#endif
	ptep = hat_vtopte_l(hatp, vaddr, &ptap);
	if (ptep != NULL)
		retval = (ppid_t)ptep->pgm.pg_pfn;
	else if ((ptap != NULL) && PG_ISPSE(&ptap->hatpt_pde))
		retval = ptap->hatpt_pde.pgm.pg_pfn + pnum(vaddr);
	else
		retval = NOPAGE;
	UNLOCK(&hatp->hat_resourcelock, savpl);
	return retval;
}

#define EBADDR	0

/*
 * paddr_t vtop(caddr_t vaddr, void *procp)
 *	Routine to translate virtual addresses to physical addresses
 *	Typically used by drivers that need physical addresses.
 *
 * Calling/Exit State:
 *	Returns the physical address corresponding to the given virtual
 *	address within the process, procp; if procp is NULL, vaddr must be
 *	a kernel address.
 *
 *	As an optimization, if a PSE mapping exists in the address space,
 *	it takes advantage of it.
 *
 *	If an invalid user address is received, vtop returns 0.
 *	It is illegal to pass an invalid (unmapped) kernel address, and if
 *	done, the system may panic.
 *
 *	The return value is stale unless the caller stabilizes the conditions;
 *	this is typically done by having the page SOFTLOCKed or by virtue of
 *	using the kernel address space.
 */
paddr_t
vtop(caddr_t vaddr, void *procp)
{
	ppid_t ppid;
	struct as *as;
	pte_t *pseck;

	if (procp == NULL || procp == uprocp) {
		pseck = kpd0 + ptnum(vaddr);
		if (PG_ISPSE(pseck))
			return((pseck->pg_pte & MMU_PAGEMASK)|PSE_PAGOFF(vaddr));
	}
	as = (procp == (proc_t *)NULL ? (struct as *)NULL
						 : ((proc_t *)procp)->p_as);

	if ((ppid = hat_vtoppid(as, (vaddr_t)vaddr)) == NOPAGE)
		return (paddr_t)EBADDR;
	return (paddr_t)mmu_ptob(ppid) + PAGOFF((vaddr_t)vaddr);
}

/*
 * page_t *hat_vtopp(struct as *as, vaddr_t vaddr, enum seg_rw rw)
 *	Routine to find the page mapped at a particular user virtual address.
 *
 * Calling/Exit State:
 *	If there is a valid visible mapping from the given virtual address
 *	to a page, the translation allows the specified access (rw), and
 *	that page can be read-locked without spinning or blocking, that page
 *	is returned read-locked.
 *	Otherwise, NULL is returned.
 */
page_t *
hat_vtopp(struct as *as, vaddr_t vaddr, enum seg_rw rw)
{
	struct hat *hatp;
	pte_t *ptep;
	page_t *pp;
	hatpt_t *ptap;

	ASSERT(as != (struct as *)NULL);
	ASSERT(!KADDR(vaddr));

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	ptep = hat_vtopte_l(hatp, vaddr, &ptap);
	if (ptep == (pte_t *)NULL || 
	    (rw == S_WRITE && !(ptep->pg_pte & PG_RW)) ||
	    (mcptomapp(ptap->hatpt_mcp + HATMCNO(vaddr)) +
			HATMCNDX(vaddr))->hat_mapv == MAPV_HIDDEN ||
	    (pp = pteptopp(ptep)) == (page_t *)NULL)
		goto nopage;

	if (TRYLOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL) == INVPL)
		goto nopage;
	if (PAGE_IS_WRLOCKED(pp))
		goto nopage_useunlock;

	ASSERT(PAGE_IN_USE(pp));	/* since we found a mapping */

	pp->p_activecnt++;	/* Acquire a read lock */
	UNLOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL);
	HATRL_UNLOCK_SVDPL(hatp);
	return pp;

nopage_useunlock:
	UNLOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL);
nopage:
	HATRL_UNLOCK_SVDPL(hatp);
	return (page_t *)NULL;
}

/*
 * int hat_exec(struct as *oas, vaddr_t ostka, ulong_t stksz, struct as *nas,
 *		vaddr_t nstka, uint_t hatflag)
 *	Move page tables and hat structures for the new stack image
 *	from the old address space to the new address space.
 *
 * Calling/Exit State:
 *	Single engine environment during exec.
 *	Moves page tables but does not flush (one is coming soon)
 *	unless no more page tables are left (very unlikely).
 */
int
hat_exec(struct as *oas, vaddr_t ostka, ulong_t stksz, struct as *nas,
		vaddr_t nstka, uint_t hatflag)
{
	ASSERT(PAGOFF(stksz) == 0);
	ASSERT(PAGOFF(ostka) == 0);
	ASSERT(PAGOFF(nstka) == 0);
	ASSERT(nas->a_segs->s_next == nas->a_segs);
	ASSERT(nas->a_hat.hat_pts == (hatpt_t *)NULL);
	ASSERT(oas == u.u_procp->p_as);

	if (hatflag) {
		/* Move the page tables themselves as the flag
		 * states that they contain only pages to be moved
		 * and the pages are properly aligned in the table.
		 */
		hatpt_t *ptap, *nptap;
		pte_t *ovpdte, *nvpdte, *endvpdte;
		struct hat *hatp, *ohatp;
		pl_t trypl;
		int delta;

		ovpdte = kpd0 + ptnum(ostka);
		nvpdte = kpd0 + ptnum(nstka);
		endvpdte = kpd0 + ptnum(ostka + stksz - 1);
		delta = nvpdte - ovpdte;

		ohatp = &oas->a_hat;
		HATRL_LOCK_SVPL(ohatp);
		ptap = ohatp->hat_pts;
		/* 
		 * The following condition can occur in rare circumstances:
		 * We can get swapped out in exec() during as_alloc() or
		 * extractargs(). Then when we get swapped back in
		 * none of the page tables for oas will be loaded. In this
		 * case we just quit and let the nas fault in the pages.
		 */
		if (ptap == (hatpt_t *)NULL) {
			HATRL_UNLOCK_SVDPL(ohatp);
			return(0); 
		}
		while (ptap->hatpt_pdtep < ovpdte) {
			if ((ptap = ptap->hatpt_forw) == ohatp->hat_pts) {
				HATRL_UNLOCK_SVDPL(ohatp);
				return(0);
			}
		}
		hatp = &nas->a_hat;
		trypl = HATRL_TRYLOCK(hatp);

		ASSERT(trypl != (pl_t)INVPL);
		ASSERT(ptap != (hatpt_t *)NULL);
		while (ptap->hatpt_pdtep <= endvpdte) {
			ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
			nptap = ptap->hatpt_forw;
			ASSERT(ptap->hatpt_pdtep >= ovpdte);
			ovpdte = ptap->hatpt_pdtep;
			ovpdte->pg_pte = 0;
			unlink_ptap(oas, ptap);
			ptap->hatpt_pdtep += delta;
			link_ptap(nas, hatp->hat_ptlast, ptap);
			oas->a_rss -= ptap->hatpt_aec - ptap->hatpt_hec;
			nas->a_rss += ptap->hatpt_aec - ptap->hatpt_hec;
			ASSERT(ptap->hatpt_locks == 0);
			if (nptap == ohatp->hat_pts || ohatp->hat_pts == (hatpt_t *)NULL)
				break;
			ptap = nptap;
		}
		HATRL_UNLOCK(hatp, trypl);
		/*
		 * If there are any page tables left,
		 * the unload will flush the TLB.
		 * So, cover the unlikely case (swapping might cause it)
		 * that moving the new stack took the last PT.
		 */
		if (ohatp->hat_pts == (hatpt_t *)NULL)
			TLBSflushtlb();
		HATRL_UNLOCK_SVDPL(ohatp);
		return(0);
	}

	/* In the case of non-aligned PTEs, the PTEs would have to be
	 * copied to new page table(s).	 Since this is an extremely
	 * complex operation, for a case which is so rare that it will
	 * probably never occur, we just unload (and swap out) the old
	 * mapping and let references via the new address space fault
	 * the pages back in.
	 */

#ifdef DEBUG
	cmn_err(CE_NOTE, "hat_exec: couldn't do special case - unload instead");
#endif

	{ struct seg	fake_seg;
		fake_seg = *nas->a_segs;
		fake_seg.s_as = oas;
		fake_seg.s_base = ostka;
		fake_seg.s_size = stksz;
		hat_unload(&fake_seg, ostka, stksz, HAT_UNLOCK);
	}
	return(0);
}

/*
 * void hat_asload(struct as *as)
 *	Add engine to active engine accounting in hat structure.
 *	Load the current process's address space into the
 *	mmu (page directory).  Called by as_exec() after setting up the stack.
 *	Called during context switch to load new as.
 *
 * Calling/Exit State:
 *	Must grab and release hat_resourcelock.
 *
 * Remarks:
 *	PSE mappings are represented in the linked list of hatpt
 *	structures in the same way that page tables are.  This
 *	allows hat_asload to load PSE mappings along with other
 *	page directory entries without any special PSE code
 *	required or any performance penalty.
 */
void
hat_asload(struct as *as)
{
	hatpt_t	*ptap, *eptap;
	struct hat	*hatp;

	ASSERT(as != (struct as *)NULL);
	ASSERT(as == u.u_procp->p_as);
	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	hatp->hat_activecpucnt++;
	EMASK_SETS(&hatp->hat_activecpubits, &l.eng_mask);
	ptap = eptap = hatp->hat_pts;
	if (ptap != (hatpt_t *)NULL) {
		do {
			ptap->hatpt_pdtep->pg_pte = ptap->hatpt_pde.pg_pte;
			ptap = ptap->hatpt_forw;
		} while (ptap != eptap);
	}
	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void hat_asunload(struct as *as, boolean_t doflush)
 *	Unload level 1 entries of as and take engine out of active engine
 *	accounting of hat structure.
 *	This is called on context switch when switching LWPs and from relvm().
 *	It does a TLB flush if the doflush is B_TRUE.
 *
 * Calling/Exit State:
 *	Must grab and release hat_resource lock.
 * Remarks:
 *      PSE mappings appear in the linked list of hatpt structures,
 *      thus this routine transparently unloads such mappings.
 */
void
hat_asunload(struct as *as, boolean_t doflush)
{
	struct hat   *hatp;
	hatpt_t	      *ptap, *eptap;
	pl_t	      savpl;

	ASSERT(as != (struct as *)NULL);
	ASSERT(as == u.u_procp->p_as);
	hatp = &as->a_hat;
	/*
	 * The following ungliness is because the context switching code
	 * calls us with l mutex held and this could be at a higher
	 * hierarchy than the hat lock.
	 */
#ifdef _LOCKTEST	/* KLUDGE for now */
	do {
		savpl = TRYLOCK(&hatp->hat_resourcelock, PLHI);
	} while(savpl == (pl_t)INVPL);
#else
	savpl = LOCK(&hatp->hat_resourcelock, PLHI);
#endif
	ptap = eptap = hatp->hat_pts;
	if (ptap != (hatpt_t *)NULL) {
		do {
			ptap->hatpt_pdtep->pg_pte = (uint)0;
			ptap = ptap->hatpt_forw;
		} while (ptap != eptap);
	}
	if (doflush)
		TLBSflushtlb();

	/*
	 * Remove the CPU from the hat accounting.
	 */
	hatp->hat_activecpucnt--;
	ASSERT(hatp->hat_activecpucnt >= 0);
	ASSERT(EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask));
	EMASK_CLRS(&hatp->hat_activecpubits, &l.eng_mask);

	UNLOCK(&hatp->hat_resourcelock, savpl);
}

/*
 * boolean_t hat_kas_agerange(vaddr_t addr, vaddr_t endaddr)
 * 	Function to age address range in kernel visible
 *	mapping space.
 *
 * Description:
 *	This function goes through this address range and unloads the entries
 *	for whom the reference bit in the PTE is cleared  and clears the
 *	reference bit for the entries whose reference bit is set.
 *
 * Calling/Exit State:
 *	No locks need be held on entry and no locks are held on exit.
 *	The caller guarantees that the specified range is not being
 *	accessed for the duration of this routine. The page uselock
 *	is acquired and dropped within the function in order to traverse
 *	the mapping chain of the page.
 *
 *	This function does not protect against races with hat_pageunload,
 *	and therefore is only appropriate for anon pages.
 *
 *	Return value indicates if TLB will be required before the memory
 *	may be accessed.
 * Remarks:
 *      Does not handle PSE mappings.
 */
boolean_t
hat_kas_agerange(vaddr_t addr, vaddr_t endaddr)
{
	pte_t *ptep;
	hatmap_t *mapp;
	page_t *pp;
	boolean_t doflush = B_FALSE;

	ASSERT((addr & POFFMASK) == 0);

	ptep = kvtol2ptep(addr);
	mapp = KVIS_PTETOMAP(ptep);

	for (; addr < endaddr; addr += PAGESIZE, ptep++, mapp++) {
	    if (ptep->pg_pte == 0) {
		ASSERT(mapp->hat_mapv == 0);
		continue;
	    }
	    ASSERT(!(ptep->pg_pte & PG_LOCK));
	    doflush = B_TRUE;
	    if (!(ptep->pg_pte & PG_REF)) {
	        pp = pteptopp(ptep);
		ASSERT(pp != NULL);
		PAGE_USELOCK(pp);
		ASSERT(pteptopp(ptep) == pp);
		if (ptep->pgm.pg_mod)
			pp->p_mod = 1;

		/*
		 * Must remove pte from its p_hat_mapping list
		 */
		(void) hat_remptel(mapp, pp, HAT_NOFLAGS);
		ptep->pg_pte = 0;
	    }
	    else /* PG_REF bit set */
		ptep->pg_pte &= ~PG_REF;
	}

	return (doflush);
}

/*
 * boolean_t local_age_lock(proc_t *procp)
 *	
 *	Obtain the necessary serialization for in-context aging.
 *	For MMUs that can support transparent maintenance of modify
 *	bits [see broader description in vm_hat.h], multiple LWPs 
 *	within the same address space can be actively referencing 
 *	the address space even as one of them ages the address space. 
 *	In this case, it suffices for the aging LWP to hold the address
 *	space read-locked, since this will guarantee that the layout of
 *	the address space does not change during aging.
 *
 * Calling/Exit State:
 *	Called with process p_mutex held, which is dropped before return.
 * 	The function can block. If the current context loses the race to
 *	age the address space during the time that it blocks, then it
 *	will return B_FALSE to its caller. Otherwise, B_TRUE will be
 *	returned, and the necessary serialization for aging the address
 *	space would be achieved. 
 *	It is assumed that the caller will set the P_LOCAL_AGE flag, 
 *	under the same cover of p_mutex just before calling this function. 
 *	If B_TRUE is returned, the flag will remain set. 
 *
 * Remarks:
 *	The only serialization necessary is the obtainment of the address
 *	space read lock. First, an attempt is made to acquire the lock
 *	without blocking. If this fails, then the current context must
 *	block. During the time that it blocks, the LWP must declare the
 *	process available for aging/swapping by other contexts, in order
 *	to prevent resource deadlocks.
 *
 *	This is the only hat layer routine that contests the AS sleep lock.
 *	For this reason, it may be appropriate to locate it to some another 
 *	family specific file.
 */
boolean_t
local_age_lock(proc_t *procp)
{
	struct as *asp = procp->p_as;
	
	ASSERT(KS_HOLD1LOCK());
	ASSERT((procp->p_flag & P_LOCAL_AGE) != 0);

	if (as_tryrdlock(asp)) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return(B_TRUE);
	}
	/*
	 * Must block for the AS read lock. Expose process to selection 
	 * by swapper in the interim.
	 */
	procp->p_flag &= ~P_LOCAL_AGE;
	UNLOCK(&procp->p_mutex, PLBASE);
	as_rdlock(asp);
	
	(void) LOCK(&procp->p_mutex, PLHI);
	/*
	 * Did someone beat us to the punch ?
 	 */
	if (CAN_SKIP_AGING(procp) || !AGING_EVENT_POSTED()) {
		UNLOCK(&procp->p_mutex, PLBASE);
		as_unlock(asp);
		return(B_FALSE);
	}

	procp->p_flag |= P_LOCAL_AGE;
	UNLOCK(&procp->p_mutex, PLBASE);
	return(B_TRUE);
}

/*
 * enum hat_next_type
 * hat_unload_range(struct as *as, hatpt_t **nextptap, hatpt_t **eptap,
 *		    vaddr_t *addr, u_int num_ptes, uint_t *physfreed_pgs,
 *		    u_int *freedvirt_pgs, enum age_type howhard)
 *	
 *  Unload num_ptes number of page table entries stored in the 
 *  l.ptesbuf structure.
 *
 * Calling/Exit State:
 *	The hat resource lock for the as is held on entry and is still
 *	held on exit.  Only those ptes whose page uselock can be trylocked
 *	is unloaded. Otherwise the pte is reinstantiated with its
 *	original value from the l.ptesbuf structure. Returns an indication
 *	to the caller whether to move on to the next mapping chunk (NEXT_MC),
 *	next page table (NEXT_PT) or continue on with the next address.
 *	NEXT_MC and NEXT_PT is possible if the unloading of the current
 *	pte causes mapping chunk active entry count to go to zero or
 *	the page table active entry count to go to zero. In both case
 *	outarg addr will be adjusted. In the NEXT_PT case, outarg
 *	nextptap will also be set to point to the next page table.
 *	Two other outargs physfreed_pgs and freedvirt_pages are perf.
 *	counters incremented in this function.
 *
 * Remarks:
 *	This routine is called by hat_agerange() only in the case of
 *	AS_AGE and AS_TRIM. Thus it is tolerable to skip unloading
 *	some translations since we will get them the next time around.
 *	We skip unloading translations whose pages we cannot trylock.
 */
STATIC enum hat_next_type
hat_unload_range(struct as *as, hatpt_t **nextptap, hatpt_t **eptap,
		 vaddr_t *addr, u_int num_ptes, u_int *physfreed_pgs,
		 u_int *freedvirt_pgs, enum age_type howhard)
{
    pte_array_t *ptesbufp;
    hat_t *hatp = &as->a_hat;
    page_t *pp;
    pte_t *ptep;
    hatmap_t *mapp, *nmapp;
    hatptcp_t *ptcp;
    hatpt_t *ptap, *nptap;
    vaddr_t savaddr;
    boolean_t last_one = B_FALSE;
    u_int savnum_ptes = num_ptes;
    ushort_t refcnt;
    int mcndx, indx;
    hat_stats_t *modstatsp;

    ASSERT(howhard != AS_SWAP);

    for (ptesbufp = l.ptesbuf; num_ptes; num_ptes--, ptesbufp++) {
	ASSERT(as->a_rss >= num_ptes);
	mapp = (hatmap_t *)ptesbufp->pte_mapp;
	ptep = mapptoptep(mapp);
	ASSERT(mapp && ptep);
	ASSERT((ptep->pg_pte & PG_LOCK) == 0);
	ASSERT(mapp->hat_mapv != MAPV_HIDDEN);

	pp = pteptopp(ptep);
	ASSERT(pp != NULL);
	if (TRYLOCK(&pp->p_uselock, VM_HAT_RESOURCE_IPL) == INVPL) {
	    /* 
	     * Give up since we can't trylock. Need to reload the pte.
	     */
	    ASSERT(ptesbufp->pteval);
	    ptep->pg_pte = ptesbufp->pteval;
	    continue;
	}
	l.puselockpl = VM_HAT_RESOURCE_IPL;

	ASSERT(pp->p_hat_mapping);

	/* 
	 * Do we need to sync? The only reason we need to sync is when
	 * the refcnt is non zero, this is an aging step and the agecnt
	 * has exceeded the threshold. If the refcnt is non-zero and
	 * the agecnt is less than the threshold value, hat_agerange()
	 * will not call this function. Because of race conditions,
	 * however this can happen and thus is not assertable.
	 */
	if (pp->p_hat_refcnt > 0 && (pp->p_hat_agecnt >= HAT_SYNC_THRESH) &&
	    (howhard != AS_TRIM)) {
		nmapp = pp->p_hat_mapping;
		refcnt = 0;
		while (nmapp != NULL) {
			mcndx = HATMAPMCNDX(nmapp);
			if (BITMASK1_TEST1(mapptowasrefp(nmapp), mcndx))
				refcnt++;
			nmapp = nmapp->hat_mnext;
		}
		pp->p_hat_refcnt = refcnt;
		pp->p_hat_agecnt = 0;
		/*
		 * After syncing we find that the refcnt is non-zero.
		 * Reinstall the page table entry and continue.
		 */
		if (refcnt != 0) {
		    ASSERT(ptesbufp->pteval);
		    ptep->pg_pte = ptesbufp->pteval;
		    PAGE_USEUNLOCK(pp);
		    continue;
		}
	}

	ptap = pteptoptap(ptep);

	if (ptep->pgm.pg_mod) {
	    PAGE_SETMOD(pp);
	    if (hatp->hat_modstatsp != NULL) {
		modstatsp = hat_findstats(hatp, ptap->hatpt_pdtep);
		if (modstatsp != NULL && 
			modstatsp->stats_pdtep == ptap->hatpt_pdtep) {

		    indx = HATMAPMCNO(mapp) * HAT_EPMC + HATMAPMCNDX(mapp);
		    BITMASKN_SET1(modstatsp->stats_modinfo, indx);
		}
	    }
	}

	if (hat_remptel(mapp, pp, HAT_NOFLAGS))
	    ++*physfreed_pgs;

	BUMP_RSS(as, -1);

	++*freedvirt_pgs;

	ptcp = mapptoptcp(mapp);
	ptep->pg_pte = 0;

	BUMP_AEC(ptcp, ptap, -1);

	if (ptcp->ptp.hat_mcaec == 0) {
	    if ((savnum_ptes == MAXL2_PTES) && (num_ptes == 1))
		last_one = B_TRUE;

	    ptap->hatpt_mcp[HATMAPMCNO(ptcp->hat_ptpv)].hat_mcp = 0;
	    hat_mcfree(mapp);
	    if (ptap->hatpt_aec == 0) {
		ASSERT(ptap->hatpt_hec == 0);
		ASSERT((savnum_ptes != MAXL2_PTES) || 
				(ptap->hatpt_forw != ptap) || last_one);
		nptap = ptap->hatpt_forw;
		if (*eptap == ptap)
			*eptap = nptap;
		FREE_PT(ptap, nptap, as, savaddr)
		if (last_one) {
		     ASSERT(*nextptap == ptap);	
		     ASSERT(last_one);
		    *addr = savaddr;
		    *nextptap = nptap;  
		    return NEXT_PT;
		}
		ASSERT(as->a_rss >= num_ptes - 1);
	    }
	    if (last_one) {
		*addr = (vaddr_t)(((uint)*addr + HATMCSIZE) & HATVMC_ADDR);
		return NEXT_MC;
	    }
	}  else /* if mcaec == 0 */
	  ASSERT(ptap->hatpt_aec);
    }  /* for loop */
    return NEXT_ADDR;
}

/*
 * vaddr_t hat_agerange(struct as *as, vaddr_t addr, vaddr_t endaddr,
 *			enum age_type howhard)
 *	Age the range [addr, endaddr] in specified as.
 *	Never touch locked translations (page tables containing only
 *	locked translations can be skipped, not accounting at MC level).
 *	If howhard is AS_AGE, unreferenced (and unlocked) translations
 *	are removed and the page is freed, if appropriate. 
 *	Referenced translations are made unreferenced.
 *	If howhard is AS_SWAP, all unlocked translations are removed.
 *	If howhard is AS_TRIM, MAXTRIM unlocked translations starting
 *	from addr are removed. The addr returned is the last removed
 *	addr + PAGESIZE.
 *
 * Calling/Exit State:
 *	This routine should not be called for hidden mappings.
 *
 *	The return addr is valid only in the case of AS_TRIM and it would be
 *	the next unloaded pte. The return would be endaddr if we have no more
 *	translations loaded for the as and trimcnt < MAXTRIM.
 *
 *	This function does not block.
 *
 * Remarks:
 *	Refer to the design doc. for the design and implementation of the
 *	shootdown algorithm used in this function.
 *	In case of AS_AGE and AS_TRIM, some translations that fit the
 *	criteria to get thrown out would be skipped if the trylock of the
 *	page the tranlation is pointing to is unsuccessful. This is 
 *	tolerable since will get them the next time around.
 *
 *	Segments mapped via PSE mappings should not be calling
 *	this routine; such segments call pse_hat_agerange instead.
 */
vaddr_t
hat_agerange(struct as *as, vaddr_t addr, vaddr_t endaddr,
	     enum age_type howhard)
{
    pte_t *evpdte, *ptep;
    hatpt_t *ptap, *savptap = (hatpt_t *)NULL, *eptap;
    hat_t *hatp;
    int doflush = 0, mcndx, mcnum, doshoot = 1;
    hatmap_t *mapp;
    u_int freedphys_pgs = 0, freedvirt_pgs = 0, scanned_pgs = 0;
    hatptcp_t *ptcp;
    hatmcp_t *mcp;
    pl_t origpl;
    pte_array_t  *ptesbufp = l.ptesbuf;
    int num_ptes = 0;
    enum hat_next_type ret;
    page_t *pp;
    uint_t *wasrefp;
    hat_stats_t *modstatsp;

    ASSERT(endaddr <= (vaddr_t)UVEND);
    ASSERT(as != &kas);
    hatp = &as->a_hat;
    ASSERT(endaddr >= addr);

    evpdte = kpd0 + ptnum(endaddr-1);

    origpl = HATRL_LOCK(hatp);

    ASSERT(num_ptes == 0);
    ASSERT (hatp->hat_activecpucnt == 0 || howhard != AS_SWAP);

    ptap = hat_findfirstpt(&addr, hatp); 

    if (ptap == (hatpt_t *)NULL) {
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL ||
		hatp->hat_pts->hatpt_back->hatpt_pdtep < kpd0 + ptnum(addr));
	HATRL_UNLOCK(hatp, origpl);
	return endaddr;
    }

    ASSERT(ptap->hatpt_aec);
    ASSERT(ptap->hatpt_pde.pg_pte && ptap->hatpt_pdtep);

    if (ptap->hatpt_pdtep > evpdte) {
	HATRL_UNLOCK(hatp, origpl);
	return endaddr;
    }

    if (addr >= endaddr)
	goto done;

    eptap = hatp->hat_pts;

    do {	/* ptap loop */
	if (addr >= endaddr)
	    goto done;

	if (savptap) {
	    eptap = savptap;
	    savptap = (hatpt_t *)NULL;
	}

	ASSERT(ptap->hatpt_pdtep >= kpd0);

	ASSERT(ptap->hatpt_aec != 0);
	ASSERT(ptap->hatpt_aec >= ptap->hatpt_locks);
	if (ptap->hatpt_aec == ptap->hatpt_locks) {
	    ptap = ptap->hatpt_forw;
	    continue;
	}
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
	/* Now setup the mapping chunk pointers (mcnum, mcndx also) */
	SETMAPP(addr, mcnum, mcp, mcndx, ptap);
	if (howhard == AS_SWAP) {
	    for (; mcnum < HAT_MCPP; mcp++, mcnum++, mcndx = 0) {
		if (mcp->hat_mcp == 0) {
		    addr = (vaddr_t)(((uint)addr + HATMCSIZE) & HATVMC_ADDR);
		    if (addr >= endaddr)
			goto done;
		    continue;
		}
		SETPTRS(mcp, mcndx, mapp, ptcp, ptep, ptap);

		for (; mcndx < HAT_EPMC; addr += PAGESIZE, mapp++, ptep++, mcndx++) {
		    if (addr >= endaddr) 
			goto done;
		    if (ptep->pg_pte == 0) {
			ASSERT(mapp->hat_mapv == 0);
			continue;
		    }
		    if (ptep->pg_pte & PG_LOCK)
			continue;

		    freedvirt_pgs++;

		    /*
		     * since howhard == AS_SWAP, we free the pte with
		     * 		flags == HAT_DONTNEED, below.
		     */
		    wasrefp = mapptowasrefp(mapp);
		    if (BITMASK1_TEST1(wasrefp, mcndx)) {
			BITMASK1_CLR1(wasrefp, mcndx);
			pp = pteptopp(ptep);
			ASSERT(pp);
			pp->p_hat_refcnt--;
		    }
		    if (ptep->pgm.pg_mod) {
			if (hatp->hat_modstatsp != NULL) {
			    modstatsp = hat_findstats(hatp, ptap->hatpt_pdtep);
			    if (modstatsp != NULL && 
				modstatsp->stats_pdtep == ptap->hatpt_pdtep)
				BITMASKN_SET1(modstatsp->stats_modinfo, mcndx);
			}
		    }

		    if (hat_unloadpte(ptep, ptap, ptcp, mapp,
			hatp, 0, origpl, HAT_DONTNEED, B_FALSE))
			freedphys_pgs++;		 
		    if (ptcp->ptp.hat_mcaec == 0) {
			/* also sets addr */	
			FREE_MC(ptap, ptcp, mcp, mapp, addr);
			if (ptap->hatpt_aec == 0) {
			    ASSERT(ptap->hatpt_hec == 0);
			    savptap = ptap->hatpt_forw;
			    /*
			     * We need to reset eptap here. This is because the
			     * hat lock can be dropped in unloadpte operation.
			     * What could happen is that page tables may have
			     * been aged or swapped in the mean time and we 
			     * could have eptap pointing to bogus pointer. Thus
			     * the loop would never end.
			     */
			    eptap = hatp->hat_pts;

			    /* also sets addr */
			    FREE_PT(ptap, savptap, as, addr);
			    ptap = savptap;
			    goto endptloop;
			}
			if (addr >= endaddr)
			    goto done;

			break;
		    }  /* if mcaec == 0 */
		}  /* for mcndx loop */
	    } /* for mcnum loop */
	    /*
	     * We need to reset eptap here. This is because the
	     * hat lock can be dropped in unloadpte operation.
	     * What could happen is that page tables may have
	     * been aged or swapped in the mean time and we 
	     * could have eptap pointing to bogus pointer. Thus
	     * the loop would never end.
	     */
	    eptap = hatp->hat_pts;
	} else {   /* if (howhard == AS_SWAP)? */
	    for (; mcnum < HAT_MCPP; mcp++, mcnum++, mcndx = 0) {
		if (mcp->hat_mcp == 0) {
		    addr = (vaddr_t)(((uint)addr + HATMCSIZE) & HATVMC_ADDR);
		    if (addr >= endaddr)
			goto done;
		    continue;
		}
		SETPTRS(mcp, mcndx, mapp, ptcp, ptep, ptap);
		for (; mcndx < HAT_EPMC; addr += PAGESIZE, mapp++, ptep++, mcndx++) {
		    ASSERT (howhard != AS_SWAP);
		    if (addr >= endaddr) 
			goto done;
		    if (ptep->pg_pte == 0) {
			ASSERT(mapp->hat_mapv == 0);
			continue;
		    }
		    if (ptep->pg_pte & PG_LOCK)
			continue;

		    ASSERT(mapp->hat_mapv != MAPV_HIDDEN);

		    pp = pteptopp(ptep);
		    ASSERT(pp);
		    pp->p_hat_agecnt++;

		    wasrefp = mapptowasrefp(mapp);
		    ASSERT(wasrefp);

		    if (howhard != AS_AGE || !(ptep->pg_pte & PG_REF)) {
			/* MAXTRIM ptes removed for AS_TRIM ? */
			if (howhard == AS_TRIM && 
			    num_ptes + freedvirt_pgs == MAXTRIM)
			    goto done;

			scanned_pgs++;	/* SAR counter */

			if (BITMASK1_TEST1(wasrefp, mcndx)) {
				BITMASK1_CLR1(wasrefp, mcndx);
				pp->p_hat_refcnt--;
			}
			/*
			 * Shared page with outstanding reference count.
			 */
			if (pp->p_hat_refcnt > 0 && 
			    pp->p_hat_agecnt < HAT_SYNC_THRESH && 
			    howhard != AS_TRIM)
				continue;

			ptesbufp->pte_mapp = mapp;
			ptesbufp->pteval = atomic_fnc_bit((uint_t *)ptep,
							ORD_PG_V);
			ptesbufp->pteval |= ptep->pg_pte; 

			num_ptes++;
			ptesbufp++;

			if (num_ptes == MAXL2_PTES) {
			    doflush |= hat_uas_shootdown_l(hatp);
			    /* if freeing same mcp or ptap, advance addr */
			    ASSERT(as->a_rss >= num_ptes);
			    ret = hat_unload_range(as, &ptap, &eptap,
					&addr, num_ptes, &freedphys_pgs, 
					&freedvirt_pgs, howhard);
			    num_ptes = 0;
			    ptesbufp = l.ptesbuf;

			    if (addr >= endaddr)
				goto done;
			    if (ret == NEXT_MC)
				break;
			    if (ret == NEXT_PT) {
				savptap = ptap;
				goto endptloop;
			    }
			}
		    } else { /* howhard == AS_AGE && PG_REF */
			atomic_and(&ptep->pg_pte, ~PG_REF);
			/* SAR counter */
			scanned_pgs++;
			doshoot = 1;
			if (!BITMASK1_TEST1(wasrefp, mcndx)) {
			    BITMASK1_SET1(wasrefp, mcndx);
			    pp->p_hat_refcnt++;
			}
		    }
		} /* mcndx loop */
	    }  /* mcnum loop */
	} /* howhard != AS_SWAP */
	ptap = ptap->hatpt_forw;
	addr = (vaddr_t) ((ptap->hatpt_pdtep - kpd0) * VPTSIZE);
endptloop:	;
    } while ((ptap != eptap) && (hatp->hat_pts != NULL));
    /* for the AS_TRIM case. */
    addr = endaddr;	
done:
    if (num_ptes != 0) {
	ASSERT(as->a_rss >= num_ptes);
	ASSERT(hatp->hat_pts != NULL);
	doflush |= hat_uas_shootdown_l(hatp);
	/* if freeing same mcp or ptap, advance addr */
	(void)hat_unload_range(as, &ptap, &eptap, &addr, num_ptes,
				&freedphys_pgs, &freedvirt_pgs, howhard);
    } else {
	if (doshoot)
	    doflush |= hat_uas_shootdown_l(hatp);
    }

    if (doflush)
	TLBSflushtlb();    

    HATRL_UNLOCK(hatp, origpl);
    /* update sar counters */
    if (howhard == AS_AGE) {
	MET_VIRSCAN(scanned_pgs);
	MET_PHYSFREE(freedphys_pgs);
	MET_VIRFREE(freedvirt_pgs);
    } else if (howhard == AS_SWAP) {
	MET_PSWPOUT(freedphys_pgs);
	MET_VPSWPOUT(freedvirt_pgs);
    }
    return addr;
}

/*
 * void
 * hat_free_modstats(struct as *as)
 * 	Free the modstats structure.
 *
 * Calling/Exit State:
 *	The context is exiting and hence single threaded.
 *	The caller has already verified that this process has the
 *	modbit stat structure allocated.
 *
 * Remarks:
 *	Since we are single threaded at this point, we do not acquire the
 *	hat resource lock. 
 */
STATIC 
void hat_free_modstats(struct as *as)
{
	hat_t *hatp = &as->a_hat;
	hat_stats_t *modstatsp, *nmodstatsp;

	ASSERT(hatp->hat_modstatsp != NULL);
	modstatsp = hatp->hat_modstatsp;
	while (modstatsp != NULL) {
		nmodstatsp = modstatsp->stats_next;
		ASSERT(modstatsp->stats_refcnt != 0);
		kmem_free(modstatsp->stats_modinfo, NPGPT);
		kmem_free(modstatsp, sizeof(hat_stats_t));
		modstatsp = nmodstatsp;
	}
}

/*
 * void
 * hat_start_stats(struct as *as, vaddr_t addr, ulong_t len)
 *	Start keeping mod bit history information for the given address
 *	range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *
 * Remarks:
 *	There should be a matching pair of hat_start_stats() and 
 *	hat_stop_stats() for the given range.
 */
void
hat_start_stats(struct as *as, vaddr_t addr, ulong_t len)
{
    hat_t *hatp = &as->a_hat;
    pte_t *vpdtep, *evpdtep;
    hat_stats_t *hat_modstatsp, *modstatsp = NULL;

    ASSERT((addr & POFFMASK) == 0);
    ASSERT((len & POFFMASK) == 0);
    ASSERT(u.u_procp->p_as == as);

    vpdtep = kpd0 + ptnum(addr);
    evpdtep = kpd0 + ptnum(addr + len - 1);

    HATRL_LOCK_SVPL(hatp);
    do {
tryagain:
	/* Returns the predecessor stats structure */
	hat_modstatsp = hat_findstats(hatp, vpdtep);
	if (hat_modstatsp != NULL && hat_modstatsp->stats_pdtep == vpdtep) {
	    hat_modstatsp->stats_refcnt++;
	} else {
	    if (modstatsp == NULL) {
		HATRL_UNLOCK_SVDPL(hatp);
		modstatsp = kmem_alloc(sizeof(hat_stats_t), KM_SLEEP);
		modstatsp->stats_modinfo = kmem_zalloc(NPGPT, KM_SLEEP);
		HATRL_LOCK_SVPL(hatp);
		goto tryagain;
	    }
	    if (hat_modstatsp == NULL) {
		modstatsp->stats_next = hatp->hat_modstatsp;
		hatp->hat_modstatsp = modstatsp;
	    } else {
		modstatsp->stats_next = hat_modstatsp->stats_next;
		hat_modstatsp->stats_next = modstatsp;
	    }

	    modstatsp->stats_refcnt = 1;
	    modstatsp->stats_pdtep = vpdtep;
	    /* Used up both the structures */
	    modstatsp = NULL;
	}
    } while (++vpdtep <= evpdtep);

    HATRL_UNLOCK_SVDPL(hatp);
    if (modstatsp != NULL) {
	kmem_free(modstatsp->stats_modinfo, NPGPT);
	kmem_free(modstatsp, sizeof(hat_stats_t));
    }
}

/*
 * void
 * hat_stop_stats(struct as *as, vaddr_t addr, ulong_t len)
 *	Stop the stats collection of the mod bit history info.
 *	for the given range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *
 * Remarks:
 *	There should be a matching pair of hat_start_stats() and 
 *	hat_stop_stats() for the given range.
 */
void
hat_stop_stats(struct as *as, vaddr_t addr, ulong_t len)
{
    hat_t *hatp = &as->a_hat;
    pte_t *vpdtep, *evpdtep;
    hat_stats_t *prev_modstatsp, *hat_modstatsp, *tmp_modstatsp;

    ASSERT((addr & POFFMASK) == 0);
    ASSERT((len & POFFMASK) == 0);
    ASSERT(u.u_procp->p_as == as);

    vpdtep = kpd0 + ptnum(addr);
    evpdtep = kpd0 + ptnum(addr + len - 1);

    HATRL_LOCK_SVPL(hatp);
    /*
     * Temporary hack for close on exec problem. If not for this
     * problem, we should be able to assert this condition is not true.
     */
    if (hatp->hat_modstatsp == NULL) {
	HATRL_UNLOCK_SVDPL(hatp);
	return;
    }

    hat_modstatsp = hatp->hat_modstatsp;
    if (hat_modstatsp->stats_pdtep == vpdtep)
	prev_modstatsp = NULL;
    else {
	while (hat_modstatsp->stats_next->stats_pdtep != vpdtep) {
	    hat_modstatsp = hat_modstatsp->stats_next;
	    ASSERT(hat_modstatsp != NULL);
	}
	prev_modstatsp = hat_modstatsp;
	hat_modstatsp = hat_modstatsp->stats_next;
    }
    do {
	ASSERT(hat_modstatsp->stats_pdtep == vpdtep);
	ASSERT(hat_modstatsp->stats_refcnt > 0);
	ASSERT(prev_modstatsp == NULL || 
		prev_modstatsp->stats_next == hat_modstatsp);

	if (--hat_modstatsp->stats_refcnt == 0) {
	    kmem_free(hat_modstatsp->stats_modinfo, NPGPT);
	    if (prev_modstatsp == NULL) {
		ASSERT(hatp->hat_modstatsp == hat_modstatsp);
		hatp->hat_modstatsp =  hat_modstatsp->stats_next;
	    } else {
		prev_modstatsp->stats_next = hat_modstatsp->stats_next;
	    }
	    tmp_modstatsp = hat_modstatsp->stats_next;
	    kmem_free(hat_modstatsp, sizeof(hat_stats_t));
	    hat_modstatsp = tmp_modstatsp;
	} else {
	    prev_modstatsp = hat_modstatsp;
	    hat_modstatsp = hat_modstatsp->stats_next;
	}
    } while(++vpdtep <= evpdtep);

    HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void
 * hat_check_stats(struct as *as, vaddr_t addr, ulong_t len, u_int *vec,
 *			boolean_t clear)
 *	Check the modify bit information for the given address range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *	Argument vec is an outarg, which on return, would contain a
 *	bit vector corresponding to the address range. If the corresponding
 *	bit is on, the page has been modified.
 *	vec is not assumed to be initialized by this function.
 *
 * Remarks:
 *	A preceding hat_start_stats() for the address range should have been 
 *	called.
 *
 * Description:
 *	Checks both the page table entry (if present) as well as the
 * 	stats structure for the mod bit information. Clears the stats
 *	mod bit information and the pte mod bit after recording, if the
 *	clear flag is set.
 */
void
hat_check_stats(struct as *as, vaddr_t addr, ulong_t len, uint_t *vec, 
		boolean_t clear)
{
    hat_t *hatp = &as->a_hat;
    pte_t *vpdtep, *evpdtep, *ptep;
    hat_stats_t	*modstatsp;
    int indx, vec_indx, mcnum, mcndx, i;
    hatpt_t *ptap;
    off_t ptlen;
    hatmcp_t *mcp;
    boolean_t flush = B_FALSE;
    page_t *pp;
#ifdef DEBUG
    off_t savlen = btop(len);
#endif

    ASSERT((addr & POFFMASK) == 0);
    ASSERT((len & POFFMASK) == 0);
    ASSERT(u.u_procp->p_as == as);

    vpdtep = kpd0 + ptnum(addr);
    evpdtep = kpd0 + ptnum(addr + len - 1);

    HATRL_LOCK_SVPL(hatp);

    modstatsp = hat_findstats(hatp, vpdtep);
    ASSERT(modstatsp != NULL);
    ASSERT(modstatsp->stats_pdtep == vpdtep);

    indx = pnum(addr);
    len = btop(len);
    ASSERT(len != 0);
    vec_indx = 0;
    do {
	ASSERT(modstatsp != NULL);
	ASSERT(modstatsp->stats_pdtep == vpdtep);
	ASSERT(modstatsp->stats_refcnt >= 1);

	ptap = hat_findpt(hatp, vpdtep);
	if (ptap == NULL || ptap->hatpt_pdtep != vpdtep) {
	    ptlen = min(len, NPGPT);
	    for (i = 0; i < ptlen; i++, indx++, vec_indx++) {
		if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
		    BITMASKN_SET1(vec, vec_indx);
		else
		    BITMASKN_CLR1(vec, vec_indx);

		if (clear)
			BITMASKN_CLR1(modstatsp->stats_modinfo, indx);
	    }
	    len -= ptlen;
	} else {	/* page table exists for this address */
	    /*
	     * Reset addr when entering a new page table.
	     */
	    if (indx == 0)
		addr = (vaddr_t) ((ptap->hatpt_pdtep - kpd0) * VPTSIZE); 

	    SETMAPP(addr, mcnum, mcp, mcndx, ptap);
	    for (; mcnum < HAT_MCPP && len != 0; mcp++, mcnum++, mcndx = 0) {
		if (mcp->hat_mcp == 0) {
		    for (; mcndx < HAT_EPMC && len != 0; mcndx++, indx++,
						vec_indx++, len--) {
			if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
			    BITMASKN_SET1(vec, vec_indx);
			else
			    BITMASKN_CLR1(vec, vec_indx);

			BITMASKN_CLR1(modstatsp->stats_modinfo, indx);
		    }
		    continue;
		}

		ptep = mapptoptep(mcptomapp(mcp)) + mcndx;
		for (; mcndx < HAT_EPMC && len != 0; ptep++, mcndx++, indx++,
							vec_indx++, len--) {
		    if (ptep->pg_pte == 0) {
			if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
			    BITMASKN_SET1(vec, vec_indx);
			else
			    BITMASKN_CLR1(vec, vec_indx);
		    } else {
			if (ptep->pg_pte & PG_M) {
			    BITMASKN_SET1(vec, vec_indx);
			    if (clear) {
				pp = pteptopp(ptep);
				if (PAGE_TRYUSELOCK(pp) != INVPL) {
				    atomic_and(&ptep->pg_pte, ~PG_M);
				    pp->p_mod = 1;
				    PAGE_SETMOD(pp);
				    flush = B_TRUE;
				    hat_uas_shootdown_l(hatp);
				    UNLOCK(&pp->p_uselock,VM_HAT_RESOURCE_IPL);
				}
			    }
			} else {
			    if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
				BITMASKN_SET1(vec, vec_indx);
			    else
				BITMASKN_CLR1(vec, vec_indx);
			}
		    }
		    if (clear)
			BITMASKN_CLR1(modstatsp->stats_modinfo, indx);
		}
	    }
	}
	indx = 0;
	modstatsp = modstatsp->stats_next;
    } while (++vpdtep <= evpdtep && len != 0);

    ASSERT(vec_indx == savlen);

    if (flush)
	TLBSflushtlb();

    HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * vaddr_t hat_dup(struct seg *pseg, struct seg *cseg, uint_t flags) 
 *	Preload child page tables based on parent PT entries and flags.
 *
 * Calling/Exit State:
 *	No locks need be held on entry and none are held on exit.
 *	The cseg is pre-established. Calls to kmem_zalloc and kpg_alloc are
 *	possible (constraint on hierarchy of any spinlocks relative to KMA
 *	locks held by caller of hat_dup). The child AS's hat resource lock
 *	must be available to grab, since hat_dup must grab both the
 *	parent's and the child's lock and there can be contention for the
 *	parent's lock. Alas, there CAN also be contention for the child's
 *	lock after the first hat_dup (i.e., after the first segment), since
 *	hat_pageunload() (and hat_pagesyncmod()) can grab the lock. This is
 *	one of the few cases where a LOCK_SH is permissible (No possibility
 *	of an AB/BA deadlock). Parent's pages are also PAGE_TRYUSELOCKed by
 *	hat_dupmc() which is called by hat_dup. Returns the first addr not
 *	processed.
 *
 * Description:
 *	This function calls hat_dupmc() for each mapping chunk that is
 *	being dup'ed. No sleeping is done in this execution path. We 
 *	skip any pages that cannot be tryuselocked and we exit on resource
 *	exhaustion.
 * Remarks:
 *	Does the child's as hide pages from the paging policy?
 *	If it does, this may result in a deadlock. We cannot do ageing on
 *	the child address space since we will holding the child's hat lock
 *	during hat_dup(). We can do a memresv for each page of the parent
 *	we break COW.
 *
 *	Segments mapped via PSE mappings should not be calling
 *	this routine; such segments call pse_hat_dup instead.
 */
vaddr_t
hat_dup(struct seg *pseg, struct seg *cseg, uint_t flags)
{
    vaddr_t addr = pseg->s_base;
    vaddr_t endaddr = pseg->s_base + pseg->s_size;
    hatpt_t *new_ptap = (hatpt_t *)NULL;
    hatmc_t *new_mc = (hatmc_t *)NULL;
    hatpt_t *pptap;
    hat_t *phatp = &pseg->s_as->a_hat;
    hat_t *chatp = &cseg->s_as->a_hat;
    pl_t trypl;
    pte_t *pptep;
    int mcnum, mcndx;
    hatmcp_t *pmcp;
    hatpgt_t *ppt;
    enum hat_cont_type res;

    ASSERT(pseg->s_base == cseg->s_base);
    ASSERT(pseg->s_size == cseg->s_size);

    for (;;) {
	/*
	 * We pre-allocate the page table page and the mapping
	 * chunk for the child in order not to drop locks later.
	 */
start:
	if (new_ptap == (hatpt_t *)NULL)
	    new_ptap = hat_ptalloc(HAT_REFRESH);
	if (new_ptap == (hatpt_t *)NULL) {
	    if (new_mc)
		hat_mcfree(new_mc->hat_map);
	    return(addr);
	}
	if (new_mc == (hatmc_t *)NULL) {
	    new_mc = hat_mcalloc(HAT_REFRESH); /* just refreshed */
	    if (new_mc == (hatmc_t *)NULL) {
		hat_ptfree(new_ptap);
		return(addr);
	    }
	}

	HATRL_LOCK_SVPL(phatp);

	/*
	 * The child hat lock is locked by LOCK_SH primitive because the
	 * parent lock is held at this time. LOCK_SH is o.k. here since
	 * deadlock should not happen here.
	 * If that is found to be FALSE, then hat_dup must stop after
	 * one TRYLOCK and return(addr).
	 */
	trypl = HATRL_LOCK_SH(chatp);

	/*
	 * Find the page table in the address range that is left
	 * in the seg being duped.
	 */
	while ((pptap = hat_findfirstpt(&addr, phatp)) != (hatpt_t *)NULL) {
	    if (addr >= endaddr)
		goto nomoreret;
	    ASSERT(addr < endaddr && addr >= pseg->s_base);

	    ASSERT(!PG_ISPSE(&pptap->hatpt_pde));
	    phatp->hat_ptlast = pptap;
	    ppt = pptap->hatpt_ptva;

	    /*
	     * now see if there is a mapping chunk in range.
	     */
	    SETMAPP(addr, mcnum, pmcp, mcndx, pptap);
	    for (; mcnum < HAT_MCPP; pmcp++, mcnum++, mcndx = 0) {
		if (pmcp->hat_mcp == 0) {
		    addr = (addr + HATMCSIZE) & HATVMC_ADDR;
		    if (addr >= endaddr)
			goto nomoreret;
		    continue;
		}
		pptep = ppt->hat_pgtc[mcnum].hat_pte + mcndx;
		for (; mcndx < HAT_EPMC; pptep++, mcndx++) {
		    if (pptep->pg_pte != 0) {
			res = hat_dupmc(cseg, pptep, &new_mc, &new_ptap, &addr,
					endaddr, mcnum, mcndx, flags);
			if (res == NOMORE)
			    goto nomoreret;
			if (res == ALLOCPT) {
			    HATRL_UNLOCK(chatp, trypl);
			    HATRL_UNLOCK_SVDPL(phatp);
			    goto start;
			}
			if (res == NEXTMC) {
			    if (addr >= endaddr)
				goto nomoreret;
			    break;	
			}
		    }  /* if pte was valid */
		    addr += PAGESIZE;
		    if (addr >= endaddr)
			goto nomoreret;
		}   /* mapping entries within a chunk loop */
	    } /* mapping chunks loop */
	} /* while  loop */
	if (pptap == (hatpt_t *)NULL)
		break;
    } /* for ever loop */
nomoreret:
		UNLOCK(&chatp->hat_resourcelock, trypl);
		HATRL_UNLOCK_SVDPL(phatp);
		if (new_ptap)
			hat_ptfree(new_ptap);
		if (new_mc)
			hat_mcfree(new_mc->hat_map);
		return(addr);
}

/*
 * enum hat_cont_type
 * hat_dupmc(struct seg *cseg, pte_t *pptep, hatmc_t **new_mc,
 *		hatpt_t **new_ptap, vaddr_t *addr, vaddr_t endaddr,
 *		int mcnum, int mcndx, uint_t flags)
 *
 * 	Loop over the parent's chunk and copy the parent's translations
 *	to the child. Skip copying for translations that have the write
 *	permission in the parent's AS if the flags argument is set to
 *	HAT_NOPROTW, meaning it is a MAP_PRIVATE segment.
 *
 * Calling/Exit State:
 *	The caller of this functions is expected to hold the hat resource
 *	locks of the parent and the child. This function TRYUSELOCKs the
 * 	parent's pages and if it fails returns. It does not block.
 *	There are four returns possible from this function:
 *	NEXTMC tells the caller to go on to the next mapping chunk;
 *	ALLOCPT tells the caller that it failed to allocate pages
 * 	for page table or mapping chunk after using new_ptap or new_mc;
 *	NOMORE tells the caller that it's time to bail out either
 *	because of resource exhaustion or we have finished dup'ing the parent.
 *
 * Remarks:
 *	The parent's permissions are never changed. If HAT_HIDDEN is specified
 *	in flags, the translations are inherently shared, but do not appear on
 *	any possible p_hat_mapping lists. If HAT_HIDDEN is not in flags,
 *	there must be page_t's for each translation. In the latter case,
 *	processing depends on whether pseg is shared or private. Either pseg is
 *	shared (HAT_NOPROTW not specified in flags), which means no permissions
 *	checks are needed, or it is private (HAT_NOPROTW is in flags) and we
 *	immediately need to skip duping writable translations into the child.
 */
STATIC enum hat_cont_type
hat_dupmc(struct seg *cseg, pte_t *pptep, hatmc_t **new_mc, hatpt_t **new_ptap,
	  vaddr_t *addrp, vaddr_t endaddr, int mcnum, int mcndx, uint_t flags)
{
	hatpt_t *cptap;
	hatmcp_t *cmcp;
	pte_t *cptep, cpteval, *vpdtep;
	hatmap_t *cmapp;
	hatptcp_t *cptcp;
	hatpgt_t *cpt;
	page_t *cpp;
	struct as *cas = cseg->s_as;
	hat_t *chatp = &cseg->s_as->a_hat;
	boolean_t gotone;
	int skip_writable;
	vaddr_t addr = *addrp;

	ASSERT(*new_mc);
	/*
	 * flag that tells if there are any cowable pages in this range
	 * in pseg.
	 */
	skip_writable = flags & HAT_NOPROTW;
	vpdtep = kpd0 + ptnum(addr);
	/*
	 * now that parent is set up with something to work on,
	 * see what state the child is in.
	 */
	cptap = hat_findpt(chatp, vpdtep);

	/*
	 * Now cptap is either NULL (no entries in chatp yet),
	 * it is the desired ptap (cptap->hatpt_pdtep == vpdtep),
	 * or it is the prevptap for link_ptap() and the desired entry
	 * is absent.
	 */
	if (flags & HAT_HIDDEN) {
	    /*
	     * INIT_PTAP links the new_ptap into cas and reallocates
	     * new_ptap and new_mc, if cptap is NULL and sets
	     * hatpt_last for chatp.
	     */
	    INIT_PTAP(cas, chatp, cptap, vpdtep, *new_ptap);
	    if (*new_ptap == (hatpt_t *)NULL) {
		if (ptnum(endaddr - 1) > ptnum(addr))
		    *new_ptap = hat_ptalloc(HAT_POOLONLY);
	    }

	    /*
	     * INIT_MAPP allocates mapping chunk, if necessary. It also
	     * sets variables cmcp, cpt, cptep and cptcp.
	     */
	    INIT_MAPP(cmcp, cpt, cmapp, mcnum, *new_mc, cptap, cptep,cptcp);
	    if (*new_mc == (hatmc_t *)NULL) {
		if (*new_ptap || (HATMCNO(endaddr - 1) > mcnum))
		    *new_mc = hat_mcalloc(HAT_POOLONLY);
	    }
	    for (; mcndx < HAT_EPMC; addr += PAGESIZE, pptep++, mcndx++) {
		if (addr >= endaddr) {
		    *addrp = addr;
		    return NOMORE;
		}
		if (pptep->pg_pte & PG_V) {
		    cptep[mcndx].pg_pte = (pptep->pg_pte & ~PG_LOCK);
		    cmapp[mcndx].hat_mapv = MAPV_HIDDEN;
		    cptap->hatpt_hec++;
		    BUMP_AEC(cptcp, cptap, 1);
		}
	    }
	    *addrp = addr;
	    goto bye;	
	}

	/*
	 * Only visible mappings are left (pages backed by real
	 * memory as opposed to magic memory, like a mapped I/O bus).
	 * Translations appear on page_t p_hat_mapping chains.
	 */

	gotone = B_FALSE; /* gotone means found 1st pte in this mapping chunk*/
	for (; mcndx < HAT_EPMC; addr += PAGESIZE, pptep++, mcndx++) {
	    if (addr >= endaddr) {
		*addrp = addr;
		return NOMORE;
	    }
	    if ((pptep->pg_pte & PG_V) == 0)
		continue;
	    /* Don't propogate lock bit to child.  */
	    cpteval.pg_pte = (pptep->pg_pte & ~PG_LOCK);
	    cpp = pteptopp(pptep);
	    ASSERT(cpp != NULL);
	    if (skip_writable && (pptep->pg_pte & PG_RW))
		continue;
	    /*
	     * We found a loaded page, and have no instructions to skip it.
	     * We are going to just copy the translation over to the child's
	     * address space. We need to USELOCK the page ourselves to do
	     * this. Because we hold the HAT lock, however, we can't get the
	     * p_uselock w/o violating the lock hierarchy. We trylock it; if
	     * we can't get it first time, the child is just going to have to
	     * fault this one in itself later.
	     */

	    if (PAGE_TRYUSELOCK(cpp) == INVPL) {
		continue;
	    }
	    l.puselockpl = VM_HAT_RESOURCE_IPL;

	    ASSERT(pteptopp(&cpteval) == cpp);

	    if (!gotone) {    /*first translation in the child page table? */
		gotone = B_TRUE;

		/*
		 * INIT_PTAP links the new_ptap into cas and
		 * reallocates new_ptap and new_mc, if cptap is NULL
		 * and sets hatpt_last for chatp.
		 */
		INIT_PTAP(cas, chatp, cptap, vpdtep, *new_ptap);

		if (*new_ptap == (hatpt_t *)NULL) {
		    if (ptnum(endaddr - 1) > ptnum(addr))
			*new_ptap = hat_ptalloc(HAT_POOLONLY);
		}

		/*
		 * INIT_MAPP allocates mapping chunk, if necessary.
		 * It also sets variables cmcp, cpt, cptep and cptcp.
		 */
		INIT_MAPP(cmcp, cpt, cmapp, mcnum, *new_mc, cptap, cptep, 
				cptcp);
		if (*new_mc == (hatmc_t *)NULL) {
		    if (*new_ptap || (HATMCNO(endaddr - 1) > mcnum))
			*new_mc = hat_mcalloc(HAT_POOLONLY);
		}
	    }
	    /*
	     * Load the translation into the child using the
	     * the same permissions we found in the parent.
	     * Link into p_hat_mapping.
	     */
	    cptep[mcndx].pg_pte = cpteval.pg_pte;
	    cmapp[mcndx].hat_mnext = cpp->p_hat_mapping;
	    cpp->p_hat_mapping = cmapp + mcndx;

	    ASSERT(pteptopp(mapptoptep(cmapp + mcndx)) == cpp);

	    ASSERT(cpteval.pg_pte);
	    ASSERT((cpteval.pg_pte & PG_LOCK) == 0);

	    BUMP_RSS(cas, 1);
	    BUMP_AEC(cptcp, cptap, 1);

	    PAGE_USEUNLOCK(cpp);
	    if (psm_intrpend(PLBASE)) {
		addr += PAGESIZE;
		*addrp = addr;
	        if (addr >= endaddr)
		    return NOMORE;
	        else
		    return ALLOCPT;
	    }
	}
	*addrp = addr;
bye:
	/* need pagetable ? */
	if (*new_ptap == NULL) {
	    if (ptnum(endaddr - 1) > ptnum(addr))
		return ALLOCPT;
	}
	if (*new_mc == NULL) {
	    /* need mapping chunk ? */
	    if ((HATMCNO(endaddr - 1) > mcnum) || 
			(ptnum(endaddr - 1) > ptnum(addr)))
		return ALLOCPT;
	}

	return NEXTMC;
}

/*
 * hatpt_t *
 * hat_findpt(hat_t *hatp, pte_t *vpdtep)
 *	Find the page table corresponding to the l1 entry.
 *
 * Calling/Exit State:
 *	The hat resource lock for hatp is held and is returned held.
 *	This routine doesn't block. If desired pt exists, return the
 *	pt. If not, return the preceding page table. If no page table
 *	exists for the address space, return NULL.
 * Remarks:
 * 	Global so that it can be called from pse_hat.c
 */
hatpt_t *
hat_findpt(hat_t *hatp, pte_t *vpdtep)
{
	hatpt_t *eptap, *ptap;

	ptap = hatp->hat_ptlast;
	if ((ptap == (hatpt_t *)NULL) || (ptap->hatpt_pdtep == vpdtep)) {
		return ptap;
	}
	eptap = ptap;
	if (ptap->hatpt_pdtep > vpdtep) {
		do {
			ptap = ptap->hatpt_back;
		} while ((ptap->hatpt_pdtep > vpdtep) &&
			 (ptap->hatpt_pdtep < eptap->hatpt_pdtep));
		/*
		 * Either we found the ptap desired or it is missing and
		 * we found the ptap that would be the predecessor in the
		 * ptap list.
		 */
	} else {
		do {
			ptap = ptap->hatpt_forw;
		} while ((ptap->hatpt_pdtep < vpdtep) &&
			 (ptap->hatpt_pdtep > eptap->hatpt_pdtep));
		/*
		 * If all are before the desired place, we stop
		 * at one too far. If not, we back up one.
		 */
		if (ptap->hatpt_pdtep != vpdtep)
			ptap = ptap->hatpt_back;
	}
	return ptap;
}

/*
 * STATIC hat_stats_t *
 * hat_findstats(hat_t *hatp, pte_t *vpdtep)
 *	Find the stats structure corresponding to the l1 entry.
 *
 * Calling/Exit State:
 *	The hat resource lock for hatp is held and is returned held.
 *	This routine doesn't block. If desired stats struct. exists,
 *	return the struct. If not, return the preceding structure.
 *	If the l1 pointer is less than any existing stats structs or
 *	if there are no existing structs for the address space,
 *	returns NULL.
 */
STATIC
hat_stats_t *
hat_findstats(hat_t *hatp, pte_t *vpdtep)
{
	hat_stats_t *modstatsp;

	if ((modstatsp = hatp->hat_modstatsp) == NULL ||
			vpdtep < modstatsp->stats_pdtep)
		return NULL;

	 while (modstatsp->stats_next != NULL) {
		ASSERT(vpdtep >= modstatsp->stats_pdtep);
		if (modstatsp->stats_pdtep == vpdtep ||
			vpdtep > modstatsp->stats_next->stats_pdtep)
			return modstatsp;

		modstatsp = modstatsp->stats_next;
	}

	return modstatsp;
}

/*
 * STATIC hatpt_t *
 * hat_findfirstpt(vaddr_t *addr, hat_t *phatp)
 *	Find the first existing page table in an address space starting
 *	from addr.
 *
 * Calling/Exit State:
 *	The hat resource lock for phatp is held and is returned held.
 */
STATIC hatpt_t *
hat_findfirstpt(vaddr_t *addr, hat_t *hatp)
{
	hatpt_t *ptap;
	pte_t *vpdtep;
#ifdef DEBUG
	vaddr_t savaddr = *addr;
#endif

	ptap = hatp->hat_ptlast;
	if (ptap == (hatpt_t *)NULL) {
		ASSERT(hatp->hat_pts ==(hatpt_t *) NULL);
		return NULL;
	}

	vpdtep = kpd0 + ptnum(*addr);

	/* check if the l1 entry is what we are looking for */
	if (ptap->hatpt_pdtep == vpdtep)
		return ptap;

	/*
	 * the following complex looking condition just checks if
	 * the the addr passed in is beyond the last page table
	 * for this AS.
	 */
	if (hatp->hat_pts->hatpt_back->hatpt_pdtep < vpdtep)
	  	return ((hatpt_t *)NULL);

	/*
	 * start from hat_pts if ptlast is beyond the pt
	 * we are looking for.
	 */
	if (ptap->hatpt_pdtep > vpdtep) {
		ptap = hatp->hat_pts;
		hatp->hat_ptlast = ptap;
	}

	while (ptap->hatpt_pdtep < vpdtep) {
		ptap = ptap->hatpt_forw;
		ASSERT(ptap != hatp->hat_pts);
	}

	hatp->hat_ptlast = ptap;

	ASSERT(ptap->hatpt_pdtep >= vpdtep);

	if (ptap->hatpt_pdtep != vpdtep)
		*addr = (vaddr_t)((ptap->hatpt_pdtep - kpd0) * VPTSIZE);

	ASSERT(*addr >= savaddr);

	return ptap;
}


/* The following are for debugging only */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * pte_t *
 * hat_kvtol1ptep_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte_t *
hat_kvtol1ptep_dbg(vaddr_t addr)
{
	return(kvtol1ptep(addr));
}

/*
 * pte_t *
 * hat_kvtol2ptep_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte_t *
hat_kvtol2ptep_dbg(vaddr_t addr)
{
	return(kvtol2ptep(addr));
}

/*
 * pte_t *
 * hat_kvtol2pteptep_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte_t *
hat_kvtol2pteptep_dbg(vaddr_t addr)
{
	return(kvtol2pteptep(addr));
}

/*
 * pte_t *
 * hat_kvtophys_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
paddr_t
hat_kvtophys_dbg(vaddr_t addr)
{
	return(kvtophys(addr));
}

/*
 * pte_t *
 * hat_kvtopp_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
page_t *
hat_kvtopp_dbg(vaddr_t addr)
{
	return(kvtopp(addr));
}

/*
 * pte_t *
 * hat_kvis_maptopte_dbg(hatmap_t *mapp)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte_t *
hat_kvis_maptopte_dbg(hatmap_t *mapp)
{
	return(KVIS_MAPTOPTE(mapp));
}

/*
 * pte_t *
 * hat_kvis_ptetomap_dbg(const pte_t *ptep)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
hatmap_t *
hat_kvis_ptetomap_dbg(const pte_t *ptep)
{
	return(KVIS_PTETOMAP(ptep));
} 

/*
 * ulong_t
 * hat_mapmcno_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
ulong_t
hat_mcno_dbg(vaddr_t addr)
{
	return(HATMCNO(addr));
}

/*
 * ulong_t
 * hat_mapmcndx_dbg(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
ulong_t
hat_mcndx_dbg(vaddr_t addr)
{
	return(HATMCNDX(addr));
}

/*
 * hatptcp_t *
 * hat_mapptoptcp_dbg(const hatmap_t *mapp)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
hatptcp_t *
hat_mapptoptcp_dbg(const hatmap_t *mapp)
{
	return(mapptoptcp(mapp));
}

/*
 * vaddr_t
 * hat_pteptokv_dbg(const pte_t *ptep)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
vaddr_t
hat_pteptokv_dbg(const pte_t *ptep)
{
	return(pteptokv(ptep));
}

/*
 * pte_t *
 * hat_mapptoptep_dbg(const hatmap_t *mapp)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte_t *
hat_mapptoptep_dbg(const hatmap_t *mapp)
{
	return(mapptoptep(mapp));
}

/*
 * uint_t *
 * hat_mapptowasrefp_dbg(const hatmap_t *mapp)
 * 	Debug routine.
 *
 * Calling/Exit State:
 *      None.
 */
uint_t *
hat_mapptowasrefp_dbg(const hatmap_t *mapp)
{
        return(mapptowasrefp(mapp));
}
#endif /* DEBUG || DEBUG_TOOLS */
