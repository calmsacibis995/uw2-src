/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/vm_anon.c	1.74"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/buf.h>
#include <fs/vnode.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg_kmem.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * Exported Data
 */
anoninfo_t 		anoninfo;	/* anonfs reservation info */
sleep_t			anon_table_lck;	/* anonfs cluster table lock */
ulong_t			anon_allocated;	/* anon_t(s) currently allocated */
fspin_t			anon_free_lck;	/* Protects the free list of anon_t */
					/* structures and anon_allocated */

/*
 * STATIC data
 *
 *	The following are protected by the anon_table_lck.
 */
STATIC anode_t		*anon_anode;	/* currently available anode */
STATIC off_t		anon_offset;	/* next available offset for */
					/* above anode (anon_anode) */

/*
 * The following variables are counts, in units of anon_t structures.
 * These variables are protected as follows:
 *
 *	The following are protected by the anon_table_lck:
 *
 *		anon_max, anon_clmax
 *
 *	The following is protected by both the anon_table_lck and the
 *	anon_free_lck. Holding either lock is sufficient to read. Both
 *	locks are held to write.
 *
 *		anon_clcur
 *
 *	The following is protected by the anon_free_lck:
 *
 *		anon_allocated
 */
STATIC ulong_t		anon_max;	/* maximum potentially needed */
STATIC ulong_t		anon_clmax;	/* capacity of the cluster table */
STATIC ulong_t		anon_clcur;	/* currently in the cluster table */

/*
 * The following are indices into the anon_table. These variables are
 * protected by the anon_table_lck.
 */
uint_t             	anon_tabinuse;  /* # of anon_table entries in use */
uint_t			anon_tabgrow;	/* the anon_table entry currently */
					/* growing */

/*
 * The anonfs cluster table.
 *
 * Writes to the table are protected by the anon_table_lck. However, once
 * the fields are written, they remain constant. Therefore, readers need
 * no locking whatsoever. In particular, anon_antovp() takes advantage of
 * this fact to avoid all locking.
 */
anon_cluster_t	*anon_table[ANON_TABSIZE];

/*
 * Free list of anon_t(s)
 * ----------------------
 *
 * The free list contains anon_t(s) whose pages have been aborted and whose
 * swap space is free, and The list is a NULL terminated singly linked list,
 * treated as a LIFO (push down list).
 *
 *	The following is protected by the anon_free_lck.
 */
STATIC anon_t		*anon_free_listp;

/*
 * Other STATIC data
 */
STATIC fspin_t		an_ref_lck;	/* Protects all refcnt fields in all */
					/* anon structs */

/*
 *+ global lock protecting the linked list of anodes
 */
STATIC LKINFO_DECL(anonfs_lkinfo, "MP:anonfs:anon_table_lck",0);

#ifdef ANON_DEBUG
STATIC fspin_t		anon_hist_lock;	/* protects an_history fields */
#endif

/*
 * Declarations for internal functions
 */
STATIC void anon_expand_table(ulong_t allocated);
STATIC void anon_relocate(anon_t *ap, uchar_t ridx);
STATIC boolean_t anon_add_swap_l(ulong_t npages);

/*
 * STATIC int 
 * anonfs_badop()
 *	Bad op placeholder for vnodeops table.
 *
 * Calling/Exit State:
 *	NONE.
 *
 * Remarks:
 *	This function deliberately declared in a non-ANSI style to avoid 
 *	complaints about argument mismatches with the real VOPs it replaces. 
 *	Function declared to return an int for similar reasons.
 */
STATIC int
anonfs_badop(void)
{
	/*
	 *+ A VOP was performed on an anonfs anode. This should never
	 *+ happen since anodes are not used for any normal file operations
	 *+ and anonfs `filesystems' are not accessible from the normal
	 *+ UNIX file namespace. This is a catastrophic error. I suggest
	 *+ taking your machine out back and shooting it.
	 */
	cmn_err(CE_PANIC, "VOP called on anonfs anode");
	/* NOTREACHED */

	return 0;
}

vnodeops_t anonfs_ops = {
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (void(*)())anonfs_badop, (void(*)())anonfs_badop, 
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
	(void(*)())anonfs_badop, 
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop, (int (*)())anonfs_badop,
        (int (*)())anonfs_badop
};

/*
 * void
 * anon_conf(uint_t size, uint_t kma_resv, uint_t pages_pp,
 *		  uint_t pages_dkma, void *space, uint_t space_size)
 *	Perform one-time initializations.
 *
 * Calling/Exit State:
 *	The size passed in (first parameter) is in pages and represents the
 *	size of the swappable memory. The second parameter (kma_resv)
 *	represent the portion reserved for non-discretionary kma. The third
 *	parameter (pages_pp) represents the number of pages which may not
 *	be locked down by user request (e.g. via memcntl(2)). The fourth
 *	parameter (pages_dkma) represents the number of pages which may not
 *	be consumed by discretionary kma.
 *
 *	The last two parameters <space, space_size> represent some otherwise
 *	unused chunk of space which is being handed over to anon. The space
 *	is appropriately aligned for a data structure containing a pointer
 *	and a long integer. The contents have already been zeroed.
 *
 *	Called by kvm_init() immediately after kma initialization.
 */
void
anon_conf(uint_t size, uint_t kma_resv, uint_t pages_pp,
	       uint_t pages_dkma, void *space, uint_t space_size)
{
	ulong_t nanon;
	anon_t *ap;
	struct {
		anode_t		ai_anode;
		anon_cluster_t	ai_cluster;
		anon_t		ai_anon[1];
	} *initp = space;

	/*
	 * Initialize anoninfo. Main memory counts as available swappable
	 * memory.
	 */
	anoninfo.ani_kma_max = size;
	anoninfo.ani_max = size - kma_resv;
	anoninfo.ani_dkma_max = anoninfo.ani_max - pages_dkma;
	anoninfo.ani_user_max = anoninfo.ani_max - pages_pp;

	/*
	 * Initialize the global locks for anon management.
	 */
	SLEEP_INIT(&anon_table_lck, 0, &anonfs_lkinfo, KM_NOSLEEP);
	FSPIN_INIT(&anon_free_lck);
	FSPIN_INIT(&an_ref_lck);
#ifdef ANON_DEBUG
	FSPIN_INIT(&anon_hist_lock);
#endif

	/*
	 * Take the initial chunk of space we got from kvm_init and carve
	 * it into the first anode, the anon structures for the first
	 * cluster table entry, plus the cluster table entry itself.
	 */
	if (space_size >= sizeof(*initp)) {

		/*
		 * Initialize the anode.
		 */
		ANODE_TO_VP(&initp->ai_anode)->v_flag = VSWAPBACK | VNOSYNC;
		ANODE_TO_VP(&initp->ai_anode)->v_type = VUNNAMED;
		ANODE_TO_VP(&initp->ai_anode)->v_op = &anonfs_ops;

		/*
		 * Compute how many anon_t's we are going to get out
		 * of this chunk.
		 */
		nanon = (space_size - sizeof(*initp)) / sizeof(anon_t) + 1;

		/*
		 * Add the cluster table entry to the cluster table.
		 */
		anon_table[0] = &initp->ai_cluster;
		anon_tabinuse = anon_tabgrow = 1;
		anon_clcur = anon_clmax = nanon;
		anon_anode = &initp->ai_anode;
		anon_offset = ptob(nanon);

		/*
		 * Initialize the cluster table entry.
		 */
		ap = initp->ai_anon;
		initp->ai_cluster.ancl_pages =
			initp->ai_cluster.ancl_max = nanon;
		initp->ai_cluster.ancl_anodep = &initp->ai_anode;
		initp->ai_cluster.ancl_anonp = ap;

		/*
		 * Initialize the new anon_t(s). Thread them onto the
		 * free list.
		 */
		anon_free_listp = ap;
		while (nanon != 0) {
			SWAPLOC_MAKE_EMPTY(&ap->an_swaploc);
			ap->an_next = ap + 1;
			++ap;
			--nanon;
		}
		(ap - 1)->an_next = NULL;
	} else {
		/*
		 * Since no anodes are yet allocated, we set anon_offset to
		 * force an allocation at the first call to anon_expand_table().
		 */
		anon_offset = ANON_MAXOFFSET;
	}

	/*
	 * Grow the cluster table so that it can handle main memory.
	 */
	if (!anon_add_swap_l(anoninfo.ani_max)) {
		/*
		 *+ During system startup, the OS failed to configure
		 *+ in the anonymous memory tables due to lack of memory.
		 */
		cmn_err(CE_PANIC, "anon_conf: out of space");
	}
}

/*
 * boolean_t
 * anon_add_swap(ulong_t npages)
 *	Function to inform anonfs that swap space is being added to the
 *	system, and to obtain anonfs' permission to proceed.
 *
 * Calling/Exit State:
 *	npages is the number of pages of swap being added. B_TRUE is
 *	returned if anonfs grants permission to perform the add, and
 *	B_FALSE otherwise.
 *
 *	No SPIN LOCKS are held at entry to or exit from this function.
 *
 * Remarks:
 *	This function is called when swapadd() is adding the new swap file
 *	to the swap table, but the space is not yet ready for use.
 *	Since the add can still fail, it is not possible for us to
 *	adjust the reservation counts in anoninfo. Thus, the swapadd() will
 *	need to call anon_add_swap_resv() if the add succeeds, or
 *	anon_del_swap() if it fails.
 */
boolean_t
anon_add_swap(ulong_t npages)
{
	boolean_t ret;

	ASSERT(KS_HOLD0LOCKS());

	ANON_TAB_LOCK();
	ret = anon_add_swap_l(npages);
	ANON_TAB_UNLOCK();

	return (ret);
}

/*
 * STATIC boolean_t
 * anon_add_swap_l(ulong_t npages)
 *	Similar to anon_add_swap, except that the anon_table is locked
 *	on entry to this function.
 *
 * Calling/Exit State:
 *	npages is the number of pages of swap being added. B_TRUE is
 *	returned if anonfs grants permission to perform the add, and
 *	B_FALSE otherwise.
 *
 *	A new anode will be allocated if required.
 *
 *	No SPIN LOCKS are held at entry to or exit from this function.
 *
 *	The caller either owns the ANON_TAB_LOCK, or the caller is
 *	anon_conf() and thus private holds the anon_table.
 */
STATIC boolean_t
anon_add_swap_l(ulong_t npages)
{
	ulong_t pgs_needed, nanon;
	anode_t *anop;
	vnode_t *vp;
	anon_cluster_t *clp;
	vaddr_t addr;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Do we need to add a new entry to the cluster table?
	 */
	if (anon_max + npages > anon_clmax) {
		/*
		 * Compute the number of pages of anon_t(s) needed.
		 */
		pgs_needed = btopr((anon_max + npages - anon_clmax) *
				   sizeof(anon_t));
		nanon = ptob(pgs_needed) / sizeof(anon_t);

		/*
		 * Answer: yes
		 */
		if (anon_tabinuse == ANON_TABSIZE) {
			/*
			 * Table full. Fail the request.
			 */
			return (B_FALSE);
		}

		/*
		 * We need memory for the pages of anon_t(s) and virtual
		 * space in which to map them.  It is not necessary to
		 * allocate and map them now, only to reserve them.
		 */
		if (!mem_resv(pgs_needed, M_KERNEL_ALLOC)) {
			return (B_FALSE);
		}
		if ((addr = kpg_vm_alloc(pgs_needed, KM_NOSLEEP)) == NULL) {
			mem_unresv(pgs_needed, M_KERNEL_ALLOC);
			return (B_FALSE);
		}

		/*
		 * We are now committed to the expansion of the cluster
		 * table.
		 */

		/*
		 * The pages we just reserved can never become anonymous,
		 * because we never give up the reservation. Actually,
		 * all permanent M_KERNEL_ALLOCs should have the effect
		 * of reducing anon_max. However, for efficiency reasons,
		 * interfaces have not been created to achieve this. In
		 * addition, the resulting wastage of M_KERNEL_ALLOC
		 * reservations and kpg virtual space is a very minor
		 * problem.
		 */
		npages -= pgs_needed;

		/*
		 * Now allocate the cluster structure.
		 */
		clp = kmem_alloc(sizeof(anon_cluster_t), KM_SLEEP);
		clp->ancl_pages = 0;
		clp->ancl_max = nanon;
		clp->ancl_anonp = (anon_t *)addr;

		/*
		 * Allocate a vnode/offset range to the anon_t(s).
		 * If necessary, make a new anode.
		 */
		if (ptob(anon_offset) + nanon > ptob(ANON_MAXOFFSET)) {
			/*
			 * Allocate a new anode structure.
			 */
			anop = kmem_zalloc(sizeof(anode_t), KM_SLEEP);

			/*
			 * Complete initialization of vnode: mark vnode as
			 * being an ANONVP (for IS_ANONVP() tests) and set
			 * vops to point to an invalid set of calls!
			 *
			 * A VNINIT is not needed because no VOPs are
			 * supported.
			 */
			vp = ANODE_TO_VP(anop);
			vp->v_flag = VSWAPBACK | VNOSYNC;
			vp->v_type = VUNNAMED;
			vp->v_op = &anonfs_ops;

			/*
			 * Reset the anode allocation information.
			 */
			anon_anode = anop;
			anon_offset = 0;
		}
		clp->ancl_anodep = anon_anode;
		clp->ancl_base = anon_offset;
		anon_offset += ptob(nanon);

		/*
		 * Complete cluster table expansion by attaching the new
		 * cluster to the table.
		 */
		anon_table[anon_tabinuse++] = clp;
		anon_clmax += nanon;
	}
	anon_max += npages;

	return (B_TRUE);
}

/*
 * void
 * anon_add_swap_resv(ulong_t npages);
 *	Increase the pool of M_SWAP reservation available when a swap
 *	file has been added.
 *
 * Calling/Exit State:
 *	npages is the number of pages of swap being added.
 *
 *	A FAST SPIN LOCK is not held at entry or exit from this function.
 */
void
anon_add_swap_resv(ulong_t npages)
{
	/*
	 * Grab the vm_memlock (which protects anoninfo) and then
	 * adjust the counts.
	 */
	FSPIN_LOCK(&vm_memlock);
	anoninfo.ani_user_max += npages;
	anoninfo.ani_dkma_max += npages;
	anoninfo.ani_max += npages;
	anoninfo.ani_kma_max += npages;
	FSPIN_UNLOCK(&vm_memlock);
}

/*
 * void
 * anon_del_swap_resv(ulong_t npages);
 *	Decrease the pool of M_SWAP reservation available when a swap
 *	file is to be deleted.
 *
 * Calling/Exit State:
 *	npages is the number of pages of swap being added.
 *
 *	Returns B_TRUE if the deletion is to be permitted, and B_FALSE
 *	otherwise.
 *
 *	A FAST SPIN LOCK is not held at entry or exit from this function.
 */
boolean_t
anon_del_swap_resv(ulong_t npages)
{
	boolean_t permitted = B_FALSE;

	/*
	 * Grab the vm_memlock (which protects anoninfo) and then
	 * adjust the counts if the operations is permitted.
	 */
	FSPIN_LOCK(&vm_memlock);
	if (anoninfo.ani_dkma_max >= anoninfo.ani_resv + npages) {
		anoninfo.ani_user_max -= npages;
		anoninfo.ani_dkma_max -= npages;
		anoninfo.ani_max -= npages;
		anoninfo.ani_kma_max -= npages;
		permitted = B_TRUE;
	}
	FSPIN_UNLOCK(&vm_memlock);

	return (permitted);
}

/*
 * void
 * anon_del_swap(ulong_t npages, uchar_t ridx, anon_ds_t type);
 *	Decrease the pool of available anon resources when swap
 *	file ``ridx'' is to be deleted.
 *
 * Calling/Exit State:
 *	npages is the number of pages of swap being deleted.
 *	Type is ANON_SWAP_DELETE in the usual ``swap delete'' case.
 *	Type will be ANON_ADD_BACKOUT if a swapadd(), or portion thereof,
 *	needs to be backed out before anon_add_swap_resv() was called.
 *
 *	If type == ANON_SWAP_DELETE, then we are being called by swapdel().
 *	In this case the swap file specified by ridx has been marked
 *	ST_INDEL. Furthermore, this marking was done with swap_lck held.
 *
 *	No SPIN LOCKs are held at entry to or exit from this function.
 *
 * Remarks:
 *	The ANON_ADD_BACKOUT flag represents an optimization for the
 *	case where the swap file is not yet in use, so that no scan of
 *	of the cluster table is necessary.
 *
 *	A "portion" of a swap file needs to be "backed out" in the case
 *	where the file system (via VOP_STABLESTORE) rounds the length
 *	of the swap file back to a multiple of its blocksize.
 */
void
anon_del_swap(ulong_t npages, uchar_t ridx, anon_del_t type)
{
	uint_t tabinuse, i;
	anon_cluster_t *clp;
	anon_t *ap, *eap;

	ASSERT(KS_HOLD0LOCKS());

	if (type == ANON_SWAP_DELETE) {
		ASSERT(swaptab[ridx]->si_flags & ST_INDEL);

		/*
		 * Take the anon table lock in order to stabilize our
		 * view of the portion of the table actually in use.
		 */
		ANON_TAB_LOCK();
		tabinuse = MIN(anon_tabgrow + 1, anon_tabinuse);
		ANON_TAB_UNLOCK();

		/*
		 * Now scan all clusters for potential users of the given
		 * swap device.
		 */
		for (i = 0; i < tabinuse; ++i) {
			clp = anon_table[i];
			ap = clp->ancl_anonp;
			eap = ap + clp->ancl_pages;
			while (ap < eap) {
				/*
				 * The following unlocked read(s) of
				 * ap->an_swaploc is acceptable because:
				 *
				 * 1) anon_relocate() is tolerant of the
				 *    case where the page is not actually
				 *    on the specified swap file.
				 *
				 * 2) It is unacceptable to skip the call
				 *    to anon_relocate() for a page which is
				 *    on the specified swap file.
				 *
				 * 3) The swap file specified by ridx
				 *    has been marked as ST_INDEL by our
				 *    caller swapdel(). swapdel() held the
				 *    swap_lck while it wrote the ST_INDEL
				 *    flag.
				 *
				 * 4) The swap_lck is held in anon_pageout(),
				 *    protecting both the swap allocation and
				 *    the assignment of the swap location to
				 *    ap->an_swaploc.
				 *
				 * 5) Consequently, anyone who is writing to
				 *    ap->an_swaploc must have already
				 *    freed the space, or must have
				 *    observed that the space was free and
				 *    is allocating new space in another
				 *    swap file.
				 */
				if (SWAPLOC_IDX_EQUAL(&ap->an_swaploc, ridx))
					anon_relocate(ap, ridx);
				++ap;
			}
		}
	}

	/*
	 * Proceed with the adjustment to anon_max.
	 * Protect these calculations with the anonfs table lock.
	 */
	ANON_TAB_LOCK();
	anon_max -= npages;
	ANON_TAB_UNLOCK();

	return;
}

/*
 * void
 * anon_relocate(anon_t *ap, uchar_t ridx)
 *	Relocate ap from the swap file specified by ridx into main
 *	memory.
 *
 * Calling/Exit State:
 *	ap is potentially on the specified swap file. If the page is not
 *	in main memory, then we must bring it in. The swap space associated
 *	with ap is freed.
 *
 *	No spin LOCKs are held upon entry to this function. This function
 *	returns in the same state.
 */

void
anon_relocate(anon_t *ap, uchar_t ridx)
{
	page_t *page_array[2];
	page_t *pp;
	uint_t prot;
	int err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * We need to grab a hold on the anon_t (by bumping an_ref_cnt) in
	 * order to generate an anon_getpage(). However, the anon_t might
	 * already be free, in which case it is no longer possible to hold
	 * it. Luckily, anon_decref() frees the swap space of free anon_t(s).
	 */
	ANON_SAVE_HIST(ap, ANON_H_RELOCATE);
	FSPIN_LOCK(&an_ref_lck);
	if (ap->an_refcnt == 0) {
		FSPIN_UNLOCK(&an_ref_lck);

		/*
		 * The anon_t is already free. If only we wait, anon_decref()
		 * will abort the page and free the swap.  However, we might
		 * have to wait for an I/O to complete.
		 */
		while (SWAPLOC_IDX_EQUAL(&ap->an_swaploc, ridx)) {
			/*
			 * It's not worth it to set up tight wakeup
			 * synchronization for this very infrequent
			 * case. So, just sleep a little and re-inspect
			 * the swap allocation.
			 */
			LBOLT_WAIT(PRISLEP);
		}

		return;

	} /* an_refcnt == 0 */

	/*
	 * grab a hold on the anon_t
	 */
	++ap->an_refcnt;
	FSPIN_UNLOCK(&an_ref_lck);

	/*
	 * Read in the page. We pass in S_WRITE in the hope that
	 * anon_getpage() will break doubles and free the swap
	 * space, saving us some work. In the unlikely case that we
	 * are disturbing the wrong guy, this loses performance. However,
	 * the result is not incorrect.
	 */
	ANON_SAVE_HIST(ap, ANON_H_RELOCATE_GET);
	err = anon_getpage(&ap, &prot, page_array, PAGESIZE, S_WRITE, sys_cred);
	if (err) {
		/*
		 * I/O error case: the anon_t has been marked as such.
		 * All future anon_getpage() attempts to that anon_t will
		 * fail with EIO.
		 *
		 * We would like to simply ASSERT that
		 * SWAPLOC_IS_IOERR(&ap->an_swaploc). However, the swap space
		 * for the anon_t might have reallocated before
		 * anon_getpage() obtained the ANON_T_LOCK lock.
		 */
		ASSERT(SWAPLOC_TO_IDX(&ap->an_swaploc) != ridx ||
		       SWAPLOC_IS_IOERR(&ap->an_swaploc));
		anon_decref(ap);
		return;
	}

	pp = page_array[0];
	ASSERT(PAGE_IS_RDLOCKED(pp));
	ASSERT(ap->an_page && pp == ap->an_page);
	ASSERT(ANON_PGPRV(pp)->a_ap == ap);

	/*
	 * Verify ridx and free the swap space. Note that we are
	 * breaking doubles here.
	 */
	if (SWAPLOC_IDX_EQUAL(&ap->an_swaploc, ridx)) {
		/*
		 * Wait for any in progress pageouts to complete. Note that
		 * we are actually obtaining an ANON_T_LOCK here. In
		 * addition, page_setmod_iowait() has the beneficial side
		 * effect of setting the page dirty bit (which is of course
		 * required here).
		 */
		page_setmod_iowait(pp);
		PAGE_USELOCK(pp);
		anon_freeswap(ap, B_TRUE);
		PAGE_USEUNLOCK(pp);
	}

	/*
	 * All done.
	 */
	page_unlock(pp);
	anon_decref(ap);

	return; 
}

/*
 * anon_t *
 * anon_alloc(void)
 * 	Return a single anon ``slot''.
 *
 * Calling/Exit State:
 *	The caller does not hold any spinlocks on entrance to this
 *	function. The reason for this is that we may need to allocate more
 *	anon_t(s).
 *
 *	The anon free list fast spin lock is acquired as part of this
 *	function.
 *
 * 	The identity of the returned anon slot is guaranteed not to be
 * 	associated with any real memory. Any page which previously 
 * 	``assumed'' this identity is guaranteed to be ``clean'' and not 
 * 	on any identity list. 
 *
 *	Note that this function cannot fail since the caller is
 *	expected to have made a swap reservation by a previous call to
 *	mem_resv(..., M_SWAP).
 */
anon_t *
anon_alloc(void)
{
	anon_t *ap;
	ulong_t allocated;

	ASSERT(KS_HOLD0LOCKS());

	FSPIN_LOCK(&anon_free_lck);
	while (anon_free_listp == NULL) {
		/*
		 * Sample anon_clcur while we still hold the
		 * the anon_free_lck(). We need this value to pass
		 * into anon_expand_table().
		 */
		allocated = anon_clcur;
		FSPIN_UNLOCK(&anon_free_lck);

		/*
		 * The list was empty. So, expand it.
		 */
		ANON_TAB_LOCK();
		anon_expand_table(allocated);
		ANON_TAB_UNLOCK();

		FSPIN_LOCK(&anon_free_lck);
	}

	/*
	 * Remove an anon_t from the free list.
	 */
	ap = anon_free_listp;
	anon_free_listp = ap->an_next;
	++anon_allocated;
	FSPIN_UNLOCK(&anon_free_lck);

	/*
	 * No need for locking here as the anon_t is privately held.
	 */
	ASSERT(ap->an_refcnt == 0);
	ap->an_refcnt = 1;
	SWAPLOC_MAKE_EMPTY(&ap->an_swaploc);
	ap->an_page = NULL;
	ANON_SAVE_HIST(ap, ANON_H_CREATE);

	return (ap);
}

/*
 * int
 * anon_dup(anon_t **old, anon_t **new, u_int size)
 * 	Increment the references to size bytes worth of anon pages.
 * 	Used when duplicating a MAP_PRIVATE segment.
 *
 * Calling/Exit State:
 *	The caller must guarantee the coherence of the anon arrays pointed
 *	too by old and new. 
 *	The number of anon structs actually duped is returned.
 *
 */
int
anon_dup(anon_t **old, anon_t **new, u_int size)
{
        int i;
	int num_dup = 0;

        i = btopr(size);
	FSPIN_LOCK(&an_ref_lck);
        while (i-- > 0) {
                if ((*new = *old) != NULL) {
                        (*new)->an_refcnt++;
			num_dup++;
                }
                old++;
                new++;
        }
	FSPIN_UNLOCK(&an_ref_lck);
	return num_dup;
}


/*
 * STATIC void
 * anon_expand_table(ulong_t allocated)
 *	Expand the capacity of the anon_table by mapping some more pages
 *	of anon_t(s) into the anon cluster table.
 *
 * Calling/Exit State:
 *	Either the caller owns the ANON_TAB_LOCK, or the anon cluster
 *	table is privately held (i.e. this is being called at system
 *	initialization).
 *
 *      No spin LOCKs are held upon entry to this function. This function
 *	returns in the same state.
 */
STATIC void
anon_expand_table(ulong_t allocated)
{
	ulong_t nanon;
	anon_t *ap, *firstap, *lastap;
	anon_cluster_t *clp;
	vaddr_t addr;
	page_t *pp;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * If another lwp has allocated first, then our job is done.
	 */
	if (allocated < anon_clcur)
		return;

	/*
	 * We can almost ASSERT that anon_allocated < anon_clmax, but not
	 * quite. During swap delete, it is possible for a free anon_t to be
	 * held by anon_relocate(). It is not worth it to set up tight wakeup
	 * synchronization for this rarest of the rare cases. So, just sleep
	 * a little and return to anon_alloc().
	 */
	if (anon_allocated == anon_clmax) {
		LBOLT_WAIT(PRISLEP);
		return;
	}

	/*
	 * Okay, we are committed to the grow.
	 * First, locate the cluster we are about to grow.
	 */
	clp = anon_table[anon_tabgrow];
	ASSERT(clp);

	/*
	 * Compute the virtual address at which to grow.
	 */
	firstap = &clp->ancl_anonp[clp->ancl_pages];
	addr = (((vaddr_t)firstap - 1) & PAGEMASK) + PAGESIZE;

	/*
	 * Map the page into place.
	 */
	pp = page_get(PAGESIZE, SLEEP | P_NODMA);
	segkmem_pl_mapin(kpgseg, addr, 1, pp, PROT_READ | PROT_WRITE);

	/*
	 * Initialize the new anon_t(s). Thread them together in
	 * preparation for threading on the free list.
	 */
	nanon = (addr + PAGESIZE - (vaddr_t)firstap) / sizeof(anon_t);
	ap = firstap;
	lastap = firstap + nanon - 1;
	while (ap <= lastap) {
		SWAPLOC_MAKE_EMPTY(&ap->an_swaploc);
		ap->an_vidx = (uchar_t)anon_tabgrow;
		ap->an_refcnt = 0;
		ap->an_next = ap + 1;
#ifdef ANON_DEBUG
		ap->an_history = 0;
#endif ANON_DEBUG
		++ap;
	}

	/*
	 * Increment clp->ancl_pages anon_clcur ancl_pages by the number of
	 * new anon structures available. Note that we need to hold the
	 * anon_free_lck to write to anon_clcur. If necessary, advance
	 * anon_tabgrow.
	 */
	clp->ancl_pages += nanon;
	ASSERT(clp->ancl_pages <= clp->ancl_max);
	FSPIN_LOCK(&anon_free_lck);
	anon_clcur += nanon;
	if (clp->ancl_pages == clp->ancl_max)
		++anon_tabgrow;

	/*
	 * Attach the new anon_t(s) to the anon_free_list.
	 */
	lastap->an_next = anon_free_listp;
	anon_free_listp = firstap;
	FSPIN_UNLOCK(&anon_free_lck);
}

/*
 * void
 * anon_decref(anon_t *ap)
 * 	Decrement the reference count on an anon slot. If the count goes to 
 *	zero, free the slot and ATTEMPT a synchronous non-blocking abort. 
 *	If successful, free the swap space and mark the page as aborted. If
 *	the attempt fails, then anon_alloc will handle the situation on
 *	reuse. Hopefully, the page will be gone by then anyway.
 *
 * Calling/Exit State:
 *	There are no special entry conditions to this function. If the refcnt
 *	on the anon struct passed in drops to 0, anon_decref not attempt to 
 *	block (i.e. unconditionally abort page identity which will require 
 *	exclusive access to the page) since the caller may be a ZOMBIE (i.e
 * 	running on a plocal stack) and be unable to block. It is not expected 
 *	that inability to abort the page synchronously without first blocking 
 *	will be a frequent event. Furthermore, anonfs guarantees that a page 
 *	so freed (asynchronously aborted) will not be resued without first 
 *	re-attempting the abort.
 *
 *	If an anon struct is passed in with a refcnt == 1 and an_page is
 *	set then it is expected that the caller has removed the page from 
 *	any p_mapping chains and that the page is free.
 *
 * Remarks:
 *	Previous versions of SRV4 were unable to perform synchronous aborts
 *	on the page in the case that the caller was a zombie process
 *	freeing up its u-block. u-block pages are now memfs, so that this
 *	is no longer a concern.
 */
void
anon_decref(anon_t *ap)
{
	vnode_t *vp;
	off_t off;
	page_t *pp;

	ANON_SAVE_HIST(ap, ANON_H_DECREF);
	FSPIN_LOCK(&an_ref_lck);

        ASSERT(ap->an_refcnt != 0);

        if (--ap->an_refcnt) {
		FSPIN_UNLOCK(&an_ref_lck);
                return;
	} 

	FSPIN_UNLOCK(&an_ref_lck);

        /* 
	 * Get the identity of this guy.
         */
        anon_antovp(ap, &vp, &off);

	/*
	 * If this slot had an active translation, take care of 
	 * cleaning it now. The page will already have been freed
	 * and is on the freelist. 
	 *
	 * NOTES:
	 *
	 *	an_page is sampled without any special locking. It will be
	 *	in one of two possible states: NULL (no page currently
	 *	exists with this identity) or set. If set, it is possible
	 *	that the page is being reused and that the current identity
	 *	is being concurrently aborted by an_hashout. However,
	 *	an_hashout will not touch an_page because an_ref_cnt == 0.
	 *	This means pp might be stale by the time we test it.
	 *	page_abort_identity() tolerates abort races by (in effect)
	 *	acquiring the ANON_T_LOCK (see comments in file mem/anon.h)
	 *	before it actually aborts the page.
	 *
	 *	The only transitions of an_page from NULL to non-NULL occur
	 *	in anon_zero() and anon_getpage(). In those cases
	 *	an_refcnt > 0.
	 *
	 * This code assumes ATOMIC access to pointers.
	 */
	pp = ap->an_page;
	if (pp)
		page_abort_identity(pp, vp, off);

	/*
	 * We pass B_FALSE to this function because this
	 * page ONLY exists on swap and is therefore not
	 * doubly associated.
	 */
	anon_freeswap(ap, B_FALSE);

	/*
	 * Page aborted and swap freed. This anon_t is now suitable
	 * for the free list.
	 */
	FSPIN_LOCK(&anon_free_lck);
	ap->an_next = anon_free_listp;
	anon_free_listp = ap;
	--anon_allocated;
	FSPIN_UNLOCK(&anon_free_lck);
}

/*
 * void
 * anon_free(anon_t **app, u_int size)
 * 	Free a group of "size" anon pages, size in bytes.
 *
 * Calling/Exit State:
 *	As this function is essentially a wrapper to anon_decref, see
 *	the comment block for that function for substantive information.
 */
void
anon_free(anon_t **app, u_int size)
{
        int i;

        i = btopr(size);
        while (i-- > 0) {
                if (*app != NULL) {
                        anon_decref(*app);
                        *app = NULL;
                }
                app++;
        }
}

/*
 * page_t *
 * anon_zero(anon_t **app, mresvtyp_t mtype)
 * 	Allocate a private zero-filled anon page.
 *
 * Calling/Exit State:
 *	The caller must be prepared to block and hold no spinlocks.
 *
 *	This function cannot fail (i.e. will block waiting for memory) and 
 *	always passes the caller a pointer to the newly created page. The page 
 *	is writelocked and it is the caller's responsibility to link it into 
 *	p_mapping (to mark it ``IN_USE'') and call page_unlock. 
 *
 *	The only valid values for mtype are M_REAL, M_REAL_USER, and M_NONE.
 *	Value M_REAL and M_REAL_USER indicate that the caller holds a
 *	reservation which is to be assigned to the new anon page identity via
 *	pvn_cache_memresv().
 *
 * Remarks:
 *	This function has been cloned into anon_zero_aligned -- see below.
 *	Most changes must be reflected in both.
 */
page_t *
anon_zero(anon_t **app, mresvtyp_t mtype)
{
        anon_t *ap;
        page_t *pp;
        vnode_t *vp;
	off_t off;

	ASSERT(*app == NULL);
	ASSERT(mtype == M_NONE || mtype == M_REAL || mtype == M_REAL_USER);

	/*
	 * Get a clean anon slot. This operation cannot fail. We pass
	 * an argument of 0 to anon_alloc to indicate that we can block
	 * if necessary to clean a slot.
	 */

        ap = anon_alloc();

	ASSERT(ap != NULL);

        anon_antovp(ap, &vp, &off);

	/*
	 * Note that page_create will mark the page modified for us
	 */

	pp = page_create(vp, off);
	ASSERT(pp);
	ASSERT(PAGE_IS_RDLOCKED(pp));

        ap->an_page = pp;
	ANON_PGPRV(pp)->a_ap = ap;

        /*
         * Now zero the contents of the page, which is held by the caller.
	 * Even though the target page is only read locked, it is also 
	 * privately held. So therefore, the pagezero is permitted here.
	 */

        pagezero(pp, 0, PAGESIZE);
	ASSERT(!SV_BLKD(&(pp)->p_wait));        /* no waiters on page */

	/*
	 * If so requested, cache memory reservation done for this
	 * page to pvn.
	 */

	if (mtype != M_NONE)
		pvn_cache_memresv(vp, off, mtype);

        lm.cnt.v_zfod++;

	/*
	 * finally, fill the anon_map slot
	 */
        *app = ap;

        return (pp);
}

/*
 * page_t *
 * anon_private(anon_t **app, const page_t *opp, mresvtyp_t mtype)
 * 	Turn a reference to a shared anon page into a private
 * 	page with a copy of the data from the original page.
 *
 * Calling/Exit State:
 *	On entry, the caller guarantees that the segment is appropriately
 *	locked. This routine cannot fail unless flags.
 *
 *	The the newly created page will be returned PAGE_RDLOCKed. 
 *
 *	The only valid values for mtype are M_REAL, M_REAL_USER, and M_NONE.
 *	Value M_REAL and M_REAL_USER indicate that the caller holds a
 *	reservation which is to be assigned to the new anon page identity via
 *	pvn_cache_memresv().
 *
 */
page_t *
anon_private(anon_t **app, const page_t *opp, mresvtyp_t mtype)
{
        anon_t *new;
        page_t *pp;
        vnode_t *vp;
	off_t off;

	ASSERT(mtype == M_NONE || mtype == M_REAL || mtype == M_REAL_USER);

        /*
	 * Get a new anon slot.
	 */

        new = anon_alloc();
	ASSERT(new);

	/*
	 * Allocate the new page.
	 */

	anon_antovp(new, &vp, &off);

	/*
	 * Note that page_create will mark the page modified for us
	 */

	pp = page_create(vp, off);
	ASSERT(pp);
	ASSERT(PAGE_IS_RDLOCKED(pp));

        new->an_page = pp;
	ANON_PGPRV(pp)->a_ap = new;

        /*
         * Now copy the contents from the original page,
         * which is held by the caller. Even though the target page is
         * only read locked, it is also privately held. So therefore,
         * the ppcopy is permitted here.
         */

        ppcopy((page_t *)opp, pp);
	ASSERT(!SV_BLKD(&(pp)->p_wait));        /* no waiters on page */

       	*app = new;

        /*
         * If we have been requested to cache a memory reservation
	 * by our caller, do so now.
         */

	if (mtype != M_NONE)
		pvn_cache_memresv(vp, off, mtype);

        return (pp);
}

/*
 * int 
 * anon_getpage(anon_t **app, u_int *protp, page_t *pl[], u_int plsz,
 *              enum seg_rw rw, cred_t *cred)
 *	Bring anonymous pages back in from swap or reclaim them from the
 *	page cache.
 *
 * Calling/Exit State:
 *	Called with the segment read or write locked as appropriate.
 *
 *	On success, we return zero and pl is filled out with the requested 
 *	anonymous pages. The requested pages will be PAGE_RDLOCK()ed and 
 *	contain valid data. protp will be set to indicate the appropriate 
 *	virtual protections (based on the refcnt of the anon structure).	
 *
 *	On failure, no pages are returned and the return value is a non-
 *	zero error code.
 *
 *	Called at PLBASE with no spin LOCKs held, and returns the same way.
 *	The caller is prepared to block.
 *
 * Remarks:
 * 	Currently no one calls this function with a plsz greater than
 *	PAGESIZE (i.e. we only ever request a single page). This is
 *	really the only sane thing to ask for since we only reclaim
 *	a single page and if we need to call VOP_GETPAGELIST there is
 *	the problem that we need to `create' pages for the anode which
 *	may not exist or may not have backing on the same device the 
 *	requested page is using.
 *
 */
/* ARGSUSED */
int 
anon_getpage(anon_t **app, u_int *protp, page_t *pl[], u_int plsz,
             enum seg_rw rw, cred_t *cred)
{
	anon_t *ap = *app;
	vnode_t *vp;
	off_t off;
	page_t *pp;
	int err;
	swapinfo_t *rswp;
	vnode_t *stvp;
	off_t roff;
	void *storeobj;

	ASSERT(plsz == PAGESIZE);
	ASSERT(ap != NULL);
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
         * Return permissions based on the refcnt. If the count
         * is 1, then the page is assumed to be private to the
         * caller. If > 1, then we assume a shared mapping and
         * return permissions appropriate for COW.
         */

	if (ap->an_refcnt == 1)
		*protp = PROT_ALL;
	else
		*protp = PROT_ALL & ~PROT_WRITE;

	/*
	 * Call lookup or create for the single requested page. If
	 * we find it (page comes back PAGE_RDLOCK()ed), then our
	 * work is done. If we can't reclaim the page (page comes
	 * back PAGE_WRLOCK()ed) then we will need to call VOP_GETPAGELIST
	 * to restore it. 
	 */

	anon_antovp(ap, &vp, &off);
	pp = page_lookup_or_create3(vp, off, P_NODMA);
	pl[0] = pp;
	pl[1] = NULL;

	if (PAGE_IS_RDLOCKED(pp)) {
		ASSERT(ap->an_page && pp == ap->an_page);
		ASSERT(ANON_PGPRV(pp)->a_ap == ap);
		ANON_SAVE_HIST(ap, ANON_H_GETPAGE);

		/*
		 * PERF (swap space usage):
		 *
		 * If this is an S_WRITE operation, it might be a good idea
		 * to break double association here. However, we would
		 * first have to wait for any potential page cleanings in
		 * progress at this time (i.e. we would need to call
		 * page_setmod_iowait(pp)). We choose to avoid this overhead.
		 */

		return 0; 
	}

	/*
	 * Set an_page and `short cut' pointer in page structure.
	 *
	 * Note that the pseudo ANON_T_LOCK is held at this point. This is
	 * required to protect the an_page, a_ap, and an_swaploc fields.
	 */

	ap->an_page = pp;
	ANON_PGPRV(pp)->a_ap = ap;

	/*
	 * If an I/O error previously occured in anon_getpage(), when the
	 * swap file was being deleted, then we need to return an EIO now.
	 */
	if (SWAPLOC_IS_IOERR(&ap->an_swaploc)) {
		page_abort(pp);
		return (EIO);
	}

	/*
	 * The page is not in core so we need to bring it back from swap.
	 * It comes back filled with data and read locked. We wait for 
	 * this I/O to complete.
	 */

	ASSERT(ANON_HAS_SWAP(ap));
	rswp = swaptab[SWAPLOC_TO_IDX(&ap->an_swaploc)];
	stvp = rswp->si_stvp;
	roff = SWAPLOC_TO_OFF(&ap->an_swaploc);
	storeobj = rswp->si_obj;

	/*
	 * Note: to get the right accounting for swapdoubles (which
	 * will be decremented via page_abort/anon_hashout if the I/O
	 * fails!) we need to increment it first. Sigh.
	 */

       	(void) LOCK(&swap_lck, VM_SWAP_IPL);
        swapdoubles++;
       	UNLOCK(&swap_lck, PLBASE);

	err = VOP_GETPAGELIST(stvp, roff, PAGESIZE, pp, storeobj, 0, cred);	
	ANON_SAVE_HIST(ap, ANON_H_GETGETPAGE);

	if (err) {
		/*
		 * VOP_GETPAGELIST has already taken care of aborting
		 * the page.
		 */
		return (err);
	}

	ASSERT(PAGE_IS_WRLOCKED(pp));

	/*
	 * Return the swap space we were using if this page is being soiled. 
	 * Next time this page is pushed we'll allocate it again. Otherwise
	 * we have just created a new doubly associated page and leave the
	 * already adjusted swapdoubles counter.
	 *
	 * Note that we pass B_TRUE to anon_freeswap because we incremented 
	 * swapdoubles before the I/O (because of the error case potential).
	 *
	 * We still hold the ANON_T_LOCK because the page is still WRITEr
	 * locked.  The ANON_T_LOCK is required to call anon_freeswap().
	 *
	 * Even though we are writing, we need to dirty the page (page_setmod),
	 * for we might not actually get to write before we get swapped out.
	 */

	if (rw == S_WRITE)  {
		page_setmod(pp);
		anon_freeswap(ap, B_TRUE);
		ANON_SAVE_HIST(ap, ANON_H_GETBRK);
	}

	/*
	 * downgrade the lock for the caller
	 */
	page_downgrade_lock(pp);

	return 0;
}

/*
 * void
 * anon_freeswap(anon_t *ap, boolean_t breakdouble)
 *	delete physical backing storage for this an struct (if any allocated)
 *
 * Calling/Exit State:
 *	There are no special entry conditions to ASSERT about the anon_t  
 *	passed in. The structure may or may not still be in use and may 
 *	or may not have actual backing storage allocated.
 *
 *	breakdouble is passed opaquely to swap_free; see this functions
 *	for details of how its used.
 *
 *	The caller holds the pseudo ANON_T_LOCK for *ap (see the comments
 *	in mem/anon.h, immediately above the definition of the anon_t
 *	structure).
 * 
 *	If the passed structure is currently using physical backing storage,
 *	then the block is 'freed' back to the bitmap used by the swapping
 *	subsystem for this purpose, free counters are incremented, and the
 *	ap->an_swaploc field is set to indicate that no backing store is
 *	allocated.
 *
 */
void
anon_freeswap(anon_t *ap, boolean_t breakdouble)
{
	if (ANON_HAS_SWAP(ap)) {
		swap_free(1, &ap->an_swaploc, breakdouble);
		ANON_SAVE_HIST(ap, ANON_H_SWFREED);
		SWAPLOC_MAKE_EMPTY(&ap->an_swaploc);
	}
}

/*
 * void
 * anon_hashout(page_t *pp)
 *	Called to NULL an_page and as sanity function for page_abort.
 *
 * Calling/Exit State:
 *	Page is PAGE_USELOCK()ed, but we don't ASSERT this. 
 *
 * Remarks:
 *	an_page is cleared here without any interlocks. Functions which
 *	use this field of the anon struct (page_abort_identity, indirectly)
 *	do so in a manner while lets them tolerate it transitioning to NULL
 *	(i.e. processing a stale pp). 
 * 
 *	We are called because this anon page is being deleted from the
 *	cache either via anon_decref/page_abort_identity or via page_reuse.
 *	In either case we want to remove swap and have swapdoubles adjusted
 *	to indicate this page only consumes real memory.
 */
void
anon_hashout(page_t *pp)
{
	anon_t *ap;
	pl_t ospl;

	ASSERT(IS_ANON_PAGE(pp));

	/*
	 * Convert page to anon pointer
	 */

	ap = anon_pptoan(pp);

	ANON_SAVE_HIST(ap, ANON_H_HASHOUT);

	/*
	 * If the anon_t is not free, then clear ap->an_page.
	 */
	FSPIN_LOCK(&an_ref_lck);
	if (ap->an_refcnt != 0) {
		ASSERT(ap->an_page == pp);
		ap->an_page = NULL;
	}
	FSPIN_UNLOCK(&an_ref_lck);

	/*
	 * If this page still has a swap component
	 * then we are throwing away its incore portion. Adjust 
	 * swapdoubles to reflect this change.
	 *
	 * If the swap space is being deleted, then nobody is ever going to
	 * be able to read it again. We need to mark the anon_t as such,
	 * especially for the case where the abort was caused by an I/O
	 * error within anon_relocate().
	 */

	if (ANON_HAS_SWAP(ap)) {
		if (pp->p_invalid &&
		    (swaptab[SWAPLOC_TO_IDX(&ap->an_swaploc)]->si_flags &
			     ST_INDEL)) {
			swap_free(1, &ap->an_swaploc, B_TRUE);
			SWAPLOC_MAKE_IOERR(&ap->an_swaploc);
			ANON_SAVE_HIST(ap, ANON_H_HASHOUT_FREE);
		} else {
			ospl = LOCK(&swap_lck, VM_SWAP_IPL);
			swapdoubles--;    
			UNLOCK(&swap_lck, ospl);
			ANON_SAVE_HIST(ap, ANON_H_HASHOUT_DBL);
		}
	}
}

/*
 * int 
 * anon_pageout(int pgs, clock_t stamp, clock_t interval)
 *      Push up to pgs pages out to backing storage from the
 *      page_dirtyalist. Actual number of pages taken depends
 *	on the length of the list and aging information (stamp and
 *	interval) passed in by our caller. Returns the actual number of 
 * 	pages taken.
 *
 * Calling/Exit State:
 *      Called without any MP locks held and returns the same way. Since we
 *	may wind up calling VOP_PUTPAGELIST, we cannot tolerate being called
 *	with any locks either. 
 * 
 *	Acquires the swap_lck to atomically determine if swap space exists 
 *	to push the requested number of pages (consults nswappgfree). The 
 *	minimum of pgs and the number of free swap space will be passed as 
 *	an argument to page_anonpageout. Fewer pages may be returned if the 
 *	dirtyalist is not long enough or pages on it are not yet sufficiently 
 *	aged.
 *
 *	If one or more swap devices are configured then on exit up to pgs 
 *	worth of pages will have been asynchronously pushed to one or more 
 *	swap devices. 
 *
 * Remarks:
 *      All pages on the dirtyalist are now ``free'' as the result of having
 *      been passed to page_free (who put them on this list to begin with).
 *      As such, anon_freeswap was called for each of them and they no
 *      longer have any backing store (so called ``double association'').
 *
 * 	If no swap devices are configured then this routine will return
 *	leaving all pages on the dirtyalist `as is' to be reclaimed or
 *	aborted. Systems with no swap space should be tuned such that 
 *	swapping does NOT kick in before process growth is limited by
 *	memresv(M_SWAP) as this will impact system performance without
 *	reducing memory demands. The page aging interval should also be
 *	lengthened.
 *
 *	Note on use of swapdoubles and page_swapreclaim(B_TRUE):
 *	========================================================
 *
 *	Whenever an anon page is first pushed to swap it consumes both an 
 *	incore page (an_page != NULL) and swap space (an_stvp != NULL). At 
 *	this point the page is said to be `doubly associated.' Pages remain 
 *	doubly associated until they are: 
 *
 *		- cleaned and reused for another identity (since we can 
 *		  bring the page in from swap to reconstitute it)
 *
 * 	or:
 *
 *		- brought in from swap to be immediately modified (S_WRITE 
 *		  fault)
 *
 * 	or:
 * 		- aborted when they are freed and an_refcnt transitions from 
 *		  1 to 0. 
 *
 *	As explained in vm_page.c/page_swapreclaim, double association is 
 *	tolerated to the extent that it provides an optimization (pages don't 
 *	need to be paged in as often) and does not interfere with the normal 
 *	working assumptions of the VM subsystem (that anon pages only consume 
 *	real OR swap memory resources, never both). As the system experiences 
 *	greater degrees of memory duress, fewer and fewer doubly associated 
 *	pages can be tolerated.
 *
 *	Once each second (via pooldaemon()) page_swapreclaim is called to
 *	scan a fraction of physical memory looking for doubly associated
 *	pages whose space space can be reclaimed. In addition, whenever we 
 *	run out of swap space (nswappgfree == 0) and have any doubly 
 *	associated pages we need to scan ALL of physical memory and force
 *	all such pages to give up their swap resources lest the system 
 *	deadlock on us. 
 *
 *	The counter swapdoubles is used for this purpose. It is incremented
 *	here in anon_pageout when pages are first doubly associated and
 *	then decremented as pages give up either their real or swap components
 *	as detailed in the list above.
 */
int 
anon_pageout(int pgs, clock_t stamp, clock_t interval)
{
	pl_t ospl;
	int maxpgs;
	page_t *plist;
	page_t *pp;

	ASSERT(KS_HOLD0LOCKS());

	ospl = LOCK(&swap_lck, VM_SWAP_IPL);

	/*
	 * Our first task is to gather dirty pages and swap space
	 * We consult nswappgfree and see if we need to `clip' the
	 * pgs argument passed to us as we can't push more than we have
	 * swap space for. We then ask the page subsystem to gather the
	 * pages up (we may actually get less).
	 *
	 * Note that we have a lock ordering problem and can't hold 
	 * swap_lck across the call to page_anonpagepout. As a result
	 * we decrement our nswappgfree to `reserve' swap space before
	 * we make the call to swap_alloc. This is safe because delswap
	 * requests will take our reservation into account if one occurs
	 * before we actually call swap_alloc.
	 */

	maxpgs = min(pgs, nswappgfree);

	/*
	 * Just return if no swap space is available. 
	 */

	if (maxpgs == 0) {
		UNLOCK(&swap_lck, ospl);

		/*
		 * If there are any doubly associated pages lurking around,
		 * now is the time to aggressively free them up since we are
		 * out of swap space and clearly low on memory too. Passing 
		 * B_TRUE to page_swapreclaim() forces it to modifiy all 
		 * doubly associated pages and break swap. All pages on the
		 * system are scanned. You can run, but you can't hide.
		 */

		if (swapdoubles) 
			page_swapreclaim(B_TRUE);
		
		return 0;
	}
	nswappgfree -= maxpgs;
	UNLOCK(&swap_lck, ospl);

	plist = NULL;
	pgs = page_anonpageout(maxpgs, stamp, interval, &plist);

	/*
	 * Get swap_lck again for the actual calls to swap_alloc
	 */

	ospl = LOCK(&swap_lck, VM_SWAP_IPL);

	/*
	 * Adjust our swap free count for the number of pages actually needed.
	 */
	if (pgs < maxpgs) {
		nswappgfree += (maxpgs - pgs);
		maxpgs = pgs;

		/*	
		 * No pages on dirtyalist or nothing old enough to push.
		 */
		if (maxpgs == 0) {
			UNLOCK(&swap_lck, ospl);
			return 0;
		}
	}

	/*
	 * Make a `scratch' copy of the pointer we set via page_anonpageout.
	 * We save plist for the call(s) to VOP_PUTPAGELIST. Also remember 
	 * how many pages we're going to push in maxpgs.
	 */

	pp = plist;

	/*
	 * Next task is to get swap space and associate it with the pages we
	 * got back from page_anonpageout. There's enough space but it may
	 * not all be on the same swap device. We loop around, calling 
	 * swap_alloc to get as much space as possible at once. If we need to
	 * break the request up, we'll start backing off and breaking the
	 * request into smaller piceces. In this case, we use a_next 
	 * pointers in the pages to link the smaller lists together before 
 	 * dropping swap_lck and making VOP_PUTPAGELIST calls.
	 */
	
	while (pgs) {

		int pgs2;
		swaploc_t swaploc;
		anon_t *ap;
		page_t *basepp;
		int i; 

		pgs2 = pgs;

		/*
 	 	 * Attempt to allocate a chunk of swap space. We always
 	 	 * try for all of what's left to push (initially everything)
	 	 * but back off by halving this if we can't find a single
	 	 * chunk large enough on any device.
	 	 */

		while (!(swap_alloc(pgs2, &swaploc))) {

			ASSERT(pgs2 != 1);

			/*
		 	 * Couldn't find a single chunk big enough to
		 	 * handle pgs worth of pages. Back off by half 
			 * and try again. 
		 	 */

			pgs2 = pgs2 / 2;
		}

		/*
	 	 * Got space. Now loop through the pages on the list we got
	         * back from page_anonpageout, convert pp's to ap's and fill
	  	 * these out. When we are done basepp will point to the head
		 * of the list and pp will point to the p_next of the last
		 * page we filled out. If we processesed everything, then pp
		 * will point to basepp.
	 	 *
 	 	 * Note that if we are processing less than pgs worth of pages
	 	 * from the list we need to break the list into two pieces.
	 	 */

		 for (i = 0, basepp = pp; i < pgs2; pp = pp->p_next, i++) {

			ASSERT(pp != NULL);

			ap = anon_pptoan(pp);

                	ASSERT(SWAPLOC_IS_EMPTY(&ap->an_swaploc));

                	ap->an_swaploc = swaploc;
			ANON_SAVE_HIST(ap, ANON_H_PGO_SWP);
			SWAPLOC_ADD_OFFSET(&swaploc, PAGESIZE);
		}

		/*
		 * Less than entire list processed? Split this doubly
		 * linked circular list into two pieces and set plist
		 * to where we left off. We could do page_adds and subs
		 * but that's really inefficient.
		 */

		if (pgs2 < pgs) { 
			page_t *tpp;

			ASSERT(pp != basepp);

			pp->p_prev->p_next = basepp;
			tpp = basepp->p_prev;
			basepp->p_prev = pp->p_prev;

			tpp->p_next = pp;
			pp->p_prev = tpp;
					
			/*
			 * `thread' the two lists together via the a_next
			 * linkage.
			 */
		
			ANON_PGPRV(basepp)->a_next = pp;
			ANON_PGPRV(pp)->a_next = NULL;
		}
		else
			ANON_PGPRV(basepp)->a_next = NULL;

		pgs -= pgs2;
	}

	/*
	 * Increment swapdoubles counter since we have just created
	 * a bunch of doubly associated pages. See page_swapreclaim() for
	 * details. 
	 */

	swapdoubles += maxpgs;

	/*
	 * Done with the lock
	 */

	UNLOCK(&swap_lck, ospl);

	/*
	 * Final step: reset pp and start pushing pages out. If we had to
	 * break the request into smaller pieces than two or more page
	 * lists will be threaded together via a_next. These are
	 * pushed seperately.
	 */

	pp = plist;

	while (pp) {
		page_t *pp2; 
		anon_t *ap;
		swapinfo_t *sip;
		off_t off;
#ifdef DEBUG
		int err;
#endif

		pp2 = pp;

		/*
		 * Convert first pp in list to ap; get sip and base off.
		 */

		ap = anon_pptoan(pp2);
		sip = swaptab[SWAPLOC_TO_IDX(&ap->an_swaploc)];
		ASSERT(sip != NULL);
		off = SWAPLOC_TO_OFF(&ap->an_swaploc);
		ASSERT(ANON_HAS_SWAP(ap));

		/*
		 * Set pp to next (potential) list and clear a_next in
		 * the head pp (pp2) of the list we're about to push.
		 */

		pp = ANON_PGPRV(pp2)->a_next;
		ANON_PGPRV(pp2)->a_next = NULL;
		
		/*
	 	 * Push the gathered pages out.
	 	 */

#ifdef DEBUG
		err =
#else
		(void)
#endif
        	VOP_PUTPAGELIST(sip->si_stvp, off, pp2, sip->si_obj,
			        B_ASYNC | B_PAGEOUT, sys_cred);

        	ASSERT(err == 0);
	}

	return (maxpgs);	
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_anon_info(void)
 *	Debug only function to print information about anonfs to the
 *	console.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
void
print_anon_info(void)
{
	int i, onswap, free, incore;
	int tot_alloc, tot_onswap, tot_free, tot_incore;
	int flist_size;
	anon_cluster_t *clp;
	anon_t *ap, *eap;

	/*
	 * Go through the anon_table and print information on each cluster
	 */
	tot_alloc = tot_onswap = tot_free = tot_incore = 0;
	for (i = 0; i < anon_tabinuse; ++i) {
		clp = anon_table[i];
		onswap = free = incore = 0;
		ap = clp->ancl_anonp;
		eap = ap + clp->ancl_pages;
		while (ap < eap) {
			if (ap->an_refcnt == 0)
				++free;
			else if (ap->an_page)
				++incore;
			if (ANON_HAS_SWAP(ap))
				++onswap;
			++ap;
		}
		debug_printf("cluster(%d) at %x:\n",  i, clp);
		debug_printf("    %d pages, %d max, %d free, %d onswap,"
			      " %d incore\n",
			     clp->ancl_pages, clp->ancl_max, free, onswap,
			     incore);
		if (debug_output_aborted())
			return;
		tot_alloc += clp->ancl_pages;
		tot_onswap += onswap;
		tot_free += free;
		tot_incore += incore;
	}

	/*
	 * go through the free list and count
	 */
	ap = anon_free_listp;
	flist_size = 0;
	while (ap) {
		++flist_size;
		ap = ap->an_next;
	}

	/*
	 * Now print summary information.
	 */
	debug_printf("%d allocated, %d on-swap, %d free, %d incore\n",
	             tot_alloc, tot_onswap, tot_free, tot_incore);
	debug_printf("%d on the free list\n", flist_size);
	debug_printf("anon_max = %d, anon_clmax = %d,"
		      " anon_clcur = %d, anon_allocated = %d\n",
		     anon_max, anon_clmax, anon_clcur, anon_allocated);
}

#endif /* DEBUG || DEBUG_TOOLS */


#ifdef ANON_DEBUG

/*
 * void
 * print_anon_hist(const anon_t *ap)
 *	Debug only function to print the history information from
 *	an anon_t.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */

void
print_anon_hist(const anon_t *ap)
{
	uint_t action, history;
	int cursor;
	static char h_create[] = "created by anon_alloc()";
	static char h_decref[] = "anon_decref() called";
	static char h_getpage[] = "found in memory by anon_getpage()";
	static char h_getgetpage[] = "read from disk by getpage()";
	static char h_hashout[] = "page discarded by anon_hashout()";
	static char h_hashout_free[] = "swap freed by anon_hashout()";
	static char h_hashout_dbl[] = "doubles broken by anon_hashout()";
	static char h_swfreed[] = "swap freed by anon_freeswap()";
	static char h_getbrk[] = "anon_getpage() breaks doubles";
	static char h_recbrk[] = "page_swapreclaim() breaks doubles";
	static char h_pagefreeswap[] = "page_free_l() frees swap space";
	static char h_pgo_swp[] = "anon_pageout() allocates swap";
	static char h_reclaim[] = "anon_exists_uselock() reclaims";
	static char h_relocate[] = "anon_relocate() called";
	static char h_relocate_get[] = "anon_relocate() calls anon_getpage()";
	static char *history_titles[] = {h_create, h_decref, h_getpage,
		h_getgetpage, h_hashout, h_hashout_free, h_hashout_dbl,
		h_swfreed, h_getbrk, h_recbrk, h_pagefreeswap,
		h_pgo_swp, h_reclaim, h_relocate, h_relocate_get};

	debug_printf("History of anon struct @%08x\n", ap);
	history = ap->an_history;
	for (cursor = 28; cursor >= 0; cursor -= 4) {
		action = ((uint_t)history >> cursor) & 0xf;
		if (action != 0)
			debug_printf("       %s\n", history_titles[action - 1]);
	}
}

/*
 * void
 * anon_save_history(anon_t *ap, uint_t h)
 *	Debug only function to save some history in an anon_t.
 *
 * Calling/Exit State:
 * 	Called with anon_hist_lock unlocked and returns that way.
 *
 * Remarks:
 *	All calls to this function should be via the ANON_SAVE_HIST()
 *	macro.
 */

void
anon_save_history(anon_t *ap, uint_t h)
{
	FSPIN_LOCK(&anon_hist_lock);
	ap->an_history = (ap->an_history << 4) | h;
	FSPIN_UNLOCK(&anon_hist_lock);
}

#endif /* ANON_DEBUG */
