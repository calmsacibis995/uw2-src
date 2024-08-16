/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fsflush.c	1.31"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/fs_subr.h>		/* temporarily for fs_nosys */
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/tuneable.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

STATIC void	fsflush_pagesync();

extern void	bdflush();

/* event for fsflush deamon signalled from clock.c */

event_t fsflush_event;	

static int fsf_toscan;		/* subset of the pagepool to be scanned */
static page_t *fsf_next_page;	/* next page for fsflush to scan */
static clock_t fsf_age_time;	/* aging interval for the dirty pages */

/*
 * void
 * fsflush(void *argp)
 *	Fsflush daemon.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, and it never exits.
 *
 * Description:
 *	As part of file system hardening, this daemon is woken
 *	every tune.t_fsflushr seconds to flush cached data which includes the
 *	buffer cache, the inode cache and mapped pages.
 *
 *	We should be awakened more frequently than the time we have to
 *	clean a dirty page, so we don't have to do all the work in one pass.
 *	Therefore v.v_autoup should be > tune.t_fsflushr, but we tolerate it
 *	if it's not, in case an administrator configures crazily.
 */
/* ARGSUSED */
void
fsflush(void *argp)
{
	ulong_t autoup;
	extern void sync(int);

	ASSERT(tune.t_fsflushr > 0);
	ASSERT(v.v_autoup > 0);

	u.u_lwpp->l_name = "fsflush";

	/* EVENT_INIT(&fsflush_event) is done in clock_init(). */

	autoup = v.v_autoup * HZ;

	/*
	 * Number of pages to be scanned is calculated based on 
	 * the time interval we are waking up this daemon and on
	 * v.v_autoup. We will be looking at the same page every
	 * v.v_autoup/2 seconds rather than v.v_autoup seconds since we need
	 * this to maintain the guarantee that we flush the page within
	 * v.v_autoup seconds. The following can be better read as the 
	 * following formula:
	 *     ((epages - pages) * tune.t_fsflushr) / (v.v_autoup / 2)
	 * but we multiply by 2 to get better rounding.
	 */
	fsf_toscan = ((epages - pages) * tune.t_fsflushr * 2) / v.v_autoup;

	fsf_next_page = pages;

	/*
	 * PERF: Scan rate can be adjusted more/less frequently thereby
	 * adjusting more/less the ageing interval.
	 * v.v_autoup is the number of seconds before which a dirty page
	 * would be synced. However, we sync a page back even if the page
	 * has aged greater than half the v_autoup seconds. We have to do
	 * this to maintain the guarantee that we flush the page within autoup
	 * seconds.
	 */
	fsf_age_time = autoup / 2;

	for (;;) {
		ASSERT(KS_HOLD0LOCKS());
		EVENT_WAIT(&fsflush_event, PRIBUF);

	/*
	 * Flush all delayed-write buffers from the buffer cache
	 * that are older than autoup.
	 */
		bdflush(autoup);
	/*
	 * Scan a portion of the pagepool looking for dirty pages to flush.
	 * With multiple passes, we'll scan all of the pages within autoup/2.
	 */
		fsflush_pagesync();
	/*
	 * Flush the inode cache by calling each installed file
	 * system type's vfs_sync() with SYNC_ATTR option.
	 * The frequency of inode flushing is called every second.
	 * It depends on the dependent file system level to decide
	 * when to flush out.
	 * The vfs_sync() searches the inode list and commits
	 * modified inodes to stable storage.
	 */
		sync(SYNC_ATTR);
	}
}

#define ADVANCE_PP(pp) {	\
	if (++(pp) >= epages)	\
		(pp) = pages;	\
}

/*
 * void
 * fsflush_pagesync(void)
 *	Syncs the page cache.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *	This function flushes all the file pages that have been modified
 *	and have remained dirty for no more than v.v_autoup seconds. We 
 *	scan fsf_toscan subset of pages in the pagepool. We traverse this
 * 	subset, PAGE_USELOCK each of the pages and VOP_PUTPAGE the
 *	pages asynchronously.
 */
STATIC void
fsflush_pagesync(void)
{
	u_int	i;
	vnode_t *vp;
	off_t off;

	for (i = fsf_toscan; i != 0; i--) {
		/* 
		 * We should get the PAGE_USELOCK() here but we optimize for
		 * the anonfs case. If this page identity is going to change
		 * then we will catch it the next time around.
		 */	 
		if (((vp = fsf_next_page->p_vnode) == NULL) ||
		    (vp->v_flag & VSWAPBACK)) {
			ADVANCE_PP(fsf_next_page);
			continue;
		}
		PAGE_USELOCK(fsf_next_page);
		/* recheck the page identity - still a file page? */
		if (((vp = fsf_next_page->p_vnode) != NULL) &&
		    !(vp->v_flag & VNOSYNC)) {
			/* it's a file page */
			hat_pagegetmod(fsf_next_page);
			if (fsf_next_page->p_mod) {
			    /* 
			     * If the timestamp is 0, then it means that
			     * we are not on the free list or we got here
			     * before page_free did.
			    */
			    if (fsf_next_page->p_timestamp == 0)
				fsf_next_page->p_timestamp = lbolt;

			    else if (IS_PAGE_AGED(fsf_next_page, lbolt,
							fsf_age_time)) {
				    /*
				     * We need to establish a SOFTHOLD on the
				     * vnode to prevent it from disappearing
				     * during the VOP_PUTPAGE operation.
				     *
				     * Save the page offset before releasing
				     * the PAGE_USELOCK.
				     */
				    off = fsf_next_page->p_offset;
				    VN_SOFTHOLD(vp);
				    PAGE_USEUNLOCK(fsf_next_page);
				    VOP_PUTPAGE(vp, off, PAGESIZE,
						B_ASYNC, sys_cred);
				    VN_SOFTRELE(vp);
				    ADVANCE_PP(fsf_next_page);
				    continue;
			    }
			}	/* if p_mod bit set */
		}	/* if file page */
		PAGE_USEUNLOCK(fsf_next_page);
		ADVANCE_PP(fsf_next_page);
	} /* for loop */
}
