/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_PAGE_H	/* wrapper symbol for kernel use */
#define _MEM_PAGE_H	/* subject to change without notice */

#ident	"@(#)kern:mem/page.h	1.85"
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

#ifdef _KERNEL_HEADERS

#include <mem/anon.h>	/* REQUIRED */
#include <mem/mem_hier.h> /* REQUIRED */
#include <mem/vm_hat.h> /* REQUIRED */
#include <mem/vmparam.h> /* REQUIRED */
#include <svc/systm.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <util/param.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/param.h> /* REQUIRED */
#include <sys/systm.h>	/* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <sys/vmparam.h> /* REQUIRED */
#include <vm/anon.h>	/* REQUIRED */
#include <vm/vm_hat.h> /* REQUIRED */
#include <vm/mem_hier.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined _KERNEL || defined _KMEMUSER

/*
 * struct page describes a physical page frame state and contents.
 * -----------
 *
 * Overview:
 *
 * Each physical page has a page structure, which is used to maintain
 * these pages as a cache.  A page can be found via a hashed lookup based
 * on the [vp, offset].  If a page has a [vp, offset] identity, then it
 * is entered on a doubly linked circular list off the vnode (v_pages) using
 * the vpnext/vpprev pointers.   If the p_free bit is on, then the page is
 * also on a doubly linked circular free list using the p_next/p_prev pointers.
 * If the p_pageout bit is on, then the page is currently being written to
 * backing store.  If the p_invalid bit is on, then the page is being
 * filled in with valid data (usually by reading from backing store).  In
 * the case of such I/O, the p_next/p_prev pointers are used to link the pages
 * together for a consecutive I/O request (and p_free must be off).
 * The p_next/p_prev pointers can be used for other lists (only if p_free
 * is off and there is no I/O in progress on the page).
 * 
 * Detail:
 *
 * Each struct page has a pseudo reader/writer sleeplock associated with it.
 * A standard reader/writer sleeplock is not used here because the conditions
 * for lock aquisition involve page-specific state information in addition to
 * the standard number of current readers and writers.
 *
 * p_activecnt/p_invalid indicate the presence of reader/writer locks for
 * the page:
 *
 *	p_activecnt  p_invalid
 *	-----------  ---------
 *	     0		0	No locks
 *	     0		1	INVALID LOCK STATE ("can't happen")
 *	    >0		0	Reader (shared) lock(s) held.
 *	     1		1	Writer (exclusive) lock held.
 *	    >1		1	INVALID LOCK STATE ("can't happen")
 *
 * A reader lock is acquired when the caller needs to ensure that the
 * page state, identity (vnode, offset), and data are valid and remain so.
 *
 * A writer lock is acquired when the callers wishes to change the
 * page state, identity, or data contents.
 *
 * When a reader/writer lock cannot be immediately acquired, the caller
 * waits on the p_wait synchronization variable via PAGE_WAIT(pp).
 *
 * All waiters on p_wait are awoken via PAGE_BROADCAST(pp) whenever:
 *	- p_pageout is cleared
 *	- p_invalid is cleared (drop writer lock or downgrade to reader lock)
 *	- p_activecnt is decremented to zero
 *
 * Note that this procedure does not grant page reader/writer locks in
 * any particular order; all waiters race to get the lock.  Thus it is
 * possible for lock contenders (especially writers) to starve.
 *
 * Page reader/writer locks are held to guarantee kernel correctness of
 * operation.  (I.e., the lock holder would panic or Do Bad Things if the
 * lock were subverted.)
 *
 *
 *
 * Fields in struct page are mutexed by a collection of locks including
 * the global VM_PAGEIDLOCK() and VM_PAGEFREELOCK() spinlocks, the p_uselock
 * spinlock, and the page reader/writer lock.  (Note that vm_pagefreelock,
 * vm_pageidlock and p_uselock should always be manipulated via their
 * appropriate macros, never directly. This is because the priority levels 
 * associated with these locks are cached since the locking and the
 * unlocking agents can be in different functions. See, e.g.,
 * VM_PAGEFREELOCK() , VM_PAGEIDRDLOCK() and PAGE_USELOCK().)
 *
 * - All fields are mutexed by p_uselock except as described below.
 *
 * - p_next/p_prev are mutexed by the appropriate lock for whichever
 *   page list is currently using p_next/p_prev.  (E.g., VM_PAGEFREELOCK()
 *   when used by the free lists; or implicitly locked when used on
 *   private I/O lists.)
 *
 * - p_nio is used exclusively by the current logical I/O operation in
 *   progess on the page.  On file systems where the block size is
 *   smaller than the page size, there may be multiple concurrent 
 *   physical I/O operations using the page to accomplish the single
 *   logical I/O operation.
 *
 *   When p_nio is set by VOP_{GET,PUT}PAGE to indicate the number of
 *   concurrent physical I/O operations there will be, there is no mutexing
 *   required since logical I/O operations are themselves serialized
 *   and p_nio is set prior to initiating the physical I/O operations.
 *
 *   When p_nio is decremented by pvn_done(), p_uselock is used to mutex it.
 *   This is because multiple physical I/O operations could be simultaneously
 *   completing on the page.
 *
 * - Modification of the page identity fields requires holding both
 *   the VM_PAGEIDLOCK() and PAGE_USELOCK(pp) while holding a page writer
 *   lock. Thus holding either VM_PAGEISLOCK() or PAGE_USELOCK(pp) is
 *   sufficient to read the following fields:
 *		p_vnode, p_offset, p_hash, p_vpnext, p_vpprev.
 *
 * - Modification of the p_free field requires holding both
 *   VM_PAGEFREELOCK() and PAGE_USELOCK(pp) while holding a page writer lock.
 *   Thus holding either VM_PAGEFREELOCK() or PAGE_USELOCK(pp) is sufficient
 *   to read the p_free bit.
 *
 * - p_pgprv_data is controlled by the "owner of the page identity".  That is,
 *   if the page has vnode identity, p_pgprv_data is controlled by that vnode's
 *   filesystem (including anonfs if it's an anon page); otherwise, if the
 *   page is in use exclusively by one agent (e.g. by HAT as a page table),
 *   p_pgprv_data is controlled by that agent.  Any necessary mutexing within
 *   use by the controlling agent is the responsibility of that agent.
 *
 * - p_hat is mutexed by the hat layer as it sees fit with the exception
 *   of p_mapping which is mutexed by p_uselock.
 */

#define PGPRVSZ	4	/* # of words of use-private data */

struct pageheader {	/* separate so marker pages don't need whole page_t */
	struct page	*pgh_vpnext;	/* next page in vnode list */
	struct page	*pgh_vpprev;	/* prev page in vnode list */
	struct vnode 	*pgh_vnode;	/* logical vnode this page is from */
	off_t		 pgh_offset;	/* offset into vnode for this page */
};
#define p_vpnext	p_pghdr.pgh_vpnext
#define p_vpprev	p_pghdr.pgh_vpprev
#define p_vnode		p_pghdr.pgh_vnode
#define p_offset	p_pghdr.pgh_offset

typedef struct page {

	/*
	 * Page Identity Information:
	 *	Mutexed by VM_PAGEIDLOCK(), PAGE_USELOCK(pp) and page
	 *	writer lock:
	 */

	struct pageheader p_pghdr;	/* vnode identity info */
	struct page	*p_hash;	/* hash by [vnode, offset] */

	/* Mutexed by appropriate page list lock: */

	struct page	*p_next;	/* next page in free/intrans list */
	struct page	*p_prev;	/* prev page in free/intrans list
					   (max value is # fileblocks/page) */

	/* Mutexed by p_uselock (except p_free; see above): */

	ulong_t		 p_activecnt;	/* number of reader/writer locks held
					      (p_invalid implies writer lock) */

	clock_t		 p_timestamp;	/* time page was found to be dirty */

	sv_t		 p_wait;	/* to wait for page state change */
	lock_t		 p_uselock;	/* mutex for most struct page fields */


	/* Mutexed by p_uselock or the current I/O operation in progress: */

	uchar_t		 p_nio;		/* number of outstanding I/O reqs */

	/* Mutexed by p_uselock: */
			 /*
			  * The following two fields are only used if
			  * restricted DMA support is compiled into the
			  * kernel and rdma_mode != RDMA_DISABLED.
			  */
	uint_t		 p_type: 2,	/* STD_PAGE, DMA_PAGE, or PAD_PAGE */
			 p_physdma: 1,	/* page is physically DMAable */
			 /*
			  * End of restricted DMA support.
			  */

			 p_pageout: 1,	/* data for page is intransit out */
			 p_invalid: 1,	/* data invalid; e.g. being paged in */
			 p_mod: 1,	/* software copy of modified bit */
			 p_ioerr: 1,	/* I/O error when p_nio > 1 */
			 p_free: 1;	/* page is on a clean/dirty free list */

	/* No mutexing, because initialized once, then read-only: */

	uchar_t	 	 p_chidx;	/* cached pagepool chunk # */
	uint_t		 p_pfn;		/* cached pfn, to speed page_pptonum */

	/* p_mapping mutexed by p_uselock: */

	union phat	 p_hat;		/* HAT-specific structure.
					   This structure is for those fields
					   which must be valid for all pages
					   and can't use the page-private data.
					   One field, in particular, must be
					   part of this structure: mappings
					   (referred to as p_mapping),
					   whose exact semantics are hat-
					   specific, but which must always,
					   generically, be non-zero if the page
					   has any translations mapped to it,
					   and zero otherwise. */

	/*
	 * p_pgprv_data mutexing controlled by "owner of page identity".
	 * If not exclusively owned, p_pgprv_data is not used.
	 */

	ulong_t	 	p_pgprv_data[PGPRVSZ];	/* use-private data */

	lid_t		p_lid;
} page_t;

#define p_mapping	p_hat.mappings

#endif /* _KERNEL || _KMEMUSER */


/*
 * VM_PAGEIDLOCK(), VM_PAGEFREELOCK() & PAGE_USELOCK(pp) Lock Acquisition
 * Order:
 * --------------------------------------------------
 *
 * When all these locks need to be concurrently held, PAGE_USELOCK(pp) must be
 * acquired first, then the VM_PAGEIDLOCK() and lastly the VM_PAGEFREELOCK().
 *
 * The motivation for choosing this order is as follows:
 *
 * There are different parts of the VM system which would like
 * different orders.  (E.g., page allocation wants to acquire
 * VM_PAGEFREELOCK first and then PAGE_USELOCK(pp) for the allocated
 * page; however, page deallocation wants the other order.
 *
 * So we had to choose one.  It seemed better to first acquire the finer
 * grain lock (PAGE_USELOCK(pp)) and hold it while spinning for the coarser
 * grain lock (VM_PAGEIDLOCK() or VM_PAGEFREELOCK()); this would result in
 * less hold time (and hence less lock contention) for the coarser grain 
 * locks (VM_PAGEIDLOCK() and VM_PAGEFREELOCK()).
 *
 * Callers may NOT concurrently hold PAGE_USELOCK(pp) for multiple pp's.
 *
 * The motivation for this is simple deadlock avoidance.
 * This rule is assumed by PAGE_USELOCK() when it caches the current
 * ipl in l.puselockpl which is only unique per-processor.
 *
 *
 * There is also a decision to be made regarding the locking order
 * between the VM_PAGEIDLOCK() and the VM_PAGEFREELOCK(). The following are
 * the code paths where both locks need to be acquired:
 * 1)  page_lookup_or_create() would like to hold the page id lock
 *     first and if it couldn't find a page with the right identity, 
 *     get the page free list lock to call page_get_l().
 * 2)  But page_get_l() itself would like to acquire the free list lock
 *     first and THEN the id list lock; this is because if it found a
 *     page on the free list which still has vnode identity, it needs to
 *     hashout the page from the vp list and this would require the id
 *     list lock.
 * 3)  page_unfree_l() also needs to acquire the id list lock
 *     while holding a free lock if it races with page_get_l(); if
 *     page_get_l() gets there first, then it needs to hash out this
 *     page from this identity	so that the caller could get a new page.
 * 4)  hat_map() pre-loads translations for pages associated with a
 *     vnode and if it finds the page is on the free list, it reclaims the
 *     page from the free list. Thus it would like to acquire the id lock
 *     first and the free list lock second.
 *
 * We decided to require that the id list lock be acquired BEFORE the 
 * free list lock since cases 2 & 3 are infrequent and we optimize for
 * high runners (1 & 4).
 */

/*
 * Acquiring/releasing VM_PAGEFREELOCK() and PAGE_USELOCK out of order:
 * ------------------------------------------------------------
 *
 * In cases where it is necessary to acquire VM_PAGEIDLOCK() or
 * VM_PAGEFREELOCK() before (try locking) PAGE_USELOCK:
 *
 *	The ipl value associated with both the VM_PAGEIDLOCK() and the
 *	VM_PAGEFREELOCK() is VM_PAGE_IPL and this could be potentially
 *	different from 	the ipl value associated with the page uselock. 
 *	Because of this, the VM_PAGEFREELOCK & PAGE_USELOCK(pp)
 *	routines cannot be used because they hardcode the ipl values and
 *	assume the lock order will guarantee that the ipl level will not be
 *	inappropriately weakened. In this case, explicit calls to TRYLOCK()
 *	and UNLOCK() must be used.  (See below.)
 *
 * In cases where it is necessary to release these locks out of order
 * with respect to their acquisition order:
 *
 *	Because the ipl values active at lock acquisition time are opaquely
 *	cached by the VM_PAGEFREELOCK() & PAGE_USELOCK(pp) routines, the
 *	caller of these routines must release the locks in the reverse
 *	order they were acquired to restore the correct ipl levels.
 *
 *	When the caller cannot follow this order, and the caller will
 *	release both locks, the caller can explicitly call UNLOCK()
 *	passing the appropriate ipl value.
 *
 *	When the caller cannot follow this order, and the caller will
 *	NOT release both locks, the caller explicitly calls UNLOCK()
 *	for the lock it is releasing (passing the appropriate ipl level)
 *	and then updates the opaquely cached ipl value for the other
 *	lock so that a subsequent VM_PAGEFREEUNLOCK()/PAGE_USEUNLOCK(pp)
 *	will retore the correct ipl level.
 *
 *
 * EXAMPLES:
 *
 * Lock stronger lock; remember initial ipl.
 * Trylock weaker (out of order) lock; ignore returned ipl value.
 * 
 * 	ipl = LOCK(&VM_PAGEFREELOCK, VM_PAGE_IPL);
 * 	if (TRYLOCK(&pp->p_uselock, VM_PAGE_IPL) == INVPL) {
 * 		goto failed;	/ * trylock failed * /
 * 	}
 * 
 * 	...perform operation needing locking...
 * 
 * then either drop in acquisition order:
 * 
 * 	/ * retain ipl of stronger lock: * /
 * 	UNLOCK(&pp->p_uselock, VM_PAGE_IPL);
 * 	UNLOCK(&vm_pagefreelock, ipl);
 * 
 * or drop out of acquisition order:
 * 
 * 	/ * downgrade to ipl of weaker lock: * /
 * 	UNLOCK(&vm_pagefreelock, VM_PUSELOCK_IPL);
 * 	UNLOCK(&pp->p_uselock, ipl);
 * 
 * or delayed drop out of acquisition order:
 * 
 * 	/ * downgrade to ipl of weaker lock: * /
 * 	UNLOCK(&vm_pagefreelock, VM_PUSELOCK_IPL);
 * 	/ * cache initial ipl for later PAGE_USEUNLOCK(pp): * /
 * 	l.puselockpl = ipl;
 */

#ifdef _KERNEL

/*
 * VM_PAGEFREELOCK() is a global spinlock which protects:
 *
 *	page free lists:
 *		mem_freemem, page_freelist, page_cachelist, page_dirtyalist, 
 *		page_dirtyflist, p_next, p_prev, p_free
 *
 * VM_PAGEIDLOCK() is global read/write spinlock which protects: 
 *
 *	page identity lists:
 *		page_hash, p_hash, v_pages, p_vpnext, p_vpprev,
 *		p_vnode, p_offset
 *
 * Note that manipulating most p_* fields also requires holding p_uselock
 * and possibly also a page reader/writer lock on the effected page.
 * See the discussion of struct page.
 *
 * vm_pagefreelock should always be manipulated via VM_PAGEFREELOCK() and
 * VM_PAGEFREEUNLOCK().
 * vm_pageidlock should always be manipulated via VM_PAGEIDRDLOCK(),
 * VM_PAGEIDWRLOCK() and VM_PAGEIDUNLOCK().
 *
 */

extern lock_t	vm_pagefreelock;    /* mutex page free lists and ID lists */
extern pl_t	vm_pagefreelockpl;  /* pl for current vm_pagefreelock holder */
extern rwlock_t	vm_pageidlock;	/* mutex page free lists and ID lists */

/*
 * VM_PAGEFREELOCK()
 * VM_PAGEFREEUNLOCK()
 *
 *	Spinlock (unlock) the global page free lists.
 *
 * Calling/Exit State:
 *
 *	VM_PAGEFREELOCK() caches the current ipl level for later use
 *	by VM_PAGEFREEUNLOCK().  The ipl value is cached in the global
 *	variable vm_pagefreelockpl. In order for this to work, the caller
 *	must release this in the reverse order they were acquired.
 */
#if (!VM_INTR_IPL_IS_PLMIN)

#define VM_PAGEFREELOCK()	(vm_pagefreelockpl = LOCK(&vm_pagefreelock, VM_PAGE_IPL))
#define VM_PAGEFREEUNLOCK()	UNLOCK(&vm_pagefreelock, vm_pagefreelockpl)

#else 	/* (VM_PAGE_IPL = PLMIN) */

#define VM_PAGEFREELOCK()	(vm_pagefreelockpl = LOCK_PLMIN(&vm_pagefreelock))
#define VM_PAGEFREEUNLOCK()	UNLOCK_PLMIN(&vm_pagefreelock, vm_pagefreelockpl)

#endif /* (!VM_INTR_IPL_IS_PLMIN) */

/*
 * VM_PAGEFREELOCK_OWNED()
 *
 *	Returns true if the calling context holds vm_pagefreelock,else false.
 *	(DEBUG-only)
 */

#ifdef DEBUG
#define VM_PAGEFREELOCK_OWNED()	LOCK_OWNED(&vm_pagefreelock)
#endif /* DEBUG */

/*
 * VM_PAGEFREELOCK_LOCKED()
 *
 *	Returns true if anyone holds vm_pagefreelock else false.
 *	(DEBUG-only)
 */

#ifdef DEBUG
#define VM_PAGEFREELOCK_LOCKED()	IS_LOCKED(&vm_pagefreelock)
#endif /* DEBUG */

/*
 * VM_PAGEIDRDLOCK()
 * VM_PAGEIDWRLOCK()
 * VM_PAGEIDUNLOCK()
 *
 *	Read/Write spinlock (unlock) the global page identity lists.
 *
 * Calling/Exit State:
 *
 *	VM_PAGEIDRDLOCK() acquires the vm_pageidlock lock in shared mode
 *	and caches the current ipl in l.vmpagelockpl.
 *	VM_PAGEIDWRLOCK() caches the current ipl in l.vmpagelockpl and
 *	acquires the lock in exclusive mode.
 *	VM_PAGEIDUNLOCK() unlocks the id list lock and sets the current ipl to
 *	level set in l.vmpageidlockpl.
 */
#if (!VM_INTR_IPL_IS_PLMIN)

#define VM_PAGEIDRDLOCK()	l.vmpageidlockpl = RW_RDLOCK(&vm_pageidlock, VM_PAGE_IPL)
#define VM_PAGEIDWRLOCK()	l.vmpageidlockpl = RW_WRLOCK(&vm_pageidlock, VM_PAGE_IPL)
#define VM_PAGEIDUNLOCK()	RW_UNLOCK(&vm_pageidlock, l.vmpageidlockpl)

#else /* (!VM_INTR_IPL_IS_PLMIN) */

#define VM_PAGEIDRDLOCK()	l.vmpageidlockpl = RW_RDLOCK_PLMIN(&vm_pageidlock)
#define VM_PAGEIDWRLOCK()	l.vmpageidlockpl = RW_WRLOCK_PLMIN(&vm_pageidlock)
#define VM_PAGEIDUNLOCK()	RW_UNLOCK_PLMIN(&vm_pageidlock, l.vmpageidlockpl)

#endif	/* (!VM_INTR_IPL_IS_PLMIN) */

/*
 * VM_PAGEIDLOCK_OWNED()
 *
 *	Returns true if the calling context holds VM_PAGEIDLOCK() else false.
 *	(DEBUG-only)
 */

#ifdef DEBUG
#define VM_PAGEIDLOCK_OWNED()	RW_OWNED(&vm_pageidlock)
#endif /* DEBUG */

/*
 * Macros to access PAGE_USELOCK.  For VM modules, _MEM_INTERNAL_
 * is defined, and the macros access the data structures directly.
 * For non-VM modules, _MEM_INTERNAL_ is not defined, and the
 * macros are defined as procedure calls.
 */
#ifdef _MEM_INTERNAL_
/*
 * PAGE_USELOCK(pp)
 * PAGE_USEUNLOCK(pp)
 *
 *	Spinlock (unlock) the page.
 *
 * Calling/Exit State:
 *
 *	PAGE_USELOCK() caches the current ipl level for later use
 *	by PAGE_USEUNLOCK().  In order for this to work, the caller
 *	must release this (and other spinlocks) in the reverse order
 *	they were acquired.
 *
 * Description:
 *
 *	We cache the current ipl in a per-processor variable.  This
 *	works since a processor only PAGE_USELOCK()'s one page at
 *	a time and must release the lock before context switching
 *	to a new processor.
 */
#if (!VM_INTR_IPL_IS_PLMIN)

#define PAGE_USELOCK(pp) \
		(l.puselockpl = LOCK(&(pp)->p_uselock, VM_PAGE_IPL))

#define PAGE_USEUNLOCK(pp)	UNLOCK(&(pp)->p_uselock, l.puselockpl)

#define PAGE_TRYUSELOCK(pp)	TRYLOCK(&(pp)->p_uselock, VM_PAGE_IPL)

#else /* VM_INTR_IPL_IS_PLMIN */

#error say what?

#define PAGE_USELOCK(pp) \
		(l.puselockpl = LOCK_PLMIN(&(pp)->p_uselock))

#define PAGE_USEUNLOCK(pp)	UNLOCK_PLMIN(&(pp)->p_uselock, l.puselockpl)

#define PAGE_TRYUSELOCK(pp)	TRYLOCK_PLMIN(&(pp)->p_uselock)

#endif	/* (!VM_INTR_IPL_IS_PLMIN) */

/*
 * PAGE_USELOCK_OWNED(pp)
 *
 *	Returns true if the calling context holds PAGE_USELOCK(pp) else false.
 *	(DEBUG-only)
 */

#ifdef DEBUG
#define PAGE_USELOCK_OWNED(pp)	LOCK_OWNED(&(pp)->p_uselock)
#endif /* DEBUG */

/*
 * PAGE_USELOCK_LOCKED(pp)
 *
 *	Returns true if anyone holds PAGE_USELOCK(pp) else false.
 *	(DEBUG-only)
 */

#ifdef DEBUG
#define PAGE_USELOCK_LOCKED(pp)	IS_LOCKED(&(pp)->p_uselock)
#endif /* DEBUG */

#else /* _MEM_INTERNAL_ */

extern void page_uselock(page_t *);
extern void page_useunlock(page_t *);
extern pl_t page_tryuselock(page_t *);

#define PAGE_USELOCK(pp)	page_uselock(pp)
#define PAGE_USEUNLOCK(pp)	page_useunlock(pp)
#define PAGE_TRYUSELOCK(pp)	page_tryuselock(pp)

#ifdef DEBUG

extern boolean_t page_uselock_owned(page_t *);
extern boolean_t page_uselock_locked(page_t *);

#define PAGE_USELOCK_OWNED(pp)	page_uselock_owned(pp)
#define PAGE_USELOCK_LOCKED(pp)	page_uselock_locked(pp)

#endif /* DEBUG */

#endif /* _MEM_INTERNAL_ */


/*
 * PAGE_WAIT(pp)
 *
 *	If bclnlist is not empty, then call cleanup hoping that the page
 *	you are waiting for changes state. If bclnlist is empty then
 *	Wait on page synchronization variable for page state to change.
 *	Call cleanup again after being woken up. This is done so that
 *	if PAGE_BROADCAST_U is called from basyncdone() then there may be
 *	other pages associated with the buffer header that could be
 *	available. Another advantage is that we call cleanup from 
 *	process context rather than feom pageout daemon, thus helping
 * 	the system wide performance.
 *
 * Calling/Exit State:
 *
 *	Caller must currently hold PAGE_USELOCK(pp).
 *	It is dropped but reacquired before returning.
 */

#define P_WAIT_PRI	(PRIMEM-1)	/* disp for p_wait SV_WAIT */

#define WAITERS_ON_PAGE(pp) SV_BLKD(&(pp)->p_wait)
	
#define PAGE_WAIT(pp) \
	{ \
		ASSERT(l.puselockpl == PLBASE); \
		if (bclnlist == NULL)	\
			SV_WAIT(&(pp)->p_wait, P_WAIT_PRI, &(pp)->p_uselock); \
		else	\
			PAGE_USEUNLOCK(pp); \
		if (bclnlist)	\
			cleanup();	\
		PAGE_USELOCK(pp); \
		ASSERT(l.puselockpl == PLBASE); \
	}

/*
 * PAGE_BROADCAST(pp)
 *
 *	Wakeup everyone blocked on a page synchronization variable.
 *
 * Calling/Exit State:
 *
 *	Caller must currently hold PAGE_USELOCK(pp).
 *
 * Description:
 *
 *	We only do this if there is someone currently blocked on the
 *	synchronization variable.  We use SV_BLKD() to check if there
 *	are any waiters as a performance optimization;  this is safe to
 *	do since any new blockers must hold PAGE_USELOCK(pp) and we already
 *	hold it here. We have a PAGE_BROADCAST_U macro that gets called
 *	from basyncdone(). There is a handshaking effort between
 *	PAGE_BROADCAST_U and PAGE_WAIT in calling cleanup.
 */

/* Unconditional page_broadcast : Don't check if anybody blocking */

#define PAGE_BROADCAST_U(pp)	SV_BROADCAST(&(pp)->p_wait, 0)

#define PAGE_BROADCAST(pp) \
	{ \
		if (SV_BLKD(&(pp)->p_wait)) \
			SV_BROADCAST(&(pp)->p_wait, 0); \
	}

/*
 * PAGE_IS_RDLOCKED(pp)
 * PAGE_IS_WRLOCKED(pp)
 * PAGE_IS_LOCKED(pp)
 *
 *	Returns true if the page is read locked, write locked, or
 *	locked in either mode.
 *
 * Calling/Exit State:
 *
 *	The returned result is stale unless the caller holds
 *	a page reader/writer lock or PAGE_USELOCK(pp).
 */

#define PAGE_IS_RDLOCKED(pp)	((pp)->p_activecnt > 0 && !(pp)->p_invalid)
#define PAGE_IS_WRLOCKED(pp)	((pp)->p_invalid)
#define PAGE_IS_LOCKED(pp)	((pp)->p_activecnt > 0)

enum page_lock_mode {
	NOT_LOCKED,
	WRITE_LOCKED,
	READ_LOCKED
};

/*
 * PAGE_IN_USE(pp)
 *
 *	Returns true if the physical page, pp, is currently in use.
 *
 * Calling/Exit State:
 *
 *	Caller must currently hold PAGE_USELOCK(pp).
 *
 *	A struct page is in use if it has any reader/writer locks or
 *	any active translations.  A page should not be page_free()'d
 *	while it is in use.  Whenever a reader/writer lock is released
 *	(p_activecnt decremented) or a translation removed, the page
 *	should be freed via page_free() iff !PAGE_IN_USE(pp).
 *
 *	The complementary statement is that before a reader/writer lock is
 *	acquired (p_activecnt incremented) or a translation established, the
 *	page must have been first reclaimed from the free list if it is free.
 *	Usually this is done by routines which find pages such as page_lookup().
 */

#define PAGE_IN_USE(pp)		(pp->p_activecnt || pp->p_mapping)

/*
 * PAGE_HAS_IDENTITY(pp, vp, off)
 *
 *	Returns true if the physical page, pp, has the <vnode, offset> identity.
 *
 * Calling/Exit State:
 *
 *	The returned result is stale unless the caller holds one or more of
 *	a page reader/writer lock, PAGE_USELOCK(pp), or VM_PAGEIDLOCK().
 */

#define PAGE_HAS_IDENTITY(pp, vp, off) \
	    (((vp) == (pp)->p_vnode) && ((off) == (pp)->p_offset))


/*
 * PAGE_RECLAIM(page_t *pp)
 *
 *	Attempt to reclaim the given page from the free list, if it is free.
 *
 * Calling/Exit State:
 *
 *	See page_reclaim() for further description.
 */

#define PAGE_RECLAIM(pp)	(!pp->p_free || page_reclaim(pp))

/*
 * IS_PAGE_AGED(page_t *pp, clock_t stamp, clock_t interval)
 *
 *	Check if page has aged more than the given age interval.
 *
 * Calling/Exit State:
 *
 *	No locks need to be held since this is only an approximation.
 */

#define IS_PAGE_AGED(pp, stamp, interval) ((stamp - (pp)->p_timestamp) > interval)

/*
 * void
 * PAGE_SETMOD(page_t *pp)
 *	Set the modify flag in a page and in the associated vnode.
 *
 * Calling/Exit State:
 *	The PAGE_USELOCK is held by the caller.
 *
 * Description:
 *	This function assumes that all writable translations installed
 *	into visible mappings in a user address space are for pages with
 *	vnode identity. Actually, all pages installed into any visible
 *	mapping have vnode identity at this time.
 */
#define PAGE_SETMOD(pp) {				\
	vnode_t *vp = (pp)->p_vnode;			\
							\
	ASSERT(PAGE_USELOCK_OWNED(pp));			\
	ASSERT(vp != NULL);				\
							\
	(pp)->p_mod = 1;				\
	VN_SETMOD(vp);					\
}

/*
 * uint_t pageroundup(uint_t size)
 * uint_t pagerounddown(uint_t size)
 *
 *	Round a byte count or address up or down to a page boundary.
 */
#define pageroundup(size)	(((size) + PAGESIZE - 1) & PAGEMASK)
#define pagerounddown(size)	((size) & PAGEMASK)

/*
 * page_get() request flags.
 */

/*
 * The following two flags, which control the type of pages
 * given out, are ignored if restricted DMA support is not
 * compiled into the kernel.
 */
/* SLEEP and NOSLEEP defined in <param.h> */
#define P_DMA			0x0002	/* must have DMA-able pages */
#define P_NODMA			0x0004	/* prefer non-DMA-able pages */

#define P_RETURN_PAGEUSELOCKED	0x0008
#define P_TRYPAGEUSELOCK	0x0010
#define P_SETMOD		0x0020

/* 
 * The following flags are used by page_unfree_l() to indicate if the
 * caller intends to create the page with this identity. If not, then
 * this routine does not drop any previously held locks in the failure
 * case in the race with page_get_l().
 */
#define P_LOOKUP	0x0040
#define P_LOOKUP_OR_CREATE	0x0080
#endif	/* _KERNEL */


#if defined _KERNEL || defined _KMEMUSER

/*
 * Page types:
 *
 *	If restricted DMA support is compiled into the kernel, pages will
 *	be divided into several types. In the following discussion, the
 *	meaning of the word ``DMAable'' is family specific. However, the
 *	family will generally use ``DMAable'' to mean that all devices
 *	performing DMA can access the page, whereas ``non-DMAable'' means
 *	that some devices performing DMA cannot access the memory. This
 *	division of memory will work conveniently for a family with at most
 *	two classes of I/O devices: ``old'' devices with a DMA restriction
 *	(where all old devices have the same restriction), and ``new''
 *	devices with no restriction.
 *
 *	STD_PAGE
 *
 *		Pages which are not necessarily DMAable.
 *
 *	DMA_PAGE
 *
 *		Pages which are DMAable and are allocated to clients which
 *		require DMAability. The only way a VM client can allocate
 *		memory from these pages is through kmem_alloc() family
 *		functions.
 *
 *	PAD_PAGE
 *
 *		Pages which are DMAable. These pages are allocated to clients
 *		which prefer DMAable pages.
 *
 *	NO_PAGE
 *
 *		Under no circumstances do pages of type NO_PAGE exist. Type
 *		NO_PAGE exists as part of an optimization for the
 *		freemem_resv() family of functions. It provides a
 *		``secondary page type'' which can never be allocated from.
 *
 * Memory is divided into these page types according to the rdma_mode in
 * effect:
 *
 *	RDMA_DISABLED, RDMA_MICRO:
 *
 *		All pages become STD_PAGE. In this case STD_PAGE pages
 *		are in fact DMAable (since all of memory is DMAable).
 *
 * 	RDMA_SMALL:
 *
 *		Pages are divided three pools: with all non-DMAable pages
 *		in the STD_PAGE pool, and the DMAable pages divided between
 *		the DMA_PAGE and PAD_PAGE pools.
 *
 *	RDMA_LARGE:
 *
 *		Pages are divided into two pools: with all non-DMAable pages
 *		in the STD_PAGE pool, and the DMAable pages divided between
 *		the DMA_PAGE and STD_PAGE pools.
 */

#define STD_PAGE	0
#ifdef NO_RDMA
#define NPAGETYPE	1
#define page_ntype_used	1
#else /* !NO_RDMA */
#define DMA_PAGE	1
#define PAD_PAGE	2
#define NO_PAGE		3
#define NPAGETYPE	4
extern int page_ntype_used;
#endif /* NO_RDMA */

/*
 * Page accounting structures which allow for a non-contiguous memory layout.
 * For contiguous memory architectures, set MAX_MEM_SEG to 1.
 */

struct pp_chunk {		/* page pool accounting structure */
	uint_t	pp_pfn;		/* page frame number of first page in chunk */
	uint_t	pp_epfn;	/* pfn of first page after chunk */
	struct page *pp_page;	/* pointer to first page structure */
	struct page *pp_epage;	/* pp_page + (pp_epfn - pp_pfn) */
	struct pp_chunk *pp_next; /* Next page pool chunk in search order */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern struct pp_chunk *pagepool;	/* array of pagepool chunks */
extern struct pp_chunk *pp_first;	/* first chunk in search order */
extern uint_t n_pp_chunk;		/* # chunks in pagepool table */
extern page_t **page_cs_table;		/* contiguous section table */
#ifdef DEBUG
extern int page_cs_table_size;		/* # entries in page_cs_table */
#endif

/*
 * uint_t
 * page_pptonum(const page_t *pp)
 *
 *	Translate page_t to hardware physical page frame number.
 *
 * Calling/Exit State:
 *
 *	Returns the page frame number corresponding to "pp".
 */
#define page_pptonum(pp) \
	(ASSERT((pp) >= pages && (pp) < epages), \
	 (pp)->p_pfn)

/*
 * page_t *
 * page_numtopp_idx(uint_t pfn, uint_t idx, uint_t idx_res)
 *
 *	Translate a hardware physical page frame number into its
 *	corresponding page_t, using the chunk index # to speed lookup.
 *
 * Calling/Exit State:
 *
 *	pfn is the page frame number of an MMU_PAGESIZE sized page.
 *
 *	idx is the index into the pagepool array of the chunk which
 *		contains the indicated page, or it is the low-order bits
 *		of that index if idx_res < the total number of chunks.
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
 *	If the idx value is not available, or the pfn might not correspond
 *	to a page_t, use page_numtopp() instead.
 */
#define page_numtopp_idx(pfn, idx, idx_res) \
	( (idx_res) >= n_pp_chunk ? \
		( ASSERT((pfn) >= pagepool[idx].pp_pfn), \
		  ASSERT((pfn) < pagepool[idx].pp_epfn), \
		  &pagepool[idx].pp_page[((pfn) - pagepool[idx].pp_pfn) / \
					  (PAGESIZE/MMU_PAGESIZE)] ) \
	  : \
		page_numtopp_idx_lowres(pfn, idx, idx_res) )


/* page structure address for 1st page pool page (of a type) in the system */
extern page_t	*pages;		/* == pagepool[0].pp_page */

/* one past last page pool page */
extern page_t	*epages;	/* == pagepool[n].pp_epage */

extern int mem_freemem[NPAGETYPE];	/* # free pages per type */
extern int page_dirtylists_size[NPAGETYPE];
					/*
					 * # dirty but not-in-use pages (both
					 * file and non-file) of each type.
					 */
extern int maxfreemem[NPAGETYPE];	/* max # free pages per type */

/*
 * Paging control thresholds.
 *
 *	Theses values are computed before the transition the (possible)
 *	transition to mode RDMA_SMALL. Thus, the PAD_PAGE entries are
 *	not used for these arrays.
 */
extern int mem_lotsfree[NPAGETYPE];
extern int mem_desfree[NPAGETYPE];
extern int mem_minfree[NPAGETYPE];

#define lotsfree	(mem_lotsfree[STD_PAGE])
#define desfree		(mem_desfree[STD_PAGE])
#define minfree		(mem_minfree[STD_PAGE])


/*
 * Macros for checking mem_freemem and maxfreemem against threshholds of
 * various types.
 */
#ifdef NO_RDMA
#define freemem 	(mem_freemem[STD_PAGE])
#define max_freemem() 	(maxfreemem[STD_PAGE])
#define page_dl_size	(page_dirtylists_size[STD_PAGE])
#define page_type(pp)	STD_PAGE
#else /* !NO_RDMA */
#define freemem 	(mem_freemem[STD_PAGE] + mem_freemem[PAD_PAGE])
#define max_freemem() 	(maxfreemem[STD_PAGE] + maxfreemem[PAD_PAGE])
#define page_dl_size	(page_dirtylists_size[STD_PAGE] + \
			 page_dirtylists_size[PAD_PAGE])
#define page_type(pp)	((pp)->p_type)
#endif /* NO_RDMA */
#define min_mem_notinuse()	(freemem + MIN(page_dl_size, nswappgfree))

/*
 * Variables controlling locking of physical memory.
 */
extern uint_t pages_pp_maximum;		/* tuning: lock + claim <= max */
extern uint_t pages_dkma_maximum;

/*
 * Page hash table is a power-of-two in size, externally chained
 * through the hash field.  The hash function is obtained from
 * PAGEID_HASHFUNC().  Its size is computed at sysinit time by
 * pageid_compute_hashsz() and remembered in page_hashsz.
 */

extern uint_t page_hashsz;	/* # slots in page hash table (power of 2) */
extern page_t **page_hash;	/* page hash table */

#define	PAGE_HASHFUNC(vp, off) \
		PAGEID_HASHFUNC(vp, off, page_hashsz)

#ifdef DEBUG
#define EXTRA_PAGE_STATS 1
#endif

#if defined(EXTRA_PAGE_STATS) || defined(lint)

/*
 * The statistics in struct page_tcnt are only adjusted
 * by routines which hold either the VM_PAGEFREELOCK() or the
 * VM_PAGEIDLOCK().
 *
 * The following are protected by VM_PAGEFREELOCK():
 * 	pc_free* stats, pc_get* stats, pc_reclaim* stats.
 *
 * The following are protected by VM_PAGEIDLOCK():
 *	pc_exists* stats, 
 *	
 */

struct page_tcnt {
uint_t	pc_free;		/* pages freed */
uint_t	pc_free_dontneed;	/* pages freed with dontneed */
uint_t	pc_free_free;		/* pages freed into free list */
uint_t	pc_free_cache_clean;	/* pages freed into clean cache list */
uint_t	pc_free_cache_dirty;	/* pages freed into dirty cache list */
uint_t	pc_get;			/* get's (successful + failed) */
uint_t	pc_get_fail_NOSLEEP;	/* get's failed due to NOSLEEP */
uint_t	pc_get_fail_TRYLOCK;	/* get's failed due to P_TRYPAGEUSELOCK */
uint_t	pc_get_no_TRYLOCK;	/* get's not satisfied thru just TRYLOCK */
uint_t	pc_get_cache;		/* pages gotten from cache list */
uint_t	pc_get_free;		/* pages gotten from free list */
uint_t	pc_getaligned;		/* get_aligned's (successful + failed) */
uint_t	pc_getaligned_fail_NOSLEEP; /* get_aligned's failed due to NOSLEEP */
uint_t	pc_getaligned_unavail;	/* get_aligned's not immediately available */
uint_t	pc_reclaim;		/* reclaim's during & not during pageout */
uint_t	pc_reclaim_pgout;	/* reclaim's during pageout */
uint_t	pc_reclaim_lost;	/* attempted reclaim's lost due to race */
uint_t	pc_exist_hit;		/* exist's that find page */
uint_t	pc_exist_miss;		/* exist's that don't find page */
#define	PC_HASH_CNT	8
uint_t	pc_exist_hashlen[PC_HASH_CNT+1]; /* histogram of search chain lengths */
#define	PC_GET_CNT	10
uint_t	pc_get_npages[PC_GET_CNT+1]; /* histogram of get's num pages */
};

extern struct page_tcnt pagecnt;

#define	BUMPPGCOUNT(x)	++(x)

#else	/* defined(EXTRA_PAGE_STATS) || defined(lint) */

#define	BUMPPGCOUNT(x) 

#endif	/* defined(EXTRA_PAGE_STATS) || defined(lint) */


/*
 * Page frame operations.
 */

struct buf;

void	page_init_chunk(page_t *, uint_t, paddr_t, page_t **, boolean_t);
void	page_init(void);
void	page_cs_init(void);
page_t *page_get(uint_t, uint_t);
page_t *page_get_aligned(uint_t, paddr_t, paddr_t, paddr_t, uint_t);
void	page_free(page_t *, uint_t);
void	page_free_l(page_t *, uint_t);
page_t *page_create(struct vnode *, off_t);
page_t *page_create_aligned(struct vnode *, off_t, paddr_t, paddr_t);
page_t *page_lookup_or_create3(struct vnode *, off_t, uint_t);
page_t *page_lazy_create(struct vnode *, off_t);
page_t *page_exists_uselock(struct vnode *, off_t, enum page_lock_mode);
void	page_downgrade_lock(page_t *);
void	page_unlock(page_t *);
void	page_list_unlock(page_t *);
void	page_find_iowait(struct vnode *, off_t);
void	page_find_unlock(struct vnode *, off_t, uint_t);
void	page_find_unload(struct vnode *, off_t);
void    page_find_zero(vnode_t *, off_t);
void	page_add(page_t **, page_t *);
void	page_sub(page_t **, page_t *);
void	page_sortadd(page_t **, page_t *);
void	page_setmod(page_t *);
void	page_setmod_iowait(page_t *);
int	page_reclaim(page_t *);
int	page_reclaim_l(page_t *, enum page_lock_mode, u_int flags);
void	page_abort(page_t *);
void	page_abort_l(page_t *);
void	page_abort_identity(page_t *, struct vnode *, off_t);
void	page_assign_identity(page_t *, vnode_t *, off_t);
page_t *page_numtopp(uint_t pfn);
page_t *page_numtopp_idx_lowres(uint_t pfn, uint_t idx, uint_t idx_res);
page_t *page_phystopp(paddr_t);
paddr_t	page_pptophys(const page_t *);
page_t *getnextpg(struct buf *, const page_t *);
void	pagezero(page_t *, ulong_t, ulong_t);
void	ppcopy(page_t *, page_t *);
void	ppcopyrange(page_t *, page_t *, uint_t, uint_t);
void	pageout_init(void);
int	page_anonpageout(int, clock_t, clock_t, page_t **);
void	page_swapreclaim(boolean_t);
boolean_t page_cache_query(struct vnode *, off_t);

#define page_lookup_or_create(vp, off)	page_lookup_or_create3(vp, off, 0)

#endif	/* _KERNEL */


/*
 * The following routines are used by functions to manipulate
 * "marker" pages placed in v_pages page lists in order to keep
 * their place while dropping locks.
 */

/*
 * CREATE_MARKER_PAGE(page_t *mpp, void (*function_address)())
 * CREATE_2_MARKER_PAGES(page_t *mpp1, page_t *mpp2, void (*function_address)())
 * CREATE_2_MARKER_PAGES_PD(page_t *mpp1, page_t *mpp2,
 *			    void (*function_address)())
 * CREATE_2_MARKER_PAGES_NS(page_t *mpp1, page_t *mpp2,
 *			    void (*function_address)())
 *
 *	Create one or two page_t markers for traversing v_pages.
 *	CREATE_2_MARKER_PAGES_PD is similar to CREATE_2_MARKER_PAGES
 *	except that it uses a private pool of 2 markers for the pageout
 *	daemon. CREATE_2_MARKER_PAGES_NS is similar to CREATE_2_MARKER_PAGES
 *	except that it does not sleep.
 *
 * Calling/Exit State:
 *
 *	mpp, mpp1, and mpp2 are (page_t *) variables which will be set to
 *		point to the returned marker page(s).
 *	function_address is the address of the calling function
 *		(e.g., &pvn_vplist_dirty).
 *	CREATE_2_MARKER_PAGES_PD uses private markers for the pageout daemon.
 *
 *	Caller tolerates blocking and hence holds no non-blocking locks
 *	(except for CREATE_2_MARKER_PAGES_NS).
 *
 * Description:
 *	The implementation of CREATE_2_MARKER_PAGES_PD assumes that the
 *	pagoeut daemon is single threaded (i.e. contains one LWP).
 */

#define CREATE_MARKER_PAGE(mpp, function_address) { \
	ALLOC_MARKER_PAGE(mpp, KM_SLEEP); \
	INIT_MARKER_PAGE(mpp, function_address); \
}
#define CREATE_2_MARKER_PAGES(mpp1, mpp2, function_address) { \
	ALLOC_2_MARKER_PAGES(mpp1, mpp2, KM_SLEEP); \
	INIT_MARKER_PAGE(mpp1, function_address); \
	INIT_MARKER_PAGE(mpp2, function_address); \
}
#define CREATE_2_MARKER_PAGES_PD(mpp1, mpp2, function_address) { \
	if (IS_PAGEOUT()) { \
		(mpp1) = (page_t *)(&pvn_pageout_markers[0]); \
		(mpp2) = (page_t *)(&pvn_pageout_markers[1]); \
	} else { \
		ALLOC_2_MARKER_PAGES(mpp1, mpp2, KM_SLEEP); \
	} \
	INIT_MARKER_PAGE(mpp1, function_address); \
	INIT_MARKER_PAGE(mpp2, function_address); \
}
#define CREATE_2_MARKER_PAGES_NS(mpp1, mpp2, function_address) { \
	ALLOC_2_MARKER_PAGES(mpp1, mpp2, KM_NOSLEEP); \
	if ((mpp1) != NULL) { \
		INIT_MARKER_PAGE(mpp1, function_address); \
		INIT_MARKER_PAGE(mpp2, function_address); \
	} \
}

#define ALLOC_MARKER_PAGE(mpp, kmflags) { \
	(mpp) = kmem_alloc(sizeof(struct pageheader), kmflags); \
}
#define ALLOC_2_MARKER_PAGES(mpp1, mpp2, kmflags) { \
	(mpp1) = kmem_alloc(2 * sizeof(struct pageheader), kmflags); \
	(mpp2) = (page_t *)((struct pageheader *)(mpp1) + 1); \
}

#ifdef DEBUG
#define INIT_MARKER_PAGE(mpp, function_address) { \
	(mpp)->p_vnode = (struct vnode *)(function_address); \
	(mpp)->p_offset = (off_t)u.u_lwpp; \
}
#else
#define INIT_MARKER_PAGE(mpp, function_address) { \
	(mpp)->p_vnode = (struct vnode *)(function_address); \
}
#endif /* DEBUG */

/*
 * INSERT_MARKER_PAGE(page_t *pp, page_t *mpp)
 *
 *	Insert marker page, mpp, after the current page, pp, in v_pages.
 */

#define INSERT_MARKER_PAGE(pp, mpp) { \
		(mpp)->p_vpprev = (pp); \
		(mpp)->p_vpnext = (pp)->p_vpnext; \
		(pp)->p_vpnext = (mpp); \
		(mpp)->p_vpnext->p_vpprev = (mpp); \
	}

/*
 * INSERT_END_MARKER_PAGE(page_t *mpp, vnode_t *vp)
 *
 *	Insert marker page, mpp, at the end of the v_pages list of vnode, vp.
 */

#define INSERT_END_MARKER_PAGE(mpp, vp) { \
		(mpp)->p_vpnext = (vp)->v_pages; \
		(mpp)->p_vpprev = (vp)->v_pages->p_vpprev; \
		(vp)->v_pages->p_vpprev = (mpp); \
		(mpp)->p_vpprev->p_vpnext = (mpp); \
	}

/*
 * ADVANCE_FROM_MARKER_PAGE(page_t *pp, page_t *mpp, vnode_t *vp)
 *
 *	Set pp to the next unprocessed page after the marker page, mpp.
 *	Remove the marker page, mpp, from the v_pages list of vnode, vp.
 *
 *	The caller must guarantee that there is at least one other (marker)
 *	page on the v_pages list.
 */

#define ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp) { \
		(pp) = (mpp)->p_vpnext; \
		(mpp)->p_vpnext->p_vpprev = (mpp)->p_vpprev; \
		(mpp)->p_vpprev->p_vpnext = (mpp)->p_vpnext; \
		if ((vp)->v_pages == (mpp)) \
			(vp)->v_pages = (pp); \
	}

/*
 * REMOVE_MARKER_PAGE(page_t *mpp, struct vnode *vp)
 *
 *	Remove marker page, mpp, from the v_pages list of vp.
 */

#define REMOVE_MARKER_PAGE(mpp, vp) { \
	if ((vp)->v_pages == (mpp)) { \
		if ((mpp)->p_vpnext == (mpp)) \
			(vp)->v_pages = NULL; \
		else { \
			(vp)->v_pages = \
				(mpp)->p_vpprev->p_vpnext = (mpp)->p_vpnext; \
			(mpp)->p_vpnext->p_vpprev = (mpp)->p_vpprev; \
		} \
	} else { \
		(mpp)->p_vpprev->p_vpnext = (mpp)->p_vpnext; \
		(mpp)->p_vpnext->p_vpprev = (mpp)->p_vpprev; \
	} \
}

/*
 * DESTROY_MARKER_PAGE(page_t *mpp)
 * DESTROY_2_MARKER_PAGES(page_t *mpp1, page_t *mpp2)
 * DESTROY_2_MARKER_PAGES_PD(page_t *mpp1, page_t *mpp2)
 *
 *	Destroy one or two page_t markers created via CREATE_MARKER_PAGE(),
 *	CREATE_2_MARKER_PAGES(), or CREATE_2_MARKER_PAGES_PD().
 */

#define DESTROY_MARKER_PAGE(mpp) { \
	kmem_free((mpp), sizeof(struct pageheader)); \
}
#define DESTROY_2_MARKER_PAGES(mpp1, mpp2) { \
	ASSERT((mpp2) == (page_t *)((struct pageheader *)(mpp1) + 1)); \
	kmem_free((mpp1), 2 * sizeof(struct pageheader)); \
}
#define DESTROY_2_MARKER_PAGES_PD(mpp1, mpp2) { \
	ASSERT((mpp2) == (page_t *)((struct pageheader *)(mpp1) + 1)); \
	if ((mpp1) != (page_t *)(&pvn_pageout_markers[0])) { \
		kmem_free((mpp1), 2 * sizeof(struct pageheader)); \
	} \
}


#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_PAGE_H */
