/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*      Copyright (c) 1990 UNIX System Laboratories, Inc.       */
/*      Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T   */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*      UNIX System Laboratories, Inc.                          */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

#ident	"@(#)kern:util/setrun.c	1.12"

#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/disp.h>
#include <proc/usync.h>
#include <util/debug.h>
#include <proc/class.h>


/*
 *
 * void setrun(lwp_t *lwpp) 
 *	Set the target LWP running.
 *
 *	The target LWP must not already be in the SRUN state, unless
 *	EVF_PL_SEIZE was set (and has just been cleared by the caller
 *	by a vm_unseize() request).
 *
 * Calling/Exit State:
 *	The l_mutex lock of the LWP to be set running must be held by
 *	the caller.  It remains held upon return.
 *
 */

void
setrun(lwp_t *lwpp)
{

	ASSERT(LOCK_OWNED(&lwpp->l_mutex));
	ASSERT(lwpp->l_stat != SIDL);

	/*
	 * The most common case of setrun() invocation is for when the
	 * LWP is in the SSLEEP state.  Hence, we test for this case
	 * first....
	 */

	if (lwpp->l_stat == SSLEEP) {

		switch (lwpp->l_stype) {

		case ST_COND:
			if (! SV_UNSLEEP(lwpp->l_syncp, lwpp)) {

				/*lost the race to a normal wakeup*/
				return;
			}
			
			/* LWP was dequeued */
			break;

		case ST_EVENT:
                        if (! EVENT_UNSLEEP(lwpp->l_syncp, lwpp)) {                
                                /*lost the race to a normal wakeup*/
                                return;
                        }
                        
                        /* LWP was dequeued */
                        break;

		case ST_SLPLOCK:
                        if (! SLEEP_UNSLEEP(lwpp->l_syncp, lwpp)) {                
                                /*lost the race to a normal wakeup*/
                                return;
                        }
                        
                        /* LWP was dequeued */
                        break;

		case ST_USYNC:
			if (! usync_unsleep(lwpp->l_syncp, lwpp)) {
				return;
			}
			break;

		default:
			/* Should not occur */
			/*
			 *+ The setrun function tried to unsleep an LWP
			 *+ sleeping on an unknown type of sleep object.
			 *+ This indicates a kernel software problem.
			 */
			cmn_err(CE_PANIC, "setrun(): unknown sleep object\n");
		}

	} else if (lwpp->l_stat == SSTOP) {

		if (lwpp->l_flag & (L_JOBSTOPPED|L_PRSTOPPED)) {
			/*
			 * The LWP is stopped for job control and/or
			 * /proc.  Return without disturbing the LWP
			 * (l_mutex remains held).
			 */
			return;
		}

		/* 
		 * Note that clearing of p_wcode and p_wdata was acomplished in
		 * dbg_setrun.
		 *
		 */

	} else if (lwpp->l_stat == SRUN) {

		/*
		 * Either we're being redundantly setrun() (should not
		 * happen), or we were setrun() before, but were blocked
		 * from actually entering execution because we were
		 * seized via EVF_PL_SEIZE.
		 */

		ASSERT((lwpp->l_trapevf & EVF_PL_SEIZE) == 0);
							/* Redundant setrun */

		/*
		 * We were blocked from actually entering execution because
		 * we were previously seized via EVF_PL_SEIZE.  Let the class
		 * specific code place the LWP on the dispatcher queue,
		 * and awake the swapper if necessary. Note that CL_SERUN()
		 * will handle the seize flag.
		 */
		CL_SETRUN(lwpp, lwpp->l_cllwpp);    /* LWP state lock held */
		return;				    /* l_mutex still held */

	} else {

		ASSERT(lwpp->l_stat != SONPROC);

	}

	/*
	 * Change the state of the LWP to runnable, though it may still
	 * be blocked from execution because of a seize request.
	 */

	lwpp->l_stat = SRUN;
	lwpp->l_stype = ST_NONE;		/* no sync type */
	lwpp->l_syncp = NULL;
	lwpp->l_whystop = 0;
	lwpp->l_whatstop = 0;

	/*
	 * Let the class specific code place the LWP on the
	 * dispatcher queue. Note that CL_SETRUN will handle the 
	 * seize flag.
	 */

	CL_SETRUN(lwpp, lwpp->l_cllwpp);	/* LWP state lock held */

	/* Return with l_mutex held */
}
