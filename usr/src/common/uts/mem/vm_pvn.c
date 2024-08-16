/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)kern:mem/vm_pvn.c	1.73"
#ident	"$Header: $"

/*
 * VM - paged vnode.
 *
 * This file supplies vm support for the vnode operations that deal with pages.
 */

#include <fs/buf.h>
#include <fs/vnode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pageidhash.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/vmmeter.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>		/* needed for lbolt */
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

STATIC pvn_memresv_entry **pvn_memresv_lookup(vnode_t *vp, off_t offset,
						pl_t oldpl, uint_t flag);
STATIC void pvn_memresv_destroy(pvn_memresv_entry **p);
STATIC pvn_memresv_entry **pvn_memresv_find_l(vnode_t *vp, off_t offset);

uint_t failed_binval_writes = 0;  /* count failed pvn_done(B_INVAL, B_WRITE) */

/* abort_buf_mutex:  for pvn_fail() private buf structure */

STATIC sleep_t abort_buf_mutex;

/*
 *+ pvn_fail() private buf structure sleep lock
 */
STATIC LKINFO_DECL(abort_buf_lkinfo, "MP::abort_buf_mutex", 0);

/* pvn_memresv_lock: for pvn_mem_resv() */

STATIC lock_t pvn_memresv_lock;

/*
 *+ pvn_mem_resv() lock for pvn_memresv_hash[]
 */
STATIC LKINFO_DECL(pvn_memresv_info, "MP::pvn_memresv_lock", 0);

pvn_memresv_entry **pvn_memresv_hash;

/* private markers for the pageout daemon */

STATIC struct pageheader pvn_pageout_markers[2];

#ifdef DEBUG
/*
 * void
 * CHECK_DIRTY_LIST(page_t *pp, vnode_t *vp)
 *	Check the page list of pages prepared by pvn_getdirty_range().
 *
 * Calling/Exit State:
 *	The pages on the list are in increasing offset order and
 *	pageout locked.
 */
#define CHECK_DIRTY_LIST(dirty, vp, rlen) {		\
	off_t pg_offset = (dirty)->p_offset;		\
	page_t *pp = (dirty)->p_next;			\
							\
	ASSERT((dirty)->p_pageout == 1);		\
	ASSERT((dirty)->p_vnode == (vp));		\
	ASSERT((dirty)->p_activecnt);			\
	while (pp != (dirty)) {				\
		ASSERT(pp->p_vnode == (vp));		\
		ASSERT(pp->p_pageout == 1);		\
		ASSERT(pp->p_activecnt);		\
		ASSERT(pp->p_offset > pg_offset);	\
		pg_offset = pp->p_offset;		\
		pp = pp->p_next;			\
	}						\
}
#else /* DEBUG */
#define CHECK_DIRTY_LIST(dirty, vp, rlen)
#endif /* DEBUG */

/*
 * void
 * pvn_init()
 *
 *	Initialize the pvn routines during system initialization.
 *
 * Calling/Exit State:
 *
 *	None.
 */
void
pvn_init(void)
{
	SLEEP_INIT(&abort_buf_mutex, 0, &abort_buf_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&pvn_memresv_lock, VM_PVNMEMRESV_HIER, VM_PVNMEMRESV_IPL,
			&pvn_memresv_info, KM_NOSLEEP);
}

/*
 * void
 * pvn_fail(page_t *plist, int flags)
 *
 *	Abort a pageio page list in lieu of performing I/O.
 *
 * Calling/Exit State:
 *
 *	To be used by page r/w subr's and other such routines which
 *	want to report an error and abort a list of pages setup for pageio
 *	which do not go though the normal pvn_done() processing.
 *
 *	Caller supplies a list of dirty pages.  Each page is either reader 
 *	locked or, if flags includes B_INVAL or B_READ, writer locked.
 *	(See pvn_done().)
 *
 *	The pages may be consumed by pvn_done(). If so the caller does not
 *	need to unlock those pages (and should not reference them).
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 */
void
pvn_fail(page_t *plist, int flags)
{
	static struct buf abort_buf;
	struct buf *bp;
	page_t *pp;

	bp = &abort_buf;
	pp = plist;
	do {
		plist = pp;
		SLEEP_LOCK(&abort_buf_mutex, PRIMEM-2);
		bp->b_numpages = 0;
		do { 
			++bp->b_numpages;
		} while (((pp = pp->p_next) != plist) && 
				bp->b_numpages < (u_char)((1 << NBBY) - 1));
		/* ~B_ASYNC makes sure pvn_done does not pageio_done the bp */
		bp->b_flags = B_KERNBUF | B_ERROR | B_PAGEIO | 
				(flags & ~B_ASYNC);
		bp->b_pages = plist;
		pvn_done(bp);		/* let pvn_done do all the work */
		SLEEP_UNLOCK(&abort_buf_mutex);
	} while (pp != plist);
}

/*
 * void
 * pvn_done(buf_t *bp)
 *
 *	Routine to be called when pageio's complete.
 *
 * Calling/Exit State:
 *
 *	Can only be called from process context, not from interrupt level.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. 
 *	They remain unlocked at return.
 *	
 *
 *	bp->b_flags:			Caller supplies:
 *	
 *	B_READ			=> pages write locked; ASSERT(!B_INVAL)
 *	B_INVAL			=> pages write locked; ASSERT(!B_READ)
 *	B_WRITE			=> pages read  locked (unless B_INVAL)
 *	
 *					Actions:
 *	
 *	B_ASYNC			=> pvn_done() does page_unlock
 *	B_INVAL			=> pvn_done() does page_unlock (via page_abort)
 *	B_WRITE			=> pvn_done() does page_unlock
 *	B_READ   &&  B_ERROR	=> pvn_done() does page_unlock (via page_abort)
 *	Otherwise		=> caller     does page_unlock after return
 *
 *	Thus the *only* time the caller does page_unlock is:
 *		B_READ && !B_ASYNC && !B_ERROR
 *		(i.e., synchronous read when there is no error)
 *				              
 *	NOTE:	pvn_done() will page_abort()/page_unlock the page if *any* of
 *		the p_nio concurrent B_READ requests have B_ERROR set.
 *		Therefore, the caller must not page_unlock the page if *any*
 *		of the p_nio biowait() requests return an error.
 *
 *	B_PAGEIO should always be set.
 *	B_PAGEOUT should be set by pageout() and no one else.
 *
 *	Note that (unlike in previous releases) it is the caller's job
 *	to call bp_mapout() for B_REMAPPED buffers.  The bp_mapout() call
 *	must also be done before checking B_PAGEIO to decide whether to call
 *	pvn_done(), since B_PAGEIO is cleared while a buffer is B_REMAPPED.
 *
 * Remarks:	
 *	pvn_done() can page_abort() in the case of 
 *	B_INVAL && !B_READ && B_ERROR.  The caller can figure out
 *	this happened, but if it's B_ASYNC, the caller is cleanup()
 *	and the file system is never informed.  
 *
 *	We can't assert p_nio is > 0 here. This is because only the 
 *	nio count of the front page in a buffer is kept. In filesystems
 *	where block size is > pagesize, all pages except the front page
 *	associated with the buffer will have nio set to 0.
 *
 *	Do we need an interface for cleanup() to inform the VFS
 *	of the failed B_INVAL write?  (VOP_GUESSAGAIN()?)
 *	If so, how does cleanup() known what the vp, offset of
 *	the page is?
 */
void
pvn_done(buf_t *bp)
{
	page_t *pp;
	uint_t numpages;
#ifdef _PAGEIO_HIST
	extern clock_t *pageio_stamp;
#endif

#if (B_READ == 0)
#error pvn_done() code assumes B_READ != 0
#endif

	ASSERT((bp->b_flags & (B_PAGEIO|B_REMAPPED)) == B_PAGEIO);

	PAGEIO_LOG(bp, "pvn_done: entry");

	/*
	 * Handle each page in the I/O operation.
	 */

	numpages = bp->b_numpages;

	if (bp->b_flags & B_READ) {
		MET_PGIN(1);
		if (bp->b_pages->p_nio <= 1)
			MET_PGPGIN(numpages);
	}

	while (numpages-- > 0) {

		pp = bp->b_pages;

		ASSERT(pp != NULL);

		/*
		 * Check assumptions:
		 *
		 * B_READ implies:
		 *	- page is writer locked
		 *	- no pageout in progress
		 *	- no invalidates
		 *
		 * B_WRITE implies:
		 *	- page is writer locked if B_INVAL
		 *	  else page is reader locked
		 *	- pageout in progress
		 */
		
		PAGE_USELOCK(pp);

		ASSERT(pp->p_activecnt > 0);

#ifdef DEBUG
		if (bp->b_flags & B_READ) {
			ASSERT(pp->p_invalid);
			ASSERT(!pp->p_pageout);
			ASSERT(!(bp->b_flags & B_INVAL));

		} else /* B_WRITE */ {
			if (bp->b_flags & B_INVAL) {
				ASSERT(pp->p_invalid);
			} else {
				ASSERT(!pp->p_invalid);
			}

			ASSERT(pp->p_pageout);
		}
#endif /* DEBUG */

		/*
		 * Check for multiple IO requests on this page.
		 */

		if (pp->p_nio > 1) {
			/*
			 * There were multiple I/O requests outstanding
			 * for this particular page.  This can happen
			 * when the file system block size is smaller
			 * than PAGESIZE.  Since there are more I/O
			 * requests still outstanding, we don't process
			 * the page given on the buffer now.
			 */

			if (bp->b_flags & B_ERROR) {
				/*
				 * Defer error processing until all I/O done.
				 */
				pp->p_ioerr = 1;
			}
			pp->p_nio--;
			PAGE_USEUNLOCK(pp);

			/*
			 * page remains read/write locked for
			 * the remaining I/O operations
			 */
			break;
		}

		pp->p_nio = 0;
		page_sub(&bp->b_pages, pp);

		/*
		 * Check if we are to be doing invalidation.
		 * XXX - Failed writes with B_INVAL set are
		 * not handled appropriately.
		 */
		if (bp->b_flags & B_INVAL) {

			ASSERT(!(bp->b_flags & B_READ));
			/*
			 * This is a B_INVAL write request.
			 * If this I/O had an error, or if a prior
			 * concurrent I/O for this page had an error,
			 * then announce a warning.
			 */
			if ((bp->b_flags & B_ERROR) || pp->p_ioerr) {
				pp->p_ioerr = 0;
				failed_binval_writes++;
			}
			pp->p_pageout = 0;
#ifdef _PAGEIO_HIST
			pageio_stamp[pp - pages] = 0;
			PAGEIO_LOG_PP(bp, pp, bp->b_flags,
				      "pvn_done: B_INVAL done");
#endif
			/* PAGE_BROADCAST(pp) done by page_abort() */
			page_abort_l(pp);
			continue;
		}

		/*
		 * Check to see if the page has (or already had) an I/O error.
		 */

		if ((bp->b_flags & B_ERROR) || pp->p_ioerr) {
			pp->p_ioerr = 0;

			if (bp->b_flags & B_READ) {
				page_abort_l(pp);   /* page is writer locked */
				continue;

			} else /* B_WRITE */ {
				/*
				 * If the write operation failed, we don't want
				 * to abort (or free) the page.  We set the mod
				 * bit again so it will get written back again
				 * later when things are hopefully better.
				 */
				pp->p_mod = 1;
				if (pp->p_timestamp == 0)
					pp->p_timestamp = lbolt;
			}
		}

		/*
		 * Drop the page reader/writer lock unless we're doing
		 * a synchronous read.  (Note that a synchronous read
		 * with B_ERROR never gets here; it is handled above.)
		 *
		 * Clear pageout state in case we're doing a pageout.
		 *
		 * If we were the last page locker or we were doing a pageout,
		 * wakeup any page waiters.
		 */

		if ((bp->b_flags & (B_READ | B_ASYNC)) != B_READ) {
			pp->p_invalid = 0; /* if hold a writer lock, drop it */
			pp->p_activecnt--;
			PAGEIO_LOG_PP(bp, pp, bp->b_flags,
				      "pvn_done: dropping active");
		}

		if (pp->p_activecnt == 0 || pp->p_pageout) {
			pp->p_pageout = 0;
#ifdef _PAGEIO_HIST
			pageio_stamp[pp - pages] = 0;
			PAGEIO_LOG_PP(bp, pp, bp->b_flags,
				      "pvn_done: clearing pageout");
#endif
			PAGE_BROADCAST(pp);
		}

		/*
		 * Update statistics:
		 *
		 * B_PAGEOUT implies this is a page which is being paged out
		 * by pageout().  Once we've dropped pageout()'s page
		 * reader/writer lock (above), such pages should no longer
		 * be in use, unless they've been reclaimed during I/O.
		 * In this case, we bump reclaim statistics here since
		 * page_reclaim() was not called (since a page undergoing
		 * I/O is not on the free list).
		 */

		if (bp->b_flags & B_PAGEOUT) {
			MET_PGPGOUT(1);
			if (PAGE_IN_USE(pp)) {
				/*
				 * The attach was already counted,
				 * but not as an attach from the free list.
				 */
				MET_ATCHFREE(1);
				MET_ATCHFREE_PGOUT(1);
#ifdef EXTRA_PAGE_STATS
				/* Stats variables mutexed by vm_pgfreelk */
				VM_PAGEFREELOCK();
				BUMPPGCOUNT(pagecnt.pc_reclaim);
				BUMPPGCOUNT(pagecnt.pc_reclaim_pgout);
				VM_PAGEFREEUNLOCK();
#endif /* EXTRA_PAGE_STATS */
			}
		}

		/*
		 * Free or unlock the page.
		 */
		if (!PAGE_IN_USE(pp)) {
			page_free(pp, (bp->b_flags & B_DONTNEED));
		} else {
			PAGE_USEUNLOCK(pp);
		}
	}
	/*
	 * Release the buf struct associated with the operation if async.
	 */
	if (bp->b_flags & B_ASYNC)
		pageio_done(bp);
}


/*
 * int
 * pvn_getdirty(page_t *pp, page_t **dirty, int flags, boolean_t *lock_failed,
 *		boolean_t *idlock_held, enum page_lock_mode lockmode)
 *
 *	Determine if a page is dirty and prepare it for cleaning.
 *
 * Calling/Exit State:
 *
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED, B_DELWRI}.
 *	B_DELWRI indicates that this page is part of a kluster operation and
 *	is only to be considered if it doesn't involve any waiting here.
 *	It is an error to specify both B_DELWRI and B_INVAL (i.e., kluster
 *	and invalidate).
 *
 *	An additional flag, B_TRYLOCK, is supported for use specifically
 *	by pvn_getdirty_range().  It behaves like B_DELWRI, but works with
 *	B_INVAL.  Additionally, if B_TRYLOCK is set, and we return due to
 *	a failure to get the page lock, the out arg, *lock_failed, is set
 *	to B_TRUE (it is not touched otherwise).
 *
 *	Called with page PAGE_USELOCK()'d.  The caller has verified the
 *	page identity.  The caller need not verify any other aspects of
 *	page state.  In particular, the caller need not (and preferably
 *	should not) reclaim the page if it is free.
 *
 *	The VM_PAGEIDLOCK() may or may not be held on return; it is 
 *	indicated by the idlock_held argument. However, if this routine
 *	sets outarg lock_failed to be B_TRUE, then it never drops the 
 *	VM_PAGEIDLOCK().
 *
 *	VM_PAGEFREELOCK() remains unlocked.
 *
 *	Returns zero if page does not require cleaning in which case the
 *	page is freed if it is not in use.
 *
 *	Returns non-zero if page added to dirty list in which case the
 *	page is either reader locked or, if B_INVAL, writer locked; also
 *	p_mod has been cleared and p_pageout set.
 *
 *	In either return case, the page is PAGE_USEUNLOCK()'d.
 *
 *	NOTE:  May block unless B_DELWRI specified.
 */
STATIC int
pvn_getdirty(page_t *pp, page_t **dirty, int flags, boolean_t *lock_failed,
		boolean_t *idlock_held, enum page_lock_mode lockmode)
{
	vnode_t *vp;
	uint_t offset;
	int reclm_succeed;
#ifdef _PAGEIO_HIST
	extern clock_t *pageio_stamp;
#endif

	/*
	 * The only locks that should be held are the uselock and possibly
	 * the VM_PAGEIDLOCK().
	 */
	ASSERT(*idlock_held || KS_HOLD1LOCK());

	/* It is an error to specify both invalidate and kluster */
	ASSERT((flags & (B_INVAL|B_DELWRI)) != (B_INVAL|B_DELWRI));

	/*
	 * If we're not B_INVAL then a free, clean page can be skipped.
	 *
	 * Note that if the page is currently free then p_mod is
	 * guaranteed to be up-to-date (since there are no hat
	 * translations; p_mapping == NULL).
	 *
	 * Later on we perform essentially the same check again
	 * (since a page which is not free now may become free after
	 * we block waiting to lock it).  We redundantly perform the
	 * check here for performance because it is a common case.
	 */

	if (pp->p_free && !pp->p_mod && !(flags & B_INVAL)) {
		if (*idlock_held) {
			l.vmpageidlockpl = l.puselockpl;
			l.puselockpl = VM_PAGE_IPL;   /* out of order unlock */
		}
		PAGE_USEUNLOCK(pp);
		return (0);
	}

	vp = pp->p_vnode;
	offset = pp->p_offset;

	if (pp->p_pageout || pp->p_invalid) {
		if ((flags & (B_INVAL | B_ASYNC)) == B_ASYNC) {
			/*
			 * Don't wait for an intrans page if we are
			 * not doing invalidation and this is an
			 * async operation.
			 */
			goto droppage;
		}
		if (flags & (B_DELWRI|B_TRYLOCK)) {
			/*
			 * This is a klustering case that would
			 * cause us to block, just give up.
			 * (Note that B_DELWRI implies !B_INVAL,
			 * so we would block only for an in progress
			 * pageout or a page writer lock.  For B_TRYLOCK
			 * and B_INVAL, the extra case is handled below.)
			 */
			if (flags & B_TRYLOCK)
				*lock_failed = B_TRUE;
			goto droppage;
		}
	} else if ((flags & (B_TRYLOCK|B_INVAL)) == (B_TRYLOCK|B_INVAL) &&
			pp->p_activecnt) {
			*lock_failed = B_TRUE;
		goto droppage;
	}

	/* 
	 * Wait for:
	 *	- in progress pageout (to serialize pageouts)
	 *	- writer lock to unlock (to set a reader page lock)
	 *	- reader locks to unlock (to set a writer page lock
	 *		if we need to invalidate the page)
	 */
	while (pp->p_pageout || pp->p_invalid || 
		(pp->p_activecnt && (flags & B_INVAL))) {

		if (*idlock_held) {
			VM_PAGEIDUNLOCK();
			*idlock_held = B_FALSE;
		}

		PAGE_WAIT(pp);

		/*
		 * Re-verify page state since it could have
		 * changed while we were sleeping.
		 */

		if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
			/*
			 * Lost the page - nothing to do.
			 */
			goto droppage;
		}
	}

	/* 
	 * The page is now effectively reader/writer locked.
	 *
	 * I.e., there are no writer locks and no pageouts in progress.
	 * Further, if B_INVAL, there are no reader locks.
	 *
	 * We have not yet asserted a reader lock (bumped p_activecnt),
	 * or, in the case of B_INVAL, asserted a writer lock
	 * (set p_invalid), but we still hold PAGE_USELOCK(pp) which
	 * prevents anyone else from asserting any page locks.
	 */
	
	/*
	 * Reclaim the page if it's free.
	 *
	 * The page may have been free when passed in or it may have
	 * become free while we waited for the page above.
	 */

	if (pp->p_free) {
		if (*idlock_held) {
			ASSERT(VM_PAGEIDLOCK_OWNED());
			VM_PAGEFREELOCK();
			reclm_succeed = page_reclaim_l(pp, lockmode, 
						P_LOOKUP);
			VM_PAGEFREEUNLOCK();
			if (!reclm_succeed)
				goto droppage;
		} else {
			if (!page_reclaim(pp))
				goto droppage;
		}
	}

	/*
	 * Now determine if page is dirty and take appropriate action.
	 */

	if (flags & B_INVAL) {
		/*
		 * Invalidate the page.
		 *
		 * We have an effective page writer lock.
		 */
		hat_pageunload(pp);	/* also syncs mod bits */

		ASSERT(!PAGE_IN_USE(pp));
		ASSERT(PAGE_USELOCK_OWNED(pp));
		if (!pp->p_mod) {
			if (*idlock_held) {
				VM_PAGEIDUNLOCK();
				*idlock_held = B_FALSE;
			}
			page_abort_l(pp);	
			return (0);
		}
		/*
		 * Page is dirty; need to write it.
		 * Already sync'd mod bits via hat_pageunload().
		 */
	} else {
		/*
		 * Do not invalidate the page.
		 *
		 * We have an effective page reader lock.
		 */
		hat_pagegetmod(pp);

		if (!pp->p_mod)
		    goto droppage;		/* page is clean */

		/*
		 * Page is dirty; need to write it.
		 * sync mod bits via hat_pagesyncmod().
		 */
		hat_pagesyncmod(pp);
	}

	pp->p_mod = 0;
	pp->p_timestamp = 0;

	ASSERT(!pp->p_free && !pp->p_invalid && !pp->p_pageout);

	/*
	 * Convert the effective page reader/writer lock into an
	 * actual lock for pageout purposes.
	 *
	 * Adopt a page writer lock (p_invalid) for asynchronous or
	 * synchronous invalidates to prevent other processes
	 * from accessing the page in the window after the I/O is
	 * complete but before the page is aborted. If this is not
	 * done, updates to the page before it is aborted will be lost.
	 */
	pp->p_activecnt++;
	pp->p_pageout = 1;
#ifdef _PAGEIO_HIST
	pageio_stamp[pp - pages] = lbolt;
	PAGEIO_LOG_PP(NULL, pp, 0, "pvn_getdirty");
#endif
	pp->p_invalid = (flags & B_INVAL) ? 1 : 0;

	if (*idlock_held) {	/* out of order unlock ? */
		l.vmpageidlockpl = l.puselockpl;
		l.puselockpl = VM_PAGE_IPL;
	}
	PAGE_USEUNLOCK(pp);

	page_sortadd(dirty, pp);
	ASSERT(!(*idlock_held) || VM_PAGEIDLOCK_OWNED());
	return (1);


droppage:
	/*
	 * We decided we don't want/need to clean this page.
	 *
	 * Since we may have reclaimed the page if it was free,
	 * we must free the page if it's not in use.
	 *
	 * We first ensure that the page isn't already free
	 * because it could have become free while we waited
	 * for appropriate page state above.
	 */
	if (*idlock_held) {	/* out of order unlock ? */

		ASSERT(VM_PAGEIDLOCK_OWNED());

		l.vmpageidlockpl = l.puselockpl;
		l.puselockpl = VM_PAGE_IPL;
	}
	if (!pp->p_free && !PAGE_IN_USE(pp))
		page_free(pp, (flags & B_DONTNEED));
	else
		PAGE_USEUNLOCK(pp);

	ASSERT(!(*idlock_held) || VM_PAGEIDLOCK_OWNED());
	return (0);
}


/*
 * int
 * pvn_getdirty_range(int (*func)(), vnode_t *vp, off_t roff, uint_t rlen,
 *		      off_t doff, uint_t dlen, off_t filesize, int flags,
 *		      cred_t *cr)
 *
 *	Collect a list of locked dirty pages ready for cleaning;
 *	clean them by calling func.
 *
 * Calling/Exit State:
 *
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED, B_DELWRI}.
 *	B_DELWRI indicates that this page is part of a kluster operation and
 *	is only to be considered if it doesn't involve any waiting here.
 *	It is an error to specify both B_DELWRI and B_INVAL (i.e., kluster
 *	and invalidate).
 *
 *	For B_INVAL & !B_ASYNC, the goal is to have the v_pages list empty
 *	upon return.  However, this can be guaranteed only if the caller
 *	somehow prevents any new pages from being added to the v_pages list.
 *	(I.e., prevents VOP_GETPAGE() against this vnode.)
 *
 *	Called with VM_PAGEIDLOCK and VM_PAGEFREELOCK unlocked.
 *	They remain unlocked at return.
 *
 *	For all dirty pages in the "required" range [roff, roff + rlen),
 *	and, if we don't have to go to too much trouble, dirty pages in
 *	the "desired" range [doff, doff + dlen) as well, the function,
 *	func, will be called to clean the pages.  The dirty pages are
 *	passed in a link list to one or more calls to func.
 *
 * Description:
 *
 *	As noted above, in certain cases the caller will want to prevent
 *	concurrent VOP_GETPAGE() against this vnode.  In cases where the
 *	caller does not do this, this routine is still guaranteed to
 *	terminate (rather than loop "forever" examining new pages which
 *	are continually being added) since page_hashin() adds new pages
 *	to the end of the v_pages linked list.  Thus new pages are added
 *	after our end marker; i.e., between the list head (v_pages)
 *	and our end marker.  We stop examining pages when we reach the
 *	end marker and thus do not examine any new pages added after
 *	we enter our end marker page.  The presence of the end marker
 *	also ensures that page_hashout() of other pages does not cause
 *	us to miss any pages we haven't looked at yet which already
 *	existed when we started.
 *
 *	In case of B_INVAL, we make sure we don't go to sleep holding 
 *	one or more page write locks. This could cause a deadlocks w.r.t
 *	multi-segment softlocks. We also go with the vplist method if
 *	B_INVAL is specified to make the code less complex because of the
 *	deadlock reason.
 */
int
pvn_getdirty_range(int (*func)(), vnode_t *vp, off_t roff, uint_t rlen,
		   off_t doff, uint_t dlen, off_t filesize, int flags,
		   cred_t *cr)
{
	page_t *dirty = NULL;
	page_t *pp, *epp, *mpp;
	off_t off, maxoff;
	boolean_t required, lock_failed, idlock_held;
	int gflags;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ASSERT(func != (int (*)())NULL);
	ASSERT(!(vp->v_flag & VNOMAP));

	/* It is an error to specify both invalidate and kluster */
	ASSERT((flags & (B_INVAL|B_DELWRI)) != (B_INVAL|B_DELWRI));
	ASSERT(!(flags & B_TRYLOCK));	/* only used internally */

	ASSERT(filesize >= 0);
	ASSERT(rlen == 0 || dlen != 0);
	ASSERT(doff <= roff);
	ASSERT(dlen == 0 || rlen <= dlen + (roff - doff));
	ASSERT((roff & PAGEOFFSET) == 0);
	ASSERT((rlen & PAGEOFFSET) == 0);
	ASSERT((doff & PAGEOFFSET) == 0);
	ASSERT((dlen & PAGEOFFSET) == 0);

	/*
	 * Skip klustering if B_INVAL.
	 */

	if (flags & B_INVAL) {
		doff = roff;
		dlen = rlen;
		/*
		 * If B_INVAL is specified, we directly go to the vplist
		 * method. We also skip the null v_pages stale check done
		 * later, since we are required to clean the pages in the
		 * range.
		 */
		goto binval_start;
	}

	/*
	 * We use one of two algorithms to find the desired pages:
	 *
	 * The first is the "range" method, which looks up pages by
	 * identity, scanning the required range in order, then
	 * klustering outward, conditionally for the rest of the
	 * desired range; this method is best when the number of
	 * pages in the desired range is small relative to the
	 * number of pages on the v_pages list.
	 *
	 * The second is the "vplist" method, which scans the v_pages
	 * list, skipping pages outside of the desired range, picking
	 * up as many pages at a time as it can without getting into
	 * locking order problems; this method is best when most of
	 * the pages on the v_pages list are in the desired range.
	 *
	 * Based on the conditions we pick the algorithms which we hope
	 * will be best suited.  We'd like to make this decision based
	 * on the size of the v_pages list, but we don't have this
	 * available.  Instead we estimate, using the file size.
	 *
	 * If the desired range is at least half of the current file
	 * size, we use the "vplist" method, otherwise we use the
	 * "range" method.
	 */

	if (dlen == 0 || 2 * dlen >= filesize) {

		/* "vplist" method */

		/*
		 * We peek at v_pages without holding VM_PAGEIDLOCK.
		 * This works as long as v_pages can be read atomically
		 * on the bus.  We do this to defer locking VM_PAGEIDLOCK
		 * until after kmem_zalloc(). 
		 */

		if (vp->v_pages == NULL)
			return (0);

binval_start:
		/*
		 * Allocate two marker pages.
		 */

		CREATE_2_MARKER_PAGES_PD(mpp, epp, pvn_getdirty_range);

		VM_PAGEIDWRLOCK();
		if ((pp = vp->v_pages) == NULL) {
			VM_PAGEIDUNLOCK();
			DESTROY_2_MARKER_PAGES_PD(mpp, epp);
			return (0);
		}

		/*
		 * Insert an end marker at the end of the v_pages list,
		 * so we'll know when we've traversed the entire list.
		 * (See the function header comment above for more detail.)
		 */

		INSERT_END_MARKER_PAGE(epp, vp);

		maxoff = 0;
		idlock_held = B_TRUE;

		do {
			ASSERT(idlock_held && VM_PAGEIDLOCK_OWNED());

			/*
			 * Other routines add marker pages in v_pages lists,
			 * just like we do.  Skip over them if we see any.
			 */
			if (pp->p_vnode != vp) {
				pp = pp->p_vpnext;
				continue;
			}

			/*
			 * If this page is outside the desired range, skip it.
			 */
			off = pp->p_offset;
			if (off < doff || (dlen != 0 && off - doff >= dlen)) {
				pp = pp->p_vpnext;
				continue;
			}

			/*
			 * See if this page is in the required range.
			 */
			required = (off >= roff &&
				    (rlen == 0 || off - roff < rlen));

			/*
			 * We need to examine page fields which require holding
			 * PAGE_USELOCK(pp).  We must drop VM_PAGEIDLOCK in
			 * order to acquire PAGE_USELOCK(pp).
			 *
			 * Since we're dropping VM_PAGEIDLOCK, we can race
			 * with others changing the identity of the page.
			 * We put a marker after the current page so we don't
			 * lose our place when/if the current page is removed
			 * from the v_pages list.
			 */

			INSERT_MARKER_PAGE(pp, mpp);

			if (psm_intrpend(PLBASE) ||
			    TRYLOCK(&pp->p_uselock, VM_PAGE_IPL) == INVPL) {

				VM_PAGEIDUNLOCK();
				PAGE_USELOCK(pp);

				if (!PAGE_HAS_IDENTITY(pp, vp, off)) {
					/*
					 * We lost the race after dropping
					 * VM_PAGEIDLOCK.
					 */
					PAGE_USEUNLOCK(pp);
					idlock_held = B_FALSE;
					goto contin;
				}
				VM_PAGEIDWRLOCK();
			} else {
				l.puselockpl = l.vmpageidlockpl;
				l.vmpageidlockpl = VM_PAGE_IPL;
			}

			gflags = flags;
			if (required) {
				lock_failed = B_FALSE;
				/*
				 * Specify B_TRYLOCK if we want to lock pages
				 * out of offset order or there are already
				 * pages write locked (dirty list not empty).
				 * If we go ahead and wait, we would cause 
				 * deadlocks w.r.t. to multi segment softlocks.
				 */
				if ((off < maxoff) || ((flags & B_INVAL) &&
							dirty != NULL))
					gflags |= B_TRYLOCK;
			} else
				gflags |= B_DELWRI;

			if (pvn_getdirty(pp, &dirty, gflags, &lock_failed,
					&idlock_held, WRITE_LOCKED)) {
				if (off > maxoff)
					maxoff = off;
			} else if (required && lock_failed) {
				/*
				 * We've got as much as we can in this batch.
				 * Send it off, then pick up where we left off.
				 */
				ASSERT(dirty != NULL);
				/*
				 * If lock_failed is set to true by 
				 * pvn_getdirty, then it is guranteed not to
				 * drop the idlock.
				 */
				ASSERT(idlock_held);
				/*
				 * Reposition the marker page to be before
				 * the current page, so that we can examine
				 * the same page when we continue.
				 */
				REMOVE_MARKER_PAGE(mpp, vp);
				INSERT_MARKER_PAGE(pp->p_vpprev, mpp);
				/* 
				 * We need to drop the id lock since
				 * the putpage function can block.
				 */
				idlock_held = B_FALSE;
				VM_PAGEIDUNLOCK();

				CHECK_DIRTY_LIST(dirty, vp, rlen);
				error = (*func) (vp, dirty, flags, cr);
				if (error) {
					VM_PAGEIDWRLOCK();
					REMOVE_MARKER_PAGE(mpp, vp);
					REMOVE_MARKER_PAGE(epp, vp);
					VM_PAGEIDUNLOCK();
					DESTROY_2_MARKER_PAGES_PD(mpp, epp);
					return (error);
				}
				dirty = NULL;
				maxoff = 0;
			}
contin:
			if (!idlock_held) {
				idlock_held = B_TRUE;
				VM_PAGEIDWRLOCK();
			}
			ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);

		} while (pp != epp);

		/* remove epp and deallocate markers */

		REMOVE_MARKER_PAGE(epp, vp);
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES_PD(mpp, epp);

	} else {

		/* "range" method */

		/*
		 * First do all the pages in the required range.
		 *
		 * We allow pvn_getdirty() to block while locking the page
		 * even though we hold pages locked from prior iterations.
		 * We eliminate deadlock problems by always locking pages
		 * in ascending vnode offset order.
		 */

		VM_PAGEIDRDLOCK();
		idlock_held = B_TRUE;
		
		for (off = roff; off - roff < rlen; off += PAGESIZE) {

			pp = page_exists_uselock(vp, off, READ_LOCKED);

			ASSERT(VM_PAGEIDLOCK_OWNED());

			if (pp) {
				ASSERT(PAGE_USELOCK_OWNED(pp));
				(void) pvn_getdirty(pp, &dirty, flags, 
					NULL, &idlock_held, READ_LOCKED);
				if (!idlock_held) {
					VM_PAGEIDRDLOCK();
					idlock_held = B_TRUE;
				}
			}
		}

		gflags = flags | B_DELWRI;

		/*
		 * Now scan backwards looking for pages to kluster.
		 *
		 * No deadlock problem since pvn_getdirty() will not block
		 * (since B_DELWRI is specified).
		 */

		ASSERT(idlock_held);
		ASSERT(VM_PAGEIDLOCK_OWNED());

		for (off = roff - PAGESIZE; off >= doff; off -= PAGESIZE) {

		    pp = page_exists_uselock(vp, off, READ_LOCKED);

		    if (pp == NULL)
			break;	/* willing to try only this hard */

		    ASSERT(PAGE_USELOCK_OWNED(pp));
		    ASSERT(VM_PAGEIDLOCK_OWNED());
	
		    if (!pvn_getdirty(pp, &dirty, gflags, NULL, &idlock_held,
				READ_LOCKED))
			break;	/* willing to try only this hard */

		    if (!idlock_held) {
			idlock_held = B_TRUE;
			VM_PAGEIDRDLOCK();
		    }
		}

		/*
		 * Now scan forwards looking for pages to kluster.
		 *
		 * No deadlock problem since pvn_getdirty() will not block
		 * (since B_DELWRI is specified).
		 */

		if (!idlock_held) {
			VM_PAGEIDRDLOCK();
			idlock_held = B_TRUE;
		}
		ASSERT(VM_PAGEIDLOCK_OWNED());

		for (off = roff + rlen; off - doff < dlen; off += PAGESIZE) {

		    pp = page_exists_uselock(vp, off, READ_LOCKED);

		    if (pp == NULL)
			break;	/* willing to try only this hard */

		    ASSERT(PAGE_USELOCK_OWNED(pp));
		    ASSERT(VM_PAGEIDLOCK_OWNED());
		    ASSERT(idlock_held);

		    if (!pvn_getdirty(pp, &dirty, gflags, NULL, &idlock_held,
					READ_LOCKED))
				break;	/* willing to try only this hard */
		   
		    if (!idlock_held) {
			VM_PAGEIDRDLOCK();
			idlock_held = B_TRUE;
		    }
		}

		if (idlock_held)
		    VM_PAGEIDUNLOCK();
	}

	/*
	 * Send off last batch (only batch for "range" method), if any.
	 */

	if (dirty) {
		CHECK_DIRTY_LIST(dirty, vp, rlen);
		return (*func) (vp, dirty, flags, cr);
	}

	return (0);
}


/*
 * void
 * pvn_unload(vnode_t *vp)
 *
 *	Unload translations for all pages of the vnode, vp.
 *
 * Calling/Exit State:
 *
 *	Called with VM_PAGEIDLOCK and VM_PAGEFREELOCK unlocked.
 *	They remain unlocked at return.
 *
 * Description:
 *
 *	See the pvn_getdirty_range() function header comment for a description
 *	on how a start marker page is used to traverse the v_pages list.
 */
void
pvn_unload(vnode_t *vp)
{
	page_t *pp, *epp, *mpp;
	uint_t offset;

	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * We peek at v_pages without holding VM_PAGEIDLOCK.
	 * This works as long as v_pages can be read atomically on the bus.
	 * We do this to defer locking VM_PAGEIDLOCK until after kmem_zalloc().
	 */
	if (vp->v_pages == NULL)
		return;

	/*
	 * Allocate two marker pages.
	 */

	CREATE_2_MARKER_PAGES(mpp, epp, pvn_unload);

	VM_PAGEIDWRLOCK();
	if ((pp = vp->v_pages) == NULL) {
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES(mpp, epp);
		return;
	}

	/*
	 * Insert an end marker at the end of the v_pages list,
	 * so we'll know when we've traversed the entire list.
	 * (See the function header comment above for more detail.)
	 */

	INSERT_END_MARKER_PAGE(epp, vp);

	do {
		/*
		 * Other routines add marker pages in v_pages lists,
		 * just like we do.  Skip over them if we see any.
		 */
		if (pp->p_vnode != vp) {
			pp = pp->p_vpnext;
			continue;
		}

		/*
		 * Now we need to examine page fields which require holding
		 * PAGE_USELOCK(pp).  We must drop VM_PAGEIDLOCK in order to
		 * acquire PAGE_USELOCK.  This means we race with others
		 * changing the identity of the page.  We put a marker
		 * after the current page so we don't lose our place
		 * when/if the current page is removed from the v_pages list.
		 */

		offset = pp->p_offset;

		INSERT_MARKER_PAGE(pp, mpp);

		/*
		 * There is some very tricky pl manipulation going on here.
		 * Typically, when we acquire a lock out of order
		 * through a trylock, we would need to readjust the pl
		 * values. But in this case, we don't do that because
		 * we are going to a out of order unlock as well via
		 * page_free(). We do perform pl manipulation when we
		 * acquire lock in the right order for the out of order
		 * lock release.
		 */
		if (TRYLOCK(&pp->p_uselock, VM_PAGE_IPL) == INVPL) {
			VM_PAGEIDUNLOCK();
			PAGE_USELOCK(pp);

			if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
				PAGE_USEUNLOCK(pp);
				VM_PAGEIDWRLOCK();
				goto contin;
			}
			VM_PAGEIDWRLOCK();
			l.vmpageidlockpl = l.puselockpl;
		}

		l.puselockpl = VM_PAGE_IPL;
	
		/*
		 * Unload the page's translations.
		 */

		if (pp->p_mapping) {
			hat_pageunload(pp);
			ASSERT(!pp->p_mapping);
			/* racing with pvn_done() ? */
			if (!PAGE_IS_LOCKED(pp)) {
				page_free(pp, 0);
				goto contin;
			}
		}

		PAGE_USEUNLOCK(pp);
contin:
		/*
		 * Continue loop iteration when we hold PAGE_USELOCK(pp)
		 * instead of VM_PAGEIDLOCK().
		 */
		ASSERT(VM_PAGEIDLOCK_OWNED());

		ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);

	} while (pp != epp);

	/* remove epp and deallocate markers */

	REMOVE_MARKER_PAGE(epp, vp);
	VM_PAGEIDUNLOCK();
	DESTROY_2_MARKER_PAGES(mpp, epp);
}


/*
 * void
 * pvn_abort_range(vnode_t *vp, off_t off, uint_t len)
 *
 *	Invalidate all vnode, vp, pages completely contained in [off, off+len-1]
 *	or, if len == 0, in [off, EOF].
 *
 * Calling/Exit State:
 *
 *	The caller must ensure that, for this vnode, there are:
 *		- no concurrently running VOP_GETPAGE() operations that
 *		  that would re-instantiate the pages we are invalidating.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 *
 *	Typically the caller first calls pvn_trunczero() and then calls
 *	pvn_abort_range().
 *
 * Description:
 *
 *	See the pvn_getdirty_range() function header comment for a description
 *	on how a start marker page is used to traverse the v_pages list.
 */
void
pvn_abort_range(vnode_t *vp, off_t off, uint_t len)
{
	page_t *pp, *epp, *mpp;
	uint_t offset;

	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * We peek at v_pages without holding VM_PAGEIDLOCK().
	 * This works as long as v_pages can be read atomically on the bus.
	 * We do this to defer VM_PAGEIDRDLOCK() until after kmem_zalloc().
	 */
	if (vp->v_pages == NULL) {
		return;
	}

	/*
	 * Allocate two marker pages.
	 */

	CREATE_2_MARKER_PAGES(mpp, epp, pvn_abort_range);

	VM_PAGEIDWRLOCK();
	if ((pp = vp->v_pages) == NULL) {
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES(mpp, epp);
		return;
	}

	/*
	 * Insert an end marker at the end of the v_pages list,
	 * so we'll know when we've traversed the entire list.
	 * (See the function header comment above for more detail.)
	 */

	INSERT_END_MARKER_PAGE(epp, vp);

	do {
		/*
		 * Other routines add marker pages in v_pages lists,
		 * just like we do.  Skip over them if we see any.
		 */
		if (pp->p_vnode != vp) {
			pp = pp->p_vpnext;
			continue;
		}

		/*
		 * If this page is (even partially) outside the
		 * abort range, it stays as is.
		 */
		if ((pp->p_offset < off)  ||
		    ((len != 0) && (pp->p_offset + PAGESIZE > off + len))) {
			pp = pp->p_vpnext;
			continue;
		}
		offset = pp->p_offset;

		/*
		 * Now we need to examine page fields which require holding
		 * PAGE_USELOCK(pp).  We must drop VM_PAGEIDLOCK in order to
		 * acquire PAGE_USELOCK.  This means we race with others
		 * changing the identity of the page.  We put a marker
		 * after the current page so we don't lose our place
		 * when/if the current page is removed from the v_pages list.
		 */

		INSERT_MARKER_PAGE(pp, mpp);

		VM_PAGEIDUNLOCK();

		PAGE_USELOCK(pp);
		if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
			/* Lost the race after dropping VM_PAGEIDLOCK */
			goto contin;
		}

		/* 
		 * Wait for exclusive access to the page.
		 */

		while (pp->p_activecnt) {
				PAGE_WAIT(pp);

				/*
				 * Re-verify page state since it could have
				 * changed while we were sleeping.
				 */

				if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
					/*
					 * Lost the page - nothing to do.
					 */
					goto contin;
				}
		}
		ASSERT(!pp->p_invalid);

		/* We now have an effective writer lock on the page */

		/*
		 * If we can reclaim it from the free list (if necessary)
		 * then abort the identity.  Otherwise the reclaim attempt
		 * aborted the identity for us.
		 */

		if (PAGE_RECLAIM(pp)) {
			page_abort_l(pp);
			goto contin1;
		}

		/*
		 * If PAGE_RECLAIM() failed to get the page,
		 * it at least destroyed the identity for us.
		 */
		ASSERT(!PAGE_HAS_IDENTITY(pp, vp, offset));

contin:
		/*
		 * Continue loop iteration when we hold PAGE_USELOCK(pp)
		 * instead of VM_PAGEIDLOCK.
		 */
		PAGE_USEUNLOCK(pp);
contin1:
		VM_PAGEIDWRLOCK();

		ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);

	} while (pp != epp);

	/* remove epp and deallocate markers */

	REMOVE_MARKER_PAGE(epp, vp);
	VM_PAGEIDUNLOCK();
	DESTROY_2_MARKER_PAGES(mpp, epp);
}

/*
 * boolean_t
 * pvn_abort_range_nosleep(vnode_t *vp, off_t off, uint_t len)
 *
 *	Invalidate all vnode, vp, pages completely contained in [off, off+len-1]
 *	or, if len == 0, in [off, EOF].
 *
 * Calling/Exit State:
 *
 *	The caller must ensure that, for this vnode, there are:
 *		- no concurrently running VOP_GETPAGE() operations that
 *		  that would re-instantiate the pages we are invalidating.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 *
 *	Typically the caller first calls pvn_trunczero() and then calls
 *	pvn_abort_range().
 *
 *	This function does not block.
 *
 *	Returns B_TRUE if successful and B_FALSE otherwise.
 *
 * Description:
 *
 *	See the pvn_getdirty_range() function header comment for a description
 *	on how a start marker page is used to traverse the v_pages list.
 */
boolean_t
pvn_abort_range_nosleep(vnode_t *vp, off_t off, uint_t len)
{
	page_t *pp, *epp, *mpp;
	uint_t offset;

	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * We peek at v_pages without holding VM_PAGEIDLOCK().
	 * This works as long as v_pages can be read atomically on the bus.
	 * We do this to defer VM_PAGEIDRDLOCK() until after kmem_zalloc().
	 */
	if (vp->v_pages == NULL) {
		return B_TRUE;
	}

	/*
	 * Allocate two marker pages.
	 */

	CREATE_2_MARKER_PAGES_NS(mpp, epp, pvn_abort_range_nosleep);
	if (mpp == NULL)
		return B_FALSE;

	VM_PAGEIDWRLOCK();
	if ((pp = vp->v_pages) == NULL) {
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES(mpp, epp);
		return B_TRUE;
	}

	/*
	 * Insert an end marker at the end of the v_pages list,
	 * so we'll know when we've traversed the entire list.
	 * (See the function header comment above for more detail.)
	 */

	INSERT_END_MARKER_PAGE(epp, vp);

	do {
		/*
		 * Other routines add marker pages in v_pages lists,
		 * just like we do.  Skip over them if we see any.
		 */
		if (pp->p_vnode != vp) {
			pp = pp->p_vpnext;
			continue;
		}

		/*
		 * If this page is (even partially) outside the
		 * abort range, it stays as is.
		 */
		if ((pp->p_offset < off)  ||
		    ((len != 0) && (pp->p_offset + PAGESIZE > off + len))) {
			pp = pp->p_vpnext;
			continue;
		}
		offset = pp->p_offset;

		/*
		 * Now we need to examine page fields which require holding
		 * PAGE_USELOCK(pp).  We must drop VM_PAGEIDLOCK in order to
		 * acquire PAGE_USELOCK.  This means we race with others
		 * changing the identity of the page.  We put a marker
		 * after the current page so we don't lose our place
		 * when/if the current page is removed from the v_pages list.
		 */

		INSERT_MARKER_PAGE(pp, mpp);

		VM_PAGEIDUNLOCK();

		PAGE_USELOCK(pp);
		if (!PAGE_HAS_IDENTITY(pp, vp, offset)) {
			/* Lost the race after dropping VM_PAGEIDLOCK */
			goto contin;
		}

		/* 
		 * Fail this abort attempt if the page is busy.
		 */

		if (pp->p_activecnt) {
			PAGE_USEUNLOCK(pp);
			VM_PAGEIDWRLOCK();
			REMOVE_MARKER_PAGE(epp, vp);
			REMOVE_MARKER_PAGE(mpp, vp);
			VM_PAGEIDUNLOCK();
			DESTROY_2_MARKER_PAGES(mpp, epp);
			return B_FALSE;
		}
		ASSERT(!pp->p_invalid && !pp->p_pageout);

		/* We now have an effective writer lock on the page */

		/*
		 * If we can reclaim it from the free list (if necessary)
		 * then abort the identity.  Otherwise the reclaim attempt
		 * aborted the identity for us.
		 */

		if (PAGE_RECLAIM(pp)) {
			page_abort_l(pp);
			goto contin1;
		}

		/*
		 * If PAGE_RECLAIM() failed to get the page,
		 * it at least destroyed the identity for us.
		 */
		ASSERT(!PAGE_HAS_IDENTITY(pp, vp, offset));

contin:
		/*
		 * Continue loop iteration when we hold PAGE_USELOCK(pp)
		 * instead of VM_PAGEIDLOCK.
		 */
		PAGE_USEUNLOCK(pp);
contin1:
		VM_PAGEIDWRLOCK();

		ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);

	} while (pp != epp);

	/* remove epp and deallocate markers */

	REMOVE_MARKER_PAGE(epp, vp);
	VM_PAGEIDUNLOCK();
	DESTROY_2_MARKER_PAGES(mpp, epp);

	return B_TRUE;
}

/*
 * int
 * pvn_trunczero(vnode_t *vp, uint_t off, uint_t zbytes)
 *
 *	First zero out zbytes worth of file starting at off.
 *
 * Calling/Exit State:
 *
 *	The caller must ensure that, for this vnode, there are:
 *		- no concurrently running pvn_trunczero() operations
 *		  for this vnode.
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 *
 *	[off, off+zbytes) must fall entirely within a MAXBSIZE file chunk.
 *
 *	If "zbytes" is non-zero, we segmap_getmap() [off, off+zbytes),
 *	but we kzero() not only that range but also roundup to a page
 *	boundary if the entire segmap_getmap() range falls within a
 *	single page.  This results in zeroing zbytes in
 *	the file's backing store and, in addition, possibly zeroing the
 *	remainder of the page.  These will typically be the same for
 *	blocksize >= PAGESIZE but may be different if blocksize < PAGESIZE.
 *	(See the comments in the code below for further explanation of
 *	why this is done.)
 *
 *	If "zbytes" is zero, which means "off" is on the file system
 *	logical block boundary. We don't need to zero the file's backing
 *	store. However, we still need to zero the remainder of the page
 *	if it is currently cached in the page cache. We call 
 *	page_find_zero() to do so.
 *
 *	Returns zero if successful, else returns the error.
 *	In the case of error, it is undefined whether any or all
 *	of the zbytes have been zeroed.
 *
 *	Typically the caller first calls pvn_trunczero() and then calls
 *	pvn_abort_range().
 */
int
pvn_trunczero(vnode_t *vp, uint_t off, uint_t zbytes)
{
	char *addr;
	int error = 0, error2;

	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * Zero out zbytes worth of file starting at offset, off.
	 *
	 * This accomplishes two things:
	 *	- partial zeroing of the page containing off
	 *	- partial zeroing of the file system block containing off
	 *
	 *
	 * In the case where the file system block size >= PAGESIZE,
	 * off + zbytes will always align with a page boundary.
	 * The partial zeroing operation creates and dirties pages
	 * beyond off to the end of the file system block.  After
	 * the zeroing, the caller finds and page_abort()'s these
	 * pages (typically via pvn_abort_range()).  If the I/O has
	 * already started, pvn_abort_range() will wait for it to finish
	 * (it'll gain a page writer lock) before doing the page_abort().
	 * But if the I/O hadn't started, the pages would be destroyed and the
	 * file system block wouldn't be zeroed on disk.  (This would be bad.)
	 *
	 * In order to ensure the I/O has started before the subsequent
	 * page_abort()'s are attempted, we do *not* specify SM_ASYNC
	 * to segmap_release().  This is because segmap_release()
	 * translates SM_ASYNC to VOP_PUTPAGE(B_ASYNC) which
	 * causes the pvn dirty functions to ignore the page if
	 * it can't be immediately locked.  Thus if there were any
	 * contention for the page during VOP_PUTPAGE(), the I/O
	 * would not be started.
	 *
	 *
	 * In the case where the file system block size < PAGESIZE,
	 * off + zbytes may not align with a page boundary.
	 * If the zbytes fall entirely within the same page (the page
	 * containing the EOF, off) then we zero not only the zbytes
	 * within the page but also any bytes in the page beyond the
	 * zbytes.  This addresses the case where the page contains
	 * file system block(s) entirely beyond the zbytes and these
	 * blocks contain non-zero data before the truncate (and hence
	 * would be present in the page we're zeroing) but these blocks
	 * will not exist after the truncate.  Subsequent page_abort()'s
	 * (typically done via pvn_abort_range()) will not touch this
	 * page because it contains the EOF (and hence valid data).  We
	 * must ensure the page's data for these (soon to be non-existent)
	 * blocks is zero.
	 */

	if ((zbytes + (off & MAXBOFFSET)) > MAXBSIZE) {
		/*
		 *+ A caller of pvn_trunczero() requested zeroing
		 *+ data beyond a segmap chunk size (MAXBSIZE).
		 *+ This indicates a kernel software problem.
		 *+ Corrective action: none.
		 */
		cmn_err(CE_PANIC, "pvn_trunczero: zbytes");
	}
	if (zbytes != 0) {
		addr = segmap_getmap(segkmap, vp, off, zbytes, S_WRITE,
				     B_TRUE, NULL);
		error = kzero(addr + (off & MAXBOFFSET),
			MAX(zbytes, PAGESIZE - (off & PAGEOFFSET)));
		error2 = segmap_release(segkmap, addr, 0);
		if (!error)
			error = error2;
	} else
		page_find_zero(vp, off);

	return(error);
}

/*
 * int
 * pvn_getpages(int (*getapage)(), vnode_t *vp, off_t off, uint_t len,
 * 	        uint_t *protp, page_t *pl[], uint_t plsz, struct seg *seg,
 * 	        vaddr_t addr, enum seg_rw rw, struct cred *cred)
 *
 * 	Handles common work of the VOP_GETPAGE routines when more than
 * 	one page must be returned by calling a file system specific
 * 	operation to do most of the work.
 *
 * Calling/Exit State:
 *
 *	Must be called with the vp already locked by the VOP_GETPAGE routine.
 *
 *	off and addr are not required to be PAGESIZE aligned; however,
 *		they must agree with each other.
 *	plsz is the number of bytes covered by pl[]; i.e., it is
 *		sizeof(pl[]) * PAGESIZE.
 */
int
pvn_getpages(int (*getapage)(), vnode_t *vp, off_t off, uint_t len,
	     uint_t *protp, page_t *pl[], uint_t plsz, struct seg *seg,
	     vaddr_t addr, enum seg_rw rw, struct cred *cred)
{
	page_t **ppp;
	off_t o, eoff;
	uint_t sz, n;
	int err;

	ASSERT(!(vp->v_flag & VNOMAP));

	ASSERT((off & PAGEOFFSET) == (addr & PAGEOFFSET));
	ASSERT(pl == NULL || (plsz & PAGEOFFSET) == 0);
	/* insure that we have enough space */
	ASSERT(pl == NULL || btop(plsz) >= btopr((off & PAGEOFFSET) + len));

	/*
	 * Loop one page at a time and let getapage function fill
	 * in the next page in array.  We only allow one page to be
	 * returned at a time (except for the last page) so that we
	 * don't have any problems with duplicates and other such
	 * painful problems.  This is a very simple minded algorithm,
	 * but it does the job correctly.  We hope that the cost of a
	 * getapage call for a resident page that we might have been
	 * able to get from an earlier call doesn't cost too much.
	 */
	ppp = pl;
	sz = PAGESIZE;
	eoff = off + len;
	for (o = off; o < eoff; o += n, addr += n) {

		/*
		 * Calculate transfer size within this page.
		 * It will be less than PAGESIZE if the starting offset
		 * is not page aligned, or if the specified length
		 * stops mid page.
		 */

		n = PAGESIZE - (o & PAGEOFFSET);
		if (n > len) {
			n = len;
		}

		if (o + n >= eoff) {
			/*
			 * Last time through - allow all of
			 * what's left of the pl[] array to be used.
			 */
			sz = plsz - ((o & PAGEMASK) - (off & PAGEMASK));
		}
		err = (*getapage)(vp, o, n, protp, ppp, sz, seg, addr, rw, cred);
		if (err) {
			/*
			 * Release any pages we already got.
			 * If the page data is not valid,
			 * we must abort the page.
			 */
			if (o > off && pl != NULL) {
				for (ppp = pl; *ppp != NULL; *ppp++ = NULL) {
					if ((*ppp)->p_invalid) {
						page_abort(*ppp);
					} else
						page_unlock(*ppp);
				}
			}
			break;
		}
		if (pl != NULL)
			ppp++;

		len -= n;
	}

	return (err);
}


/*
 * page_t *
 * pvn_kluster(vnode_t *vp, off_t off, struct seg *seg, vaddr_t addr,
 * 	    off_t *offp, uint_t *lenp, off_t vp_off, uint_t vp_len, page_t *pp)
 *
 *	Returns a list of klustered pages for the caller to fill in.
 *
 * Calling/Exit State:
 *
 *	When reading from a filesystem where the filesystem block size is
 *	larger than the page size, it is desirable to read in all the pages
 *	in the block with a single I/O.  (Assuming the pages are not already
 *	present in memory and that the relevant segment can use those pages
 *	[i.e., has that portion of the file mapped] and that spare memory
 *	is readily available.)  It is not really useful to do such klustering
 *	when the file system blocksize is <= PAGESIZE.  However it may still
 *	be useful to call this function to perform a lazy page creation
 *	of the "center" page in the case of read-ahead (see below).
 *
 *	Find the largest contiguous block which contains `addr' for
 *	file offset `off' in it while living within the file system
 *	block sizes (`vp_off' and `vp_len') and the address space
 *	limits for which no pages currently exist and which map to
 *	consecutive file offsets.
 *
 *	off, addr, and vp_off must be PAGESIZE aligned.  vp_len need not be
 *	PAGESIZE aligned (e.g., the EOF occurs the middle of a page, or the
 *	file system blocksize < PAGESIZE).
 *
 *	pp, if !NULL, points to the "center" page in the kluster which should
 *	be already allocated with the proper identity and page writer locked.
 *	If pp == NULL, pvn_kluster() will create the "center" page if it
 *	doesn't already exist and if sufficient memory is immediately available.
 *	The caller supplies pp == NULL when it is doing a read-ahead operation;
 *	otherwise the caller supplies pp pointing to the writer locked page
 *	returned by page_lookup_or_create().
 *
 *	Called with VM_PAGEIDLOCK() and VM_PAGEFREELOCK() unlocked. They
 *	remain unlocked at return.
 *
 *	Returns an indication of the found "largest contiguous block":
 *
 *	*offp contains the (PAGESIZE aligned) starting vnode offset of the block
 *
 *	*lenp contains the size (in bytes) of the the block.  This size will
 *	not be a multiple of PAGESIZE in the case where vp_len is not a
 *	multiple of PAGESIZE.  However the returned page list contains a
 *	page for any page portion represented by *lenp.
 *
 *	Also returns a list of newly created pages for the block:
 *	Each page in the returned page list is page writer locked.
 *	It is the caller's responsibility to fill in valid data.
 *
 *	NOTE:  If the caller supplies pp == NULL and if the "center" page
 *	already exists or memory is not readily available, then this
 *	function will return a NULL list of pages, and both *offp and
 *	*lenp are undefined.
 */
page_t *
pvn_kluster(vnode_t *vp, off_t off, struct seg *seg, vaddr_t addr,
	    off_t *offp, uint_t *lenp, off_t vp_off, uint_t vp_len, page_t *pp)
{
	page_t *pplist;
	off_t	vp_end = vp_off + vp_len;
	int	delta, delta2;

	ASSERT(!(vp->v_flag & VNOMAP));
	ASSERT(off >= vp_off && off < vp_end);
	ASSERT((addr & PAGEOFFSET) == 0);	/* addr is page aligned */
	ASSERT((off & PAGEOFFSET) == 0);	/* off is page aligned */
	ASSERT((vp_off & PAGEOFFSET) == 0);	/* vp_off is page aligned */

	/* Allocate "center" page, if caller hasn't */

	if (pp == NULL) {
		if ((pp = page_lazy_create(vp, off)) == NULL) {
			 return(NULL);	/* page already exists or no memory */
		}
	}
	pplist = pp;		/* pplist initially contains pp */

	/*
	 * Scan forward from back.
	 * (For each page within vp_len beyond center page.)
	 */

	for (delta = PAGESIZE; off + delta < vp_end; delta += PAGESIZE) {
		/*
		 * Call back to the segment driver to verify that
		 * the klustering/read ahead operation makes sense.
		 */
		if (SOP_KLUSTER(seg, addr, delta)) {
			break;		/* page not eligible */
		}
		if ((pp = page_lazy_create(vp, off + delta)) == NULL) {
			 break;		/* page already exists or no memory */
		}
		page_sortadd(&pplist, pp);
	}
	delta2 = delta;

	/*
	 * Scan backward from front.
	 * (For each page within vp_len before center page.)
	 */

	for (delta = -PAGESIZE; off + delta >= vp_off; delta -= PAGESIZE) {
		/*
		 * Call back to the segment driver to verify that
		 * the klustering/read ahead operation makes sense.
		 */
		if (SOP_KLUSTER(seg, addr, delta)) {
			break;		/* page not eligible */
		}
		if ((pp = page_lazy_create(vp, off + delta)) == NULL) {
			 break;		/* page already exists or no memory */
		}
		page_sortadd(&pplist, pp);
	}

	/*
	 * Report back to the caller our chosen vnode offset and length.
	 */

	*offp = off + delta + PAGESIZE;
	*lenp = delta2 - (delta + PAGESIZE);
	ASSERT(*offp >= vp_off);

	/*
	 * The ending vnode offset we've chosen is PAGESIZE aligned.
	 * However, this may be beyond the allowed ending vnode offset
	 * since the latter is file system blocksize aligned which may
	 * not be PAGESIZE aligned (if blocksize < PAGESIZE).
	 * If this is the case, reduce our chosen ending vnode offset.
	 */

	if ((*offp + *lenp) > vp_end) {
		ASSERT(*offp < vp_end);
		*lenp = vp_end - *offp;
	}

	return(pplist);
}

/*
 * void
 * pvn_syncsdirty(vnode_t *vp)
 *	Sync the dirty bits from the translations for the pages of vnode
 *	``vp'' into the corresponding pp->p_mod bits, and also into the
 *	VMOD bit in vp->v_flag.
 *
 * Calling/Exit State:
 *	Called at PLBASE with no LOCKs held and returns that way.
 *
 * Description:
 *	This function has the side effect of clearing the dirty bits in
 *	the translations which it has synced.
 */
void
pvn_syncsdirty(vnode_t *vp)
{
	page_t *pp, *epp, *mpp;
	boolean_t idlock_held;
	boolean_t pmod;
	off_t off;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * We peek at v_pages without holding VM_PAGEIDLOCK.
	 * This works as long as v_pages can be read atomically
	 * on the bus.  We do this to defer locking VM_PAGEIDLOCK
	 * until after kmem_zalloc().
	 */
	if (vp->v_pages == NULL)
		return;

	/*
	 * Allocate two marker pages.
	 */
	CREATE_2_MARKER_PAGES(mpp, epp, pvn_syncsdirty);

	VM_PAGEIDWRLOCK();
	if ((pp = vp->v_pages) == NULL) {
		VM_PAGEIDUNLOCK();
		DESTROY_2_MARKER_PAGES_PD(mpp, epp);
		return;
	}

	/*
	 * Initialize boolean.
	 */
	idlock_held = B_TRUE;

	/*
	 * Insert an end marker at the end of the v_pages list,
	 * so we'll know when we've traversed the entire list.
	 * (See the function header comment above for more detail.)
	 */
	INSERT_END_MARKER_PAGE(epp, vp);

	do {
		ASSERT(idlock_held && VM_PAGEIDLOCK_OWNED());

		/*
		 * Other routines add marker pages in v_pages lists,
		 * just like we do.  Skip over them if we see any.
		 */
		if (pp->p_vnode != vp) {
			pp = pp->p_vpnext;
			continue;
		}

		/*
		 * We need to examine page fields which require holding
		 * PAGE_USELOCK(pp).  We must drop VM_PAGEIDLOCK in
		 * order to acquire PAGE_USELOCK(pp).
		 *
		 * Since we're dropping VM_PAGEIDLOCK, we can race
		 * with others changing the identity of the page.
		 * We put a marker after the current page so we don't
		 * lose our place when/if the current page is removed
		 * from the v_pages list.
		 */

		off = pp->p_offset;
		ASSERT(getpl() == VM_PAGE_IPL);
		if (psm_intrpend(PLBASE) ||
		    TRYLOCK(&pp->p_uselock, VM_PAGE_IPL) == INVPL) {

			INSERT_MARKER_PAGE(pp, mpp);
			VM_PAGEIDUNLOCK();
			idlock_held = B_FALSE;
			PAGE_USELOCK(pp);

			if (!PAGE_HAS_IDENTITY(pp, vp, off)) {
				/*
				 * We lost the race after dropping
				 * VM_PAGEIDLOCK.
				 */
				goto contin;
			}
		} else {
			l.puselockpl = VM_PAGE_IPL;
		}

		/*
		 * Ask hat if the page is modified.
		 */
		hat_pagesyncmod(pp);

contin:
		PAGE_USEUNLOCK(pp);
		if (!idlock_held) {
			VM_PAGEIDWRLOCK();
			idlock_held = B_TRUE;
			ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);
		} else {
			pp = pp->p_vpnext;
		}
	} while (pp != epp);

	/* remove epp and deallocate markers */

	REMOVE_MARKER_PAGE(epp, vp);
	VM_PAGEIDUNLOCK();
	DESTROY_2_MARKER_PAGES(mpp, epp);
}


/*
 * int
 * pvn_memresv(vnode_t *vp, off_t offset, mresvtyp_t mtype, uint_t flag)
 *
 *	Reserve locked real memory to hold a page with the
 *	specified <vp, offset> identity. mtype indicates which
 *	type of memory accounting to perform (passed to mem_resv).
 *
 * Calling/Exit State:
 *
 *	A reservation is made which guarantees that sufficient locked real
 *	memory can be made available to hold a page with the specified
 *	<vp, offset> identity.  Multiple pvn_memresv()'s for the same identity
 *	are folded into a single underlying locked real memory reservation.
 *	The reservation remains intact until the last pvn_memunresv() is
 *	done for this identity.
 *
 *	If (flag == SLEEP), the caller holds no locks since this function 
 *	may block.
 *
 *	This routine does not block to wait for a reservation. If (flag ==
 *	NOSLEEP) then it will not block for any other reason as well. If
 *	(flag == SLEEP), it may block for other reasons. If the reservation 
 *	cannot be immediately established, it returns failure, regardless
 *	of the value of flag. The request will also fail if the appropriate
 *	reservation counter would overflow by accepting it.
 *
 *	On success, returns non-zero.  On failure, returns zero.
 *
 * Remarks:
 *
 *	This interface and all associated ones are necessarily complicated
 *	by the need to track two different types of memory reservation:
 *	those done for kernel pages (e.g. softlocks) and those done for
 *	user pages (e.g. plock/memcntl requests). Additionally, when memory
 *	locking pages for which no M_SWAP has been previously done, a
 *	reservation for swap may be needed in addition to a reservatin for
 *	real memory. This is reflected in the mtype argument whose values
 *	may be M_REAL, M_REAL_USER, M_BOTH, or M_BOTH_USER. Since separate
 *	accounting paradigms in mem_resv/mem_unresv (see:
 *	kern:mem/memresv.c) limit the number of M_*_USER requests, we need
 *	to honor this accounting and not overcommit memory resources.
 *
 *	Commensurate with this goal, we must track the reservations using
 *	two separate counters: one for M_SWAP requests and one for M_REAL
 *	requests. A flag pml_realuser is used to track user level
 *	(M_REAL/M_SWAP) requests.
 *
 *	The M_REAL/M_REAL_USER counter (pml_rcount) represents M_REAL
 *	(kernel level reservation) if pml_realuser == 0 and M_REAL_KERNEL
 *	(user level reservation) if pml_realuser == 1. Similarly, the
 *	M_SWAP/M_SWAP_KERNEL counter (pml_scount) represents M_SWAP (kernel
 *	level reservation) if pml_realuser == 0 and M_SWAP_KERNEL (user
 *	level reservation) if pml_realuser == 1.
 * 
 *	NOTE that in the unreserving cases, if (typ == (M_BOTH || M_BOTH_USER))
 *	and pml_rcount transitions from 1->0, it is ASSERTable that 
 *	pml_scount must also transition from 1->0 since we do not process
 *	M_SWAP requests separately.
 *
 * 	Some limitation arise from this implementation which uses only
 *	a single bit (pml_realuser) to indicate if the resrvation is
 *	at user level:
 *
 *	(i) The swap reservation must be promoted to user level if the
 *	    real reservation is so promoted.
 *	
 *	(ii) If any one client of pvn_memresv promotes the reservation
 *	     to user mode, the reservation will stay promoted until
 *	     all clients have released their claims.
 */
int
pvn_memresv(vnode_t *vp, off_t offset, mresvtyp_t mtype, uint_t flag)
{
	pl_t oldpl;
	pvn_memresv_entry **p;

#if (NOSLEEP == 0)
#error pvn_memresv() code assumes NOSLEEP != 0
#endif

	ASSERT(mtype == M_REAL || mtype == M_REAL_USER || \
	       mtype == M_BOTH || mtype == M_BOTH_USER);
	ASSERT(flag == SLEEP || flag == NOSLEEP);
	ASSERT(flag == NOSLEEP || KS_HOLD0LOCKS());

	oldpl = PVN_MEMRESV_LOCK();

	/*
	 * Lookup/create an entry for this <vp, offset>.  (May block.)
	 */

	p = pvn_memresv_lookup(vp, offset, oldpl, flag);

	if (p == NULL) {
		ASSERT(flag == NOSLEEP);
        	PVN_MEMRESV_UNLOCK(oldpl);
		return 0;
	}

	/*
	 * If this it the first time we are reserving memory against this
	 * <vp, offset> identity, both counts will be zero and we need to get 
	 * a reservation based on mtype unconditionally.
	 */

	if ((*p)->pml_rcount == 0) {

		ASSERT((*p)->pml_scount == 0);
		ASSERT((*p)->pml_realuser == 0);

		if (!mem_resv(1, mtype)) {

			/*
		 	 * Couldn't reserve the memory - back out and return
		 	 * failure.
		 	 */

        		pvn_memresv_destroy(p);
			goto error_exit1;
		}

		(*p)->pml_rcount = 1;
		switch (mtype) {
		case M_REAL:
			break;
		case M_REAL_USER:
			(*p)->pml_realuser = 1;
			break;
		case M_BOTH:
			(*p)->pml_scount = 1;
			break;
		default /* M_BOTH_USER */:
			(*p)->pml_realuser = 1;
			(*p)->pml_scount = 1;
			break;
		}
	} else if (++(*p)->pml_rcount == 0) {
		goto error_exit2; /* overflow */
	} else {
		/*
		 * We already hold an M_REAL or M_REAL_USER reservation
		 * (M_REAL_USER iff pml_realuser == 1). If this is an
		 * M_REAL_USER or M_BOTH_USER request, and we hold an
		 * M_REAL reservation, we will need to promote it to
		 * M_REAL_USER level.
		 *
		 * If pml_scount > 0, then we already hold an M_SWAP_KERNEL
		 * or M_SWAP reservation (M_SWAP iff pml_realuser == 1).
		 * If this is an M_REAL_USER or M_BOTH_USER, and we already
		 * hold an M_SWAP_KERNEL reservation, then we will need to
		 * promote it to M_SWAP. If pml_scount == 0, and this is an
		 * M_BOTH_USER request (or an M_BOTH request but
		 * pml_realuser == 1), then we will need to obtain an M_SWAP
		 * reservation. If pml_scount == 0, this is an M_BOTH
		 * request, and pml_realuser == 0, then we will need to
		 * obtain an M_SWAP_KERNEL reservation.
		 */
		switch (mtype) {
		case M_REAL:
			break;
		case M_REAL_USER:
			if ((*p)->pml_realuser == 0) {
				if ((*p)->pml_scount == 0) {
					if (!MEM_RESV_PROMOTE(1, M_REAL_USER))
						goto error_exit2;
				} else {
					if (!MEM_RESV_PROMOTE(1, M_BOTH_USER))
						goto error_exit2;
				}
				(*p)->pml_realuser = 1;
			}
			break;
		case M_BOTH:
			if (++(*p)->pml_scount == 0)
				goto error_exit3; /* overflow */
			if ((*p)->pml_scount == 1) {
				if ((*p)->pml_realuser == 0) {
					if (!mem_resv(1, M_SWAP_KERNEL))
						goto error_exit3;
				} else {
					if (!mem_resv(1, M_SWAP))
						goto error_exit3;
				}
			}
			break;
		default /* M_BOTH_USER */:
			if (++(*p)->pml_scount == 0)
				goto error_exit3; /* overflow */
			if ((*p)->pml_realuser == 0) {
				if (!MEM_RESV_PROMOTE(1, M_REAL_USER))
					goto error_exit3;
				if ((*p)->pml_scount == 1 &&
				    !mem_resv(1, M_SWAP)) {
					MEM_RESV_DEMOTE(1);
					goto error_exit3;
				}
				(*p)->pml_realuser = 1;
			} else { /* (*p)->pml_realuser == 1 */
				if ((*p)->pml_scount == 1 &&
				    !mem_resv(1, M_SWAP))
					goto error_exit3;
			}
			break;
		}
	}

	PVN_MEMRESV_UNLOCK(oldpl);
	return(1);

error_exit3:
	--(*p)->pml_scount;
error_exit2:
	--(*p)->pml_rcount;
error_exit1:
	PVN_MEMRESV_UNLOCK(oldpl);
	return 0;
}

/*
 * void
 * pvn_memunresv(vnode_t *vp, off_t offset, mresvtyp_t mtype)
 *
 *	Release a locked real memory reservation made via pvn_memresv()
 *	[or pvn_cache_memresv()] for the specified <vp, offset> identity.
 *	mtype indicates which type of memory accounting to perform (passed 
 *	to mem_unresv).
 *
 * Calling/Exit State:
 *
 *	If this is the last outstanding pvn_memresv() for this identity,
 *	the underlying locked real memory reservation is removed and the
 *	corresponding pvn_memresv_entry is destroyed.
 */
void
pvn_memunresv(vnode_t *vp, off_t offset, mresvtyp_t mtype)
{
	pl_t oldpl;
	pvn_memresv_entry **p;

	ASSERT(mtype == M_REAL || mtype == M_REAL_USER || \
		mtype == M_BOTH || mtype == M_BOTH_USER);

	oldpl = PVN_MEMRESV_LOCK();

	/*
	 * Lookup an entry for this <vp, offset>.  It had better exist.
	 * (Thus this shouldn't block.)
	 */

	p = pvn_memresv_lookup(vp, offset, oldpl, NOSLEEP);
	ASSERT(p);
	ASSERT((*p)->pml_rcount); /* Not just created */

	/*
	 * Take care of requests containing M_SWAP components first.
	 */

	if (mtype == M_BOTH || mtype == M_BOTH_USER) {

		ASSERT((*p)->pml_scount);

		if (--(*p)->pml_scount == 0)
			mem_unresv(1, M_SWAP);
	}

	/*
	 * Now take care of M_REAL component of request. If this is
	 * the last rcount and the realuser bit is set means that we
	 * now return the M_REAL_USER reservation.
	 */

	if (--(*p)->pml_rcount == 0) {

		ASSERT((*p)->pml_scount == 0);
#ifndef DEBUG
		mem_unresv(1, M_REAL);
#else /* DEBUG */
		if ((*p)->pml_realuser)
			mem_unresv(1, M_REAL_USER);
		else
			mem_unresv(1, M_REAL);
#endif /* DEBUG */

		pvn_memresv_destroy(p);
	}

	PVN_MEMRESV_UNLOCK(oldpl);
}

/*
 * void
 * pvn_cache_memresv(vnode_t *vp, off_t offset, mresvtyp_t mtype)
 *
 *	Register a locked real memory reservation of mtype already made by 
 *	the caller on behalf of a page with the specified <vp, offset> identity.
 *
 * Calling/Exit State:
 *
 *	This is like pvn_memresv() but the caller has already done
 *	a `mem_resv(1, mtype)' and "passes" ownership of this
 *	reservation to us to manage for the specified <vp, offset>.
 *
 *	The caller guarantees that there is no current pvn memory
 *	reservation for the specified <vp, offset>.  I.e., there
 *	is no outstanding pvn_[cache_]memresv().
 *
 *	The caller holds no locks since this function may block.
 */
void
pvn_cache_memresv(vnode_t *vp, off_t offset, mresvtyp_t mtype)
{
	pl_t oldpl;
	pvn_memresv_entry **p;

	ASSERT(mtype == M_REAL || mtype == M_REAL_USER);
	ASSERT(KS_HOLD0LOCKS());

	oldpl = PVN_MEMRESV_LOCK();

	/*
	 * Create an entry for this <vp, offset>.  (May block.)
	 */

	p = pvn_memresv_lookup(vp, offset, oldpl, SLEEP);

	/* We just created this */

	ASSERT((*p)->pml_rcount == 0 && (*p)->pml_scount == 0);

	if (mtype == M_REAL_USER)
		(*p)->pml_realuser = 1;
	(*p)->pml_rcount = 1;

	PVN_MEMRESV_UNLOCK(oldpl);
}

/*
 * boolean_t
 * pvn_memresv_swap(uint_t swresv, void (*scan_func)(), void *arg)
 *	This routine attempts to return swresv M_SWAP reservations back to
 *	the caller.  It attempts to free an M_SWAP reservation from each
 *	page returned by scan_func(), effectively performing a
 *	pvn_memunresv(... M_SWAP...) on each page. Any additional reservations
 *	required, which are not actually freed by the pvn_memunresv(s)
 *	are garnered from mem_resv().
 *
 * Calling/Exit State:
 *	Arguments:
 *		swresv		Number of M_SWAP reservations requested by the
 *				caller.
 *		arg		An opaque cookie to be passed to scan_func().
 *		scan_func	A function to be repeatedly called. scan_func()
 *				function returns the page identities processed
 *				by this function.
 *
 *	Returns B_TRUE if swresv M_SWAP reservations were obtained, and
 *	B_FALSE otherwise. If B_TRUE is returned, then all pages returned by
 *	scan_func() list have given up a count towards an M_SWAP
 *	reservation. IF B_FALSE is returned, then nothing is changed.
 *
 *	This routine never blocks.
 *
 * Description:
 *	The calling sequence to scan_func is:
 *
 *		(*scan_func)(arg, &vp, &offset, &index)
 *
 *	The first time scan_func is called, index should have the value 0.
 *	scan_func returns the page identity in vp and offset, and returns
 *	a value in index to be passed into the next invocation of scan_func.
 *	When no more pages need to be scanned, scan_func returns an index
 *	value of -1.
 */
boolean_t
pvn_memresv_swap(uint_t swresv, void (*scan_func)(), void *arg)
{
	pl_t oldpl;
	vnode_t *vp;
	off_t offset;
	int index;
        pvn_memresv_entry **p;

	oldpl = PVN_MEMRESV_LOCK();

	/*
	 * First see how many M_SWAP reservations can be freed up by
	 * counting down the swap reservations for the pages currently
	 * locked.
	 */

	index = 0;
	for (;;) {
		(*scan_func)(arg, &vp, &offset, &index);
		if (index == -1)
			break;
		p = pvn_memresv_find_l(vp, offset);
		ASSERT(*p != NULL);
		ASSERT((*p)->pml_rcount != 0);
		ASSERT((*p)->pml_scount != 0);
		if (--(*p)->pml_scount == 0) {
			--swresv;
		}
	}

	/*
	 * Without dropping the pvn_memresv_lock, try to obtain any
	 * additional reservations requried from mem_resv().  If we
	 * can obtain them, we are done. Just drop the lock and return
	 * success.
	 */

	if (swresv == 0 || mem_resv(swresv, M_SWAP)) {
		PVN_MEMRESV_UNLOCK(oldpl);
		return (B_TRUE);
	}

	/*
	 * ERROR backout case:
	 *
	 * Since we are still holding the pvn_memresv_lock, we can put
	 * everything back to just the way it was.
	 */
	index = 0;
	for (;;) {
		(*scan_func)(arg, &vp, &offset, &index);
		if (index == -1)
			break;
		p = pvn_memresv_find_l(vp, offset);
		ASSERT(*p != NULL);
		ASSERT((*p)->pml_rcount != 0);
		++(*p)->pml_scount;
		ASSERT((*p)->pml_scount != 0);
	}

	/*
	 * unlock and return failure
	 */
	PVN_MEMRESV_UNLOCK(oldpl);
	return (B_FALSE);
}

/*
 * STATIC pvn_memresv_entry **
 * pvn_memresv_find_l(vnode_t *vp, off_t offset)
 *	
 *	Look to see if an entry corresponding to the passed <vp, offset>
 *	pair currently exists in the hash
 *
 * Calling/Exit State:
 *
 *	Caller holds pvn_memresv_lock to guarantee the integrity of    
 *	the cache.
 *
 *	If an entry corresponding to <vp, offset> exists, then a pointer to 
 *	it (in the form of the forward pointer, pml_next, of the previous 
 *	entry) is returned to the caller. 
 *
 * 	If no corresponding entry is found, NULL is returned. 
 */
STATIC pvn_memresv_entry **
pvn_memresv_find_l(vnode_t *vp, off_t offset)
{
        pvn_memresv_entry **p;

        ASSERT(LOCK_OWNED(&pvn_memresv_lock));
	
        for (p = &pvn_memresv_hash[PAGE_HASHFUNC(vp, offset)];
             *p != NULL;
             p = &(*p)->pml_next) {

		ASSERT((*p)->pml_rcount);

                if ((*p)->pml_vp == vp && (*p)->pml_offset == offset) {
                        return(p);     
                }
        }
	/*
	 * If we failed to find an existing entry, then pass back a pointer
	 * to the position in the list the new entry will have to occupy.
	 */
	
	ASSERT(*p == NULL);	
	return (p);
}


/*
 * STATIC pvn_memresv_entry **
 * pvn_memresv_lookup(vnode_t *vp, off_t offset, pl_t oldpl, uint_flag)
 *
 *	Find/create the pvn_memresv_entry for the <vp, offset>.
 *
 * Calling/Exit State:
 *
 *	Caller holds pvn_memresv_lock.  It is still locked on return;
 *	however it may be dropped to allocate the new entry if (flag ==
 *	SLEEP).  To support this possibility, the caller supplies the pl 
 *	value returned from the LOCK(&pvn_memresv_lock, ...) in `oldpl'.
 *
 *	Returns a pointer to a pointer to the found/created entry.
 *	The caller must update pml_rcount and pml_realuser as appropriate.
 *	If the caller wishes to delete the found entry, the returned value 
 *	can be passed to pvn_memresv_destroy(). (The returned value points 
 *	to the previous chain pointer.)
 *
 *	If (flag == NOSLEEP) and we fail to find or allocate an entry, then
 *	NULL is returned.
 */
STATIC pvn_memresv_entry **
pvn_memresv_lookup(vnode_t *vp, off_t offset, pl_t oldpl, uint_t flag)
{
	pvn_memresv_entry **p;
	pvn_memresv_entry *new = NULL;

	ASSERT(LOCK_OWNED(&pvn_memresv_lock));
	ASSERT(vp != NULL);

	/*
	 * See if an entry already exists for this <vp, offset>.
	 */
loop:
	
	p = pvn_memresv_find_l(vp, offset); 
	if (*p) {

		/*
		 * Found an already existing entry.
		 * Free any entry we've allocated, and
		 * return the found entry.
		 */

		if (new != NULL) {
			kmem_free(new, sizeof(pvn_memresv_entry));
		}
		return(p);	/* entry already exists */
	}

	/*
	 * Need to create a new entry for this <vp, offset>.
	 */

	new = (pvn_memresv_entry *)
		kmem_alloc(sizeof(pvn_memresv_entry), KM_NOSLEEP);

	if (new == NULL) {

		if (flag == NOSLEEP)
			return((pvn_memresv_entry **)NULL);

		/*
		 * Couldn't allocate a new entry without blocking.
		 * Drop pvn_memresv_lock and block to allocate a new entry.
		 * Then loop back to recheck if we lost a race with
		 * someone else trying to create this same <vp, offset>.
		 */
		PVN_MEMRESV_UNLOCK(oldpl);
		new = (pvn_memresv_entry *)
			kmem_alloc(sizeof(pvn_memresv_entry), KM_SLEEP);
		(void)PVN_MEMRESV_LOCK();
		goto loop;
	}

	new->pml_vp = vp;
	new->pml_offset = offset;
	new->pml_rcount = 0;
	new->pml_scount = 0;
	new->pml_realuser = 0;

	ASSERT(*p == NULL);
	new->pml_next = NULL;
	*p = new;		/* add to end of hash chain */

	return (p);
}


/*
 * STATIC void
 * pvn_memresv_destroy(pvn_memresv_entry **p)
 *
 *	Destroy the pvn_memresv_entry.
 *
 * Calling/Exit State:
 *
 *	Caller holds pvn_memresv_lock.  It is still locked on return.
 *
 *	The reference count on the entry (pml_rcount && pml_scount) must 
 *	be zero.
 */
STATIC void
pvn_memresv_destroy(pvn_memresv_entry **p)
{
	pvn_memresv_entry *tmp = *p;

	ASSERT(LOCK_OWNED(&pvn_memresv_lock));
	ASSERT((*p)->pml_rcount == 0 && (*p)->pml_scount == 0);

	*p = (*p)->pml_next;
	kmem_free(tmp, sizeof(pvn_memresv_entry));
}


/*
 * boolean_t
 * pvn_memresv_query(vnode_t *vp, off_t offset)
 *
 *	Return an indication if a memory reservation has been cached
 *	by pvn for the <vp, offset> pair.
 *
 * Calling/Exit State:
 *
 *	No special MP state is required for entry or is held on exit.
 *
 *	If pvn has a memory reservation cached for the passed <vp, offset>
 *	pair then B_TRUE is returned, otherwise B_FALSE is returned.
 *
 * Remarks:
 *
 *	The indication passed back is stale unless the caller prevents this 
 *	by external means.
 */
boolean_t
pvn_memresv_query(vnode_t *vp, off_t offset)
{
	pl_t oldpl;
	boolean_t ret;

	ASSERT(vp != NULL);

	oldpl = PVN_MEMRESV_LOCK();

	ret = (*(pvn_memresv_find_l(vp, offset))) != NULL ? B_TRUE : B_FALSE;

	PVN_MEMRESV_UNLOCK(oldpl); 

	return ret;
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * boolean_t print_pvn_memresv_hash(void)
 *	Print the contents of pvn_memresv_hash[]
 *
 * Calling/Exit State:
 *	None.
 */

void
print_pvn_memresv_hash(void)
{
	uint_t i, count;
	pvn_memresv_entry *p;
	uint_t max_chain = 0;

	debug_printf("pvn_memresh_hash[0..%d]:\n\n", page_hashsz - 1);

	for (i = 0; i < page_hashsz; i++) {

		p = pvn_memresv_hash[i];

		if (p == NULL)
			continue;

		debug_printf("pvn_memresv_hash[%d]:\n", i);
		if (debug_output_aborted())
			return;

		for (count = 0; p != NULL; p = p->pml_next, count++) {
			debug_printf("\tvp 0x%x off 0x%x rcount %d"
				      " scount %d realuser %d\n",
				     p->pml_vp, p->pml_offset, p->pml_rcount,
				     p->pml_scount, p->pml_realuser);
			if (debug_output_aborted())
				return;
		}
		if (count > max_chain)
			max_chain = count;
	}

	if (max_chain != 0)
		debug_printf("Max chain length = %d\n", max_chain);
	else
		debug_printf("\tNo entries\n");
}

#endif /* DEBUG || DEBUG_TOOLS */
