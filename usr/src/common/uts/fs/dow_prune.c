/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_prune.c	1.2"
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
 *  		Notes:
 * 
 * 	There are three flush chains: 
 * 
 * 	1. a leaf chain, on which a dow appears if and
 * 	   only if it has no antecdents and it must be
 * 	   executed soon.
 * 
 * 	2. a non-leaf "pruned" chain, on which a dow appears
 * 	   if it must be executed soon but must wait because
 * 	   it has one or more antecdents that are valid (i.e,
 * 	   MODIFIED and not aborted).
 * 
 * 	3. an agelist, on which dows appear if there is no
 * 	   urgency for executing them.
 * 
 * 	Pruning refers to an operation that identifies all
 * 	leaf level antecdents/grand-antecdents of a dowid and
 * 	queues them on the leaf flush chain and simultaneously
 * 	prunes any non-leaf antecdent that has not been pruned
 * 	already.
 * 
 * 	The dow subroutines must work to ensure that once a
 * 	dow is on lists 1 or 2, that any antecdent that is
 * 	added to it forces the pruning of the antecdent as well.
 * 	For this purpose:
 * 
 * 		- dow_order must prune newly created antecedents
 * 		  and move the dependent dow off the leaf chain
 * 		  if it happens to be on the leaf chain.
 * 		  Alternatively, as a simplification, a dowid
 * 		  that is marked as being pruned does not accumulate
 * 		  any antecedents (OR, if it is pruned and is on the
 * 		  leaf chain?)
 * 	
 * 	To maintain the invariant that a dow id being pruned
 * 	must be on the leaf chain if it has no antecdents,
 * 
 * 		- dow_iodone will move any dependent that has been
 * 		  pruned and now has no antecdents to the leaf chain.
 * 
 * 	We expect that functions such as dow_handle_sync,
 * 	dow_handle_async, dow_abort, dow_order, dow_strategy,
 * 	and the dow_flush_demon (which walks the aged chain)
 * 	will call dow_prune on a given dow id or its antecdents.
 * 
 * 	The functions that populate the leaf flush chain all follow 
 * 	the protocol of enquing IOSETUP items to the head of the flush 
 * 	chain and non-IOSETUP items to the tail of the flush chain.
 * 	Nobody ever walks the "pruned" non-leaf flush chain directly,
 * 	so we may be able to get rid of it and save some overhead.
 * 	For the moment, we keep it since it helps debugging.
 */

/*
 * int
 * prune_search(dowid_t x, int recur_depth)
 *	Prune each antecedent of x that has not already been pruned.
 *	Any leaf antecedent (i.e., one that has no antecedents) should
 *	be added to the leaf flush chain if it is not already on the
 * 	leaf flush chain. If the recursion depth exceeds MAX_RECUR,
 *	return 1, else return 0. If we get an indication that 
 *	MAX_RECUR depth was exceeded, then we do not mark x as pruned;
 *	instead, we add it to the age chain for future pruning, with
 *	timestamp appropriately adjusted to cause the flush daemon to
 *	pick it up.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller. Will be held across this function.
 *
 * Remarks:
 *
 *	Normally we do not expect to exceed MAX_RECUR depth. If we do,
 *	then we handle the boundary case in a special way. All dow-ids
 *	that are found at depth == MAX_RECUR are put at the head of
 *	the aged list. Subsequently as we return to upper layers of
 *	recursion, the dependent dows are enqueued at the tail of the aged
 *      list. 
 */
int
prune_search(dowid_t x, int recur_depth, boolean_t pruneself)
{
	dowlinkid_t linkhead;
	dowlinkid_t link;
	dowid_t y;
	int err;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_ANT_CNT(x) > 0);

	/*
	 * Caller ensures that x is not INTRANS, or PRUNE.
	 * Furthermore since our pruning protocol is that 
	 * FLUSH or IOSETUP or GONE ==> PRUNE, x cannot be in
	 * FLUSH, IOSETUP, or GONE states either.
	 */


	ASSERT((DOW_STATE(x) & 
		(DOW_PRUNE|DOW_FLUSHINTRANS|DOW_GONE| DOW_IOSETUP)) == 0);

	linkhead = DOWLINK_ANT_LINKHEAD(x);

	CHECK_DOWLINK_CHAIN(linkhead);
	link = DOWLINK_NEXT(linkhead);

	if (recur_depth == MAX_RECUR) {

		/*
		 * - For each antecdent y of x, do:
		 *   - if y is already marked pruned, skip to next antecdent.
		 *   - if y is leaf
		 * 	  mark it pruned and move it to the leaf flush chain,
		 *     else
		 *	  move y to aged list, and timestamp it so that it looks
		 *	  very old to the flush demon.
		 *
		 * - After all antecdents of x are visited as above, move x to the
		 *   tail of the aged list. Make x's timestamp very old only if
		 *   pruneself is true (indicating that x must be pruned as well).
		 *
		 * - return 1.
		 */

		while (link != linkhead) {
			y = DOWLINK_DOWID(link);
			link = DOWLINK_NEXT(link);
			if (DOW_STATE(y) & DOW_PRUNE) {
				CHECK_ANTECDENTS_PRUNED(y);
				continue;
			} 
			/*
			 * PERF: We could optimize a bit here, and set the PRUNE
			 * state only if the state is MOD or MODINTRANS. Not 
			 * particularly worthwhile.
			 */
			ASSERT((DOW_STATE(y) & 
				(DOW_FLUSH|DOW_IOSETUP|DOW_GONE)) == 0);
			/*
			 * we must handle either case: whether MODIFY is set 
			 * or not set. however, at this point there is no 
			 * difference in treatment. dow_iodone knows how to
			 * handle (MODIFIED cleared and PRUNE set) cases.
			 */
			if (DOW_ANT_CNT(y) == 0) {
				if (DOW_STATE(y) & DOW_INTRANS) {
					DOW_STATE(y) |= DOW_PRUNE;
				} else {
					dow_flush_remove(y);
					DOW_STATE(y) |= DOW_PRUNE;
					dow_flush_headins(y, DOW_LEAF_FLUSHHEAD);
				}
				continue;
			} else {
				ASSERT((DOW_STATE(y) & DOW_INTRANS) == 0);
				/* make y look very old */
				dow_flush_remove(y);
				DOW_TIMESTAMP(y) = 0;
				dow_flush_headins(y, DOW_AGED_FLUSHHEAD);
				continue;
			}
		} /* while link != linkhead */
		/*
		 * we finished processing x's antecdents now handle x itself. 
		 */
		dow_flush_remove(x);
		if (pruneself) {
			DOW_TIMESTAMP(x) = 0;
		}
		dow_flush_tailins(x, DOW_AGED_FLUSHHEAD);
		return 1;
	} /* if recur depth == MAX_RECUR */

	/*
	 * -  For each antecdent y of x,
	 *    - if y is already pruned, skip to the next antecdent,
	 *    - if y is a leaf,
	 *	   # we could make the optimization that if y is INTRANS,
	 *	   # skip it (since from x's standpoint, the current disk
	 *	   # write of y will cover x's relevant modification).
	 *	   # however, we will not implement the optimization so
	 *	   # that we can simplify dow code in other places, that 
	 *	   # can then rely on the stronger guarantee that when a
	 *	   # dow is marked pruned, so are all of its antecdents.
	 *	   # this ought to be a rare instance anyway, so it does not
	 *	   # hurt that we do not exploit the optimization.
	 *	   mark  y pruned and move it to the leaf chain.
	 *       else 
	 *	   visit y recursively, and, accumulate its return into err.
	 *    
	 * -  After all of x's antecdents have been visited as above, check
	 *    the accumulated err value:
	 *	 if 0, then recursion depth remained bounded, 
	 *		- if pruneself, then mark x as pruned and move it to
	 *		  the pruned list,
	 *		- else return 0 indicating success (without touching x)
	 *
	 *	 else, we must have reached the recursion limit. so, we must
	 *		- return the non-zero error code to our caller. recall
	 *		  that x was not affected, so we may either reset the
	 *		  timestamp on x to 0 only if pruneself is true, or do
	 *		  so even if pruneself is false -- it does not make
	 *		  much difference.
	 */ 
	err = 0;
	while (link != linkhead) {
		y = DOWLINK_DOWID(link);
		link = DOWLINK_NEXT(link);
		if (DOW_STATE(y) & DOW_PRUNE) {
			CHECK_ANTECDENTS_PRUNED(y);
			continue;
		}
		ASSERT((DOW_STATE(y) & (DOW_FLUSH|DOW_IOSETUP|DOW_GONE)) == 0);
		/*
		 * only DOW_MODIFY could possibly be set. we must handle 
		 * either case: whether MODIFY is set or not. 
		 */
		if (DOW_ANT_CNT(y) == 0) {
			if (DOW_STATE(y) & DOW_INTRANS) {
				DOW_STATE(y) |= DOW_PRUNE;
			} else {
				dow_flush_remove(y);
				DOW_STATE(y) |= DOW_PRUNE;
				dow_flush_headins(y, DOW_LEAF_FLUSHHEAD);
			}
			continue;
		}
		ASSERT(recur_depth < MAX_RECUR);
		ASSERT((DOW_STATE(y) & DOW_INTRANS) == 0);
		err += prune_search(y, (1 + recur_depth), B_TRUE);
		continue;
	}
	if (err) {
		/* some antecedent of x was not fully pruned */
		dow_flush_remove(x);
		DOW_TIMESTAMP(x) = 0;
		/*
		 * IT IS VITAL that x is sent to that tail of the aged
		 * list, so that all of its unpruned antecdents precede
		 * it on that list. prune(), which calls this function,
		 * relies on that ordering when it reissues pruning 
		 * for these antecdents. 
		 */
		dow_flush_tailins(x, DOW_AGED_FLUSHHEAD);
	} else if (pruneself) {
		dow_flush_remove(x);
		DOW_STATE(x) |= DOW_PRUNE;
		dow_flush_tailins(x, DOW_PRUN_FLUSHHEAD);
		CHECK_ANTECDENTS_PRUNED(x);
	} 
	return(err);
}

/*
 * int
 * prune(dowid_t x)
 *	Prune each antecedent of x that has not already been pruned,
 * 	and then x itself. Any leaf antecedent (i.e., one that has no 
 *	antecedents) should be added to the leaf flush chain if it is 
 *	not already on the leaf chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller. Will be held across this function.
 *
 * Remarks:
 *	During pruning, we will recursively visit each antecdent in the
 *	dependency graph under x, and prune it.
 *	Normally we do not expect to exceed MAX_RECUR depth. If we do,
 *	then we handle the boundary case in a special way. All dow-ids
 *	that are found at depth == MAX_RECUR are put at the head of
 *	the aged list. Subsequently as we return to upper layers of
 *	recursion, the dependent dows are enqueued at the tail of the aged
 *      list. Finally, wehn control return here, we will walk the
 *	aged list and prune everything that is old enough until x is pruned.
 */

void
prune(dowid_t x) 
{
	dowid_t z;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(x));
	CHECK_NOTFREE(x);

	if ((DOW_STATE(x) & (DOW_PRUNE | DOW_INTRANS)) != 0) {
		if (DOW_STATE(x) & DOW_PRUNE) {
			CHECK_ANTECDENTS_PRUNED(x);
			return;
		}
		ASSERT(DOW_ANT_CNT(x) == 0);
		CHECK_NOTON_ANY_FLUSHCHAIN(x);
		DOW_STATE(x) |= DOW_PRUNE;
		return;
	} else if (DOW_ANT_CNT(x) == 0) {
		CHECK_ON_FLUSHCHAIN(x, DOW_AGED_FLUSHHEAD);
		dow_flush_remove(x);
		DOW_STATE(x) |= DOW_PRUNE;
		dow_flush_tailins(x, DOW_LEAF_FLUSHHEAD);
		return;
	} 
	
	ASSERT((DOW_STATE(x) & (DOW_PRUNE|DOW_IOSETUP|DOW_FLUSHINTRANS)) == 0);

	/*
	 * DOW_GONE:
	 *	dow_abort (which will set DOW_GONE ) for "x"  will 
	 *	prune all antecdents of "x" (and may in fact be
	 * 	calling this function) 
	 *
	 * +	We require that dow_abort do the pruning BEFORE
	 * +	setting DOW_GONE.
	 *
	 *	Subsequently, no antecdents will accumulate. So we
	 * 	can treat DOW_GONE as a no operation.
	 */

	CHECK_ON_FLUSHCHAIN(x, DOW_AGED_FLUSHHEAD);

	if (prune_search(x, 0, B_TRUE) != 0) {
		/*
		 * the top down pruning of x did not complete because it reached
		 * the set recursion limit on prune_search. this should be a very
		 * rare occurrence. we can afford to be heavy handed in this case,
		 * and so we will keep pruning aged dows off the aged list 
		 * until we have successfully pruned x. 
		 * NOTE that because all antecdents of x that could not be pruned 
		 * wound up on the aged list ahead of x, by pruning the aged list 
		 * in the normal order repeatedly, we will cover them efficiently
		 */
		
		while ((DOW_STATE(x) & DOW_PRUNE) == 0) {
			/*
			 * TODO: ADD DEBUG CODE TO DETECT INFINITE LOOPING HERE.
			 */
		   	z = DOW_FLUSHNEXT(DOW_AGED_FLUSHHEAD);
		   	ASSERT(z != DOW_AGED_FLUSHHEAD);
		   	ASSERT((DOW_STATE(z) & 
				(DOW_PRUNE|DOW_FLUSHINTRANS|DOW_IOSETUP)) == 0);
		   	if (DOW_TIMESTAMP(z) <= (lbolt - DOW_AGE_TIX)) {
				if (DOW_ANT_CNT(z) == 0) {
					dow_flush_remove(z);
					DOW_STATE(z) |= DOW_PRUNE;
					dow_flush_tailins(z, DOW_LEAF_FLUSHHEAD);
				} else {
					(void) prune_search(z, 0, B_TRUE);
				}
		   	} else {
		   		dow_flush_remove(z);
		   		dow_flush_tailins(z, DOW_AGED_FLUSHHEAD);
			}
		}
	}
	CHECK_ANTECDENTS_PRUNED(x);
}

/*
 * void
 * prune_antecdents(dowid_t x)
 *	Prune antecedents of x. For now, we will also prune x if it turns
 *	out that the dependency graph beneath x is too deep. This is done
 *	for convenience, since prune() has the necessary algorithm for
 *	handling the recursion overflow.
 *
 * Calling/Exit State:
 *	dow_mutex is held by the caller and never dropped before returning.
 */ 
void
prune_antecdents(dowid_t x)
{
	ASSERT((DOW_STATE(x) & (DOW_INTRANS|DOW_PRUNE)) == 0);
	ASSERT(DOW_ANT_CNT(x) > 0);
	if (prune_search(x, 0, B_FALSE) != 0) {
		/* prune everything, x included! */
		prune(x);
	}
}

