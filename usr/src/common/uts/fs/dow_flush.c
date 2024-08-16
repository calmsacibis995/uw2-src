/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_flush.c	1.9"
#ident	"$Header: $"

#include	<fs/dow.h>
#include	<fs/fs_hier.h>
#include	<io/conf.h>
#include	<mem/kmem.h>
#include	<mem/page.h>
#include	<proc/proc.h>
#include	<svc/clock.h>
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/ghier.h>
#include	<util/mod/moddefs.h>
#include	<util/sysmacros_f.h>
#include	<util/var.h>

/*
 * TODO:
 *	Write a debug routine that can be called periodically to verify
 *	that DOW_STATE, DOW_DEBUG_FLUSHID, DOW_DEBUG_HASHID for all the
 *	dows are consistent with each other.
 *
 * One main concern: to verify that if a dow gets to the pruned list, and 
 * is modified or iosetup, that it will get written. We don't have anything
 * that walks the pruned list (does not quite make sense), and we are depending
 * on the fact that between the iodone processing bubbling up and the pruning
 * from above and flush demon's leaf processing, we have covered all means by
 * which dows will get processed without external nudging.
 */

STATIC 	clock_t	dow_flush_daemon_woke = 0;
STATIC	sv_t	dow_pagesync_sv;
STATIC	lwp_t	*lwp_dowflushd;		/* dow_flush_daemon lwpp */
vnode_t *dow_pagesync_vp = NULL;
off_t	dow_pagesync_off = (-1);
STATIC	boolean_t	dow_pagesync_handoff = B_FALSE;
STATIC	boolean_t	dow_unloading = B_FALSE;

void		dow_pagesync_daemon();

void		dowinit(void);
void		dowpostroot(void);
STATIC int	dow_load(void);
STATIC int	dow_unload(void);
STATIC k_lwpid_t       dow_pagesync_lwpid;
STATIC k_lwpid_t       dow_flush_lwpid;
STATIC toid_t          dow_timeid;
dowid_t		pagesync_modlock_dowid = DOW_BADID;

#define DRVNAME "dow - Delayed Ordered Writes driver"

MOD_MISC_WRAPPER(dow, dow_load, dow_unload, DRVNAME);

/*
 * STATIC int
 * dow_load(void)
 *	Load the dow module.  Call spawn_sys_lwp
 *	to create our daemons as LWPs of sysproc.
 *
 * Calling/Exit State:
 *      No locks held on entry and exit. 
 *
 *    Return value:
 *       0: is returned on success.
 *       1: is returned on failure, if we can't
 *		start the daemons.  This will cause
 *		the module load to fail.
 */
STATIC int
dow_load(void)
{
	int		error;

	dowinit();
	/* 
	 * We don't set LWP_DETACHED so we can
	 * wait for these daemons to exit 
	 * if we try to unload the module.
	 */
	if (spawn_sys_lwp(&dow_flush_lwpid, 0, dow_flush_daemon, NULL))
		return 1;

	if (spawn_sys_lwp(&dow_pagesync_lwpid, 0, dow_pagesync_daemon, NULL)) {
		/*
		 * Kill the dow_flush daemon
		 * and cancel the timeout.
		 */

		dow_unloading = B_TRUE;

		DOW_FLUSH_EVENT_BROADCAST();
		error = wait_sys_lwp(dow_flush_lwpid);
		if (error) {
			cmn_err(CE_NOTE, "dow_unload: wait for dow_flush error %d\n",
				error);
		}
		untimeout(dow_timeid);
		return 1;
	}

	return 0;
}

/*
 * STATIC int
 * dow_unload(void)
 *	Unload the dow module.  Tell the daemons to exit.
 *	Wait for the daemons to exit. Cancel the timeout.
 *
 * Calling/Exit State:
 *      No locks held on entry and exit. 
 */
STATIC int
dow_unload(void)
{
	int		error;

	dow_unloading = B_TRUE;

	DOW_FLUSH_EVENT_BROADCAST();
	error = wait_sys_lwp(dow_flush_lwpid);
	if (error) {
		cmn_err(CE_NOTE, "dow_unload: wait for dow_flush error %d\n",
			error);
	}

	SV_BROADCAST(&dow_pagesync_sv, 0);
	error = wait_sys_lwp(dow_pagesync_lwpid);
	if (error) {
		cmn_err(CE_NOTE, "dow_unload: wait for dow_pagesync error %d\n",
			error);
	}

	untimeout(dow_timeid);
        return 0;
}

/*
 * void
 * dow_start_flush_timeout(void)
 *	Allocate space for configuring a periodic timeout.
 *
 * Calling/Exit State:
 *	Called at PLTIMEOUT.
 */

void
dow_start_flush_timeout(void)
{
	void	*co = itimeout_allocate(KM_SLEEP);
	dow_timeid = itimeout_a(dow_flush_timeout, (void *)NULL, 
			(10 | TO_PERIODIC), PLTIMEOUT, co);
}

/*
 * void
 * dow_flush_timeout(void)
 *	dow flush demon timeout routine. fires every 10 ticks.
 *	Wakeup dow flush demon if there are items on the leaf flush
 *	chain, or the age flush chain is non-empty and the dow flush demon
 *	has not been nudged for DOW_AGE_TIX clock ticks.
 *
 * Calling/Exit State:
 *	Called at PLTIMEOUT.
 */
void
dow_flush_timeout(void) 
{ 

	pl_t savepl;

	/*
	 * first, check to see if something needs pruning.
	 */
	savepl = DOW_MUTEX_LOCK();
	if (!EMPTY_DOW_FLUSHLIST(DOW_LEAF_FLUSHHEAD) ||
	     (!EMPTY_DOW_FLUSHLIST(DOW_AGED_FLUSHHEAD) &&
	      ((lbolt - DOW_AGE_TIX) > dow_flush_daemon_woke))) {
		dow_flush_daemon_woke = lbolt;
		DOW_FLUSH_EVENT_BROADCAST();
	}
	DOW_MUTEX_UNLOCK_SAVEDPL(savepl);
	return;
}

/*
 * void
 * dow_flush_daemon(void *arg)
 * 	System LWP to flush delayed writes or execute deferred actions
 *	whose antecedents have been written, and to initiate the
 *	pruning of those deferred writes/actions that are overdue.
 *
 * Calling/Exit State:
 *	The LWP blocks until signalled by the DOW system that there is
 *	work for it to do.
 */
/* ARGSUSED */
void
dow_flush_daemon(void *arg)
{
   	dowid_t id;
	int	cycle_count;
	dowid_t	dummy;
	struct  vnode dummy_vnode;

   	u.u_lwpp->l_name = "dow_flushd";
	lwp_dowflushd = u.u_lwpp;

	/*
	 * Initialize the dummy_vnode: zero it out, as if allocated
	 * via kmem_zalloc, and then VN_INIT it.  (Zeroing it out may
	 * not be necessary, but it's safe, and this code is executed
	 * only once, so the performance penalty is negligible.)
	 */
	bzero((caddr_t)(&dummy_vnode), sizeof(struct vnode));
	VN_INIT(&dummy_vnode, NULL, VNON, NODEV, 0, 0);

	DOW_MUTEX_LOCK();
	dummy = dow_create_page_l(&dummy_vnode, LONG_MAX, LONG_MAX, DOW_NO_WAIT);
	ASSERT(VALID_DOWID(dummy));
	dow_flush_remove(dummy);
	dow_remhash(dummy);
	/*
	 * at this point, no one should be able to find this "dummy" dowid.
	 * so it is all ours!
	 */
	DOW_STATE(dummy) = DOW_PRUNE;
	DOW_MUTEX_UNLOCK();	

        dow_start_flush_timeout();

	for(;;) {
	 	DOW_FLUSH_EVENT_WAIT();
		if (dow_unloading)
			lwp_exit();
   		DOW_MUTEX_LOCK();
		dow_flush_daemon_woke = lbolt;

		dow_flush_tailins(dummy, DOW_LEAF_FLUSHHEAD);

		while ((id = DOW_FLUSHNEXT(DOW_LEAF_FLUSHHEAD)) != dummy) {

			ASSERT((DOW_STATE(id) & (DOW_FLUSH | DOW_PRUNE)) 
				== DOW_PRUNE);

			if (dow_process_leaf_flush(id)) {
				DOW_MUTEX_LOCK();
			}
			if (dow_pagesync_handoff) {
				/* 
				 * Perhaps because dow_flush could not 
				 * perform a synchronous page push operation 
				 * it had to re-enqueue the dow structure 
				 * back on to the leaf chain.
				 *
				 * Set the dow_flush_daemon_woke back to 0
				 * to ensure that the flush daemon gets an
				 * early wakeup in this case, and break out 
				 * of the loop we are in.
				 */ 
				dow_pagesync_handoff = B_FALSE;
				dow_flush_daemon_woke = 0;
				DOW_MUTEX_UNLOCK();
	 			DOW_FLUSH_EVENT_WAIT();
		   		DOW_MUTEX_LOCK();
				break;
			}
		}
		dow_flush_remove(dummy);
		cycle_count = 0;
rescan:

		for (id =  DOW_FLUSHNEXT(DOW_AGED_FLUSHHEAD); 
		     id != DOW_AGED_FLUSHHEAD; id = DOW_FLUSHNEXT(id)) {
			ASSERT((DOW_STATE(id) & DOW_PRUNE) == 0);
 			if ((DOW_TIMESTAMP(id) <= (lbolt - FLUSHDELTAMAX)) &&
			    ((DOW_STATE(id) & DOW_MODIFIED) || 
			     (DOW_DEP_CNT(id) > 0)) ) {
				prune(id);
				ASSERT(DOW_STATE(id) & DOW_PRUNE);
				CHECK_NOTON_FLUSHCHAIN(id, DOW_AGED_FLUSHHEAD);
				if (++cycle_count < 20) {
					goto rescan;
				} 
				goto done_scan;
			}
			/* else: continue */
		} 

done_scan:
		dow_flush_daemon_woke = lbolt;
		dow_flush_tailins(dummy, DOW_LEAF_FLUSHHEAD);

		/*
		 * now handle the leaf chain a second time, if new work
		 * was created due to above pruning.
		 */

		while ((id = DOW_FLUSHNEXT(DOW_LEAF_FLUSHHEAD)) != dummy) {
			ASSERT((DOW_STATE(id) & (DOW_FLUSH | DOW_PRUNE)) 
				== DOW_PRUNE);
			if (dow_process_leaf_flush(id)) {
				DOW_MUTEX_LOCK();
			}
			if (dow_pagesync_handoff) {
				/* 
				 * Perhaps because dow_flush could not 
				 * perform a synchronous page push operation 
				 * it had to re-enqueue the dow structure 
				 * back on to the leaf chain.
				 *
				 * Set the dow_flush_daemon_woke back to 0
				 * to ensure that the flush daemon gets an
				 * early wakeup in this case, and break out 
				 * of the loop we are in.
				 */ 
				dow_pagesync_handoff = B_FALSE;
				dow_flush_daemon_woke = 0;
				break;	
			}
		}
		dow_flush_remove(dummy);
		DOW_FLUSH_EVENT_CLEAR();
		DOW_MUTEX_UNLOCK();
	}
}


/*
 * void
 * dow_pagesync_daemon(void *arg)
 * 	System LWP to flush delayed page writes, if necessary, synchronously .
 *
 * Calling/Exit State:
 *	The LWP blocks until signalled by the DOW system that there is
 *	work for it to do.
 */
/* ARGSUSED */
void
dow_pagesync_daemon(void *arg)
{
   	dowid_t id;

   	u.u_lwpp->l_name = "dow_pagesyncd";
	SV_INIT(&dow_pagesync_sv);
	DOW_MUTEX_LOCK();

	for(;;) {
		/* Wait here while there is no work to do! */
		while ((dow_pagesync_vp == NULL) ||
		       (dow_pagesync_off == (-1)) ) {
			SV_WAIT(&dow_pagesync_sv, (PRIBUF + 10), &dow_mutex);
			if (dow_unloading)
				lwp_exit();
			DOW_MUTEX_LOCK();
		}

		id = dow_lookup_page(dow_pagesync_vp, dow_pagesync_off);

		if (id != DOW_BADID) {
			/*
			 * Valid dow. We should proceed to flush it only if
			 * all of the following are true:
			 *	- it is a leaf level dow
			 *	- it is not GONE, IOSETUP, or INTRANS
			 *	- it is not already being flushed
			 *	- it is modified
			 */
			if (DOW_ANT_CNT(id) == 0 &&
			    ((DOW_STATE(id) & (DOW_MODIFIED|DOW_GONE|DOW_FLUSH|
			      DOW_IOSETUP|DOW_INTRANS)) == DOW_MODIFIED)) {
				prune(id);	
				if (!dow_startmod_trywrlock_l(id)) {
					/*
					 * cannot obtain modlock on this
					 * dow. so we should not proceed
					 * to flush right now.
					 */
					dow_rele_l(id);
					DOW_MUTEX_UNLOCK();
					delay(HZ/10);
					DOW_MUTEX_LOCK();
					continue;
				}
				pagesync_modlock_dowid = id;
				if (dow_process_leaf_flush(id)) {
					DOW_MUTEX_LOCK();
				}
				while (pagesync_modlock_dowid != DOW_BADID) {
					if (DOW_STATE(pagesync_modlock_dowid)
						& DOW_GONE) {
						pagesync_modlock_dowid =
								DOW_BADID;
						break;
					}
					DOW_MUTEX_UNLOCK();
					delay(HZ/10);
					DOW_MUTEX_LOCK();
				}
			}
			dow_rele_l(id);
		}

		dow_pagesync_vp = NULL;
		dow_pagesync_off = (-1);

	} /* for (;;) */
}

/*
 * void
 * dow_flush(dowid_t id)
 *	This function is called for forcing out those antecdent delayed
 *	writes that have not been explicitly initiated (due to a 
 *	VOP_PUTPAGE or a bwrite) themselves. 
 *
 *	WILL NOT BE ASKED TO HANDLE ABORTED or CLEAR-MODIFIED DOWIDs. 
 *	This filtering is done by the callers. dow_flush is an internal
 *	interface and is invoked by dow_handle_sync_l, dow_setmodify,
 *	and by the dow_flush_demon, all of which bypass calling it
 *	if the dow state is not MODIFIED or is aborted.
 *
 *	Also will not be asked to handle functions. 
 *
 *	The caller is expected to have done the dow_flush_remove() on id.
 *
 * Calling/Exit State:
 *	dow_mutex held on the way in, but will be dropped before returning.
 *
 */
void
dow_flush(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_STATE(id) == (DOW_MODIFIED | DOW_FLUSH | DOW_PRUNE));
	ASSERT((DOW_STATE(id) & DOW_GONE) != DOW_GONE);
	ASSERT(DOW_ANT_CNT(id) == 0);
	ASSERT((DOW_TYPE(id) == DOW_ISPAGE) || (DOW_TYPE(id) == DOW_ISBUFFER));

	if (DOW_TYPE(id) == DOW_ISPAGE) {
		{
		vnode_t *vp;
		off_t offset;
		/*
		 * XXX:
		 * 	NEED some debug code to test that the page
		 * 	is in core ?
		 */
		DOW_CHEAP_HOLD(id);
		vp = DOW_VP(id);
		offset = DOW_OFFSET(id) & PAGEMASK;
		DOW_TIMESTAMP(id) = (clock_t)(u.u_lwpp);
		DOW_MUTEX_UNLOCK();
		VOP_PUTPAGE(vp, offset, PAGESIZE, B_ASYNC, sys_cred);
		DOW_MUTEX_LOCK();
		if ((DOW_STATE(id) & DOW_FLUSH) &&
		    (DOW_TIMESTAMP(id) == (clock_t)(u.u_lwpp))) {
			/*
			 * The dow structure is still in a FLUSH state,
			 * and no other LWP issued a dow_flush on it since
			 * the dow_flush issued by current LWP.
			 * That must mean that the ASYNC VOP_PUTPAGE
			 * did not actuate an IO, (this can happen if 
			 * either the page was no longer marked dirty or
			 * if the p_pageout bit was set). If the page is
			 * not marked dirty, we should be able to clear
			 * the DOW_MODIFIED bit on the dow, but we don't
			 * know reliably that this is the case.
			 * The safe thing to do is to follow through
			 * with a synchronous putpage request.
			 *
			 * However, if we are the dow_flush_daemon, we
			 * cannot afford to be blocked on a synchronous
			 * page push, since this can be problematic if the
			 * page is covered by more than one dow structure.
			 * (This can be the case if fs_bsize < pagesize).
			 * In that case, we will pass the request on to
			 * the dow_pagesync_daemon to handle.
			 */

			if (u.u_lwpp == lwp_dowflushd) {
				/* 
				 * requeue the dowid, at the tail of the
			         * appropriate flush chain, release the
				 * hold and return.
				 */
				ASSERT((DOW_STATE(id)& DOW_PRUNE) == DOW_PRUNE);
				ASSERT((DOW_STATE(id)& DOW_GONE) == 0);
				ASSERT(DOW_ANT_CNT(id) == 0);
				DOW_STATE(id) &= ~DOW_FLUSH;
				DOW_TIMESTAMP(id) = 0;
				dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
				dow_pagesync_vp = vp;
				dow_pagesync_off = DOW_OFFSET(id);
				dow_pagesync_handoff = B_TRUE;
				SV_BROADCAST(&dow_pagesync_sv, 0);
				dow_rele_l(id);
				DOW_MUTEX_UNLOCK();
				return;
			}

			CHECK_NOTON_ANY_FLUSHCHAIN(id);
			DOW_MUTEX_UNLOCK();
			VOP_PUTPAGE(vp, offset, PAGESIZE, 0, sys_cred);
			DOW_MUTEX_LOCK();
			if ((DOW_STATE(id) & DOW_FLUSH) &&
		    	    (DOW_TIMESTAMP(id) == (clock_t)(u.u_lwpp))) {
				CHECK_NOTON_ANY_FLUSHCHAIN(id);
				DOW_STATE(id) &= ~(DOW_FLUSH|DOW_MODIFIED|
					           DOW_PRUNE);
				DOW_STATE(id) |= DOW_INTRANS;
				dow_iodone(id);
			} 
		}
		dow_rele_l(id);
		DOW_MUTEX_UNLOCK();
		return;
		}
	} else {
		{
		dev_t dev;
		int blkno;
		long bsize;
		buf_t *bp;

		dev = DOW_DEV(id);
		blkno = DOW_BLKNO(id);
		bsize = DOW_BSIZE(id);
		/*
		 * Need a debugging mode function to be able to assert
		 * that the buffer by this identity is in memory.
		 *
		 *
		 * ASSERT(BP_INCORE(dev, blkno, bsize));
		 */
		/* 
		 * As an optimization, we choose to call blookup,
		 * and only if blookup fails, drop the DOW_MUTEX and
		 * call bread. That way we can avoid the DOW_MUTEX
		 * round trip. 
		 */
		if ((bp = blookup(dev, blkno, bsize)) != NULL) {
#ifdef	ASSERTFAIL
			ASSERT((DOW_BP(id) == bp) || (DOW_BP(id) == NULL));
#endif
			DOW_BP(id) = bp;
			bp->b_writestrat = dow_strategy_buf;
			DOW_MUTEX_UNLOCK();
			bawrite(bp);
			return;
		} 
		/*
		 * We don't need to raise the hold on the DOW in order
		 * to ensure that while bread blocks the DOW structure
		 * does not get destroyed/freed. This is because we have
		 * already set the FLUSH state, which will keep dow_rele
		 * and/or dow_iodone from blowing us away. 
		 * However, we would like to initialize the b_writestrat
		 * field after bread completes, so we should deposit a
		 * hold so that we uniformly apply the
		 * "no reference without holding" rule.
		 * 
		 */
		DOW_CHEAP_HOLD(id);
		DOW_MUTEX_UNLOCK();
		bp = bread(dev, blkno, bsize);
		DOW_MUTEX_LOCK();	
		/* 
		 * while the above bread blocked, we had
		 * DOW_FLUSH set: this would cause all
		 * except the flush demon to skip handling
		 * the dow; conversely, if the dow_flush
		 * was called from outside the flush demon,
		 * the demon itself will not interfere with
		 * the caller since the caller has taken the 
		 * dow off leaf chain. 
		 *
		 * this means that an external bwrite is the 
		 * only thing that can race with this dow_flush 
		 * to write it out.
		 *
	 	 * other events that could possibly race
		 * are: dow_abort and dow_clearmod.	
		 *
		 * both will wait while state == DOW_FLUSH,
		 * so effectively we can ignore them.
		 *
		 * WHEN we do wake up out of bread, the whole world
		 * could have changed! So we just go ahead and let
		 * bawrite (dow_strategy_buf) worry about it. We
		 * could, however, test whether MOD, FLUSH bits are
		 * off and bail out with just a brelse and perhaps
		 * a dow_iodone!
		 */
#ifdef	ASSERTFAIL
		ASSERT((DOW_BP(id) == bp) || (DOW_BP(id) == NULL));
#endif
		bp->b_writestrat = dow_strategy_buf;
		DOW_BP(id) = bp;
		dow_rele_l(id);
		DOW_MUTEX_UNLOCK();
		bawrite(bp); /* will call dow_strategy_buf */
		}
	return;
	}
}

/*
 * void
 * dow_ioflush(dowid_t id)
 * 	Complete the deferred IO push for pages/buffers on which a bwrite()
 *	or a VOP_PUTPAGE() operation had been initiated but which could not
 *	be written earlier due to dependencies.
 *	For these ids, we completely trust the bp that is stored in them.
 *	Note that the buffers in this case have not been brelse'd; their
 *	ownership has been implicitly passed on to the dow_flush_daemon.
 *
 * Remarks: 
 *	Handling the race between callers of dow_ioflush and those of
 * 	dow_clearmod and dow_abort:
 *
 *	Since IO setup work has already been done when dow_ioflush
 *	gets called, we must go through with the IO that has already
 *	been set up. To ensure correctness, we force dow_abort and
 *	dow_clearmod to wait until IOSETUP state is turned off before
 *	proceeding to clear DOW_MODIFIED state.
 *
 * THEREFORE, we can assert here that DOW_MODIFIED is ON.
 */
void
dow_ioflush(dowid_t id)
{
#define	DOW_FPMIO	(DOW_FLUSH | DOW_PRUNE | DOW_MODIFIED | DOW_IOSETUP)

	buf_t *bp;

	ASSERT(DOW_MUTEX_OWNED());
	ASSERT((DOW_STATE(id) & DOW_FPMIO) == DOW_FPMIO);
	ASSERT(DOW_ANT_CNT(id) == 0);
	/*
	 * DOW_IOSETUP should never be a state for functions, so we can
	 * assert that id is not a function.
	 */
	ASSERT((DOW_TYPE(id) == DOW_ISBUFFER) || (DOW_TYPE(id) == DOW_ISPAGE));
	ASSERT((DOW_STATE(id) & DOW_INTRANS) == 0);
	ASSERT(DOW_HOLD(id) > 0);
	ASSERT(on_some_flushchain(id) == B_FALSE);

	/*
	 * XXX:
	 *	would like to verify that if this is a buffer cache write,
 	 *	then that (a) no one (except may be self) owns the bp and 
	 *	that (b) bp is not on the freelist.
	 */

	/* 
	 * before we issue the IO, we need to handle the possibility
	 * of this id being aborted. if the id has been aborted, we would 
	 * not want to turn off the DOW_PRUNE state. We still submit the
 	 * IO that has already been setup.
	 */
	DOW_TIMESTAMP(id) = INT_MAX;

	switch(DOW_TYPE(id)) {
		case DOW_ISPAGE:
			/*
			 * TBD: handle blocksize < pagesize (nio > 1)
			 * 	cases, by having the buffer chain available
			 *	in the dow structure. To do this, first
			 * 	move the buffer chain out of the dow
			 * 	structure, and then issue the strategy
			 * 	calls one-by-one.
			 *
			 *	As each io completes, dow_pgiodone will be
			 *	called; when the last dow_pgiodone is called,
			 *	dow_iodone will be called on the dowid.
			 */
			bp = dow_buffer(id);
			ASSERT(bp != NULL);
			if (DOW_STATE(id) & DOW_GONE) {
				DOW_MUTEX_UNLOCK();
				(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
				break;
			}
			if (dow_startmod_trywrlock_l(id) ||
					pagesync_modlock_dowid == id) {
				DOW_STATE(id) = DOW_INTRANS;
				dow_drop_modlock_l(id);
				if (pagesync_modlock_dowid == id)
					pagesync_modlock_dowid == DOW_BADID;
				DOW_MUTEX_UNLOCK();
				(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
				break;
			}
			if (bp->b_flags & B_ASYNC) {
				DOW_STATE(id) &= ~DOW_FLUSH;
				dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
				DOW_TIMESTAMP(id) = 0;
				DOW_MUTEX_UNLOCK();
				break;
			} 
			dow_startmod_wrlock_l(id);
			/*
			 * if we block in startmod_wrlock call
		 	 * dow could get aborted meanwhile. 
		 	 */ 
			if (DOW_STATE(id) & DOW_GONE) {
				DOW_STATE(id) = (DOW_GONE | DOW_PRUNE
							 | DOW_INTRANS);
			} else {
				DOW_STATE(id) = DOW_INTRANS;
				dow_drop_modlock_l(id);
			}
			DOW_MUTEX_UNLOCK();
			(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
			break;
		case DOW_ISBUFFER:
			bp = dow_buffer(id);
			if (DOW_STATE(id) & DOW_GONE) {
				/* assert that GONE => PRUNE */
				ASSERT(DOW_STATE(id) & DOW_PRUNE);
				DOW_STATE(id) = (DOW_GONE | DOW_PRUNE | DOW_INTRANS);
			} else {
				DOW_STATE(id) = DOW_INTRANS;
			} 

			ASSERT(bp != NULL);
			DOW_MUTEX_UNLOCK();
			(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
			break;
	}
	/*
	 * stat keeping?
	 */
	return;
}

/*
 * void
 * dow_flush_headins(dowid_t id, dowid_t fl_head)
 *	Insert the dow "id" at the head of the indicated flush chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and held on return. The function
 *	does not block or drop the lock. 
 *
 * Remarks:
 *	Caller has responsibility to modulate timestamp as appropriate.
 *
 * PERF:
 *	Convert to a macro. Preferrably assembler macro.
 */
void
dow_flush_headins(dowid_t id, dowid_t fl_head)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	DOW_FLUSHNEXT(id) = DOW_FLUSHNEXT(fl_head);
	DOW_FLUSHPREV(id) = fl_head;
	DOW_FLUSHNEXT(fl_head) = id;
	DOW_FLUSHPREV(DOW_FLUSHNEXT(id)) = id;
	DOW_DEBUG_FLUSHENTER(id, fl_head);
}
/*
 * void
 * dow_flush_tailins(dowid_t id, dowid_t fl_head)
 *	Insert the dow "id" at the tail of the flush chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and held on return. The function
 *	does not block or drop the lock. 
 *
 * Remarks:
 *	Caller has responsibility to modulate timestamp as appropriate.
 *
 * PERF:
 *	Convert to a macro. Preferrably assembler macro.
 */
void
dow_flush_tailins(dowid_t id, dowid_t fl_head)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	DOW_FLUSHNEXT(id) = fl_head;
	DOW_FLUSHPREV(id) = DOW_FLUSHPREV(fl_head);
	DOW_FLUSHPREV(fl_head) = id;
	DOW_FLUSHNEXT(DOW_FLUSHPREV(id)) = id;
	DOW_DEBUG_FLUSHENTER(id, fl_head);
}

/*
 * void
 * dow_flush_remove(dowid_t id)
 *	Remove the dow "id" from the flush chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and held on return. The function
 *	does not block or drop the lock. 
 *
 * PERF:
 *	Convert to a macro. Preferrably assembler macro.
 */

void
dow_flush_remove(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	ASSERT(on_some_flushchain(id));

	DOW_FLUSHPREV(DOW_FLUSHNEXT(id)) = DOW_FLUSHPREV(id);
	DOW_FLUSHNEXT(DOW_FLUSHPREV(id)) = DOW_FLUSHNEXT(id);

	DOW_DEBUG_FLUSHLEAVE(id);
}

/*
 * void
 * dowinit(void)
 *      Initialize the dow subsystem.
 *
 * Calling/Exit State:
 *      Should only be called during system startup - no locking issues.
 *
 * Description:
 *      Initializes the dow data structures.
 */
void
dowinit(void)
{
        dow_arrayinit();        /* initialize the dow array */
}

/*
 * void
 * dowpostroot(void)
 *      Post-root initializations.
 *
 * Calling/Exit State:
 *      Called from main() after mounting root.
 */
void
dowpostroot(void)
{

        (void) spawn_lwp(NP_SYSPROC, &dow_flush_lwpid, LWP_DETACHED, NULL,
                         dow_flush_daemon, NULL);

	(void) spawn_lwp(NP_SYSPROC, &dow_pagesync_lwpid, LWP_DETACHED, NULL,
			dow_pagesync_daemon, NULL);
}
