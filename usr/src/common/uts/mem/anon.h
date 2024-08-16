/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_ANON_H	/* wrapper symbol for kernel use */
#define _MEM_ANON_H	/* subject to change without notice */

#ident	"@(#)kern:mem/anon.h	1.23.1.20"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifdef _KERNEL_HEADERS

#include <fs/vnode.h>		/* REQUIRED */
#include <mem/memresv.h>	/* REQUIRED */
#include <mem/seg.h>		/* REQUIRED */
#include <mem/swap.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vnode.h>		/* REQUIRED */
#include <vm/memresv.h>		/* REQUIRED */
#include <vm/seg.h>		/* REQUIRED */
#include <sys/swap.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * ANONFS
 *
 * Description: essentially a virtual filesystem used to lend identity and
 * backing storage to anonymous pages. A vnode of this filesystem is called
 * an ``anode''.  Anodes are purely synthetic objects, in the sense that they
 * exist only for the purpose of lending identity to anonymous pages. Anodes
 * are not associated with the swap devices in any fashion. An anode can
 * lend its identity to as many as LONG_MAX/PAGESIZE pages. Thus, a medium
 * sized system with several swap devices may still have only a single anode.
 *
 * Unlike previous versions of SRV4, the anon structures are now allocated
 * a page at a time, as the demand is presented to anonfs by anon_alloc().
 * Once allocated, the anon structures can never be deleted.
 * 
 * When a swap device is added to the swap subsystem, it is necessary for the
 * anon layer to know about it for several reasons:
 *
 * (a) So that additional M_SWAP reservations can be made available to
 *     anonfs's clients, and
 *
 * (b) So that anonfs can reserve the ``real'' memory pages, and the kernel
 *     virtual memory space, which it might later need if it has to allocate
 *     the anon_t(s).
 *
 * swapadd() transmits this information to anonfs by through the
 * anon_add_swap() and anon_add_swap_resv() interfaces.
 *
 * A bitmap is allocated with the swapinfo structure to represent each page
 * of storage. The vp/offset passed in as part of the swapadd request is used
 * as the basis of paging but not to establish page identity. Once again, the
 * anodes are used for this latter purpose.
 * 
 * It is very important to note that anonymous pages are not assigned backing
 * storage locations until the very last moment before they are pushed. This
 * frees us to group and push dirty pages together that are from otherwise
 * disparate anodes and at disparate virtual offsets. Any I/O subsystem
 * capable of ``gather'' I/O processing can take advantage of this dynamic
 * regrouping.
 * 
 * PERF: This version of anon klusters pages when writing to backing store.
 *	 It is not capable of klustering when reading from backing store,
 *	 even when ``logically related'' pages are adjacent on the backing
 *	 store. There are two important cases of ``logically related'' to
 *	 consider here: (1) pages with sequential offset address within a
 *	 segment, and (2) the U block pages for a single process.
 * 
 * Locking considerations:
 * 
 * 	The following locks are used by anonfs (listed in acquisition order):
 * 
 * 	anon_table_lck	A single global sleep lock which guards the
 * 			anon_table and associated variables.
 * 
 * 	swap_lck	A single global spin lock guarding the swap allocation
 * 			structures. This lock is also used to guard the
 * 			``anondoubles'' variable.
 * 
 * 	anon_free_lck	A single global fast spin lock guarding the free
 * 			list of anon structures, plus the variable
 * 			``anon_tabinuse''. ``anon_tabinuse'' is also
 * 			guarded by the anon_table_lck, so that a reader can
 * 			hold either of these locks, but a writer must hold
 * 			both.
 * 
 * 	an_ref_lck	A single global fast spin lock guarding the an_ref_cnt
 * 			fields of all anon structures.
 * 
 * 	vm_memlck	A single global fast spin lock which guards various
 * 			memory reservation information, including anoninfo.
 * 			This lock is owned by memresv.c.
 * 
 * External Lock Ordering Considerations:
 * 
 * 	No spin locks are held on entry to any anon function except in the
 *	following case:
 * 
 * 	    When page_hashout() calls anon_hashout(), both VM_PAGEIDLOCK and
 *	    the PAGE_USELOCK are held. anon_hashout() acquires both the
 *	    an_ref_lck and the swap_lck.
 * 
 * The nominative lock acquisition ordering is given in the following diagram:
 *
 *							     +-> vm_memlck
 *							     |
 * anon_table_lck -> p_uselock -> vm_pageidlock -> swap_lck -+-> anon_free_lck
 *							     |
 *							     +-> an_ref_cnt
 */

/*
 * ANODE: a vnode to give identity to anonymous pages
 */

typedef struct anode {
	vnode_unnamed_t	ano_vno;	/* vnode for this anode */
} anode_t;

/*
 * ANON: locus of activity for segment drivers using anonymous pages
 *
 * Central to the concurrency control of this data structure is the
 * ANON_T_LOCK pseudo lock. Let <vp,off> be the identity of anon structure
 * *ap (given by anon_antovp(ap)). Then the ANON_T_LOCK is held for ap if any
 * one of the following conditions are true:
 *
 * 	(1) There is no page in the page cache with identity
 * 	    <vp,off> and the holder of ANON_T_LOCK is enforcing
 * 	    this fact.
 *
 * 	(2) The page WRITE lock is held for the page with identity
 * 	    <vp,off>.
 *
 *	(3) The pageout lock is held (p_pageout bit set) for the page with
 *	    identity <vp,off>.
 *	
 *	(4) The PAGE_USELOCK is held for the page with identity <vp,off>,
 *	    and the page is not WRITE locked, and p_pageout is not set.
 *
 * Note that these four conditions are mutually exclusive. Condition (1)
 * can be enforced by anon if the page is aborted and *ap is free.
 * Condition (2) is mututally exclusive with condition (3) because the
 * contents of a WRITE locked page are invalid, so that it cannot be in
 * the process of being cleaned to disk.
 *
 * The fields of this structure are protected as follows:
 *
 *	The following fields are protected by the ANON_T_LOCK:
 *
 *		an_swaploc, an_page
 *
 *	The following field is partially protected by the swap_lck, in the
 *	sense that the swap_lck is held (in addition to the ANON_T_LOCK)
 *	when swap space is assigned. However, the swap_lck is not held when
 *	swap space is freed.
 *
 *		an_swaploc
 *
 *	The following field (within a union) is only accessed when the
 *	structure is allocated (an_refcnt != 0), and thus is indirectly
 *	protected by the an_ref_lck.
 *
 *		an_page
 * 
 *	The following field is protected by the global an_ref_lck:
 *
 *		an_refcnt
 *
 *	The following field is protected by the global anon_hist_lock:
 *
 *		an_history
 *
 *	The following field (within a union) is only accessed when the
 *	structure is free (an_refcnt == 0). Thus, this field is indirectly
 *	protected by the an_ref_lck.
 *
 *		an_next
 *
 *	The following field is constant for the life of the structure:
 *
 *		an_vidx
 *
 * Also, the following fields in the page structure are protected by
 * the ANON_T_LOCK, when the page has anon identity:
 *
 *		ANON_PGPRV(pp)->a_ap
 *		ANON_PGPRV(pp)->a_next
 *
 * If you have trouble understanding this abomination, take heart. You are
 * not alone!
 */

typedef struct anon {
	swaploc_t	an_swaploc;	/* location of backing store */
	uchar_t		an_vidx;	/* index into anon cluster table */
	ushort_t	an_refcnt;      /* count of users of this page */
	union {
		struct page	*uan_page;       /* the real page */
		struct anon	*uan_next;       /* free list pointer */
	} an_ptr;
#ifdef ANON_DEBUG
	uint_t		an_history;     /* encoded history information */
#endif
} anon_t;

#define an_page		an_ptr.uan_page
#define an_next		an_ptr.uan_next

/*
 * anon_cluster: an entry in the anonfs cluster table.
 *
 *	All fields are constant for the entire life of the structure.
 *	Therefore, no mutexing is required.
 */
typedef struct anon_cluster {
	ulong_t		ancl_pages;	/* current number of anon_t(s) */
	ulong_t		ancl_max;	/* max possible anon_t(s) */
	off_t		ancl_base;	/* offset for the first anon_t */
	anode_t		*ancl_anodep;	/* anode pointer */
	anon_t		*ancl_anonp;	/* pointer to the anon_t array */
} anon_cluster_t;

#ifdef ANON_DEBUG
/*
 * history values (max value can be 15 as these values are stored in 4 bits)
 */
#define ANON_H_NONE		0	/* no value */
#define ANON_H_CREATE		1	/* created by anon_alloc() */
#define ANON_H_DECREF		2	/* anon_decref() called */
#define ANON_H_GETPAGE		3	/* found in memory by anon_getpage() */
#define ANON_H_GETGETPAGE	4	/* read from disk by getpage() */
#define ANON_H_HASHOUT		5	/* page discarded by anon_hashout() */
#define ANON_H_HASHOUT_FREE	6	/* swap freed by anon_hashout() */
#define ANON_H_HASHOUT_DBL	7	/* doubles broken by anon_hashout() */
#define ANON_H_SWFREED		8	/* swap freed by anon_freeswap() */
#define ANON_H_GETBRK		9	/* anon_getpage() breaks doubles */
#define ANON_H_RECBRK		10	/* page_anonreclaim() breaks doubles */
#define ANON_H_PAGEFREESWAP	11	/* page_free_l() frees swap space */
#define ANON_H_PGO_SWP		12	/* anon_pageout() allocates swap */
#define ANON_H_RECLAIM		13	/* anon_exists_uselock() reclaims */
#define ANON_H_RELOCATE		14	/* anon_relocate() called */
#define ANON_H_RELOCATE_GET	15	/* anon_relocate() does getpage */

/*
 * macro to save some history in an anon_t
 */
#define ANON_SAVE_HIST(ap, h)	anon_save_history(ap, h)

#else	/* !ANON_DEBUG */

#define ANON_SAVE_HIST(ap, h)

#endif	/* !ANON_DEBUG */

/*  
 * ANONINFO: anonfs accounting structure
 *
 * 	All fields are mutexed by vm_memlock (see memresv.c).
 */

typedef struct anoninfo {
	uint_t	ani_user_max;	/* maximum virtual swap pages available for */
				/* user lockdown */
	uint_t	ani_dkma_max;	/* maximum virtual swap pages available for */
				/* discretionary kma use */
	uint_t	ani_max;	/* maximum virtual swap pages available for */
				/* normal use */
	uint_t	ani_kma_max;	/* maximum virtual swap pages available for */
				/* kma use */
	uint_t	ani_resv;	/* number of virtual swap memory pages */
				/* reserved */
} anoninfo_t;

#ifdef _KERNEL
extern anoninfo_t anoninfo;		/* anonfs reservation information */
extern ulong_t anon_allocated;		/* anon_t(s) allocated */
extern fspin_t anon_free_lck;		/* free list lock */
extern anon_cluster_t *anon_table[];	/* the naon cluster table */
extern vnodeops_t anonfs_ops;		/* vnode ops vector for anonfs */

/*
 * Some anonfs constants
 */
#define ANON_MAXOFFSET		((LONG_MAX & PAGEMASK) - PAGESIZE)
#define ANON_TABSIZE		(1 << NBBY)

/*
 * The following shush the ANSI compiler about the function templates
 * below.
 */

struct page;
struct cred;
struct swapinfo;
struct vnode;

/*
 * anon_pgprv: embedded in page structure for management of linked lists of
 *        pages during anon_pageout when we need to break requests up
 *	  into smaller pieces (a_next). Also contains an anon pointer 
 *	  (a_ap) used to go from page to anon structs.
 */

struct anon_pgprv {
	struct page	*a_next;
	anon_t		*a_ap;
};

#define ANON_PGPRV(pp)	((struct anon_pgprv *)(pp)->p_pgprv_data)

typedef enum anon_delete { ANON_SWAP_DELETE, ANON_ADD_BACKOUT } anon_del_t;

void anon_antovp(const anon_t *ap, struct vnode **vpp, off_t *offp);
void anon_conf(uint_t size, uint_t kma_resv, uint_t pages_pp,
	       uint_t pages_ukma, void *space, uint_t space_size);
anon_t *anon_alloc(void);
void anon_decref(anon_t *an);
int anon_dup(anon_t **old_ptr, anon_t **new_ptr, uint_t size);
void anon_free(anon_t **app, uint_t size);
int anon_getpage(anon_t **app, uint_t *protp, struct page *pl[], uint_t plsz,
	         enum seg_rw rw, struct cred *cred);
struct page *anon_private(anon_t **app, const struct page *opp, mresvtyp_t mtype);
void anon_bind(anon_t **app, struct page *pp);
struct page *anon_zero(anon_t **app, mresvtyp_t mtype);
struct page *anon_zero_aligned(anon_t **app, paddr_t start_align, paddr_t start_off, mresvtyp_t mtype);
void anon_freeswap(anon_t *ap, boolean_t breakdouble);
void anon_hashout(struct page *pp);
int anon_pageout(int pgs, const clock_t stamp, const clock_t interval);
boolean_t anon_add_swap(ulong_t npages);
void anon_del_swap(ulong_t npages, uchar_t ridx, anon_del_t type);
void anon_add_swap_resv(ulong_t npages);
boolean_t anon_del_swap_resv(ulong_t npages);

#ifdef ANON_DEBUG
extern void anon_save_history(anon_t *, uint_t);
#endif

#endif /* _KERNEL */

/*
 * flags to anon_zero, anon_private and anon_alloc
 */

#define AN_LOCK_PAGE    0x1	/* [Original] page is M_REAL_USER locked */
#define AN_LOCK_KPAGE	0x2	/* [Original] page is M_REAL locked */

/*
 * Utility macros 
 */

/*
 * boolean_t
 * IS_ANONVP(vnode_t *vp)
 *	Determine if a vnode is an anode.
 *
 * Calling/Exit State:
 *	Returns TRUE if the argument vnode is an anode and FALSE
 *	otherwise.
 */
#define IS_ANONVP(vp) \
	((vp)->v_op == &anonfs_ops)

/*
 * boolean_t
 * IS_ANON_PAGE(page_t *pp)
 *	Determine if a page is anonymous.
 *
 * Calling/Exit State:
 *	Returns B_TRUE is the identity of the argument page is given by
 *	an anode and B_FALSE otherwise.
 */
#define IS_ANON_PAGE(pp) \
	((pp)->p_vnode && IS_ANONVP((pp)->p_vnode))

/*
 * void
 * ANON_TAB_LOCK(void)
 *	Aquire the anon table lock.
 *
 * Calling/Exit State:
 *	The caller is executing at base ipl and hold no LOCKs.
 *	Returs the same way.
 */
#define ANON_TAB_LOCK() ( \
	ASSERT(KS_HOLD0LOCKS()), \
	ASSERT(getpl() == PLBASE), \
	SLEEP_LOCK(&anon_table_lck, PRIMEM - 1) \
)

/*
 * void
 * ANON_TAB_UNLOCK(void)
 *	Release the anon table lock.
 *
 * Calling/Exit State:
 *	None.
 */
#define ANON_TAB_UNLOCK() \
	SLEEP_UNLOCK(&anon_table_lck)

/*
 * vnode_t *
 * ANODE_TO_VP(anode_t *anp)
 *	Return the vnode for a given anode.
 *
 * Calling/Exit State:
 *	Returns a pointer to the requested vnode.
 */
#define ANODE_TO_VP(anp) \
	((vnode_t *)&(anp)->ano_vno)

/*
 * anon_pptoan(page_t *pp)
 *	Convert a page pointer to an anon pointer.
 *
 * Calling/Exit State:
 *	Caller must insure that the page in question is anonymous
 *	and IN_USE or USELOCK()ed. These conditions are not asserted.
 */
#define anon_pptoan(pp)	(ANON_PGPRV(pp)->a_ap)

/*
 * boolean_t
 * ANON_HAS_SWAP(anon_t *ap)
 *	Determine if an anonymous page has swap allcoated.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if swap is allocated and B_FALSE otherwise.
 */
#define ANON_HAS_SWAP(ap)	(SWAPLOC_HAS_BACKING(&(ap)->an_swaploc))

/*
 * void
 * anon_antovp(const anon_t *ap, vnode_t **vp, off_t *offp)
 *	Convert an anon pointer into its corresponding virtual
 *	vp/off pair.
 *
 * Calling/Exit State:
 *	The vp and off pointed to by the caller are set as outargs.
 */
#define anon_antovp(ap, vpp, offp) { \
	anon_cluster_t *clp = anon_table[(ap)->an_vidx]; \
 \
	ASSERT(clp); \
	*(vpp) = ANODE_TO_VP(clp->ancl_anodep); \
	ASSERT((ap) >= clp->ancl_anonp); \
	ASSERT((ap) < clp->ancl_anonp + clp->ancl_pages); \
	*(offp) = clp->ancl_base + ptob((ap) - clp->ancl_anonp); \
}

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_ANON_H */
