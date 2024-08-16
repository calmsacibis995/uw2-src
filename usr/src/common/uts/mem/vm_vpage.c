/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)kern:mem/vm_vpage.c	1.19"
#ident	"$Header: $"

/*
 * VM - virtual page utilities.
 */

#include <mem/amp.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/seg_vn.h>
#include <mem/vpage.h>
#include <util/debug.h>
#include <util/param.h>

/*
 * boolean_t
 * vpage_lock_range(struct seg *seg, uint_t pgnumber, uint_t npages,
 *		  vp_lock_t type)
 *      Used to establish a shared or exclusive lock on a range of virtual
 *	pages. The kind of lock actually established is a function of the
 *	requirements of the caller, plus the observed contention for
 *	locks among the LWPs sharing access to the segment.
 *
 * Calling/Exit State:
 *	Pointers to objects needed to determine what kind of lock we are
 *	going to get are passed in, in addition to the vpage structure
 *	itself. The arguments are:
 * 
 *	    seg		Pointer to the segment.
 *
 *	    pgnumber	Page number (in the segment) of the first page in the
 *			affected range.
 *
 *	    npages	Number of pages in the affected range.
 *
 *	    type	One of the following values:
 *
 *		    VP_READ	Indicates that page identity will not change
 *				while the lock is held.  Thus, a READer lock
 *				will suffice.
 *		    
 *		    VP_WRITE	Indicates that page identity will change
 *				while the lock is held. Thus, a WRITEr lock
 *				is needed.
 *		    
 *		    VP_PRIV_WR	Indicates that the fault has COW and ZFOD
 *				potential (i.e. S_WRITE to a MAP_PRIVATE
 *				segment). Page identity may or may not
 *				change depending upon whether the anon_map
 *				is actually instantiated. This function
 *				tests the volatile state of the anon_map in
 *				order to determine whether a READer or
 *				WRITEr lock is needed.
 *
 *				The anon_map has already been instantiated
 *				upon entry to this function.
 *
 * 		    VP_ZFOD	Indicates that the fault has ZFOD potential,
 *				but no COW potential. Page identity may or
 *				may not change depending upon whether the
 *				anon_map entry is actually instantiated.
 *				This function tests the volatile state of
 *				the anon_map in order to determine whether
 *				a READer or WRITEr lock is needed.
 *
 *				The anon_map has already been instantiated
 *				upon entry to this function.
 *
 *	Returns B_TRUE if escalated (segment level READ/WRITE) locks were
 *	acquired and B_FALSE otherwise.
 *
 *	No LOCKs are held at entry to, or exit from this function.
 *
 * Description:
 *	This function attempts to take escalated (i.e. segment level) locks
 *	as a performance and space optimization. It is a performance
 *	optimization, since the manipulation of the vpage_lock array
 *	is avoided. It is a space optimization, since the instantiation
 *	of the vpage_lock array is also avoided.
 *
 *	The condition to use the escalated lock:
 *
 *		No contention for the segment level lock has ever been
 *		observed in the lifetime of the segment.
 *
 *	As soon as an LWP tries to get a segment level lock, but then
 *	observes that it needs to block, it instantiates the vpage_lock
 *	array. From that point onward, all new locks (obtained in this
 *	function for the same segment) will take out page granular locks
 *	(i.e. will lock a vpage_lock entry).
 *
 *	Another important point: in order to take a page granular lock,
 *	it is first necessary to take the segment INTENT lock. Thus, this
 *	function always returns with a segment level lock held, READ or
 *	WRITE in the escalated case, or INTENT in the page granular case.
 */
boolean_t
vpage_lock_range(struct seg *seg, uint_t pgnumber, uint_t npages,
		 vp_lock_t type)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	anon_t **app;
	vp_lock_t esctype;
	vpage_lock_t *vplp;
	uint_t alloc_size;
	uint_t i;

	ASSERT(npages != 0);
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * We need the spin lock to conduct pseudo-locking operations.
	 * Hold it for as long as possible in order to reduce the number of
	 * LOCK/UNLOCK roundtrips.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);

	/*
	 * attempt 1: If no vpage_lock array has yet been allocated, then
	 *	      try to use the segment level locks (called "escalated"
	 *	      locks here).
	 */
	if (svp->svd_lock == NULL) {
		/*
		 * Test per-page volatile conditions.
		 *
		 * VP_ZFOD:
		 *	The page is backed by anonymous storage. The
		 *	page identity will change (due to ZFOD) if the
		 *	anon map slot is currently empty. A page
		 *	identity change requires an exclusive lock for the
		 *	fault processing.
		 *
		 * VP_PRIV_WR:
		 *	This is an S_WRITE fault to a MAP_PRIVATE segment.
		 *	The page identity will change if the anon slot is
		 *	emtpy or if the reference count on the anon slot is
		 *	greater than 1 (indicating sharing by a close
		 *	relative process).
		 *
		 * Note that either one of these conditions, for any page
		 * in the argument range, can mandate the exlcusive lock.
		 */
		switch (type) {
		case VP_PRIV_WR:
		case VP_ZFOD:
			ASSERT(svp->svd_amp != NULL);
			ASSERT(svp->svd_swresv > 0);
			app = &svp->svd_amp->am_anon[pgnumber +
						svp->svd_anon_index];
			i = npages;
			esctype = VP_READ;
			for (;;) {
				if (*app == NULL || (type == VP_PRIV_WR &&
						     (*app)->an_refcnt > 1)) {
					esctype = VP_WRITE;
					break;
				}
				/*
				 * loop exit optimized for one page case
				 */
				if (i == 1) {
				    break;
				}
				--i;
				++app;
			}
			break;

		default:
			ASSERT(type == VP_READ || type == VP_WRITE);
			esctype = type;
			break;
		}

		/*
		 * Now go for the segment level (i.e. "escalated") lock.
		 * However, we never block here.  If we cannot get the
		 * lock, then the optimization doesn't work (indicating that
		 * another LWP is contending with us) and it will be
		 * necessary to use page granular locks.
		 */
		ASSERT(LOCK_OWNED(&svp->svd_seglock));
		if (esctype == VP_READ) {
			if (SEGTRYLOCK_READ(svp)) {
				UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
				return B_TRUE;
			}
		} else {
			if (SEGTRYLOCK_WRITE(svp)) {
				UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
				return B_TRUE;
			}
		}

		/*
		 * Attempt 1 FAILED. So allocate the vpage lock array and go
		 *		     onto page granular locking.
		 *
		 * Note that we must drop the spin lock in order to
		 * allocate the vpage lock array.  When we get it back,
		 * someone else might have instantiated the array first.
		 * In that case, we just free the uselessly allocated array.
		 */
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		alloc_size = seg_pages(seg) * sizeof(vpage_lock_t);
		vplp = kmem_zalloc(alloc_size, KM_SLEEP);
		(void) LOCK_PLMIN(&svp->svd_seglock);
		if (svp->svd_lock == NULL) {
			svp->svd_lock = vplp;
		} else {
			kmem_free(vplp, alloc_size);
		}
	} /* svp->svd_lock == NULL */

	/*
	 * attempt 2: use page granular locks.
	 *
	 *	This attempt will succeed!
	 *
	 *	First, we must grab the intent lock for the segment.
	 *	This action might block.
	 */
	SEGLOCK_INTENT(svp);
	ASSERT(svp->svd_lock != NULL);
	vplp = &svp->svd_lock[pgnumber];

	if (type == VP_PRIV_WR || type == VP_ZFOD) {
		ASSERT(svp->svd_amp != NULL);
		ASSERT(svp->svd_swresv > 0);
		app = &svp->svd_amp->am_anon[pgnumber + svp->svd_anon_index];
	} else {
		app = NULL;
	}

	/*
	 *	Now loop over each of the pages, taking a reader/writer lock
	 *	for each page as appropriate.
	 */

	for (;;) {
		ASSERT(LOCK_OWNED(&svp->svd_seglock));

		/*
		 * The following for loop waits until the lock is obtained.
		 * Following each wait, we come back to the top of the loop
		 * in order to re-evaluate which type of lock we actually
		 * want. While we were waiting, potentialy volatile
		 * conditions may have changed (e.g. COW might have been
		 * broken or ZFOD performed).
		 */
		for (;;) {
			/*
			 * Test per-page volatile conditions.
			 *
			 *	See the comment block with the same title
			 *	(above) for details.
			 */
			switch (type) {
			case VP_ZFOD:
				esctype = (*app) ? VP_READ : VP_WRITE;
				break;
			case VP_PRIV_WR:
				esctype = (*app &&
					   (*app)->an_refcnt == 1) ? VP_READ :
								     VP_WRITE;
				break;

			default:
				ASSERT(type == VP_READ || type == VP_WRITE);
				esctype = type;
				break;
			}

			if (esctype == VP_READ) {
				/*
				 * Trying for a reader virtual page lock.
				 *
				 * Block if a writer holds the lock, or if
				 * VPAGE_MAX_SHARE readers hold the lock.
				 * Since
				 *
				 *	VPAGE_WR_LOCKED == VPAGE_MAX_SHARE + 1,
				 *
				 * a single test allows us to determine if
				 * blocking is needed.
				 */
				if ((int)vplp->vpl_lcktcnt < VPAGE_MAX_SHARE) {
					++vplp->vpl_lcktcnt;
					break; /* onto the next page */
				}
				/* else fall through to blocking wait */
			} else {
				/*
				 * Trying for an exclusive virtual page lock.
				 *
				 * Block if anybody holds the lock.
				 */
				if (vplp->vpl_lcktcnt == 0) {
					vplp->vpl_lcktcnt = VPAGE_WR_LOCKED;
					break; /* onto the next page */
				}
				/* else fall through to blocking wait */
			}

			/*
			 * We can't get the lock, so we must wait.
			 * After we wake up, conditions may have changed.
			 * So therefore, we go back and re-evaluate the
			 * type of lock needed.
			 */
			SV_WAIT(&svp->svd_vpagesv, VM_VPAGE_PRI,
				&svp->svd_seglock);
			(void) LOCK_PLMIN(&svp->svd_seglock);
		}

		/*
		 * loop exit condition optimized for one page case
		 */
		if (--npages == 0) {
			break;
		}

		/*
		 * Onto the next page ...
		 */
		if (app) {
			++app;
		}
		++vplp;
	}

	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	return B_FALSE;
}

/*
 * void
 * vpage_unlock_range(struct segvn_data *svp, uint_t pgnumber, uint_t npages)
 *      Release locks on the specified range of virtual pages.
 *
 * Calling/Exit State:
 *	This function releases the locks taken by vpage_lock_range.
 *
 *	The actual action taken depends upon what type of lock had been
 *	garnered by vpage_lock_range. If vpage_lock_range took an escalated
 *	(segment level READ or WRITE lock), then we release it. If
 *	vpage_lock_range took page granular locks, then we release those.
 *
 *	Called with no LOCKS held and returns that way.
 *
 * Description:
 *	A segment level pseudo lock is always dropped. If a segment level
 *	INTENT lock was held on entry to this routine (indicating that
 *	vpage level locks were also held) then the vpage level locks are
 *	also dropped.
 */
void
vpage_unlock_range(struct segvn_data *svp, uint_t pgnumber, uint_t npages)
{
	vpage_lock_t *vplp;
	boolean_t wakeup_required = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(npages != 0);
	ASSERT(svp->svd_lckflag && svp->svd_lckcnt);

	/*
	 * We need the spin lock to conduct pseudo-locking operations.
	 * Hold it for as long as possible in order to reduce the number of
	 * LOCK/UNLOCK roundtrips.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);

	switch (svp->svd_lckflag) {
	/*
	 * page granular locking case:
	 *	Must release both the INTENT lock and
	 *	the per-virtual page locks.
	 */
	case SEGVN_IXLCK:
		ASSERT(svp->svd_lock != NULL);
		vplp = &svp->svd_lock[pgnumber];
		do {
			switch(vplp->vpl_lcktcnt) {
			/*
			 * write locked or read locked with one reader
			 */
			case VPAGE_WR_LOCKED:
			case 1:
				vplp->vpl_lcktcnt = 0;
				if (SV_BLKD(&svp->svd_vpagesv)) {
					wakeup_required = B_TRUE;
				}
				break;

			/*
			 * read lock with MAX readers
			 */
			case VPAGE_MAX_SHARE:
				if (SV_BLKD(&svp->svd_vpagesv)) {
					wakeup_required = B_TRUE;
				}
				/* FALLTHROUGH */

			/*
			 * read lock - but no wakeup transition
			 */
			default:
				--vplp->vpl_lcktcnt;
				break;
			}
			++vplp;
		} while (--npages);
		break;

	/*
	 * Either SEGVN_RDLCK or SEGVN_WRLCK.
	 *	This is the "escalated" lock case.  Only a segment level
	 *	lock is held.  There are no vpage locks to release.
	 *	However, segment lock must be released.
	 */
	default:
		ASSERT(svp->svd_lckflag == SEGVN_RDLCK ||
		       svp->svd_lckflag == SEGVN_WRLCK);
		break;
	}

	/*
	 * release the segment level lock now
	 */
	SEGLOCK_UNLOCK(svp);
	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	/*
	 * generate wakeups
	 */
	if (wakeup_required) {
		SV_BROADCAST(&svp->svd_vpagesv, 0);
	}
}

/*
 * void
 * vpage_downgrade_range(struct segvn_data *svp, uint_t pgnumber, uint_t npages)
 *      Downgrade the locks on the specified range of virtual pages to
 *	READer mode, if necessary.
 *
 * Calling/Exit State:
 *	This function downgrades the locks taken by vpage_lock_range.
 *
 *	The actual action taken depends upon what type of lock had been
 *	garnered by vpage_lock_range. If vpage_lock_range took an escalated
 *	(i.e. segment level) WRITEr lock then we downgrade it to a segment
 *	READer locks. If vpage_lock_range took page granular locks, then
 *	we downgrade those.
 *
 *	Called with no LOCKS held and returns that way.
 */
void
vpage_downgrade_range(struct segvn_data *svp, uint_t pgnumber, uint_t npages)
{
	vpage_lock_t *vplp;
	boolean_t do_wakeup = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(npages != 0);
	ASSERT(svp->svd_lckflag && svp->svd_lckcnt);

	/*
	 * We need the spin lock to conduct pseudo-locking operations.
	 * Hold it for as long as possible in order to reduce the number of
	 * LOCK/UNLOCK roundtrips.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);

	switch (svp->svd_lckflag) {
	/*
	 * SEGVN_IXLCK: page granular locks are held.
	 *
	 * The lock for each page must be downgraded (if necessary).
	 */
	case SEGVN_IXLCK:
		ASSERT(svp->svd_lock != NULL);
		vplp = &svp->svd_lock[pgnumber];
		do {
			/*
			 * turn writer locks into reader locks with one reader
			 */
			if (vplp->vpl_lcktcnt == VPAGE_WR_LOCKED) {
				vplp->vpl_lcktcnt = 1;
				do_wakeup = B_TRUE;
			}
			++vplp;
		} while (--npages);
		break;

	/*
	 * SEGVN_WRLCK: a segment level (escalated) write lock is held.
	 *
	 * Downgrade escalated (segment level) write lock to
	 * escalated (segment level) read lock.
	 */
	case SEGVN_WRLCK:
		SEGLOCK_DOWNGRADE(svp, SEGVN_RDLCK);
		break;

	/*
	 * SEGVN_RDLCK: a segment level (escalated) read lock is held.
	 *
	 * There is nothing to do for this case.
	 */
	default:
		ASSERT(svp->svd_lckflag == SEGVN_RDLCK);
		break;
	}

	/*
	 * Release the segment spin lock, and generate wakeups as required.
	 */
	if (do_wakeup && SV_BLKD(&svp->svd_vpagesv)) {
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		SV_BROADCAST(&svp->svd_vpagesv, 0);
	} else {
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
	}
}
