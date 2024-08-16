/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_io.c	1.5"
#ident	"$Header: $"

#include	<fs/dow.h>
#include	<fs/fs_hier.h>
#include	<io/conf.h>
#include	<mem/kmem.h>
#include	<mem/page.h>
#include	<svc/clock.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/ghier.h>
#include	<util/sysmacros_f.h>
#include	<util/var.h>

/*
 * void
 * dow_strategy_buf(buf_t *bp)
 *
 *	Perform dow setup for a write/flush request. This is done
 *	if a dow structure exists; else, the passed in device strategy 
 *	function is called directly. If a dow structure exists and
 *	has antecedents, the the passed in device strategy routine
 *	is stored in the buffer for deferred execution.
 *
 * Calling/Exit State:
 *	None. Acquires and releases dow_mutex within.
 */
void
dow_strategy_buf(buf_t *bp) 
{
	dowid_t id;

	DOW_MUTEX_LOCK();
	id = dow_lookup((bp->b_edev), (bp->b_blkno));
	if (!VALID_DOWID(id)) { 
		DOW_MUTEX_UNLOCK();
		bp->b_writestrat = NULL;
		(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
		return;
	}

	ASSERT(DOW_TYPE(id) == DOW_ISBUFFER);
	ASSERT((DOW_STATE(id) & (DOW_GONE|DOW_INTRANS|DOW_IOSETUP)) == 0);
	ASSERT(DOW_HOLD(id) > 0);

	if (DOW_ANT_CNT(id) == 0) {	 
		if ((DOW_STATE(id) & DOW_FLUSH) == 0) {
			/*
			 * if this write originated externally, now is the
			 * time to deque id from its flush chain.
			 */
#ifdef	DEBUG
			if (DOW_STATE(id) & DOW_PRUNE) {
			ASSERT(DOW_DEBUG_FLUSHID(id) == DOW_LEAF_FLUSHHEAD);
			} else {
			ASSERT(DOW_DEBUG_FLUSHID(id) == DOW_AGED_FLUSHHEAD);
			}
#endif
			dow_flush_remove(id);
		}
		CHECK_NOTON_ANY_FLUSHCHAIN(id);
		DOW_STATE(id) = DOW_INTRANS;
		dow_iodone_insert(id, bp, dow_buf_iodone);
		DOW_TIMESTAMP(id) = INT_MAX;
		DOW_MUTEX_UNLOCK();
		(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
		return;
	} 

	ASSERT((DOW_STATE(id) & DOW_FLUSH) == 0);
	if ((DOW_STATE(id) & DOW_PRUNE) == 0) {
		CHECK_ON_FLUSHCHAIN(id, DOW_AGED_FLUSHHEAD);
		ASSERT((DOW_STATE(id) & DOW_FLUSH) == 0);
		prune(id);
	}
	CHECK_NOTON_FLUSHCHAIN(id, DOW_AGED_FLUSHHEAD);

	dow_iodone_insert(id, bp, dow_buf_iodone);
	DOW_STATE(id) = (DOW_IOSETUP | DOW_MODIFIED | DOW_PRUNE);
	DOW_TIMESTAMP(id) = (lbolt - FLUSHDELTAMAX);
	/*
	 * since there are antecedents, we could not have come here
	 * from dow_flush. There is not much else to do; we have 
	 * already pruned the id, so we can be outta here.
	 *
	 * in a synchronous bwrite call, the caller does not do a 
	 * dow_intrans_wait; rather, it will do a biowait() after
	 * calling this strategy function. Control will return to
	 * the caller, in that case, after the dow_iodone processing
	 * has been completed.
	 */
	DOW_BP(id) = bp;
	DOW_MUTEX_UNLOCK();
	return;
}

/*
 * THIS VERSION DOES NOT HANDLE blocksize < pagesize. See file
 * multi_bp_pageio for how this will be handled. 
 *
 * ASSUMPTIONS: 	
 *	blocksize >= pagesize
 *	page clustering disabled selectively for DOW uses.
 */

/*
 * void
 * dow_strategy_page(buf_t **bpp, vnode_t *vp, off_t off, int len)
 *
 *	Perform dow setup for a page write/flush request. If a dow
 *	structure exists, and has antecedents, then the dow is marked
 *	as "IOSETUP" -- i.e., ready for a deferred call to the device
 *	strategy routine. If the dow structure has no antecedents,
 *	then the IO is submitted immediately.
 *
 *	If a dow structure corresponding to the page does not exist,
 *	then a dow structure is created. This allows the tracking of
 *	intransit pages for the DOW system, so that if a dow_create()
 *	were to occur on an in-transit page then the dow would correctly
 *	contain the "INTRANS" state. These actions are specific to
 *	page dows since a page-flush request can originate without
 *	any resource locking (unlike a buffer flush request).
 *
 * 	Finally, because multiple page write requests can be in
 *	progress at the same time (eventhough only one call to the 
 *	device strategy routine can result at a time), the dow strategy
 *	routine acquires the DOW_MOD_LOCK (pseudo rwsleep lock) in
 *	the write mode in order to clear the DOW_MODIFIED state.
 *	By clearing the MODIFIED state under the cover of the DOW_MOD_LOCK,
 *	it is assured that the MODIFIED state is not set unless the
 *	page itself is MODIFIED.
 *
 *	If the dow structure does not exist and cannot be allocated, then
 *	the write is performed immediately, but all dow_alloc's for
 *	pages are disallowed util the write completes. This is to
 *	prevent the creation of page dows with inconsistent INTRANS
 *	information.
 *
 * Calling/Exit State:
 *	Caller does not hold dow mutex. Furthermore, the caller is at
 *	basepl (though this assumption is only for convenience).
 */
void
dow_strategy_page(buf_t **bpp, vnode_t *vp, off_t off, int len)
{
	dowid_t id;
	ASSERT((bpp != NULL) && ((*bpp) != NULL));
	
	DOW_MUTEX_LOCK();
	/*
	 * assume the caller has ensured that dow_strategy is
	 * called only for the appropriate type of pages (i.e.,
	 * directory pages, for example.)
	 */
	
	id = dow_create_page_l(vp, off, len,  DOW_NO_WAIT); 
	if (!VALID_DOWID(id)) {
		dow_io_utp_setup(*bpp);
		(++untracked_pageio);
		DOW_MUTEX_UNLOCK();
		(*bdevsw[getmajor((*bpp)->b_edev)].d_strategy)(*bpp);
		return;
	} 
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	/*
	 * The following assert is true under the assumption that the file
	 * system (or dow client) serializes calls to dow_strategy for the
	 * same page such that only one call can be active through upto the
	 * point that biodone processing is done.
	 */
	ASSERT((DOW_STATE(id) & (DOW_INTRANS|DOW_IOSETUP|DOW_GONE)) == 0);
	ASSERT(DOW_HOLD(id) > 0);
	dow_iodone_insert(id, *bpp, dow_page_iodone);
	if (DOW_ANT_CNT(id) > 0) {
		if ((DOW_STATE(id) & DOW_PRUNE) == 0) {
			CHECK_ON_FLUSHCHAIN(id, DOW_AGED_FLUSHHEAD);
			ASSERT((DOW_STATE(id) & (DOW_FLUSH|DOW_IOSETUP|
					DOW_GONE)) == 0);
			prune(id);
		}
		CHECK_NOTON_FLUSHCHAIN(id, DOW_AGED_FLUSHHEAD);
		/* 
		 * We have antecdents. We could not have come here from the
		 * flush daemon. 
		 */
		ASSERT((DOW_STATE(id) & DOW_FLUSH) == 0);
		DOW_STATE(id) |= (DOW_IOSETUP|DOW_PRUNE);
		DOW_BP(id) = (*bpp);
		DOW_MUTEX_UNLOCK();
		/*
		 * In a synchronous VOP_PUTPAGE, the caller will do 
		 * the biowait on each of the buffers that comprise 
		 * the page. Therefore, by the time that the last 
		 * biowait completes, dow_iodone will get called for
		 * the dowid. So we don't need to do a 
		 * dow_intrans_wait in here. 
		 */
		return;
	}
	if ((DOW_STATE(id) & DOW_FLUSH) == 0) {
		/*
		 * if this write originated externally, now is the
		 * time to deque id from its flush chain.
		 */
		ASSERT(((DOW_STATE(id) & DOW_PRUNE) == 0) ||
			       (DOW_DEBUG_FLUSHID(id) == DOW_LEAF_FLUSHHEAD));
		ASSERT((DOW_STATE(id) & DOW_PRUNE) ||
			       (DOW_DEBUG_FLUSHID(id) == DOW_AGED_FLUSHHEAD));
		dow_flush_remove(id);
	}
	if (dow_startmod_trywrlock_l(id)) {
		DOW_STATE(id) = DOW_INTRANS;
		dow_drop_modlock_l(id);
		DOW_TIMESTAMP(id) = INT_MAX;
		DOW_MUTEX_UNLOCK();
		(*bdevsw[getmajor((*bpp)->b_edev)].d_strategy)(*bpp);
		return;
	}
	if (pagesync_modlock_dowid == id) {
		DOW_STATE(id) = DOW_INTRANS;
		dow_drop_modlock_l(id);
		DOW_TIMESTAMP(id) = INT_MAX;
		pagesync_modlock_dowid = DOW_BADID;
		DOW_MUTEX_UNLOCK();
		(*bdevsw[getmajor((*bpp)->b_edev)].d_strategy)(*bpp);
		return;
	}
	if ((*bpp)->b_flags & B_ASYNC) {
		/* declare this dow as iosetup, return */
		CHECK_NOTON_ANY_FLUSHCHAIN(id);
		DOW_STATE(id) |= (DOW_PRUNE|DOW_IOSETUP);
		DOW_STATE(id) &= ~DOW_FLUSH;
		dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
		DOW_TIMESTAMP(id) = 0;
		DOW_BP(id) = (*bpp);
		DOW_MUTEX_UNLOCK();
		return;
	}
	dow_startmod_wrlock_l(id);
	/*
	 * on return from dow_startmod_wrlock_l, the dow could 
	 * have been aborted.
	 */
	if (DOW_STATE(id) & DOW_GONE) {
		DOW_STATE(id) = (DOW_GONE | DOW_PRUNE | DOW_INTRANS);
	} else {
		DOW_STATE(id) = DOW_INTRANS;
		dow_drop_modlock_l(id);
	}
	DOW_TIMESTAMP(id) = INT_MAX;
	DOW_MUTEX_UNLOCK();
	(*bdevsw[getmajor((*bpp)->b_edev)].d_strategy)(*bpp);
	return;
}

/*
 * void
 * dow_buf_iodone(buf_t *bp)
 *	Perform the chained iodone action for a dow on completion of a 
 *	buffer write.
 *
 * Calling/Exit State:
 *	Called at interrupt level from biodone. Acquires and releases
 *	dow_mutex.
 */

void
dow_buf_iodone(buf_t *bp)
{
	dowid_t id;
	pl_t savepl;
	savepl = DOW_MUTEX_LOCK();
	id = DOW_P_TO_ID((bp->b_misc));
	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_HOLD(id) > 0);
	ASSERT(DOW_DEV(id) == bp->b_edev);
	ASSERT(DOW_BLKNO(id) == bp->b_blkno);
	ASSERT(DOW_TYPE(id) == DOW_ISBUFFER);
	CHECK_NOTON_ANY_FLUSHCHAIN(id);
	DOW_CHEAP_RELE(id);
	dow_iodone_restore(id, bp);
	dow_iodone(id);
	DOW_MUTEX_UNLOCK_SAVEDPL(savepl);
	biodone(bp);
}

/*
 * void
 * dow_page_iodone(buf_t *bp)
 *	Perform the chained iodone action for a dow on completion of a 
 *	page write.
 *
 * Calling/Exit State:
 *	Called at interrupt level from biodone. Acquires and releases
 *	dow_mutex.
 */

void
dow_page_iodone(buf_t *bp)
{
	dowid_t id;
	pl_t savepl;

	savepl = DOW_MUTEX_LOCK();
	id = DOW_P_TO_ID((bp->b_misc));
	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_HOLD(id) > 0);
	ASSERT(DOW_VP(id) == bp->b_pages->p_vnode);
	ASSERT(((bp->b_flags & B_PAGEIO) &&
			(DOW_OFFSET(id) == ((off_t)(bp->b_pages->p_offset) + 
			(off_t)(bp->b_un.b_addr))))
		|| ((DOW_OFFSET(id) == ((off_t)(bp->b_pages->p_offset) + 
			(((unsigned long)(bp->b_un.b_addr)) % PAGESIZE)))));
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);	
	CHECK_NOTON_ANY_FLUSHCHAIN(id);
	DOW_CHEAP_RELE(id);	
	dow_iodone_restore(id, bp);
	dow_iodone(id);
	DOW_MUTEX_UNLOCK_SAVEDPL(savepl);
	biodone(bp);
}

/*
 * void
 * dow_untracked_pageio_iodone(buf_t *bp)
 *	Perform the chained iodone action for a dow on completion of a 
 *	page write, for which no dow was created (because of dow resource
 *	exhaustion).
 *
 * Calling/Exit State:
 *	Called at interrupt level from biodone. Acquires and releases
 *	dow_mutex.
 */

void
dow_untracked_pageio_done(buf_t *bp)
{
	pl_t savepl;
	/*
	 * handle untracked page io protocol.
	 *
	 * pass on to other iochain processing.
	 */
	savepl = DOW_MUTEX_LOCK();
	(--untracked_pageio);
	dow_io_utp_rele(bp);
	DOW_MUTEX_UNLOCK_SAVEDPL(savepl);
	biodone(bp);
}

/*
 * void
 * dow_iodone(dowid_t id)
 *	On write IO completion, (a) issue wakeups to LWPs that are in
 *	dow_intrans_wait, (b) break dependencies and (c) execute actions
 *	that can be, but need not be, deferred to the dow_flush_demon, 
 *	for new leaf level dows that are created as a result of (b).
 *
 * Calling/Exit State:
 *	dow_mutex held by caller. caller ensures that the dowid is held
 *	if necessary. This function is called at interrupt level, and cannot
 *	block.
 */
void
dow_iodone(dowid_t id) 
{
	dowlinkid_t	headlink, link;
	dowid_t	dep;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	ASSERT(EMPTY_IODONE_LINKAGE(id));
	DOW_STATE(id) &= ~DOW_INTRANS;
	ASSERT((DOW_STATE(id) & (DOW_FLUSHINTRANS|DOW_IOSETUP)) == 0);
	CHECK_NOTON_ANY_FLUSHCHAIN(id);

	headlink = DOWLINK_DEP_LINKHEAD(id);
	CHECK_DOWLINK_CHAIN(headlink);
	while (0 < DOW_DEP_CNT(id)) {
		link = DOWLINK_NEXT(headlink);
		ASSERT(headlink != link);
		dep = DOWLINK_DOWID(link);
		CHECK_DOWLINK_CHAIN(DOWLINK_ANT_LINKHEAD(dep));
		ASSERT(DOW_ANT_CNT(dep) > 0);
		ASSERT(id == DOWLINK_DOWID(DOWLINK_INVERSE(link)));
		dowlink_breaklink(link, dep, id);
		if (DOW_ANT_CNT(dep) > 0) {
			continue;
		}
		/*
		 * the dependent itself is a leaf level dow. proceed to
		 * handle it if it can be processed right away, OR, if
		 * cannot be processed queue it up on the proper
		 * flush chain.
		 */
		dow_flush_remove(dep);
		dow_process_leaf_iodone(dep);

	} /* while */

	/* 
	 * we have handled all the dependent links. next we need to:
	 * 
	 *	- if (hold --> 0) free the dow structure
	 *	- otherwise:
	 *		- signal blocked waiters
	 *		- if PRUNE'd and MODIFIED, reinsert on the leaf
	 *		  flush chain
	 *		- else if GONE, then turn off MODIFY, unhash and
	 *		  put away on the PRUNE list (no one looks at it)
	 *		- else turn off all states except MODIFY, and 
	 *		  insert on the tail of the aged flush chain.
	 */
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_DEP_CNT(id) == 0);
	ASSERT(EMPTY_DOWLINK_LIST(headlink));
	if (DOW_STATE(id) & DOW_GONE) {
		ASSERT(DOW_STATE(id) == (DOW_PRUNE | DOW_GONE));
		CHECK_HASH_REMOVED(id);
		if (DOW_HOLD(id) > 0) {
			if (DOW_SV_BLKD(id)) {
				/* wakeup blocked waiters */
				DOW_SV_BROADCAST(id); 
			}
			dow_flush_tailins(id, DOW_PRUN_FLUSHHEAD);
			CHECK_ANTECDENTS_PRUNED(id);
			return;
		} else {
			ASSERT(!DOW_SV_BLKD(id));
			dow_free(id);
			return;
		}
	} 
	CHECK_HASH(id);
	if (DOW_HOLD(id) == 0) {
		/* there should be no blocked waiters. */
		ASSERT(!DOW_SV_BLKD(id));
		dow_remhash(id);
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			VN_SOFTRELE(DOW_VP(id));
		}
		dow_free(id);
		return;
	}
	if (DOW_STATE(id) & DOW_PRUNE) {
		if (DOW_STATE(id) & DOW_MODIFIED) {
			dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
		} else {
			/* 
			 * someone turned on the PRUNE bit while last
			 * IO was in progress; no MOD state was deposited,
			 * however. Furthermore, this dow has not been
			 * aborted since DOW_GONE was checked earlier.
			 * just turn it back off and send this id to 
			 * aged list. Since we got rid of all of its
			 * dependents, it is okay to turn off the PRUNE
			 * bit since the protocol that "entire subtree
			 * under a pruned dow is maintained pruned" is
			 * not violated.
			 */
			DOW_STATE(id) &= ~DOW_PRUNE;
			dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);
		}
	} else {
		DOW_STATE(id) &= ~DOW_PRUNE;
		/* leave existing timestamp alone */
		dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);
	}

	if (DOW_SV_BLKD(id)) {
		/* wakeup blocked waiters */
		DOW_SV_BROADCAST(id); 
	}
	return;
}

/*
 * void
 * dow_iodone_insert(dowid_t id, buf_t *bp, void (*iodonefunc)())
 *	Insert the function "iodonefunc" on the iodone chain
 *	for the buffer indicated by bp.
 *
 * Calling/Exit State:
 *	Dow_mutex is held by the caller. 
 */
void
dow_iodone_insert(dowid_t id, buf_t *bp, void (*iodonefunc)())
{
	ASSERT(EMPTY_IODONE_LINKAGE(id));
	DOW_B_IODONE(id) = (void (*)())(bp->b_iodone);
	DOW_B_IOCHAIN(id) = (void *)(bp->b_misc);
	bp->b_iodone = iodonefunc;
	bp->b_misc = DOW_ID_TO_P(id);
}

/*
 * void
 * dow_iodone_restore(dowid_t id, buf_t *bp)
 *	Restore the iodone chain in the buffer bp after unlinking
 *	the dow_iodone action.
 *
 * Calling/Exit State:
 *	Dow_mutex is held by the caller who is at interrupt level.
 */
void
dow_iodone_restore(dowid_t id, buf_t *bp)
{
	ASSERT((!EMPTY_IODONE_LINKAGE(id)));
	bp->b_iodone = DOW_B_IODONE(id);
	bp->b_misc = DOW_B_IOCHAIN(id);
	DOW_DEBUG_IODONE_INIT(id);
}

/*
 * buf_t *
 * dow_buffer(dowid_t id)
 *	Return the buffer associated with the indicated dow.
 *
 * Calling/Exit State:
 *	Called under dow_mutex cover.
 */

buf_t *
dow_buffer(dowid_t id)
{
	ASSERT(DOW_STATE(id) & DOW_IOSETUP);
	return (DOW_BP(id));
}

/*
 * void
 * dow_bdwrite(buf_t *bp)
 * 	Perform a bdwrite in conjunction with setting up the
 *	dow write strategy indirection for the a future
 *	bwrite/bawrite call.
 *
 * Calling/Exit State:
 *	Called under dow_mutex cover.
 */
void
dow_bdwrite(buf_t *bp)
{
	bp->b_writestrat = dow_strategy_buf;
	bdwrite(bp);
}

