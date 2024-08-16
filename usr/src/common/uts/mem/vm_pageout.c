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
 * 		All rights reserved.
 *  
 */

#ident	"@(#)kern:mem/vm_pageout.c	1.42"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/fs_subr.h>		/* temporarily for fs_nosys */
#include <fs/vnode.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/tuneable.h>
#include <mem/vmmeter.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern page_t *page_dirtyflist;
extern page_t *page_dirtyalist;

lwp_t *lwp_pageout;	/* LWP of the pageout daemon */

STATIC int pushes;  /* number of pages pushed on any one run of pageout */

int mem_lotsfree[NPAGETYPE];	/* threshold values for lotsfree */
int mem_minfree[NPAGETYPE];	/* threshold values for minfree */
int mem_desfree[NPAGETYPE];	/* threshold values for desfree */

/*
 * PERF: Should be revisited at a later time for manipulation of all these
 * tunables at a later time.
 */

#define	MAX_PUSHES		20	/* Max pages pushed at one time during 
					 * pageout. */
#define MAX_LOOKS		25	/* Max pages that are going to be scanned */
#define MIN_PAGE_AGE_TIME	1	/* Aging interval when freemem < minfree */
#define DES_PAGE_AGE_TIME	2	/* Aging interval when freemem < desfree */
#define LOTS_PAGE_AGE_TIME	3	/* Aging interval when freemem < lotsfree */

/*
 * MAX_PUSHES pages must not exceed MAXBIOSIZE
 */
#if MAX_PUSHES > (MAXBIOSIZE >> PAGESHIFT)
#undef	MAX_PUSHES
#define	MAX_PUSHES	btop(MAXBIOSIZE)
#endif

event_t	  pageout_event;	/* event pageout waits on */ 

#define	RATETOSCHEDPAGING	4	/* HZ that is */

/*
 * void pageout_init(void)
 *	Called by kvm_init. Sets lotsfree, minfree, desfree.
 *	
 * Calling/Exit State:	None.
 *
 */
void
pageout_init(void)
{
	int type;

	/*
	 * Set up thresholds for paging:
	 */

	for (type = 0; type < page_ntype_used; type++) {
		/*
		 * Lotsfree is threshold where paging daemon turns on.
		 * It has an upper bound based on a fraction of maxfreemem.
		 */
		mem_lotsfree[type] = LOTSFREE / MMU_PAGESIZE;
		if (mem_lotsfree[type] > maxfreemem[type] / LOTSFREEFRACT)
			mem_lotsfree[type] = maxfreemem[type] / LOTSFREEFRACT;

		/*
		 * Desfree is amount of memory desired free.
		 * If less than this for extended period, do swapping.
		 */
		mem_desfree[type] = DESFREE / MMU_PAGESIZE;
		if (mem_desfree[type] > maxfreemem[type] / DESFREEFRACT)
			mem_desfree[type] = maxfreemem[type] / DESFREEFRACT;

		/*
		 * Minfree is minimal amount of free memory which is tolerable.
		 */
		mem_minfree[type] = MINFREE / MMU_PAGESIZE;
		if (mem_minfree[type] > mem_desfree[type] / MINFREEFRACT)
			mem_minfree[type] = mem_desfree[type] / MINFREEFRACT;
	}

	EVENT_INIT(&pageout_event);
}

/*
 * STATIC void schedpaging(void *arg)
 *	Wakes up periodically (RATETOSCHEDPAGING) to check the memory
 *	condition and wakes up pageout if freemem < lotsfree.
 *
 * Calling/Exit State:	None.
 */
/* ARGSUSED */
STATIC void
schedpaging(void *arg)
{
	static int nskipped = 0;

	/*
	 * Wakeup the pageout daemon if:
	 *
	 *	There are pages on dirty alist or dirty flist that can
	 *	be pushed out
	 *
	 *		AND EITHER
	 *
	 *	freemem is less than lotsfree,
	 *
	 *		OR
	 *
	 *	mem_avail_state is MEM_AVAIL_PLENTY and pageout has not
	 *	run in 5 seconds,
	 *
	 *		OR
	 *
	 *	mem_avail_state is tighter than MEM_AVAIL_PLENTY.
	 */
	++nskipped;
	if (page_dirtyflist == NULL && page_dirtyalist == NULL)
		return;
	if (freemem >= lotsfree) {
		switch(mem_avail_state) {
		case MEM_AVAIL_EXTRA_PLENTY:
			return;
		case MEM_AVAIL_PLENTY:
			if (nskipped < 5 * RATETOSCHEDPAGING)
				return;
			break;
		default:
			break;
		}
	}
	EVENT_SIGNAL(&pageout_event, 0);
	nskipped = 0;
}

/*
 * void start_schedpaging(void)
 *	Kick off pageout periodic timeout
 *
 * Calling/Exit State:	None.
 */
void
start_schedpaging(void)
{
	if (itimeout(schedpaging, NULL,
		     (HZ / RATETOSCHEDPAGING) | TO_PERIODIC, PLTIMEOUT) == 0) {
		/*
		 *+ Could not set up a timeout for scheduling the pageout
		 *+ daemon.  This is probably due to insufficient memory.
		 */
		cmn_err(CE_PANIC, "Can't get timeout for pageout daemon");
		/* NOTREACHED */
	}
}

/*
 * void pageout(void *pageout_argp)
 *	The pageout daemon.
 *
 * Calling/Exit State:
 *	Called once directly from main after spawning off pageout process.
 *	Otherwise never called directly. Never exits.
 *
 * Description:	
 *	It looks at the dirtyflist and dirtyalist and cleans out pages
 *	that are eligible to be cleaned. Eligibility depends on whether
 *	the page is older than the aging interval (page_age_time).
 *	Only MAX_PUSHES pages will be pushed out during any one run of
 *	pageout.
 *
 *	A vnode/offset cachelist is maintained since we cannot hold the
 *	vm_pgidlk when calling VOP_PUTPAGE(). We call anon_pageout() to clean
 *	dirty anon pages.
 */
/* ARGSUSED */
void
pageout(void *pageout_argp)
{
	int count, i;
	page_t *pp;
	clock_t	page_age_time;	/* page aging time */
	struct vpoff {
		vnode_t *vp;
		off_t	off;
	} vpoff_cachelist[MAX_PUSHES];
	int minfreemem;
	int type;
	vnode_t *vp;

	u.u_lwpp->l_name = "pageout";
	lwp_pageout = u.u_lwpp;
	u.u_flags |= U_CRITICAL_MEM;

loop:
	/*
	 * Look to see if there are any I/O headers in the ``cleaned''
	 * list that correspond to dirty pages that have been pushed
	 * asynchronously. If so, empty the list by calling cleanup().
	 * We don't hold the buf_async_lock here to check for bclnlist here,
	 * since an approximate look is good enough.
	 */
	if (bclnlist != NULL)
		cleanup();

	/* 
	 * Set the ageing interval for the pages to be pushed
	 * depending on the memory condition.  
	 */
	minfreemem = freemem;
	if (minfreemem < minfree)
		page_age_time = MIN_PAGE_AGE_TIME * HZ;
	else if (minfreemem < desfree)
		page_age_time = DES_PAGE_AGE_TIME * HZ;
	else /* minfreemem >= desfree */
		page_age_time = LOTS_PAGE_AGE_TIME * HZ;

	/*
	 * Initialize pages pushed to zero. We limit pushes to MAX_PUSHES
	 * so that we don't stress the I/O bandwidth.
	 */
	pushes = 0;
	count = 0;
	/*
	 * We have a choice here to make : Look at dirtyalist first,
	 * dirtyflist first or interleave between them. There were
	 * really no clear winners any way you go. So we look at
	 * dirtyflist first. We realize that in some situations
	 * we may not be looking at the dirtyalist for a long time
	 * but it doesn't matter much as long as we are freeing some
	 * pages up.
	 */
	/* Hold the vm_pgfreelk here to mutex the dirtyflist */
	VM_PAGEFREELOCK();
	if (page_dirtyflist == NULL) {  /* no dirty file pages */
		VM_PAGEFREEUNLOCK();
		goto anon_list;
	}
	pp = page_dirtyflist;
	/* 
	 * Loop through the dirty file list until we have pushed out
	 * MAX_PUSHES pages or we have looked at MAX_LOOKS pages
	 * an has not been able to free MAX_PUSHES pages.
	 */
	while (pushes < MAX_PUSHES && (count < MAX_LOOKS || pushes != 0)) {
		ASSERT(!PAGE_IN_USE(pp));
		ASSERT(pp->p_free);
		ASSERT(!pp->p_invalid);
		ASSERT(pp->p_vnode);
		/* Page has aged ? */
		if (IS_PAGE_AGED(pp, lbolt, page_age_time)) {
			/*
			 * We need to establish a SOFTHOLD on the
			 * vnode to prevent it from disappearing
			 * during the VOP_PUTPAGE operation.
			 */
			vp = pp->p_vnode;
			VN_SOFTHOLD(vp);
			vpoff_cachelist[pushes].vp = vp;
			vpoff_cachelist[pushes].off = pp->p_offset;
			pushes++;
		}
		count++;
		/*
		 * Have we have searched the whole list? We have to
		 * search the whole list since the pages are not ordered by
		 * timestamps. This is because pages could have already been
		 * timestamped by fsflush before page free could page_add
		 * to the free list. This is not the case for anon pages.
		 */	
		if ((pp = pp->p_next) == page_dirtyflist) 
			break; 
	}
	VM_PAGEFREEUNLOCK();
	/*
	 * push out all the pages we have collected. We don't check for 
	 * the error return for VOP_PUTPAGE here. Can't do much here 
	 * anyway
	 */
	for (i = 0; i < pushes; i++) {
		(void)VOP_PUTPAGE(vpoff_cachelist[i].vp,
					vpoff_cachelist[i].off, PAGESIZE,
					B_ASYNC | B_PAGEOUT, sys_cred);
		if ((void *)vpoff_cachelist[i].vp->v_op->vop_release ==
		    (void *)fs_nosys) {
			VN_RELE(vpoff_cachelist[i].vp); /* Temporary support */
		} else {
			VN_SOFTRELE(vpoff_cachelist[i].vp);
		}
	}
anon_list:
	if (pushes < MAX_PUSHES) {
		/*
		 * We may be doing something along these lines with the pushes
		 * value in the future: If this value is too low (say 1 or 2)
		 * then it means we are freeing a low number of pages. We may
		 * want to run pageout less frequently.
		 */
		pushes += anon_pageout(MAX_PUSHES - pushes, lbolt, page_age_time);
	}

	MET_PGOUT(pushes);

	/*
	 * Call cleanup here again hoping there may be async I/Os completed in
	 * the system in the meanwhile. Also it's only waste of a conditional
	 * check.
	 */
	if (bclnlist != NULL)
		cleanup();
	EVENT_CLEAR(&pageout_event);
	EVENT_WAIT(&pageout_event, PRIMEM - 1);
	goto loop;
}

