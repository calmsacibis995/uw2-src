/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/ublock.c	1.11"
#ident	"$Header: $"

/*
 * Ublock management --
 *
 * These routines manage collections of swappable kernel data attached
 * to processes, which are called extended ublocks.  An extended ublock
 * consists of exactly one LWP ublock for each LWP in a process, plus
 * zero or more ubmem objects (allocated by ubmem_alloc).  All of these
 * pieces will be swapped in and out together when the process is swapped.
 *
 * Whenever a ublock is actively in memory, either because the process is
 * swapped in, or because some kernel routine is accessing some other
 * process's extended ublock data, that ublock is locked and no faults
 * can occur.  Ublock data may only be accessed while the ublock is locked.
 */

#include <acc/priv/privilege.h>
#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <mem/faultcode.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/rff.h>
#include <mem/seg_kvn.h>
#include <mem/ublock.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

STATIC sleep_t ublock_resv_lock;

/*
 *+ sleeplock for private ublock memory reservation
 */
STATIC LKINFO_DECL(ublock_resv_lkinfo, "MP:ublock:ublock_resv_lock", 0);

/*
 *+ mutex for proc_ubinfo_t fields
 */
STATIC LKINFO_DECL(ub_lkinfo, "MP:ublock:ub_mutex", 0);

/*
 *+ mutex for ublock_swresv
 */
STATIC LKINFO_DECL(ublock_swresv_lkinfo, "MP:ublock:ublock_swresv_mutex", 0);

STATIC off_t ub_bs_alloc(proc_ubinfo_t *, uint_t);
STATIC void ub_bs_free(proc_ubinfo_t *, off_t, uint_t);

/*
 * The ublock subsystem maintains a private M_REAL reservation for
 * sched_swapin.
 */
uint_t ublock_swresv;
lock_t ublock_swresv_mutex;	/* mutex for ublock_swresv */

/*
 * Provisions for handling stack overflows onto the extension at switch
 * time.
 */
STATIC rff_t	ublock_rff;
STATIC fspin_t	ublock_rff_lock;
atomic_int_t	ublock_overflows;

/*
 * void
 * ublock_kmadv(void)
 *	Call kmem_advise() for ublock data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
ublock_kmadv(void)
{
	kmem_advise(sizeof(proc_ubinfo_t));
}

/*
 * void
 * ublock_calloc(void)
 *	Initialize the ublock's emergency stack overflow area.
 *
 * Calling/Exit State:
 *	Called kvm_init() time while still single-threaded.
 */
void
ublock_calloc(void)
{
	extern int ovstack_size;
	ulong_t size = ovstack_size * 1024;

	rff_init(&ublock_rff);
	rff_add_chunk(&ublock_rff, calloc(size), size);
	FSPIN_INIT(&ublock_rff_lock);
	ATOMIC_INT_INIT(&ublock_overflows, 0);
}

/*
 * void
 * ublock_init(void)
 *	Initialize the ublock subsystem.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single-threaded.
 */
void
ublock_init(void)
{
	STATIC void ublock_report(void);

	SLEEP_INIT(&ublock_resv_lock, 0, &ublock_resv_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&ublock_swresv_mutex, VM_UBRESV_HIER, PLMIN,
		  &ublock_swresv_lkinfo, KM_NOSLEEP);

	/*
	 * Hold onto one ublock's worth of M_REAL memory reservation,
	 * which can be used as a last resort for critical cases.
	 * Use of this reservation is serialized by the ublock_resv_lock.
	 */
	if (!mem_resv(USIZE, M_REAL) ||
	    !itimeout(ublock_report, NULL, (60 * HZ)|TO_PERIODIC, PLTIMEOUT)) {
		/*
		 *+ Insufficient memory was available to initialize the
		 *+ ublock subsystem.  Corrective action: reconfigure the
		 *+ system to use less memory or add more physical memory.
		 */
		cmn_err(CE_PANIC, "ublock_init: not enough memory");
	}

	ublock_init_f();
}


/*
 * int
 * ublock_proc_init(proc_t *procp)
 *	Initialize the ublock info for a (newly-created) process.
 *
 * Calling/Exit State:
 *	The process must be privately held, so no locking is needed here.
 *	The extended ublock created here is still zero length.  Space
 *	must subsequently be allocated with ublock_lwp_alloc() or
 *	ubmem_alloc().
 *
 *	This routine may block, so no locks may be held on entry, and
 *	none will be held on exit.
 *
 *	Returns 0 for success, else an errno.
 */
int
ublock_proc_init(proc_t *procp)
{
	proc_ubinfo_t *pubp;

	ASSERT(KS_HOLD0LOCKS());

	pubp = kmem_zalloc(sizeof(proc_ubinfo_t), KM_SLEEP);

	pubp->ub_mvp = memfs_create_unnamed(0,
				MEMFS_NPGZERO|MEMFS_NPASTEOF|MEMFS_NSINACT);
	ASSERT(pubp->ub_mvp != NULL);

	LOCK_INIT(&pubp->ub_mutex, VM_UBLOCK_HIER, PLMIN, &ub_lkinfo,
		  KM_SLEEP);
	SV_INIT(&pubp->ub_sv);

	pubp->ub_refcnt = 1;
	procp->p_ubinfop = pubp;

	return 0;
}


/*
 * void
 * ublock_proc_deinit(proc_t *procp)
 *	De-init ublock info for a process before destroying the process.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the process is single-threaded
 *	and commited to destruction at this point.
 *
 * Remarks:
 *	Even though the process is single-threaded, we still need to
 *	acquire ub_mutex here, since a previously-exiting LWP might
 *	have a reference to the proc_ubinfo_t struct from a "detached"
 *	ublock.
 */
void
ublock_proc_deinit(proc_t *procp)
{
	proc_ubinfo_t *pubp = procp->p_ubinfop;
	void *mapcookie;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	while ((mapcookie = pubp->ubmem_head.ubmem_nextmapcookie) != NULL) {
		pubp->ubmem_head = *pubp->ubmem_head.ubmem_next;
		segkvn_unlock(mapcookie,
			      SEGKVN_MEM_LOCK|SEGKVN_NO_MEMRESV|SEGKVN_DISCARD);
		segkvn_mapout(mapcookie);
	}

	VN_RELE(pubp->ub_mvp);

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	if (pubp->ub_locks[UB_NOSWAP_USER])
		mem_unresv(pubp->ub_npages, M_REAL_USER);
	else if (pubp->ub_locks[UB_NOSWAP])
		mem_unresv(pubp->ub_npages, M_REAL);

	ASSERT(pubp->ub_refcnt != 0);
	if (--pubp->ub_refcnt != 0) {
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
	} else {
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
		LOCK_DEINIT(&pubp->ub_mutex);
		if (pubp->ub_level != 0)
			kmem_free(pubp->ub_ubitmap.uub_ibitmapp,
				  UB_NBYTES(pubp->ub_level));
		kmem_free(pubp, sizeof(proc_ubinfo_t));
	}
}


/*
 * vaddr_t
 * ublock_lwp_alloc(lwp_t *lwpp)
 *	Allocate an LWP ublock for the given LWP.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the indicated LWP is privately held
 *	(not yet on its process's p_lwpp chain), and that lwpp->l_procp
 *	has been initialized.  The caller must also hold a ub-lock (of any
 *	type) on the process's extended ublock (see ublock_lock), and
 *	continue to hold it until the LWP is attached to the p_lwpp chain.
 *
 *	This routine may block, so no locks may be held on entry, and
 *	none will be held on exit.
 *
 *	Returns the virtual address of the LWP's ublock on success,
 *	or zero on failure (due to lack of resources).
 */
vaddr_t
ublock_lwp_alloc(lwp_t *lwpp)
{
	proc_ubinfo_t *pubp = lwpp->l_procp->p_ubinfop;
	lwp_ubinfo_t *lubp = &lwpp->l_ubinfo;
	vaddr_t ubaddr;
	off_t bs_off;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * In order to prevent real memory from becomming
	 * overcommitted due to LWP usage, always check
	 * if M_BOTH_USER has been exhausted.
	 *
	 * P_SYSOPS is exempt from this check, so that the operator can
	 * still poke around and fix things while the machine is in the
	 * overcommited state.
	 */
	if (!mem_resv_check() && pm_denied(CRED(), P_SYSOPS))
		return 0;

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	if (pubp->ub_locks[UB_NOSWAP_USER]) {
		if (!mem_resv(USIZE, M_REAL_USER)) {
			UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
			return 0;
		}
	} else if (pubp->ub_locks[UB_NOSWAP]) {
		if (!mem_resv(USIZE, M_REAL)) {
			UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
			return 0;
		}
	}
	ASSERT(pubp->ub_locks[UB_TOTAL_LOCKS] != 0);

	pubp->ub_npages += USIZE;

	/*
	 * See if we have to grow ublock_swresv.
	 *
	 *	The following unlocked test of ublock_swresv relies upon
	 *	atomic access to u_int(s).
	 */
	if (pubp->ub_npages > ublock_swresv) {
		LOCK_PLMIN(&ublock_swresv_mutex);
		if (pubp->ub_npages > ublock_swresv) {
			if (!mem_resv(pubp->ub_npages - ublock_swresv,
				      M_REAL)) {
				UNLOCK_PLMIN(&ublock_swresv_mutex, PLMIN);
				goto backout2;
			}
			ublock_swresv = pubp->ub_npages;
		}
		UNLOCK_PLMIN(&ublock_swresv_mutex, PLMIN);
	}

	bs_off = ub_bs_alloc(pubp, USIZE);
		/* ub_bs_alloc will have dropped ub_mutex */
	if (bs_off == (off_t)-1) {
		(void) LOCK_PLMIN(&pubp->ub_mutex);
		goto backout2;
	}

	ubaddr = segkvn_vp_mapin(ptob(UVOFF), ptob(USIZE),
				 ptob(UVSIZE - USIZE - UVOFF),
				 pubp->ub_mvp, bs_off,
				 SEGKVN_NOFAULT | SEGKVN_KLUSTER,
				 &lubp->ub_mapcookie);
	lubp->ub_off = bs_off;

	if (ubaddr == 0)
		goto backout1;

	if (segkvn_lock(lubp->ub_mapcookie,
			SEGKVN_MEM_LOCK|SEGKVN_NO_MEMRESV) != 0) {
		segkvn_mapout(lubp->ub_mapcookie);
		goto backout1;
	}

	lubp->ub_detached = B_FALSE;

	return ubaddr;

backout1:
	(void) LOCK_PLMIN(&pubp->ub_mutex);
	ub_bs_free(pubp, bs_off, USIZE);
backout2:
	pubp->ub_npages -= USIZE;
	if (pubp->ub_locks[UB_NOSWAP_USER])
		mem_unresv(USIZE, M_REAL_USER);
	else if (pubp->ub_locks[UB_NOSWAP])
		mem_unresv(USIZE, M_REAL);
	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
	return 0;
}


/*
 * void
 * ublock_lwp_detach(lwp_t *lwpp)
 *	Detach an LWP ublock from its process prior to exiting.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the LWP remains on its process's
 *	p_lwpp chain for the duration of this routine (usually by calling
 *	it in context).
 *
 *	This routine may block, so no locks may be held on entry, and
 *	none will be held on exit.
 */
void
ublock_lwp_detach(lwp_t *lwpp)
{
	proc_ubinfo_t *pubp = lwpp->l_procp->p_ubinfop;
	lwp_ubinfo_t *lubp = &lwpp->l_ubinfo;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(pubp->ub_locks[UB_TOTAL_LOCKS] != 0);

	if (lubp->ub_detached) {
		/* Already detached; nothing to do. */
		return;
	}

	lubp->ub_proc_ubp = pubp;
	lubp->ub_private_resv = B_FALSE;

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	while (pubp->ub_flags & UB_INTRANS) {
		SV_WAIT(&pubp->ub_sv, PRIMEM, &pubp->ub_mutex);
		(void) LOCK_PLMIN(&pubp->ub_mutex);
	}

	/*
	 * Convert existing lock(s) to an effective UB_NOSWAP,
	 * since we'll be holding this LWP's ublock privately,
	 * detached from the process.
	 */
	if (pubp->ub_locks[UB_NOSWAP_USER]) {
		MEM_RESV_DEMOTE(USIZE);
	} else if (pubp->ub_locks[UB_NOSWAP] == 0) {
		if (!mem_resv(USIZE, M_REAL)) {
			/*
			 * We failed to get a new reservation; block below
			 * to get use of the USIZE pre-reserved private pool.
			 */
			lubp->ub_private_resv = B_TRUE;
		}
	}

	lubp->ub_detached = B_TRUE;

	pubp->ub_npages -= USIZE;

	pubp->ub_refcnt++;

	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

	if (lubp->ub_private_resv)
		SLEEP_LOCK(&ublock_resv_lock, PRIMEM);
}


/*
 * void
 * ublock_lwp_free(lwp_t *lwpp)
 *	Free an LWP ublock.
 *
 * Calling/Exit State:
 *	This routine is guaranteed not to block.
 *	No locks are held on entry or exit.  It must be called at PLBASE.
 *
 *	The LWP ublock must have been previously detached by a call to
 *	ublock_lwp_detach().
 */
void
ublock_lwp_free(lwp_t *lwpp)
{
	proc_ubinfo_t *pubp;
	lwp_ubinfo_t *lubp = &lwpp->l_ubinfo;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(lubp->ub_detached);
	ASSERT(lubp->ub_stack_mem == NULL);
	ASSERT(lubp->ub_stack == NULL);
	ASSERT(lubp->ub_size == NULL);

	segkvn_unlock(lubp->ub_mapcookie,
		      SEGKVN_MEM_LOCK|SEGKVN_NO_MEMRESV|SEGKVN_DISCARD);
	segkvn_mapout(lubp->ub_mapcookie);

	if (lubp->ub_private_resv)
		SLEEP_UNLOCK(&ublock_resv_lock);
	else
		mem_unresv(USIZE, M_REAL);

	pubp = lubp->ub_proc_ubp;

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	ASSERT(pubp->ub_refcnt != 0);
	if (--pubp->ub_refcnt != 0) {
		ub_bs_free(pubp, lubp->ub_off, USIZE);
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
	} else {
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
		LOCK_DEINIT(&pubp->ub_mutex);
		if (pubp->ub_level != 0)
			kmem_free(pubp->ub_ubitmap.uub_ibitmapp,
				  UB_NBYTES(pubp->ub_level));
		kmem_free(pubp, sizeof(proc_ubinfo_t));
	}
}


/*
 * int
 * ublock_lock(proc_t *procp, ub_locktype_t locktype)
 *	Lock the extended ublock of a process into main memory.
 *
 * Calling/Exit State:
 *	The caller must guarantee either that the p_lwpp chain (list of LWPs)
 *	for the process is not changing or that another ``ub-lock'' is held
 *	for the entire duration while this one is being acquired.
 *
 *	locktype is one of:
 *
 *		UB_SWAPPABLE	- this lock does not prevent swapout from
 *				  doing a corresponding unlock, and thus
 *				  the memory can be considered swappable if
 *				  there are no other locks
 *
 *		UB_NOSWAP	- this lock will not be released by swapout;
 *				  the ublock memory will stay locked in
 *				  memory even if the process is swapped out
 *
 *		UB_NOSWAP_USER	- like, UB_NOSWAP, but generated as a result
 *				  of a user request, and therefore not allowed
 *				  to use up as much memory
 *
 *	This routine may block, so no locks may be held on entry, and
 *	none will be held on exit.
 *
 * Note:
 *	This routine is called with locktype==UB_SWAPPABLE in two cases.
 *	In both cases it does not actually obtain any reservations. This
 *	is correct because:
 *
 *	(1) called by proc_setup() at process creation
 *
 *	    In this case, there are no LWPs yet in the process.
 *	    Furthermore, the ublock is already UB_NOSWAP locked. Either of
 *	    these facts alone would suffice to obviate the need to take
 *	    any reservation.
 *	
 *	(2) called by sched_swapin()
 *
 *	   In this case, the ublock subsystem is already holding a private
 *	   reservation (ublock_swresv) on behalf of the swapper. Note that
 *	   sched_swapin is single threaded, so that a single private
 *	   reservation suffices.
 */
int
ublock_lock(proc_t *procp, ub_locktype_t locktype)
{
	proc_ubinfo_t *pubp = procp->p_ubinfop;
	boolean_t do_wakeup = B_FALSE;
	lwp_t *lwpp, *failed_lwpp;
	faultcode_t fc;
	int err;
	boolean_t downgrade_to_swappable = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	while (pubp->ub_flags & UB_INTRANS) {
		SV_WAIT(&pubp->ub_sv, PRIMEM, &pubp->ub_mutex);
		(void) LOCK_PLMIN(&pubp->ub_mutex);
	}

	if (pubp->ub_locks[locktype]++ == 0 && pubp->ub_npages != 0) {
		switch (locktype) {
		case UB_NOSWAP:
			if (pubp->ub_locks[UB_NOSWAP_USER] == 0) {
				if (!mem_resv(pubp->ub_npages, M_REAL)) {
					pubp->ub_locks[UB_NOSWAP] = 0;
					UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
					return ENOMEM;
				}
			}
			break;
		case UB_NOSWAP_USER:
			if (pubp->ub_locks[UB_NOSWAP] == 0) {
				if (!mem_resv(pubp->ub_npages, M_REAL_USER)) {
					pubp->ub_locks[UB_NOSWAP_USER] = 0;
					UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
					return ENOMEM;
				}
			} else {
				if (!MEM_RESV_PROMOTE(pubp->ub_npages,
						      M_REAL_USER)) {
					pubp->ub_locks[UB_NOSWAP_USER] = 0;
					UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
					return ENOMEM;
				}
			}
			break;
		default:
			ASSERT(locktype == UB_SWAPPABLE);
		}
	}

	err = 0;

	if (pubp->ub_locks[UB_TOTAL_LOCKS]++ == 0 && pubp->ub_npages != 0) {
		pubp->ub_flags |= UB_INTRANS;
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
		/*
		 * This is the first lock; lock down all ublock segments.
		 * No protection is needed for p_lwpp here, since the
		 * caller's guarantees, combined with this being the first
		 * lock, ensure that the p_lwpp chain is not changing.
		 */
		for (lwpp = procp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
			if (lwpp->l_ubinfo.ub_detached)
				continue;
			fc = segkvn_lock(lwpp->l_ubinfo.ub_mapcookie,
					SEGKVN_MEM_LOCK|SEGKVN_NO_MEMRESV);
			if (fc != 0)
				goto backout;
		}
		(void) LOCK_PLMIN(&pubp->ub_mutex);
		pubp->ub_flags &= ~UB_INTRANS;
		do_wakeup = B_TRUE;
	}

	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

	if (do_wakeup && SV_BLKD(&pubp->ub_sv))
		SV_BROADCAST(&pubp->ub_sv, 0);

	return err;

backout:
	/* back out */
	failed_lwpp = lwpp;
	for (lwpp = procp->p_lwpp; lwpp != failed_lwpp; lwpp = lwpp->l_next) {
		ASSERT(lwpp != NULL);
		if (lwpp->l_ubinfo.ub_detached)
			continue;
		segkvn_unlock(lwpp->l_ubinfo.ub_mapcookie, SEGKVN_MEM_LOCK |
			      				   SEGKVN_NO_MEMRESV |
							   SEGKVN_DONTNEED);
	}

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	ASSERT(pubp->ub_locks[locktype] != 0);
	if (--pubp->ub_locks[locktype] == 0) {
		switch (locktype) {
		case UB_NOSWAP:
			if (pubp->ub_locks[UB_NOSWAP_USER] == 0)
				mem_unresv(pubp->ub_npages, M_REAL);
			break;
		case UB_NOSWAP_USER:
			if (pubp->ub_locks[UB_NOSWAP] == 0)
				mem_unresv(pubp->ub_npages, M_REAL_USER);
#ifdef DEBUG
			else
				MEM_RESV_DEMOTE(pubp->ub_npages);
#endif /* DEBUG */
			break;
		default:
			ASSERT(locktype == UB_SWAPPABLE);
		}
	}

	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

	if (SV_BLKD(&pubp->ub_sv))
		SV_BROADCAST(&pubp->ub_sv, 0);

	return EIO;
}


/*
 * void
 * ublock_unlock(proc_t *procp, ub_locktype_t locktype)
 *	Unlock the extended ublock of a process from memory.
 *
 * Calling/Exit State:
 *	locktype must match the locktype of the corresponding call to
 *	ublock_lock().
 *
 *	This routine may block, so no locks may be held on entry, and
 *	none will be held on exit.
 */
void
ublock_unlock(proc_t *procp, ub_locktype_t locktype)
{
	proc_ubinfo_t *pubp = procp->p_ubinfop;
	boolean_t do_wakeup = B_FALSE;
	lwp_t *lwpp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK_PLMIN(&pubp->ub_mutex);

	while (pubp->ub_flags & UB_INTRANS) {
		SV_WAIT(&pubp->ub_sv, PRIMEM, &pubp->ub_mutex);
		(void) LOCK_PLMIN(&pubp->ub_mutex);
	}

	ASSERT(pubp->ub_locks[UB_TOTAL_LOCKS] != 0);
	if (--pubp->ub_locks[UB_TOTAL_LOCKS] == 0) {
		pubp->ub_flags |= UB_INTRANS;
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
		/*
		 * This is the last unlock; unlock all ublock segments.
		 * No protection is needed for p_lwpp here, since the
		 * caller's guarantees, combined with this being the last
		 * unlock, ensure that the p_lwpp chain is not changing.
		 */
		for (lwpp = procp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
			if (lwpp->l_ubinfo.ub_detached)
				continue;
			segkvn_unlock(lwpp->l_ubinfo.ub_mapcookie,
			    SEGKVN_MEM_LOCK|SEGKVN_NO_MEMRESV|SEGKVN_DONTNEED);
		}
		(void) LOCK_PLMIN(&pubp->ub_mutex);
		pubp->ub_flags &= ~UB_INTRANS;
		do_wakeup = B_TRUE;
	}

	ASSERT(pubp->ub_locks[locktype] != 0);
	if (--pubp->ub_locks[locktype] == 0) {
		switch (locktype) {
		case UB_NOSWAP:
			if (pubp->ub_locks[UB_NOSWAP_USER] == 0)
				mem_unresv(pubp->ub_npages, M_REAL);
			break;
		case UB_NOSWAP_USER:
			if (pubp->ub_locks[UB_NOSWAP] == 0)
				mem_unresv(pubp->ub_npages, M_REAL_USER);
#ifdef DEBUG
			else
				MEM_RESV_DEMOTE(pubp->ub_npages);
#endif /* DEBUG */
			break;
		default:
			ASSERT(locktype == UB_SWAPPABLE);
			break;
		}
	}

	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

	if (do_wakeup && SV_BLKD(&pubp->ub_sv))
		SV_BROADCAST(&pubp->ub_sv, 0);
}

/*
 * STATIC off_t
 * ub_bs_alloc(proc_ubinfo_t *pubp, uint_t npages)
 *	Allocate ``npages'' pages of space in the pubp->ub_mvp file.
 *
 * Calling/Exit State:
 *	pubp->ub_mutex is held on entry to this function. It is the only
 *	spin LOCK held on entry to this function. This function drops
 *	pubp->ub_mutex, so that no spin LOCKs are held on exit from this
 *	function.
 *
 *	On success, returns an offset into the file associated with
 *	pubp->ub_mvp. On failure, returns (off_t)-1.
 *
 * Description:
 *	If necessary, (1) the file is extended, and (2) the allocation
 *	bitmap is extended via reallocation. Either of these actions
 *	can cause this function to drop and re-acquire pubp->ub_mutex.
 *	If the file is extended, then the extension pseudo sleep
 *	lock (UB_EXTENDING) is acquired.
 */

STATIC off_t
ub_bs_alloc(proc_ubinfo_t *pubp, uint_t npages)
{
	int n;
	uint_t asize, osize;
	uchar_t level;
	uint_t *bitmap, *obitmap;
	size_t new_len;
	off_t off;
	boolean_t extended;

	ASSERT(LOCK_OWNED(&pubp->ub_mutex));
	ASSERT(KS_HOLD1LOCK());

	/*
	 * attempt to allocate some space from the bit map
	 */
	bitmap = UB_BITMAP(pubp);
	for (;;) {
		level = pubp->ub_level;
		n = BITMASKN_ALLOCRANGE(bitmap, UB_NBITS(level), npages);
		if (n != -1)
			break;
		/*
		 * Need to expand the bitmap
		 */
		asize = UB_NBYTES(level + 1);
		obitmap = bitmap;
		bitmap = kmem_zalloc(asize, KM_NOSLEEP);
		if (bitmap == NULL) {
			UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
			bitmap = kmem_zalloc(asize, KM_SLEEP);
			(void) LOCK_PLMIN(&pubp->ub_mutex);
			if (pubp->ub_level > level) {
				/* another lwp expanded it first */
				kmem_free(bitmap, asize);
				bitmap = UB_BITMAP(pubp);
				continue;
			}
		}
		osize = UB_NBYTES(pubp->ub_level);
		bcopy(obitmap, bitmap, osize);
		if (pubp->ub_level != 0)
			kmem_free(obitmap, osize);
		pubp->ub_ubitmap.uub_ibitmapp = bitmap;
		++pubp->ub_level;
	}

	off = ptob(n);
	new_len = off + ptob(npages);
	while (new_len > pubp->ub_len) {
		/*
		 * If someone else is extending the file, then
		 * just wait for that to complete and try again.
		 */
		if (pubp->ub_flags & UB_EXTENDING) {
			SV_WAIT(&pubp->ub_sv, PRIMEM, &pubp->ub_mutex);
			(void) LOCK_PLMIN(&pubp->ub_mutex);
			continue;
		}

		/*
		 * Acquire the extension pseudo sleep lock
		 */
		pubp->ub_flags |= UB_EXTENDING;
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

		/*
		 * extend the file
		 */
		extended = memfs_extend(pubp->ub_mvp, new_len);
		(void) LOCK_PLMIN(&pubp->ub_mutex);
		if (extended) {
			pubp->ub_len = new_len;
		} else {
			BITMASKN_FREERANGE(UB_BITMAP(pubp), n, npages);
			off = (off_t)-1;
		}
		pubp->ub_flags &= ~UB_EXTENDING;
		UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);
		if (SV_BLKD(&pubp->ub_sv))
			SV_BROADCAST(&pubp->ub_sv, 0);
		return off;
	}

	UNLOCK_PLMIN(&pubp->ub_mutex, PLBASE);

	return off;
}
			
/*
 * STATIC void
 * ub_bs_free(pubp, lubp->ub_off, uint_t npages)
 *	Free ``npages'' pages of space in the pubp->ub_mvp file,
 *	beginning at offset ``off''.
 *
 * Calling/Exit State:
 *	pubp->ub_mutex is held on entry to the function, during execution,
 *	and at exit.
 *
 *	``off'' is a multiple of PAGESIZE.
 *
 * Description:
 *	This function does not truncate the underlying backing store file,
 *	or otherwise abort its pages or free its backing store. It simply
 *	makes the space available for reuse via ub_bs_alloc().
 */

STATIC void
ub_bs_free(proc_ubinfo_t *pubp, off_t off, uint_t npages)
{
	ASSERT(LOCK_OWNED(&pubp->ub_mutex));
	ASSERT((off & PAGEOFFSET) == 0);

	BITMASKN_FREERANGE(UB_BITMAP(pubp), btop(off), npages);
}

/*
 * void
 * ublock_save_extension(void *stack, size_t size)
 *	Try to recover from a stack overflow at switch time by saving the
 *	portion of the stack on the extension page in some privately held
 *	memory.
 * 
 * Calling/Exit State:
 *	Called from within context switch code, while still within the
 *	context of the LWP switching out. The l_mutex is held. Returns
 *	with l_mutex held.
 *
 *	If the caller is a use_private function with no context, or if
 *	insufficient contiguous save memory is available, then this
 *	routine panics.
 *
 * Remarks:
 *	This facility is designed to ride through very rare emergencies.
 *	The OS should not be relying upon this for nominal function.
 */
void
ublock_save_extension(void *stack, size_t size)
{
	lwp_ubinfo_t *lubp;
	lwp_t *lwpp = u.u_lwpp;
	void *save_mem;

	if (lwpp == NULL) {
		/*
		 *+ A stack overflow condition occured on the per-engine
		 *+ stack. Recovery was not possible due to exhaustion of the
		 *+ memory put aside for that purpose. The system
		 *+ administrator should check with the software vendors of
		 *+ the various kernel driver packages to ascertain if fixes
		 *+ are available for excessive stack usage problems.
		 */
		cmn_err(CE_PANIC,
			"ublock_save_extension: per-engine stack overflow");
		/* NOTREACHED */
	}

	ASSERT(LOCK_OWNED(&lwpp->l_mutex));

	lubp = &lwpp->l_ubinfo;
	ASSERT(lubp->ub_stack_mem == NULL);
	ASSERT(lubp->ub_stack == NULL);
	ASSERT(lubp->ub_size == 0);
	FSPIN_LOCK(&ublock_rff_lock);

	/*
	 * Allocate some stack save memory.
	 */
	save_mem = rff_alloc(&ublock_rff, size);
	FSPIN_UNLOCK(&ublock_rff_lock);
	if (save_mem == NULL) {
		/*
		 *+ A stack overflow condition occured. Recovery was not
		 *+ possible due to exhaustion of the memory put aside for that
		 *+ purpose. The system administrator should check with the
		 *+ software vendors of the various kernel driver packages to
		 *+ ascertain if fixes are available for excessive stack usage
		 *+ problems.
		 */
		cmn_err(CE_PANIC,
			"ublock_save_extension: overflow stack exhausted");
		/* NOTREACHED */
	}
	ATOMIC_INT_INCR(&ublock_overflows);

	/*
	 * copy in the piece of the stack which needs special saving
	 */
	bcopy(stack, save_mem, size);

	/*
	 * Remember where we saved the stack extension piece.
	 */
	lubp->ub_stack_mem = save_mem;
	lubp->ub_stack = stack;
#ifdef DEBUG
	lubp->ub_size = size;
#endif
}

/*
 * void
 * ublock_restore_extension(void *stack, size_t size)
 *	Restore the stack extension page contents previously saved by
 *	ublock_save_extension().
 * 
 * Calling/Exit State:
 *	Called from within context switch code, while in the new context
 *	being switch to.
 */
void
ublock_restore_extension(void *stack, size_t size)
{
	lwp_t *lwpp = u.u_lwpp;
	lwp_ubinfo_t *lubp;

	ASSERT(lwpp != NULL);

	lubp = &lwpp->l_ubinfo;
	if (lubp->ub_stack_mem == NULL) {
		/*
		 *+ When a process  executed a fork(2) operation, one of the
		 *+ LWPs was using the extension stack page. This indicates
		 *+ a problem in the base operating system. Corrective
		 *+ action: none.
		 */
		cmn_err(CE_PANIC, "ublock_restore_extension: not saved");
		/* NOTREACHED */
	}

	ASSERT(lubp->ub_stack != NULL);
	ASSERT(lubp->ub_stack == stack);
	ASSERT(lubp->ub_size == size);

	/*
	 * Copy back the stack extension piece.
	 */
	bcopy(lubp->ub_stack_mem, stack, size);
	FSPIN_LOCK(&ublock_rff_lock);
	rff_free(&ublock_rff, lubp->ub_stack_mem, size);
	FSPIN_UNLOCK(&ublock_rff_lock);

	lubp->ub_stack_mem = NULL;
	lubp->ub_stack = NULL;
#ifdef DEBUG
	lubp->ub_size = 0;
#endif
}

/*
 * void
 * ublock_report(void)
 *	Report stack overflows once a minute.
 *
 * Calling/Exit State:
 *	Runs at PLTIMEOUT once a minute.
 */
void
ublock_report(void)
{
	int overflows;

	overflows = ATOMIC_INT_READ(&ublock_overflows);
	if (overflows) {
#ifdef DEBUG
		cmn_err(CE_WARN, "stack overflows: %d in last one minute",
		       overflows);
#else /* !DEBUG */
		/*
		 *+ This message gives the number of occurances (within the
		 *+ last one minute) in which an LWP blocked while using a
		 *+ portion of the extension stack page. These events indicate
		 *+ a potential resource problem, since the kernel has limited
		 *+ memory to save the extension stack. If this resource were
		 *+ to be exhausted, then a PANIC would result. Should these
		 *+ messages be observed frequently, then the administrator
		 *+ should check with the software vendors of the various
		 *+ kernel driver packages to ascertain if fixes are available
		 *+ for excessive stack usage problems.
		 */
		cmn_err(CE_WARN, "!stack overflows: %d in last one minute",
		       overflows);
#endif /* DEBUG */
		ATOMIC_INT_SUB(&ublock_overflows, overflows);
	}
}
