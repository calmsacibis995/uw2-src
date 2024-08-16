/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_VM_HAT_H	/* wrapper symbol for kernel use */
#define _MEM_VM_HAT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/vm_hat.h	1.42"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/param.h>	 /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */
#include <mem/immu.h> /* REQUIRED */
#include <mem/mem_hier.h> /* REQUIRED */
#include <util/emask.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>	 /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <sys/immu.h> /* REQUIRED */
#include <sys/emask.h> /* REQUIRED */
#include <vm/mem_hier.h> /* REQUIRED */

#else

#include <sys/immu.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the machine specific
 * hat data structures and the machine specific hat procedures.
 * The machine independent interface is described in <vm/hat.h>.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* TLB Shootdown cookie type: */
/*
 * A clock_t is a long.  We want unsigned so we have to declare it
 * unsigned long instead of unsigned clock_t to keep lint quiet.
 */
typedef unsigned long TLBScookie_t;

/*
 * The x86 HAT design is based on dividing a page table into
 * chunks that are a power of 2 long.
 * These chunks allow the mapping pointer overhead to be
 * reduced for the typical small process.
 * The number of chunks times the number of entries equals 1024.
 * The initial design uses 32 chunks of 32 entries.
 * If translation caching (stealable translation accounting)
 * is implemented, this may change to 16 chunks of 64 entries.
 * This allows four accounting words to be available per chunk
 * (instead of 1 word) in the accounting chunk (chunk 0) of
 * a mapping chunk page.
 * The extra words would be used for a time stamp and pointers
 * for a doubly linked list.
 * To acommodate this possible change, the following defines exist.
 */
#define HAT_MCPP	32	/* number of mapping chunks per page */
#define HAT_EPMC	32	/* number of entries per mapping chunk */

/*
 * The following union describes a single mapping cell for a chunk.
 */
typedef union hatmap {
	uint_t hat_mapv;
	union hatmap *hat_mnext;
} hatmap_t;

#define MAPV_HIDDEN	((uint_t)1)  /* hat_mapv value for hidden mappings */

/*
 * The following structure describes a mapping chunk.
 */
typedef union hatmc {
	hatmap_t hat_map[HAT_EPMC];
	union hatmc *hat_mcnext;	/* for free chunk list in page */
} hatmc_t;

/* 
 * The hatmcp_t:
 * is the typedef for a pointer to a mapping chunk.
 */
typedef struct {
	hatmc_t *hat_mcp;
} hatmcp_t;

#define HATMC_ADDR	0xFFFFFF80

/*
 * stat structure to keep the mod bit info for Locus Merge.
 */
typedef struct hat_stats {
	pte_t	*stats_pdtep;
	int	stats_refcnt;
	uint_t	*stats_modinfo;
	struct hat_stats *stats_next;
} hat_stats_t;

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 */
typedef struct hat {
	lock_t	hat_resourcelock;	/* spinlock for all hat resouce use
					 * including this structure.
					 */
	pl_t	hat_lockpl;		/* pl for current hat lock holder */
	struct	hatpt *hat_pts;		/* current page table list */
	struct	hatpt *hat_ptlast;	/* last page table to fault */
	hat_stats_t *hat_modstatsp;	/* mod bit stats for merge */
	int	hat_activecpucnt;	/* count of CPUs currently active
					 * in this address space.
					 */
	emask_t	hat_activecpubits;	/* bitmap of active CPUs */

	/*
	 * The next two fields, hat_rdonly_owner and hat_rdonly_sv, are
	 * used in conjunction with the hat_resourcelock to provide a
	 * specific type of synchronization between the LWPs that share 
	 * the same address space. The need for the synchronization is specific
	 * to the Intel386(tm) processor. The purpose of the synchronization
	 * is to ensure that while an LWP is performing write accesses in the
	 * kernel mode to a range in the encompassing address space it is 
	 * guaranteed that none of the translation protections over that 
	 * range are (or become) restricted to read-only values. 
	 *
	 * This requirement is Intel386 specific because it arises from the
	 * fact that the processor does not take a protection fault when it is
	 * in supervisor mode. Code that relies on correct recognition of
	 * the protection violation (such as algorithms that do backing store 
	 * allocation at the time of the first write access to certain
	 * pages, or the copy-on-write semantics of sharing) must be
	 * supported by explicit simulation of the faults before
	 * actual accesses are generated in kernel mode. 
	 *
	 * After an LWP has performed this type of pre-faulting, it needs to
	 * co-ordinate with other LWPs, by signalling its intention that the
	 * translations thus instantiated remain writeable until the original
	 * LWP has completed write accesses (in supervisor mode).
	 *
	 * The two fields, hat_rdonly_owner and hat_rdonly_sv, provide this
	 * signalling mechanism as follows. The LWP that needs to prevent the
	 * loading of read-only translations (or modification of existing ones
	 * to read-only) acquires an effective lock by recording its LWP 
	 * pointer into hat_rdonly_owner. Any other LWP that either needs to
	 * acquire the same lock or needs to load a read-only translation,
	 * check that the hat_rdonly_owner field is NULL -- and if it is not,
	 * they must block until the owner signals release of lock through
	 * the hat_rdonly_sv synchronization object. One exception to this rule
	 * is that the owner LWP is permitted to load read-only translations,
	 * as otherwise, it can deadlock against itself. 
	 *
	 * The hat_resourcelock is the spin lock which guards the 
	 * acquisition and release of the read-only pseudo lock. 
	 * This works because the loading of any translation 
	 * is accomplished entirely within the coverage of hat_resourcelock.
	 */
	struct 	lwp *hat_rdonly_owner;
	sv_t	hat_rdonly_sv;	
} hat_t;

/*
 * Page tables are divided into HAT_MCPP chunks of HAT_EPMC PTEs
 * to correspond to the chunking for mapping pointers.
 * The following structure describes a chunk of PTEs.
 */
typedef struct hatpgtc {
	pte_t	hat_pte[HAT_EPMC];
} hatpgtc_t;

/*
 * The mapping chunk page declarations:
 * A mapping chunk is allocated from a physical page divided into
 * HAT_MCPP chunks of HAT_EPMC mapping pointers.
 * The first chunk of the physical page is used for free chunk accounting
 * and page table chunk linkage pointers.  The latter use generates
 * the requirement that HAT_EPMC >= HAT_MCPP so that there enough room
 * for the PTE chunk pointers.
 * The second chunk of the page is used for keeping reference history 
 * information for each mapping pointer. Each word acts as a bitmask for
 * a mapping chunk with each bit in the word denoting the reference 
 * information for a mapping pointer.
 */

/*
 * The following union describes a page table chunk pointer.
 * The low order bits of the pointer would always be zero because of the
 * physical alignment rules of chunks.  Those implied zero bits
 * have been used for a needed count field that counts active slots
 * (NOTE a mapping pointer slot can contain NULL and still be active).
 * The mask HATPTC_ADDR is useful for using the value as a pointer
 * to a PTE chunk as in the statement:
 *
 *	ptep = (pte_t *)(ptcp->hat_ptpv & HATPTC_ADDR) + mcnx;
 *
 * There are situations where the count can be higher than the actual
 * number of active entries since it is artificially bumped in some
 * special cases (artificial bumps exclude each other by other means)
 * to allow much simpler scan code that might sleep during the scan.
 */

typedef union {
	struct {
		uint_t hat_mcaec : 7,	/* active PTE count for chunk.
					 * Though there are at most 32
					 * entries, it must hold an additional
					 * 33 (or so) counts for various
					 * artifical holds (32 for per-PTE
					 * holds--hat_unload only has
					 * exclusive rights to the pages
					 * it unloads, not the neighbors;
					 * 1 for permanently allocated stuff).
					 */
		      hat_ptcndx : 25;	/* high order bits of pointer
					 * to page table chunk.  Zeroed
					 * low order bits completes
					 * the pointer.
					 */
	} ptp;
	uint_t hat_ptpv;
	hatpgtc_t *hat_pgtcp;
} hatptcp_t;

#define HATPTC_ADDR	0xFFFFFF80

/*
 * The following union describes the accounting (first) chunk of a
 * mapping chunk page.  A union is used to make the index correct
 * for hatptcp without adjustment, but slot 0 is never referenced via
 * hatptcp (it contains the page structure for mc page).
 * The accounting for free chunks is in the page structure for the mc page.
 * (See the phat2 structure for the accounting.)
 */
typedef union hatmpga {
	/* struct page instead of page_t to break cycle with page.h compiles */
	struct page	*hat_mcpgpp;	/* pointer to pp of mc page */
	hatptcp_t hatptcp[HAT_MCPP];	/* pointers to the page table chunks
					 * for the 30 (2-31) mapping chunks
					 * in the page.
					 * Slot 0 & 1 correspond to the
					 * hatmpga data, so they is available
					 * for other use.
					 */
} hatmpga_t;

#define HATMCFULL	0xFFFFFFFF

/*
 * The following union describes a mapping chunk page.  Again,
 * a union is used to make the index correct for hat_mc without adjustment.
 */
typedef union hatmcpg {
	hatmpga_t hat_mcpga;
	hatmc_t hat_mc[HAT_MCPP];
} hatmcpg_t;


/*
 * The following structure describes a page table in terms of its chunks.
 */
typedef struct hatpgt {
	hatpgtc_t hat_pgtc[HAT_MCPP];
} hatpgt_t;

/* The hatpt structure is the machine dependent page table accounting
 * structure internal to the x86 HAT layer.
 * It links an address space to a currently resident page table
 * and the currently resident mapping pointer chunks for that
 * page table.
 * It contains an active PTE count and all locking information.
 * It also contains hatpt pointers for a doubly linked, circular
 * linked list of active page tables for an address space.
 */
typedef struct hatpt {
	struct	hatpt *hatpt_forw;	/* forward page table list ptr */
	struct	hatpt *hatpt_back;	/* backward page table list ptr */
	pte_t	hatpt_pde;		/* PDE for page table */
	pte_t	*hatpt_pdtep;		/* PDT entry pointer */
	hatpgt_t *hatpt_ptva;		/* virtual address of page table */
	struct	as *hatpt_as;		/* pointer back to containing as */
	cnt_t	hatpt_aec;		/* active entry count */
	cnt_t	hatpt_locks;		/* count of locked PTEs */
	cnt_t	hatpt_hec;		/* count of hidden ptes */
	hatmcp_t hatpt_mcp[HAT_MCPP];	/* mapping chunk pointer array */
} hatpt_t;

/*
 * HAT-specific parts of a page structure:
 * struct phat is always valid,
 * struct phat2 is page-private data used only when HAT owns the page
 * (for page table or mapping chunk).
 */

union phat {
	caddr_t mappings;	/* non-HAT form for existence checks */
	struct {
		hatmap_t *_p_hat_mapping;	/*
					 * List of mappings (translations) for
					 * this page; must be zero if no 
					 * mappings.
					 */
		volatile short _p_hat_refcnt; /*
						  * # of contexts that have 
						  * "recently" referenced the 
						  * page. This is only a hint.
						  */
		volatile ushort_t _p_hat_agecnt; /* info. that tells us # of
					      *	times page has been looked at
					      * when aging.
					      */
	} _hatinfo;
};

#define p_hat_mapping	p_hat._hatinfo._p_hat_mapping
#define p_hat_refcnt	p_hat._hatinfo._p_hat_refcnt
#define p_hat_agecnt	p_hat._hatinfo._p_hat_agecnt

struct phat2 {
	union {
		hatpt_t *ptap;
		hatmcpg_t *mcpgp;
	} ph_use;
	hatmc_t *pgmcfree;
	int npgmcfree;
};

#define phat_ptap	ph_use.ptap
#define phat_mcpgp	ph_use.mcpgp

#define PHAT2(pp)	((struct phat2 *)(pp)->p_pgprv_data)

#define HAT_SYNC_THRESH	25	/* used in hat_agerange() */

/*
 * Flags to pass to hat_ptalloc().
 *
 */
#define	HAT_NOSLEEP	0x1	/* return immediately if no memory */
#define HAT_CANWAIT	0x2	/* wait if no memory currently available */
#define HAT_POOLONLY	0x3	/*
				 * Internal flag for pt and chunk alloc.
				 * Attempt to service the request from
				 * the resource free pool only, never
				 * call kpg_alloc() or kmem_zalloc().
				 * Must be used when any page or hat
				 * locks are held.
				 * Used by hat_map and hat_pteload.
				 * Will be used by hat_dup, when it is coded.
				 */
#define HAT_REFRESH	0x4	/*
				 * Allocate the requested resource (NOSLEEP)
				 * and if successful, try to refresh the
				 * free pools (also a NOSLEEP operation).
				 * This allows optimal lock handling in
				 * hat_map and probably hat_dup.
				 */

#define HATMCNOSHFT	17	/* PNUMSHFT + LOG2(HAT_EPMC) */
#define HATMCNOMASK	(HAT_MCPP-1)
#define HATMCNDXSHFT	PNUMSHFT
#define HATMCNDXMASK	(HAT_EPMC-1)
#define MAPMCNOSHFT	7	
#define MAPMCNDXSHFT	2
#define HATMCNO(v)	(((vaddr_t)(v) >> HATMCNOSHFT) & HATMCNOMASK)
#define HATMCNDX(v)	(((vaddr_t)(v) >> HATMCNDXSHFT) & HATMCNDXMASK)
#define HATMAPMCNO(map)	(((vaddr_t)(map) >> MAPMCNOSHFT) & HATMCNOMASK)
#define HATMAPMCNDX(map)(((vaddr_t)(map) >> MAPMCNDXSHFT) & HATMCNDXMASK)
#define HATMCSIZE	(1 << HATMCNOSHFT)
#define HATVMC_ADDR	0xFFFE0000

#endif	/* _KERNEL || _KMEMUSER */

/* some HAT-specific macros: */

#ifdef _KERNEL

/*
 * hat_getshootcookie()
 *	Give the caller the current cookie.
 *
 * Calling/Exit State:
 *	Nothing special, no locking (or much else) involved.
 */

#define hat_getshootcookie()	((TLBScookie_t)lbolt)

/*
 * The following lock is used in conjucntion with the page layer locks
 * due the page layer / hat layer interactions; thus the lock is keyed off
 * the VM_HAT_PAGE_HIER_MIN hierarchy 
 */ 
#define VM_HAT_RESOURCE_HIER	VM_HAT_PAGE_HIER_MIN	/* hat resource lock */
#define VM_HAT_RESOURCE_IPL	VM_INTR_IPL

/*
 * The following locks are used only internal to the hat layer. Thus these
 * locks are keyed off the VM_HAT_LOCAL_HIER_MIN hierarchy.
 */
#define VM_HAT_TLBS_HIER	VM_HAT_LOCAL_HIER_MIN	/* TLBSlock */
#define VM_HAT_TLBS_IPL		VM_INTR_IPL

#define VM_HAT_PTPOOL_HIER	(VM_HAT_LOCAL_HIER_MIN + 5) /* ptpoollock */
#define VM_HAT_PTPOOL_IPL	VM_INTR_IPL

#define VM_HAT_MCPOOL_HIER	VM_HAT_PTPOOL_HIER	/* hat_mcpoollock */
#define VM_HAT_MCPOOL_IPL	VM_INTR_IPL

#if (!VM_INTR_IPL_IS_PLMIN)

#define HATRL_LOCK_SVPL(HATP)	\
	(HATP->hat_lockpl = LOCK(&HATP->hat_resourcelock, VM_HAT_RESOURCE_IPL))
#define HATRL_LOCK(HATP)	\
	LOCK(&HATP->hat_resourcelock, VM_HAT_RESOURCE_IPL)

/* release hat resource lock */
#define HATRL_UNLOCK_SVDPL(HATP)	\
	UNLOCK(&HATP->hat_resourcelock, HATP->hat_lockpl)
#define HATRL_UNLOCK(HATP, PL)	\
	UNLOCK(&HATP->hat_resourcelock, PL)

#define HATRL_TRYLOCK(HATP)	\
	TRYLOCK(&HATP->hat_resourcelock,  VM_HAT_RESOURCE_IPL)

#define HATRL_LOCK_SH(HATP)	\
	LOCK_SH(&HATP->hat_resourcelock,  VM_HAT_RESOURCE_IPL)

#else 	/* VM_INTR_IPL_IS_PLMIN */ 

#define HATRL_LOCK_SVPL(HATP)	\
	(HATP->hat_lockpl = LOCK_PLMIN(&HATP->hat_resourcelock))

#define HATRL_LOCK(HATP)	\
	LOCK_PLMIN(&HATP->hat_resourcelock)

/* release hat resource lock */
#define HATRL_UNLOCK_SVDPL(HATP)	\
	UNLOCK_PLMIN(&HATP->hat_resourcelock, HATP->hat_lockpl)

#define HATRL_UNLOCK(HATP, PL)	\
	UNLOCK_PLMIN(&HATP->hat_resourcelock, PL)

#define HATRL_TRYLOCK(HATP)	\
	TRYLOCK_PLMIN(&HATP->hat_resourcelock)

#define HATRL_LOCK_SH(HATP)	\
	LOCK_SH_PLMIN(&HATP->hat_resourcelock)

#endif	/* !VM_INTR_IPL_IS_PLMIN */

/* check the hat resource lock */
#define HATRL_OWNED(HATP)	LOCK_OWNED(&HATP->hat_resourcelock)

/* from a mapping chunk pointer to the address of mapping chunk */
#define mcptomapp(mcp)	((hatmap_t *)((mcp)->hat_mcp))

/* from a mapping pointer to the page table chunk pointer */
#define mapptoptcp(mapp) ((hatptcp_t *)((vaddr_t)(mapp) & PG_ADDR) \
                                + HATMAPMCNO(mapp))

/* from a mapping pointer to the wasref bitmask pointer for the chunk */
#define mapptowasrefp(mapp) ((uint_t *)((vaddr_t)(mapp) & PG_ADDR) \
				+ HAT_EPMC + HATMAPMCNO(mapp))

/* from a mapping pointer to the page table entry pointer */
#define mapptoptep(mapp) ((pte_t *)(((hatptcp_t *)((vaddr_t)(mapp) & PG_ADDR) \
                          + HATMAPMCNO(mapp))->hat_ptpv & HATPTC_ADDR) \
                          + HATMAPMCNDX(mapp))

/* obtain page table chunk point from ptcp */
#define ptcptoptep(ptcp) (pte_t *)(ptcp->hat_ptpv & HATPTC_ADDR) 

/* For kernel visible mappings */
#define KVIS_MAPTOPTE(MAP)      \
	((pte_t *)((vaddr_t)(MAP) + kas_vis_map_to_pte_delta))

#define KVIS_PTETOMAP(PTEP)     \
	((hatmap_t *)((vaddr_t)(PTEP) - kas_vis_map_to_pte_delta))

/* set mapping chunk pointer. (sets  mcnum, mcndx also)  */
#define SETMAPP(addr, mcnum, mcp, mcndx, ptap) {        \
		mcnum = HATMCNO(addr);  \
		mcndx = HATMCNDX(addr); \
		mcp = (ptap)->hatpt_mcp + mcnum;  \
	}

/* sets ptcp, ptep and mapp */
#define SETPTRS(mcp, mcndx, mapp, ptcp, ptep, ptap) {      \
		hatpgt_t	*pt = (ptap)->hatpt_ptva;	\
					\
		mapp = mcptomapp(mcp);  \
		ASSERT(((ulong_t)mapp & ~HATMC_ADDR) == 0);     \
		ptcp = mapptoptcp(mapp);        \
		ASSERT(ptcp->ptp.hat_mcaec);    \
		ASSERT((uint_t)(ptap)->hatpt_aec >= ptcp->ptp.hat_mcaec);   \
		ptep = pt->hat_pgtc[mcnum].hat_pte;     \
		ASSERT(ptcptoptep(ptcp) == ptep);       \
		mapp += mcndx;  \
		ptep += mcndx;  \
	}

/*
 * INIT_PTAP links the new_ptap into as and reallocates new_ptap and new_mc,
 * if ptap is NULL or ptap not the one we are looking for and sets
 * hatpt_last for chatp.
 */
#define INIT_PTAP(as, hatp, ptap, vpdtep, new_ptap) { \
	if ((ptap) == (hatpt_t *)NULL || (ptap)->hatpt_pdtep != (vpdtep)) {\
		(ASSERT(new_ptap), (new_ptap)->hatpt_pdtep = (vpdtep)); \
		link_ptap((as), (ptap), (new_ptap)); \
		(ptap) = (new_ptap); \
		(new_ptap) = (hatpt_t *)NULL; \
	} else  \
		(hatp)->hat_ptlast = ptap; \
}

/*
 * INIT_MAPP allocates mapping chunk, if neccessary. It also sets variables
 * cmcp, cpt, cptep and cptcp.
 */
#define INIT_MAPP(mcp, pt, mapp, mcnum, new_mc,ptap, ptep, ptcp) {   \
	(mcp) = (ptap)->hatpt_mcp + mcnum; \
	(pt) = (ptap)->hatpt_ptva; \
	if (((mapp) = (hatmap_t *)(mcp)->hat_mcp) == (hatmap_t *)NULL) { \
	    ASSERT(new_mc);  \
	    (mapp) = hat_mcinit((mcp),(pt)->hat_pgtc + mcnum,(new_mc)); \
	    (new_mc) = (hatmc_t *)NULL;  \
	} \
	(ptep) = (pt)->hat_pgtc[mcnum].hat_pte; \
	(ptcp) = mapptoptcp(mapp); \
}

#define FREE_MC(ptap, ptcp, mcp, mapp, addr) { \
	(mcp)->hat_mcp = 0;	\
	ASSERT((ptap)->hatpt_mcp[HATMAPMCNO((ptcp)->hat_ptpv)].hat_mcp == 0);\
	hat_mcfree((mapp)); \
	(addr) = (vaddr_t)(((uint)addr + HATMCSIZE) & HATVMC_ADDR); \
}

#define FREE_PT(ptap, next_ptap, as, addr) { \
	ASSERT((ptap)->hatpt_locks == 0); \
	if ((as) == u.u_procp->p_as) \
	    (ptap)->hatpt_pdtep->pg_pte = (uint)0; \
	hat_zerol1ptes(&(as)->a_hat, (ptap)->hatpt_pdtep, 1); \
	unlink_ptap((as), (ptap)); \
	hat_ptfree((ptap)); \
	(addr) = (vaddr_t)((next_ptap)->hatpt_pdtep - kpd0) * VPTSIZE; \
}

/*
 * Bump the rss size for the as.
 */ 
#ifdef lint
#define BUMP_RSS(as, count) { \
		(as)->a_rss += (count); \
	}
#else	/* lint */
#define BUMP_RSS(as, count) { \
		ASSERT((count) > 0 || (as)->a_rss > 0);	\
		(as)->a_rss += (count); \
	}
#endif	/* lint */

/*
 * boolean_t
 * HAT_REDUCING_PERM(pte, prot)
 *	Determines whether protection is being downgraded for the given pte.
 *
 * Calling/Exit State:
 *	Called with hat_resourcelock held.
 *
 * Remarks:
 *	There are three possible scenarios of downgrading protection:
 *	1) Going from write to read
 *	2) Going from read to prot_none
 *	3) Going from write to prot_none
 */
#define HAT_REDUCING_PERM(pte, prot) \
		(((pte) & PG_RW && !((prot) & PG_RW)) || \
		 ((pte) & PG_V && !((prot) & PG_V)))

/*
 * Bump the active pte count for chunk, and the active pte count for
 * page table.
 */
#ifdef lint
#define BUMP_AEC(ptcp, ptap, count) { \
		(ptap)->hatpt_aec += (count);	\
		(ptcp)->ptp.hat_mcaec += (count);  \
	}
#else 	/* lint */ 
#define BUMP_AEC(ptcp, ptap, count) { \
		ASSERT((count) > 0 || (ptcp)->ptp.hat_mcaec);	\
		ASSERT((count) > 0 || (ptap)->hatpt_aec);	\
		(ptap)->hatpt_aec += (count);	\
		(ptcp)->ptp.hat_mcaec += (count); \
	}
#endif 	/* lint */

#define INCR_LOCK_COUNT(ptap, as) { \
		(ptap)->hatpt_locks++; \
		ASSERT((ptap)->hatpt_locks <= NPGPT); \
		(as)->a_lockedrss++; \
	}

#define DECR_LOCK_COUNT(ptap, as) { \
		ASSERT((ptap)->hatpt_locks > 0); \
		--(ptap)->hatpt_locks; \
		ASSERT((as)->a_lockedrss != 0); \
		(as)->a_lockedrss--; \
	}

/*
 * boolean_t
 * CAN_LOAD_RDONLY(hatp)
 *	Determines whether current context can load a read-only translation
 *	into the hat.
 *
 * Calling/Exit State:
 *	Called with hat_resourcelock held.
 *
 * Remarks:
 *	This is only required for supporting the lack of write protection
 *	check in kernel mode on the Intel386(tm) processor. Can evaluate to
 *	TRUE unconditionally if such support is not needed.
 */
#define	CAN_LOAD_RDONLY(hatp) \
		(((hatp)->hat_rdonly_owner == NULL) || \
		 ((hatp)->hat_rdonly_owner == u.u_lwpp))

/* 
 * void
 * HAT_RDONLY_LOCK(struct hat *hatp)
 *
 *	Establish a pseudo lock on the hat so that no new read_only mappings 
 *	are loaded. This is done by writing the LWP pointer into 
 * 	hat_rdonly_owner. 
 *
 * Calling/Exit State:
 *	Called with hat_resourcelock held. Returns with the same held.
 *	May drop the lock and reacquire it, if it needs to block (if some 
 *	other LWP has acquired the read-only lock.) No other locks held
 *	on entry.
 *
 * Remarks:
 *	Let us call the LWP that writes its pointer in hat_rdonly_owner 
 *	as the "owner" of the pseudo lock.
 *	The pseudo locking relies on the protection given by the hat
 *	resource lock due to the following protocol. Any LWP intending to
 *	load a read-only mapping into the hat is required to check that
 *	either no LWP owns the pseudo lock, or that it itself owns the
 * 	pseudo lock. If the above condition is met, then it proceeds to
 *	load the read only mapping. The check and the action of loading are
 *	performed under the coverage of the hat resource lock. Similarly,
 *	the ownership and release of the pseudo lock are accomplished
 *	under protection of the hat_resourcelock.
 */
#define	HAT_RDONLY_LOCK(hatp) {					 \
	while (!CAN_LOAD_RDONLY(hatp)) 	{			 \
		SV_WAIT(&(hatp)->hat_rdonly_sv, PRIMEM,		 \
			&(hatp)->hat_resourcelock);		 \
		HATRL_LOCK_SVPL(hatp);				 \
	}							 \
	(hatp)->hat_rdonly_owner = u.u_lwpp;			 \
}

/* 
 * void
 * HAT_RDONLY_UNLOCK(struct hat *hatp)
 *	Release the pseudo lock on the hat, so that loading of READ-ONLY
 *	translations is permitted. 
 *
 * Calling/Exit State:
 *	Hat resource lock held across the call.
 */

#define	HAT_RDONLY_UNLOCK(hatp) {				 \
	HATRL_LOCK_SVPL(hatp);					 \
	(hatp)->hat_rdonly_owner = NULL;			 \
	if (SV_BLKD(&(hatp)->hat_rdonly_sv)) {			 \
		HATRL_UNLOCK_SVDPL(hatp);			 \
		SV_BROADCAST(&(hatp)->hat_rdonly_sv,0);	 	 \
	} else {						 \
		HATRL_UNLOCK_SVDPL(hatp);			 \
	}							 \
}


enum hat_cont_type {
	NEXTMC,		/* process next mapping chunk */
	ALLOCPT,	/* allocate page table and mc pages */
	NOMORE		/* all done, exit */
};

enum hat_next_type {
	NEXT_MC,		/* process next mapping chunk */
	NEXT_PT,		/* process next page table */
	NEXT_ADDR	/* continue with next addr */
};

/*
 * 		Notes on synchronization needed for local aging 
 *			of address spaces 
 *
 * One way to ensure that in a multithreaded address space, a single LWP
 * can undertake the address space aging step without causing other LWPs
 * to experience MMU inconsistencies is to deactivate them while the
 * aging step is in progress; this is accomplished via the 
 * vm_seize()/vm_unseize() interfaces with the process/LWP subsystems.
 *
 * However, it may be possible to support the local aging of address spaces 
 * without requiring that all LWPs in the address space be seized. 
 * This can happen, trivially, if there is only one LWP in the address 
 * space --i.e., the one that is aging the AS.
 *
 * When there are more than one LWPs in the AS, it is still possible,
 * depending upon processor MMU characteristics, to proceed with local
 * aging without arresting other LWPs. The requirement is that an MMU
 * access that can cause the MODIFY bit to become set also generates
 * a write-through update of the non-cached (i.e., not cached in MMU)
 * copy of the translation. (It is assumed that should this update fail
 * to find the translation valid, full fault processing would be
 * initiated on the missing translation -- eventhough the MMU itself
 * had not registered a miss immediately prior to the update. 
 *
 * The property described above permits the asynchronous removal of
 * page table entries provided the agent doing the removal holds the
 * page tables locked. Any concurrent modifying accesses by other
 * LWPs would be held off due to the need for full fault processing.
 * Note that removal of the entries by itself does not yet free the
 * associated pages; the page freeing is deferred to occur until after
 * TLB entries are globally removed. 
 *
 * In the asynchronous aging approach, the LWP that performs the aging step
 * must hold the address space read locked in order to guarantee segment
 * chain consistency. 
 *
 * This approach of not deactivating (not vm_seize'ing) the LWPs during
 * local aging trades off the vm_seize overhead against the possiblility
 * of busy-waiting that can result on other processors if the other LWPs 
 * fault and then busy wait for hat lock(s).
 */

extern	boolean_t		local_age_lock();
#define	LOCAL_AGE_LOCK(procp)	local_age_lock((procp))
#define	LOCAL_AGE_UNLOCK(procp)	as_unlock((procp)->p_as)


/*
 * Functions/macros used by vm_hat.c, vm_hatstatic.c, and/or pse_hat.c
 */
void hat_vprotinit(void);

#define hat_vtop_prot(prot)	vprot_array[(prot) & PROT_ALL]

/*
 * to get to per engine level 1 table at constant virtual:
 */
#define kpd0	((pte_t *)KVENG_L1PT)

extern uint_t vprot_array[];

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VM_HAT_H */
