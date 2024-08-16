/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_order.c	1.4"
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
 * dow_order family of subroutines. dow_order_nowait provides the
 * necessary base implementation; all waiting is done outside, in
 * dow_order_noreswait and dow_order routines.
 */

/*
 * In all success-but-no-link-established cases, we may want to take
 * care of functions.
 */

/*
 *
 * General Notes: 
 *
 *	1. If either of the dowids is invalid, dow_order returns
 *	   successfully without making the specified linkage. The
 *	   rest of this discussion pertains to the case when
 *	   both dowids are valid.
 *
 *	2. The caller is expected to maintain a hold on each dowid
 *	   before calling dow_order.
 *
 *	3. The dow protocol ensures:
 *
 *	   (a) 	DOW_FLUSH 	=> DOW_PRUNE
 *		DOW_IOSETUP	=> DOW_PRUNE
 *		DOW_GONE	=> DOW_PRUNE
 *	   
 *	 	That is, if a dow is picked for flushing by the 
 *		dow_flush_demon, then all of its antecedents
 *		must be flushed before the dow is flushed. Also,
 *		if dow_strategy is called for a dow, then again
 *		all its antecedents must be flushed before
 *		the dow can be flushed. In both instances, the
 *		protocols followed by dow_flush_demon and dow_strategy
 *		will ensure that this is done by pruning the 
 *		dependency subtree that is rooted at the dow.
 *
 *		dow_abort will also participate in this protocol.
 *
 *	   (b)  If any dow is pruned, then all its dependents current
 *		and future will be pruned as well. This has implications
 *		on dow_order.
 *
 *	   (c)  When a disk write does take place, a dow reverts
 *		to the non-pruned state. 
 *
 *	   (d)  DOW_INTRANS 	=> ~(DOW_FLUSH | DOW_IOSETUP | DOW_MODIFIED)
 *
 *	4.  In establishing a link, it is of interest whether the
 *	    *relevant* modification of the antecedent has already
 *	    been flushed to the disk; if so, the linkage is not
 *	    necessary (and should not be made, if no event can be
 *	    guaranteed to occur that can break the link).
 *	    Similarly, if the dependent is already being written
 *	    to disk, then linking cannot be completed until the
 *	    dependent write completes -- otherwise, the impending
 *	    modification of the dependent can become flushed to
 *	    disk while the antecedent is yet to be flushed.
 *	    These yield the following:
 *
 *	    IF antecdent is in state:
 *
 *		MODIFIED, but not INTRANS: The relevant modification of
 *			the antecedent may not be written to disk. So
 *			a link should be created unless the dependent
 *			is INTRANS,
 *
 *		INTRANS, but not MODIFIED: The relevant modification is
 *			being written (potentially), and we could definitely
 *			conclude that it is written when the current write
 *			completes. Either we wait for the write to complete
 *			before making the link, or create the link and let
 *			the write completion remove the created link.
 *
 *		        We follow the protocol that at write completion,
 *			all dependency links are removed. To ensure the
 *			correctness of this, we handle the next case
 *			specially-
 *
 *		INTRANS and MODIFIED: Since the relevant modification of 
 *			the antecedent cannot be assumed to be written to
 *			disk on completion of the current write, we cannot
 *			allow the removal of the dependency link in this
 *			case. Therefore, if we made the desired linkage,
 *			then the link would need to be flagged specially
 *			so that write-completion does not remove it.
 *			Instead, we elect not to make the link (or wait
 *			until IO completes in order to make the requested
 *			link).
 *
 *		not INTRANS and not MODIFIED: The relevant modification
 *			has already been reflected to disk; therefore,
 *			there is no need to establish the link. 
 *
 *	    IF dependent is in state:
 *
 *		FLUSH or INTRANS: The dependent is about to be written.
 *			So do not establish a linkage until after the
 *			write completes. (The modification of the
 *			dependent that we do not want to write to disk
 *			ahead of the antecedent is yet to occur so the
 *			current write of the dependent is benign).
 *
 *	    In addition to the above, these special conditions need to
 *	    be handled: 
 *
 *	 	antecedent GONE: (i.e., aborted): In this instance,
 *			there is no constraint on the scheduling of
 *			a write for the dependent. So we return
 *			success without creating the linkage.
 *
 *		dependent GONE: If the antecedent has not been aborted,
 *			then any pending write for it must be forced out.
 *			The linkage itself is not made, but the attempt
 *			is treated as a failure unless the antecedent write
 *			is not waited for.
 *
 *		The above rules (for the abort cases) could be applied in
 *		reverse, so that the dependent GONE => success and
 *		antecedent GONE => failure/wait. We have elected the
 *		first set of rules because dependent modifications
 *		are typically expected to occur in the future while
 *		antecedent modifications have already occurred and so
 *		can be flushed out right now.
 *
 *		dependent PRUNE: This state suggests that there is an
 *			urgency about scheduling a disk write for the
 *			dependent -- either because some write was
 *			initiated for the dependent or its dependents
 *			and cannot complete unless the dependent write
 *			itself completes. In such cases, we would like
 *			to ensure that any links we create also
 *			carry this urgency forward to the new antecedents
 *			to be added. An alternative may have been to
 *			treat this similarly to dependent being in
 *			INTRANS or FLUSH states (and so waiting for
 *			dependent write to complete); but we can get
 *			better write caching by making the link right now.
 *			So, if we make the link, we must ensure that
 *			the antecedent is in a PRUNE state as well.	
 *
 */

/*
 * int
 * dow_order_nowait(dowid_t dep_id, dowid_t ant_id)
 *
 *	Establish the desired dependency linkage. Do not block
 *	-- return with appropriate failure indication if the
 *	dependency could not be established either because 
 *	the status of the antecdent or dependent DOW structures
 *	indicated IOs in progress or because of a potential 
 *	dependency cycle or because of lack of linkage resources.
 *
 * Calling/Exit State:
 * 	dow_mutex is held by caller. Never dropped. If the id's are
 *	not invalid, they have positive hold counts. The following
 *	codes are returned by the function:
 *
 *		0		successful.
 *
 *	DOW_ANT_BUSY      	antecdent is being written to disk. however, 
 *				its state is MODIFIED as well; so it is not
 *				possible to determine whether this new link 
 *				can be removed when the IO completes.  hence
 *				the link is not created.
 *				
 *	DOW_DEP_BUSY		dependent is being written to disk. if the
 *				link were created and control returned to the
 *				caller, then caller's modification of the
 *				dependent could get written to disk by the
 *				IO that is in progress. (This does not matter
 *				if the antecdent state is not MODIFIED).
 *
 *	DOW_POS_CYCLE		the specified link could give rise to a cycle,
 *				and therefore has not been established.
 *
 *	DOW_DEF_CYCLE		a reverse dependency between dep_id
 *				and ant_id is known to exist.
 *
 *	DOW_CREATE_OK		the desired link can be made.
 *
 *	DOWLINK_NORES		no resources were available to establish the
 *				desired linkage.
 *
 *	DOW_DEP_GONE		the dependent has been aborted. the correct
 *				recovery is for the antecdent to be flushed.
 *
 *	DOW_DEP_PRUNE		the dependent is PRUNE'd, and has no antece-
 *				dents; it is advisable therefore, to flush
 *				the dependent before creating the link.
 *
 * XXX: Add some debugging support.
 */
int
dow_order_no_wait(dowid_t dep_id, dowid_t ant_id)
{
	uchar_t ant_state;
	uchar_t	dep_state;
	int linkage_err;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(dep_id != ant_id);
	ASSERT(VALID_DOWID(dep_id));
	ASSERT(VALID_DOWID(ant_id));
	ASSERT(DOW_HOLD(dep_id) > 0);
	ASSERT(DOW_HOLD(ant_id) > 0);

	ant_state = DOW_STATE(ant_id);
	ant_state &= (DOW_MODINTRANS | DOW_GONE); 
	dep_state = DOW_STATE(dep_id); 

	if (ant_state == 0) {
		/*
		 * debugging note:
		 * unless a dow_clearmodify operation was done on the
		 * ant_id, we expect that the antecedent cannot
		 * have any dependents (since the last write completion
		 * must have removed all the dependents). So we may
		 * want to introduce such a debug mode state, so that
		 * the following assert can be placed:
		 *
		 * ASSERT(DOW_MODCLEARED(ant_id) || 
		 *	  DOW_DEP_CNT(ant_id) == 0);
		 */
		return 0;
	} 
	ASSERT((ant_state & (DOW_MODIFIED|DOW_GONE)) !=
			(DOW_MODIFIED|DOW_GONE));
	if (ant_state == DOW_MODINTRANS) {
		/*
		 * cannot allow caller to proceed past this point 
		 * to modifying the dependent, until the current
		 * IO completes.
		 * Also, since the antecdent is INTRANS, we can
		 * assert that it has not antecdents.
		 */
		ASSERT(DOW_ANT_CNT(ant_id) == 0);
		return (DOW_ANT_BUSY);	
	} else if (ant_state & DOW_GONE) {
		/* MOD state should be clear, as asserted earlier. */
		if (ant_state & DOW_INTRANS) {
			/*
			 * the linkage is permissible in this case (so 
			 * long as dependent is not INTRANS), since the 
			 * antecdent INTRANS occurred _after_ the last 
			 * setmod operation (this can be inferred from 
			 * the fact that abort has to have come between 
			 * the setmod and the current INTRANS).
		 	 *
		 	 * however, we will instead ask the caller to retry 
			 * after the antecdent INTRANS has cleared: at that 
			 * point we can return success without creating the 
			 * link.
		 	 */
			ASSERT(DOW_ANT_CNT(ant_id) == 0);
			return (DOW_ANT_BUSY);
		}	
		return 0;
	}
	/* 
	 * At this point, the antecdent is either MODIFIED or INTRANS
	 * but not both; and it is not in an aborted state.
	 */
	if ((dep_state & (DOW_PRUNE|DOW_INTRANS)) != 0) {

		if ((dep_state & DOW_FLUSHINTRANS) != 0) {
			/* 
			 * there should be no antecdents since IO initiation 
			 * is being done or IO is in progress 
			 */
			ASSERT(DOW_ANT_CNT(dep_id) == 0);
			return (DOW_DEP_BUSY);
		}
		/* dep_state may be DOW_GONE, DOW_IOSETUP */
		if (dep_state & DOW_GONE) {
			/*
			 * PRUNE must be set, MOD must be clear.
			 */
			ASSERT((dep_state & DOW_MODPRUNE) == DOW_PRUNE);
			/*
			 * if dependent is aborted (or being aborted), then we
			 * expect the caller to flush the antecdent (and wait 
			 * until the flush completes) before proceeding.
			 */
			return (DOW_DEP_GONE);
		}
		return (DOW_DEP_PRUNE);
	} 

	ASSERT((dep_state & (DOW_GONE|DOW_FLUSHINTRANS)) == 0); 

	if ((linkage_err = linkage_detect(dep_id, ant_id)) != DOW_CREATE_OK) {

		/*
		 * either the desired link exists (linkage_err == 0),
		 * or either there is known to be a cycle or a
		 * cycle is possible but not certain.
		 */

		ASSERT( (linkage_err == DOW_POS_CYCLE) ||
			(linkage_err == DOW_DEF_CYCLE) ||
			(linkage_err == 0));

		return linkage_err; 
	}

	/*
	 * we are almost ready to create the link.
	 */

	if (!DOWLINK_AVAIL(2)) {
		return (DOWLINK_NORES);
	}

	return(dowlink_makelink(dep_id, ant_id));
}

/*
 * int
 * dow_order(dowid_t dep, dowid_t ant, uchar_t dow_order_flag) 
 *	Establish the specified ordering. dow_order_flag can be either
 *	DOW_NO_WAIT, DOW_NO_RESWAIT, or DOW_; 0 implies that all waiting is 
 *	tolerable. 
 *
 *	Returns 0 if successful; a non-zero error code if unsuccessful.
 *	The non-zero error code is the same as that returned by
 *	dow_order_no_wait. 
 *
 * Calling/Exit State:
 *	No spin locks should be held by caller, unless DOW_NO_WAIT is
 *	specified (in which case blocking will not occur).
 */
int
dow_order(dowid_t dep, dowid_t ant, uint_t dow_order_flag)
{
	int err;

	ASSERT((dow_order_flag & DOW_NO_WAIT) || KS_HOLD0LOCKS());
	if (!VALID_DOWID(dep) || !VALID_DOWID(ant))
		return(0);
	DOW_MUTEX_LOCK();
	ASSERT(dep != ant);
	ASSERT(DOW_HOLD(dep) > 0);
	ASSERT(DOW_HOLD(ant) > 0);
	if (dow_order_flag & DOW_NO_WAIT) {
 		err = dow_order_no_wait(dep, ant);
		DOW_MUTEX_UNLOCK();
		return (err);	
	}
	for (;;) {
	    switch (err = dow_order_no_wait(dep, ant)) {
		case 0: /* success */
			DOW_MUTEX_UNLOCK();
			return (0);
		case DOWLINK_NORES:
			if (dow_order_flag & DOW_NO_RESWAIT) {
				DOW_MUTEX_UNLOCK();
				return(err);
			}
			DOWLINK_FREE_SV_WAIT();
			break;
		case DOW_ANT_BUSY:
			/*
			 * wait for antecdent to be flushed out.
			 */	
			dow_intrans_wait(ant);
			break;
		case DOW_DEP_BUSY:
			/*
			 * dependent write has been initiated. wait
			 * for it to complete.
			 */
			dow_intrans_wait(dep);	
			break;
		case DOW_DEP_GONE:
			/*
			 * dependent has been aborted. flush the
			 * antecdent.
			 */
			ASSERT((DOW_STATE(ant) & DOW_GONE) == 0);
			dow_handle_sync_l(ant);
			break;
		case DOW_POS_CYCLE:
			/*
			 * potentially, there may be a chain of
			 * dependency from antecdent to dependent.
			 * flush the antecedent.
			 */
			ASSERT((DOW_STATE(ant) & DOW_GONE) == 0);
			dow_handle_sync_l(ant);
			break;
		case DOW_DEF_CYCLE:
			/*
			 * a reverse dependency from antecedent
			 * to dependent is _known_ to exist.
			 * flush the dependent.
			 */
			ASSERT((DOW_STATE(dep) & DOW_GONE) == 0);
			dow_handle_sync_l(dep);
			break;
		case DOW_DEP_PRUNE:
			/*
			 * the dependent is a leaf dow that has already
			 * been pruned. flush the dependent.
			 */
			dow_handle_sync_l(dep);
			break;
		case DOW_MAX_ANTCNT:
			dow_handle_sync_l(dep);
			break;
		case DOW_MAX_DEPCNT:
			dow_handle_sync_l(ant);
			break;
		default: /*
			  *+ unexpected return code from dow_order_nowait.
			  *+ unrecoverable software error.
			  */
			cmn_err(CE_PANIC,
				"unexpected return code 0x%x from "
				"dow_order_nowait\n", err);
			/* NOTREACHED */	
	    }/* switch */
 	    DOW_MUTEX_LOCK();
	}
}
