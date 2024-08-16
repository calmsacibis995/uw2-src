/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_leaf.c	1.2"
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
 * dow_process_leaf_func(dowid_t leafid, void (*called_from)(dowid_t))
 *
 *	Process a leaf level dow that represents a function. Called either
 *	as a result of the last antecdent going away (during iodone 
 *	processing of the last antecdent) or from flush demon or handle
 *	routines that want to execute the leaf level dow function. 
 *
 *	This function is written with the knowledge that (a) a function dow
 *	has no dependents and (b) that it should not be pushed to the aged
 *	list, since functions are to be executed in a timely manner. 
 *
 * Calling/Exit State:
 *	dow_mutex held on entry. May need to be dropped if the function is
 *	a blocking function.
 *
 *	If called_from == dow_iodone, then the dow_mutex is not dropped, 
 *	and if the function can be execcuted without blocking then it is
 *	executed. dow_mutex is held on return.
 *
 *	Otherwise, the function will be executed (subject to its MOD state
 *	being set). The dow_mutex will be reacquired before returning.
 *
 *	Caller is expected to have removed the leafid from any flush chain 
 * 	that it may have been on.
 */

void
dow_process_leaf_func(dowid_t leafid, void (*called_from)(dowid_t))
{
	void (*funcp)();
	void *argp;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(leafid));	
	ASSERT(DOW_DEP_CNT(leafid) == 0);
	ASSERT(DOW_TYPE(leafid) & DOW_ISFUNC);
	/*
	 * we should either never be aborting a function, or be doing the
	 * abort synchronously since functions can themselves have no
	 * dependents. In that case, no one should ever get here with an
	 * dowid that is marked DOW_GONE.
	 */
	ASSERT((DOW_STATE(leafid) & (DOW_GONE|DOW_INTRANS)) == 0);
	CHECK_NOTON_ANY_FLUSHCHAIN(leafid);
	CHECK_HASH(leafid);

	if (DOW_STATE(leafid) & DOW_MODIFIED) {
		if (DOW_TYPE(leafid) == DOW_ISFUNC_NBLK) {

			funcp = DOW_FUNC(leafid);
			argp = DOW_ARGP(leafid);
			(*funcp)(argp);

			goto done_execute;
		}
		if (called_from == dow_iodone) {
			/*
			 * can't handle this dow, since we can't block
			 * and the function is a potentially blocking one.
			 * let dow_flush_demon handle it.
			 */
			ASSERT((DOW_STATE(leafid) & DOW_FLUSH) == 0);
			DOW_STATE(leafid) |= DOW_PRUNE;
			dow_flush_headins(leafid, DOW_LEAF_FLUSHHEAD);
			return ;
		}
		/* 
		 * we will need to drop the mutex before we can execute the
		 * function. in preparation, we set the "DOW_INTRANS" state
		 */
		DOW_STATE(leafid) = DOW_INTRANS;
		funcp = DOW_FUNC(leafid);
		argp = DOW_ARGP(leafid);
		DOW_MUTEX_UNLOCK();
		(*funcp)(argp);
		DOW_MUTEX_LOCK();
		
		goto done_execute;
	} 

	/* 
	 * else: DOW_MODIFIED == 0. we have these alternatives:
	 *
	 * 1. treat it as a "clear-modified/aborted" case, whereby we just
	 *    proceed to handle it as a dow whose execution has already
	 *    happened.
	 *
	 * 2. a problem with (1) is that it requires that in the usual case
	 *    (no abort/clear-mod), we must never have a function dow that is
	 *    marked as unmodified -- i.e., dow_create_func must perform
	 *    dow_setmod. but that is unacceptable, since the client expects
	 *    to control the firing of this function in relation to some other
	 *    setup work being completed (i.e., only after dow_order has 
	 *    completed). but we cannot push the function dow to the aged list
	 *    either, since then we don't have a guarantee about who will do
	 *    the final release of the dow (i.e., not being able to distinguish
	 *    between whether a clear-mod was called or whether a set-mod was
	 *    _not_ called.)
	 *
	 * 3. owing to complications (1) and (2), we handle such dows just a
	 *    little differently as follows: we assume that for a function
	 *    dow, if it is NOT MODIFIED and NOT HELD, then it can be treated
	 *    as having been clear-modified; else we should treat it as a
	 *    dow on which a set-modify may yet happen. In this case, when
	 *    the final dow_rele does come through and it finds the function
	 *    dow in a clear-mod state, it just gets rid of it (which is 
	 *    appropriate, for it would do the same for any dow that has no
	 *    dependents).
	 *
	 *    Thus we choose alternative (3). See comment in dow_rele_l() for
	 *    function dows.
	 */
	if (DOW_HOLD(leafid) ==  0) {
		DOW_STATE(leafid) = 0;
		goto done_execute;
	}

	DOW_STATE(leafid) &= ~DOW_PRUNE;
	DOW_TIMESTAMP(leafid) = INT_MAX; /* unmodified, so doesn't matter */
	dow_flush_tailins(leafid, DOW_AGED_FLUSHHEAD);
	return;

done_execute: 
	/* 
	 * basically, this is specialized iodone processing for functions.
	 */
	ASSERT(DOW_MUTEX_OWNED());
	dow_remhash(leafid);
	DOW_STATE(leafid) = (DOW_GONE|DOW_PRUNE);
	if (DOW_HOLD(leafid) > 0) {
		if (DOW_SV_BLKD(leafid)) {
			DOW_SV_BROADCAST(leafid);
		}
		/*
		 * let dow_rele complete the job of freeing.
		 */
		dow_flush_headins(leafid, DOW_PRUN_FLUSHHEAD);
	} else {
		ASSERT(!DOW_SV_BLKD(leafid));
		dow_free(leafid);
	}
	return;
}

/*
 * void
 * dow_process_leaf_iodone(dowid_t id)
 *
 * 	Called from dow_iodone to process each new leaf dow that 
 *	results from breaking dependence links.
 * 	Caller is expected to have removed the leafid from any 
 *	flush chain that it may have been on.
 *
 * Calling/Exit State:
 * 	Called with dow_mutex held, returns with same held, never 
 *	dropping it.
 */
void
dow_process_leaf_iodone(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT((DOW_STATE(id) & DOW_FLUSHINTRANS) == 0);
	CHECK_NOTON_ANY_FLUSHCHAIN(id);
	/* Handover function dows to dow_process_leaf_func to do. */
	if (DOW_TYPE(id) & DOW_ISFUNC) {
		dow_process_leaf_func(id, &dow_iodone);
		ASSERT(DOW_MUTEX_OWNED());
		return;
	}

	if ((DOW_STATE(id) & DOW_PRUNE) == 0) {
		ASSERT((DOW_STATE(id) & (DOW_GONE|DOW_FLUSH|DOW_IOSETUP)) == 0);
		if ((DOW_HOLD(id) == 0) && (DOW_DEP_CNT(id) == 0)) {
			dow_remhash(id); /* ! DOW_GONE, but to be freed */
			if (DOW_TYPE(id) == DOW_ISPAGE) {
				VN_SOFTRELE(DOW_VP(id));
			}
			dow_free(id);
			return;
		}
		/*
		 * We need to ensure that we should not be here and have
		 * waiters blocked on IO completion for id! (Though this
		 * is tolerable, it is not good, particularly if the id
		 * is unmodified). With modified ids, we expect that anyone
		 * that waits does so after pruning the id. leave the
		 * existing timestamp on the id unchanged.
		 */
		dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);
		return;
	}	
	/* else: pruned */
	if (DOW_STATE(id) & (DOW_IOSETUP|DOW_MODIFIED)) {
		if (DOW_STATE(id) & DOW_IOSETUP) {
			dow_flush_headins(id, DOW_LEAF_FLUSHHEAD);
		} else {
			dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
		}
		return;
	}
	/*
	 * PRUNE'd, but !MOD, !INTRANS, !IOSETUP, !FLUSH. May/Maynot be GONE.
	 *
	 *		IF GONE:
	 *
	 * we want to do what dow_iodone might normally do for a GONE or
	 * clear-modified dow, without calling it recursively from here. 
	 * so we do the following:
	 *
	 *	HOLD == 0, DEP_CNT == 0 : unhash and free
	 *	HOLD != 0, DEP_CNT == 0 : unhash, put on pruned list
	 *				  dow_rele will get rid of it
	 *	DEP_CNT > 0 :		  don't unhash. put back on
	 *				  the leaf chain so that
	 *				  dow_iodone will be called on
	 *				  it later.
	 *
	 *		IF !GONE : Must be either clear-modified or
	 *			   not yet set-modified. In either case,
	 *			   we can let the flush demon handle the
	 *			   appropriate signalling of waiters, etc.
	 *			   We take care of the simple case here.
	 *
	 *	HOLD == 0, DEP_CNT == 0 : unhash and free
	 *	Otherwise:		: reinsert to leaf flush tail.
	 */

	ASSERT(DOW_MUTEX_OWNED());
	if (DOW_STATE(id) & DOW_GONE) {
		CHECK_HASH_REMOVED(id);
		if (DOW_DEP_CNT(id) > 0) {
			dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
			return;
		}
		if (DOW_HOLD(id) > 0) {
			dow_flush_tailins(id, DOW_PRUN_FLUSHHEAD);
			return;
		}
		ASSERT(!DOW_SV_BLKD(id));
		dow_free(id);
		return;
	}
	CHECK_HASH(id);
	if ((DOW_DEP_CNT(id) == 0) &&
	    (DOW_HOLD(id) == 0)) {
		dow_remhash(id);
		ASSERT(!DOW_SV_BLKD(id));
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			VN_SOFTRELE(DOW_VP(id));
		}
		dow_free(id);
		return;
	}
	dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
	return;
}

/*
 * boolean_t
 * dow_process_leaf_flush(dowid_t id)
 *	Perform the appropriate flush actions for a dow that has no
 *	antecedents.
 *
 * 	Called with dow_mutex held. May drop it before returning.
 * 	return: B_TRUE if DOW_MUTEX dropped.
 *
 * 	Caller has NOT removed the id from the leaf flush chain; also, 
 *	it must only be on the leaf flush chain.
 *
 * Calling/Exit State:
 *	Caller hold dow_mutex, which is dropped if either flush/ioflush
 *	actions need to performed.
 */
boolean_t
dow_process_leaf_flush(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	/* 
	 * The caller of this procedure has obtained id from the
	 * leaf flush chain. Therefore, we can be certain that no one
	 * else is currently flushing this dowid. Furthermore, we can
	 * be certain that the id has been pruned.
	 */
	CHECK_ON_FLUSHCHAIN(id, DOW_LEAF_FLUSHHEAD);
	ASSERT((DOW_STATE(id) & (DOW_FLUSHINTRANS|DOW_PRUNE)) == DOW_PRUNE);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_FLUSHID(id) == DOW_LEAF_FLUSHHEAD);
#endif
	dow_flush_remove(id);
	DOW_STATE(id) |= DOW_FLUSH;
	if (DOW_TYPE(id) & DOW_ISFUNC) {
		if ((DOW_STATE(id) & DOW_GONE) == 0) {
			dow_process_leaf_func(id, NULL);
			ASSERT(DOW_MUTEX_OWNED());
			return (B_FALSE);
		}
		CHECK_HASH_REMOVED(id);
		dow_iodone(id);
		return (B_FALSE);
	} 
	if (DOW_STATE(id) & DOW_IOSETUP) {
		dow_ioflush(id);
		return(B_TRUE);
	} else if (DOW_STATE(id) & DOW_MODIFIED) {
		dow_flush(id);
		return(B_TRUE);
	} else {
		/*
		 * !function, !IOSETUP, !MODIFIED, but PRUNE'd.
		 * may be GONE. In either case (whether GONE or
		 * not), the treatment is identical: 
		 * 	call dow_iodone
		 */
		if (DOW_STATE(id) & DOW_GONE) {
			DOW_STATE(id) = (DOW_GONE|DOW_PRUNE|DOW_INTRANS);
			CHECK_HASH_REMOVED(id);
		} else {
			DOW_STATE(id) = DOW_INTRANS;	
			CHECK_HASH(id);
		}
		dow_iodone(id);
		return(B_FALSE);
	}
}

