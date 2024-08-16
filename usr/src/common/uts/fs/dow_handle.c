/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_handle.c	1.2"
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
 * dow_handle_async(dowid_t id)
 *	
 * 	Initiate operations that would ultimately result in a
 *	write IO for id, if some operation is not already
 *	underway.
 *
 * Calling/Exit State:
 *
 *	Caller expected to pass in a dowid with a hold, if valid. 
 * 	the hold will be not be released by this function before
 * 	it returns. 
 *
 * Remarks:
 *	The real work is done by dow_handle_async_l.
 */
void
dow_handle_async(dowid_t id)
{
	if (!VALID_DOWID(id))
		return;
	DOW_MUTEX_LOCK();
	dow_handle_async_l(id); /* will drop the dow mutex */
	return;
}

/*
 * void
 * dow_handle_async_l(dowid_t id)
 *	
 * 	Initiate operations that would ultimately result in a
 *	write IO for id, if some operation is not already
 *	underway.
 *
 * Calling/Exit State:
 *
 *	Caller expected to pass in a valid dowid with a hold.
 * 	the hold will be not be released by this function before
 * 	it returns. 
 * 	dow_mutex held on the way in, dropped before returning. 
 */
void
dow_handle_async_l(dowid_t id)
{
	uchar_t id_state;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	id_state = DOW_STATE(id);
	ASSERT(DOW_HOLD(id) > 0);

	if (id_state & DOW_GONE) {
		ASSERT(id_state == (DOW_GONE|DOW_PRUNE));
		DOW_MUTEX_UNLOCK();
		return;
	}

	if (DOW_ANT_CNT(id) == 0) {
		if ((id_state & DOW_INTRANS) != 0) {
			CHECK_HASH(id);
			if (DOW_STATE(id) & DOW_MODIFIED) {
				DOW_STATE(id) |= DOW_PRUNE;
			} 
			DOW_MUTEX_UNLOCK();
			return;
		} 
		if (id_state & (DOW_IOSETUP | DOW_FLUSH)) {
			ASSERT((DOW_STATE(id) & DOW_PRUNE) != 0);
			if ((id_state & DOW_FLUSH) == 0) {
				/* Must be IOSETUP. Flush it */
				ASSERT(DOW_STATE(id) & DOW_MODIFIED);
				dow_flush_remove(id);
				DOW_STATE(id) |= DOW_FLUSH;
				dow_ioflush(id);
			}  else {
				/* 
				 * FLUSH is set. This id is already being
				 * handled by the DOW system
				 */
				DOW_MUTEX_UNLOCK();
			}
			return;
		}
		if ((id_state & DOW_MODIFIED) == 0) {
			/*
			 * This could be either because id was never
			 * modified, or it was either clear_modified
			 * or aborted.
			 */
			dow_flush_remove(id);
			DOW_STATE(id) |= DOW_INTRANS;
			dow_iodone(id);
			DOW_MUTEX_UNLOCK();
			return;
		}
		/* 
		 * Ensure that id gets out soon, by putting it at the 
		 * head of the leaf flush chain. PRUNE may or may not be 
		 * set, so we set it in any case. We want to let the flush
		 * demon take care of it since we don't want to block here.
		 */	

		dow_flush_remove(id);
		DOW_STATE(id) |= DOW_PRUNE;
		dow_flush_headins(id, DOW_LEAF_FLUSHHEAD);	
		DOW_MUTEX_UNLOCK();
		return;

	} /* if DOW_ANT_CNT(id) == 0 */

	/* If the id has not been pruned, now is the time to do it */
	if ((id_state & DOW_PRUNE) == 0) {
		prune(id);
	}

	CHECK_ANTECDENTS_PRUNED(id);
	DOW_MUTEX_UNLOCK();
	return;
}

/*
 * void
 * dow_handle_sync(dowid_t id)
 *	
 *	
 *	Wait until the caller's relevant modification of id has
 *	been reliably written out, after initiating operations
 *	(such as pruning a modified dow) that would ultimately
 * 	result in a write IO -- if a write IO is not already
 *	underway. If write IO is generated or will be generated
 *	due to this function, then wait until the IO completes.
 *
 * Calling/Exit State:
 *
 *	Caller expected to pass in a dowid with a hold, if valid. 
 * 	the hold will be not be released by this function before
 * 	it returns. 
 *
 * Remarks:
 *	The real work is done by dow_handle_sync_l.
 */
void
dow_handle_sync(dowid_t id)
{
	if (!VALID_DOWID(id)) 
		return;
	DOW_MUTEX_LOCK();
	dow_handle_sync_l(id); /* will drop the dow mutex */
	return;
}

/*
 * void
 * dow_handle_sync_l(dowid_t id)
 *	
 *	Wait until the caller's relevant modification of id has
 *	been reliably written out.
 * 
 * Calling/Exit State:
 *	Caller expected to pass in a dowid with a hold, if valid. 
 * 	the hold will be released by this function before
 * 	it returns. 
 * 	dow_mutex held on the way in, dropped before returning. 
 */
void
dow_handle_sync_l(dowid_t id)
{
	uchar_t id_state;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id)); 
	ASSERT(DOW_HOLD(id) > 0);
	id_state = DOW_STATE(id);
	if (id_state & DOW_GONE) {
		ASSERT(id_state == (DOW_GONE|DOW_PRUNE));
		DOW_MUTEX_UNLOCK();
		return;
	}
	if (DOW_ANT_CNT(id) == 0) {
		if ((id_state & DOW_INTRANS) != 0) {
			if (DOW_STATE(id) & DOW_MODIFIED) {
				DOW_STATE(id) |= DOW_PRUNE;
				dow_intrans_wait(id);
				DOW_MUTEX_LOCK();
				if (((DOW_STATE(id) & DOW_MODINTRANS) == 0) ||
				     (DOW_STATE(id) & DOW_GONE)) {
					/* mission accomplished */
					DOW_MUTEX_UNLOCK();
					return;
				}
				/* else wait for IO to complete, below */
			}
			dow_intrans_wait(id);
			return;
		} 

		if (id_state & (DOW_IOSETUP | DOW_FLUSH)) {
			ASSERT(id_state & DOW_PRUNE);
			if ((id_state & DOW_FLUSH) == 0) {
				/* Must be IOSETUP. Flush it */
				ASSERT(DOW_STATE(id) & DOW_MODIFIED);
				dow_flush_remove(id);
				DOW_STATE(id) |= DOW_FLUSH ;
				dow_ioflush(id);
				DOW_MUTEX_LOCK();
				/*
				 * and then, wait below
				 */
			} 
			/* FLUSH is set. This id is already being */
			/* handled by the DOW system */
			dow_intrans_wait(id);
			return;
		}

		if ((id_state & DOW_MODIFIED) == 0) {
			/*
			 * either the id was never modified, or was
			 * either clear_modified or aborted.
			 */
			DOW_STATE(id) |= DOW_INTRANS;
			dow_flush_remove(id);
			dow_iodone(id);
			DOW_MUTEX_UNLOCK();
			return;
		}
		
		/* 
		 * Modified, and not being flushed/written. We can
		 * assert that id has not been aborted, since dow_abort
		 * would have cleared MODIFY, or failed to proceed
		 * because of a flush in progress; but had the latter
		 * been the case, the subsequent IO that has occurred would
		 * have cleared MODIFY.
		 *
		 * Since we can tolerate blocking, let is perform the flush
		 * right here!
		 */	
		if (DOW_TYPE(id) & DOW_ISFUNC) {
			dow_flush_remove(id);
			DOW_STATE(id) |= (DOW_FLUSH | DOW_PRUNE);
			dow_process_leaf_func(id, NULL);
			DOW_MUTEX_UNLOCK();
			return;
		} else {
			dow_flush_remove(id);
			DOW_STATE(id) |= (DOW_FLUSH | DOW_PRUNE);
			dow_flush(id);
			DOW_MUTEX_LOCK();
			dow_intrans_wait(id);
			return;
		}
	}
	if ((id_state & DOW_PRUNE) == 0) {
		prune(id);
	} else {
		CHECK_ANTECDENTS_PRUNED(id);
	}
	dow_iowait_l(id);
	return;
}
