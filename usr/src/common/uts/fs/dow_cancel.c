/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_cancel.c	1.5"
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
 * dow_abort_range(vp, offset, len)
 *	Abort any dow structures that are within the specificed range and
 *	decrement the softholds on the vp appropriately.
 *
 * Calling/Exit State:
 *	The caller is expected to ensure that no races may occur whereby
 *	another agent may attempt to abort the same range. Caller does not
 *	hold the dow_mutex. The function may block, in general.
 */
void
dow_abort_range(vnode_t *vp, off_t offset, int len)
{
	dowid_t hb; /* hashbuckets */
	dowid_t i;
	vaddr_t	eoff = offset + len;

	DOW_MUTEX_LOCK();
again:
	for(hb = DOW_FIRST_HASHBUCKET; hb <= DOW_LAST_HASHBUCKET; hb++) {
		for(i = DOW_HASHNEXT(hb); i != hb ; i = DOW_HASHNEXT(i)) {
			if ( (DOW_IDENT1(i) == (long)vp) &&
			     (DOW_IDENT2(i) >= offset) && 
			     (DOW_IDENT2(i) < eoff) &&
			     (DOW_TYPE(i) == DOW_ISPAGE) ) {
				ASSERT((DOW_STATE(i) & DOW_GONE) == 0);
				DOW_CHEAP_HOLD(i);
				dow_abort_rele_l(i); /* returns w. dow_mutex */
				VN_SOFTRELE(vp);
				goto again;
			}
		}
	}
	DOW_MUTEX_UNLOCK();
}

/*
 * void
 * dow_abort_rele_l(dowid_t id)
 *	Abort the specified dow structure. Wait for any outstanding
 *	IO to complete, and then mark the structure as unmodified and 
 *	"gone". If there are no antecdents, then simulate the
 *	completion of IO on this dow; otherwise, wait until the io-done
 *	processing is performed on this dow after all its antecdents
 *	have been handled.
 *
 *	Also, the caller has acquired a hold on the dow; release it 
 *	simultaneously with the abort -- and free the dow structure if
 *	appropriate.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and returned held. The function will
 *	block in general.
 */

void
dow_abort_rele_l(dowid_t id)
{

start_over:

	ASSERT(DOW_HOLD(id) > 0);
	ASSERT((DOW_STATE(id) & DOW_GONE) == 0);
	/*
	 * dow_abort must be called only for those ids that are
	 * currently possessing identity. Before returning, this
	 * identity will be destroyed (the hold only preserves
	 * the dow structure, not its hash identity. It is assumed
	 * that the caller has followed some algorithm to ensure 
	 * that concurrent calls to dow_abort would not be made for
	 * the same dowid.
	 */
	CHECK_HASH(id);

	/*
	 * we must wait for any IO that has been started on the
	 * dow, to complete; this includes DOW_INTRANS and DOW_IOSETUP
	 * cases. 
	 *
	 * Otherwise, if IO is not ongoing or imminent, we have to
	 * wait until all antecedents complete before returning from
	 * this function. To ensure that this happens promptly, we
	 * prune the id, if it is not already pruned. 
	 */

	if (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) {

		if (DOW_STATE(id) & DOW_FLUSH) {
			DOW_MUTEX_UNLOCK();
			LBOLT_WAIT(PRIBUF);
		} else {
			dow_iowait_l(id);
		}
		DOW_MUTEX_LOCK();
		goto start_over;
	} 

	ASSERT((DOW_STATE(id) & (DOW_GONE|DOW_FLUSHINTRANS|DOW_IOSETUP)) == 0);

	/*
	 * flush demon inherits the responsibility of calling dow_iodone
	 * if this dow has antecedents. function abort should imply the absolute
	 * dismantling of the function setup itself (and should therefore be
	 * synchronous).
	 */

	if ((DOW_TYPE(id) & DOW_ISFUNC) && (DOW_STATE(id) & DOW_MODIFIED)) {
		if (DOW_ANT_CNT(id) > 0) {
			prune(id);
			dow_iowait_l(id);
			DOW_MUTEX_LOCK();
		}
		/*
		 * Can we be certain as to which flush chain this id could
		 * be on, based on its DOW_STATE? If so, may be nice to
		 * verify under DEBUG that DOW_DEBUG_FLUSHID matches it.
		 */
		dow_flush_remove(id);
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE|DOW_INTRANS);
		DOW_TIMESTAMP(id) = INT_MAX;
		dow_iodone(id);
		dow_rele_l(id);
		return;
	}
	
	if (DOW_ANT_CNT(id) > 0) {
		prune(id);
		/*
		 * before we unhash, we need to ensure that no-one stays
		 * blocked on the modlock!
		 */
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			dow_clear_modlock_l(id);
		}
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE); 
		/*
		 * Since we just cleared the MOD state, it is
		 * safe as well to reset the timestamp.
		 */
		DOW_TIMESTAMP(id) = INT_MAX;

		/*
		 * we are leaving upto dow_iodone the job of removing this id 
		 * from its hash chain so that dow_lookups cannot find it after
		 * that point. 
		 *
		 * on second though, it seems advantageous to do it right 
		 * now, since it is unclear why we need to guarantee that
		 * lookups can find this id after this point. 
		 *
		 * but, FOR NOW, let us stick with the policy of letting 
		 * dow_iodone perform the hash removal for aborted dowids. 
		 * This has two advantages: 
		 * (1) it will be possible to catch error(s) if a DOW_GONE id 
		 *     comes through dow_iodone more than once, 
		 * (2) much of the other code is written to the assumption that
		 *     dow_iodone will be responsible for unhashing (though 
		 *     that can change, when we want to follow the new policy).
		 */
		
		/*
		 * This is a special case: we cannot employ either 
		 * of dow_intrans_wait OR dow_iowait_l in order to
		 * wait for a signal from this id, since in this case
		 * we just want to wait until the upward filtering of
		 * IO completions finally reaches this dow. iowait is
		 * designed only to wait so long as the "current" modification
		 * has not been flushed out -- and here there is no such
		 * modification to flush.
		 */
		DOW_SV_WAIT(id);
		DOW_MUTEX_LOCK();
		CHECK_HASH_REMOVED(id);
		ASSERT(DOW_DEP_CNT(id) == 0);
		ASSERT(DOW_ANT_CNT(id) == 0);
	} else {

		/*
		 * Can we be certain as to which flush chain this id could
		 * be on, based on its DOW_STATE? If so, may be nice to
		 * verify under DEBUG that DOW_DEUG_FLUSHID matches it.
		 */
		dow_flush_remove(id);
		/*
		 * before we unhash, we need to ensure that no-one stays
		 * blocked on the modlock!
		 */
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			dow_clear_modlock_l(id);
		}
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE|DOW_INTRANS); 
		DOW_TIMESTAMP(id) = INT_MAX;
		dow_iodone(id);
		CHECK_HASH_REMOVED(id);
		ASSERT(DOW_DEP_CNT(id) == 0);
	}
	dow_rele_l(id);
	return;
}

/*
 * void
 * dow_abort(dowid_t id)
 *	Abort the specified dow structure. Wait for any outstanding
 *	IO to complete, and then mark the structure as unmodified and 
 *	"gone". If there are no antecdents, then simulate the
 *	completion of IO on this dow; otherwise, wait until the io-done
 *	processing is performed on this dow after all its antecdents
 *	have been handled.
 *
 * Calling/Exit State:
 *	dow_mutex not held by caller; acquired within, and released 
 *	before return. The function can block, so the caller must 
 *	not hold any locks.
 */

void
dow_abort(dowid_t id)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	if (!VALID_DOWID(id))
		return;

start_over:

	DOW_MUTEX_LOCK();
	ASSERT(DOW_HOLD(id) > 0);
	ASSERT((DOW_STATE(id) & DOW_GONE) == 0);
	/*
	 * dow_abort must be called only for those ids that are
	 * currently possessing identity. Before returning, this
	 * identity will be destroyed (the hold only preserves
	 * the dow structure, not its hash identity. It is assumed
	 * that the caller has followed some algorithm to ensure 
	 * that concurrent calls to dow_abort would not be made for
	 * the same dowid.
	 */
	CHECK_HASH(id);

	/*
	 * we must wait for any IO that has been started on the
	 * dow, to complete; this includes DOW_INTRANS and DOW_IOSETUP
	 * cases. 
	 */

	if (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) {

		if (DOW_STATE(id) & DOW_FLUSH) {
			DOW_MUTEX_UNLOCK();
			LBOLT_WAIT(PRIBUF);
		} else {
			dow_iowait_l(id);
		}

		goto start_over;
	}

	ASSERT((DOW_STATE(id) & (DOW_GONE|DOW_FLUSHINTRANS|DOW_IOSETUP)) == 0);

	/*
	 * flush demon inherits the responsibility of calling dow_iodone
	 * if this dow has antecedents. function abort should imply the absolute
	 * dismantling of the function setup itself (and should therefore be
	 * synchronous).
	 */

	if ((DOW_TYPE(id) & DOW_ISFUNC) && (DOW_STATE(id) & DOW_MODIFIED)) {
		if (DOW_ANT_CNT(id) > 0) {
			prune(id);
			dow_iowait_l(id);
			DOW_MUTEX_LOCK();
		}
		/*
		 * Can we be certain as to which flush chain this id could
		 * be on, based on its DOW_STATE? If so, may be nice to
		 * verify under DEBUG that DOW_DEBUG_FLUSHID matches it.
		 */
		dow_flush_remove(id);
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE|DOW_INTRANS);
		DOW_TIMESTAMP(id) = INT_MAX;
		dow_iodone(id);
		CHECK_HASH_REMOVED(id);
		DOW_MUTEX_UNLOCK();
		return;
	}
	
	if (DOW_ANT_CNT(id) > 0) {
		prune(id);
		/*
		 * before we unhash, we need to ensure that no-one stays
		 * blocked on the modlock!
		 */
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			dow_clear_modlock_l(id);
		}
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE); 
		/*
		 * This is a special case: we cannot employ either 
		 * of dow_intrans_wait OR dow_iowait_l in order to
		 * wait for a signal from this id, since in this case
		 * we just want to wait until the upward filtering of
		 * IO completions finally reaches this dow. iowait is
		 * designed only to wait so long as the "current" modification
		 * has not been flushed out -- and here there is no such
		 * modification to flush.
		 */
		DOW_SV_WAIT(id);
#ifdef	DEBUG
		DOW_MUTEX_LOCK();
		ASSERT(DOW_HOLD(id) > 0);
		ASSERT(DOW_DEP_CNT(id) == 0);
		ASSERT(DOW_ANT_CNT(id) == 0);
		DOW_MUTEX_UNLOCK();
#endif
		return;
	} else {

		/*
		 * Can we be certain as to which flush chain this id could
		 * be on, based on its DOW_STATE? If so, may be nice to
		 * verify under DEBUG that DOW_DEUG_FLUSHID matches it.
		 */

		dow_flush_remove(id);
		/*
		 * before we unhash, we need to ensure that no-one stays
		 * blocked on the modlock!
		 */
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			dow_clear_modlock_l(id);
		}
		dow_remhash(id);
		DOW_STATE(id) = (DOW_GONE|DOW_PRUNE|DOW_INTRANS); 
		DOW_TIMESTAMP(id) = INT_MAX;
		dow_iodone(id);
		ASSERT(DOW_DEP_CNT(id) == 0);
		DOW_MUTEX_UNLOCK();
		return;
	}
}

/*
 * void
 * dow_clearmod(dowid_t id)
 *	cancel a previously issued modification. wait for the intrans
 *	state to be cleared, if a prune/flush/write operation is 
 *	currently underway.
 *
 * Calling/Exit State:
 *	dow_mutex not held by the caller. The mutex is acquired and
 *	released within. The function can block.
 */

void
dow_clearmod(dowid_t id)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	if (!VALID_DOWID(id))
		return;

start_over:

	DOW_MUTEX_LOCK();

	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_HOLD(id) > 0);

	if (DOW_STATE(id) & DOW_GONE) {
		DOW_MUTEX_UNLOCK();
		return;
	}

	if (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) {
		if (DOW_STATE(id) & DOW_FLUSH) {
			DOW_MUTEX_UNLOCK();
			LBOLT_WAIT(PRIBUF);
		} else {
			dow_iowait_l(id);
		}
		goto start_over;
	
	} 
	ASSERT((DOW_STATE(id) & DOW_GONE) == 0);
	/*
	 * handling function dows:
	 *
	 * what to do with function dows? nothing special needed, 
	 * because:
	 *
	 * 	IF antecdent count > 0:
	 *
	 *	dow_process_leaf_func will handle a function dow
	 * that does not have its MODIFIED state set, in one of two
	 * ways depending in whether hold is 0 or not; if hold count
	 * is 0, then the dow will be freed. Otherwise it will be
	 * queued onto the aged list so that whenever the last rele
	 * occurs, dow_rele will know to free the dow. (dow_rele
	 * handles function dows differently in this way).
	 *
	 * 	ELSE, we will call dow_iodone here itself?
	 */
	DOW_STATE(id) &= ~DOW_MODIFIED;
	if (DOW_ANT_CNT(id) == 0) {
		if ((DOW_DEP_CNT(id) > 0)  ||
		    ((DOW_TYPE(id) & DOW_ISFUNC) == 0)) {
			/*
			 * Can we be certain as to which flush chain 
			 * this id could be on, based on its DOW_STATE? If 
			 * so, may be nice to verify under DEBUG that 
			 * DOW_DEUG_FLUSHID matches it.
			 *	DOW_AGED_FLUSHHEAD, perhaps?
			 */
			dow_flush_remove(id);
			DOW_STATE(id) |= DOW_INTRANS;
			DOW_TIMESTAMP(id) = INT_MAX;
			ASSERT((DOW_STATE(id) & (DOW_FLUSH|DOW_MODIFIED|
				DOW_IOSETUP)) == 0);
			dow_iodone(id);
		} 
	}
	DOW_MUTEX_UNLOCK();
	return;
}
