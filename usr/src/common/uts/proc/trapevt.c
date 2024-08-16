/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/trapevt.c	1.7"

#include <proc/proc.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>

/*
 *
 * void trapevnudge(lwp_t *lwpp, boolean_t forcerun)
 *	Nudge the target LWP. If "forcerun" is B_TRUE, then the LWP
 *	will be forced into the SRUN state if found in the SSTOP state.
 *
 * Calling/Exit State:
 *	The l_mutex lock of the target LWP must be held upon entry.
 *	In addition, if "forcerun" is B_TRUE, then p_mutex of the process
 *	containing the target LWP must also be held by the caller.
 *	These locks remain held upon return.
 */
void
trapevnudge(lwp_t *lwpp, boolean_t forcerun)
{
	switch (lwpp->l_stat) {
	case SSLEEP:
		if ((lwpp->l_flag & L_NWAKE) == 0) {
			/*
			 * The signallable trap event appears as an 
			 * unmasked signal.
			 */
			lwpp->l_flag |= L_SIGWOKE;
			setrun(lwpp);
		}
		break;

	case SRUN:
		/*
		 * We might race with the dispatching engine, and lose 
		 * the race.  The LWP may go from the SRUN state to 
		 * the SONPROC state (even though we hold l_mutex).  This
		 * race is harmless, because the LWP can not progress 
		 * further until it  obtains the l_mutex lock that we 
		 * hold.  The execution context of the LWP in either 
		 * the SRUN  state, or the SRUN->SONPROC state, and 
		 * it is not necessary to nudge it.
		 */
		break;  /* nothing to do */

	case SIDL:
		break;  /* nothing to do */

	case SSTOP:
		if (forcerun) {
			/*
			 * We setrun() the LWP if a SIGKILL signal has
			 * been sent, or an EVF_PL_DESTROY trap event is
			 * being posted.  This is indicated by the
			 * forcerun argument.
			 */
			FORCERUN_FROMSTOP(lwpp, lwpp->l_procp);
			PROC_NOWAIT(lwpp->l_procp);
		} else {
			/*
			 * If the target LWP is in a suspended state and
			 * its trap event flags include EVF_PL_RENDEZV,
			 * we must setrun() the LWP so it can join the
			 * rendezvous.  It will return to the suspended
			 * state after rendezvousing.
			 */
			if ((lwpp->l_flag & L_SUSPENDED) &&
			    !(lwpp->l_flag & L_PRSTOPPED) &&
			    (lwpp->l_trapevf & EVF_PL_RENDEZV)) {
				setrun(lwpp);
				lwpp->l_procp->p_nstopped--;
			}
		}
		break;

	case SONPROC:
		if (lwpp != u.u_lwpp) {
			RUNQUE_LOCK();  /* must hold runque lock for nudge() */
			nudge(PRIMED, lwpp->l_eng);
			RUNQUE_UNLOCK();
		}
		break;

	default:
		UNLOCK(&lwpp->l_mutex, PLHI);
		/*
		 *+ The specified LWP in trapevnudge() 
		 *+ has an l_stat value that is invalid.
		 */
		cmn_err(CE_PANIC, "trapevnudge: bad l_stat=%d, lwp=%x\n",
			(int)lwpp->l_stat, (long)lwpp);
	}
}


/*
 *
 * void trapevproc(proc_t *pp, u_int trap_events, boolean_t all)
 * 	Post the given trap event flags to all LWPs in the specified 
 *	process (all == B_TRUE), or to all LWPs in the process except
 *	the calling LWP (all == B_FALSE and pp references the calling
 *	process).
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must be held
 *	by the caller.  It remains held upon return.
 *	The l_mutex lock of the calling LWP must not be held upon
 *	entry if the LWP is part of the specified process and
 *	and trap event flag is to be posted to all LWPs
 *	int the process.
 */
void
trapevproc(proc_t *pp, u_int trap_events, boolean_t all)
{
	lwp_t *lwpp;
	lwp_t *last_lwpp;
	u_int events_mask;
	boolean_t forcerun;

	events_mask = (u_int) ~0;
	if (trap_events & QUEUEDSIG_FLAGS) {
		/*
		 * At least one of the trap events being posted
		 * is of the "signallable" variety.
		 */
		forcerun = B_FALSE;
		if ((trap_events & EVF_PL_DESTROY) != 0) {
			/*
			 * LWPs are to be destroyed. they should
			 * forced to run, if in the SSTOP state.
			 */
			forcerun = B_TRUE;
			if ((trap_events & EVF_PL_RENDEZV) == 0) {
				/*
				 * Previously pending EVF_PL_RENDEZV event
				 * flag must be cleared unless EVF_PL_RENDEZV 
				 * is specified in trap_events.  Otherwise,
				 * a partially complete rendezvous operation
				 * when interrupted by a destroy operation
				 * will leave the process in an inconsistent
				 * state (LWPs will rendezvous,
				 * when they should instead be destroyed).
				 * Set the events_mask accordingly.
				 */
				events_mask = (u_int) ~EVF_PL_RENDEZV;
			}
		}
		if (all) {
			/*
			 * ALL LWPs in the process are to receive the trap
			 * event flags.
			 */
			lwpp = (pp == u.u_procp) ? u.u_lwpp : pp->p_lwpp;
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf = (lwpp->l_trapevf & events_mask)
					| trap_events;
			trapevnudge(lwpp, forcerun);
			UNLOCK(&lwpp->l_mutex, PLHI);
		} else {
			/*
			 * All LWPs in the process except the caller are to
			 * receive the trap event flags.
			 */
			ASSERT(pp == u.u_procp);
			lwpp = u.u_lwpp;
		}
		/*
		 * Post the signallable trap event flags to all other
		 * LWPs in the process.
		 */
		last_lwpp = lwpp;
		while ((lwpp = lwpp->l_prev) != last_lwpp) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf = (lwpp->l_trapevf & events_mask)
					| trap_events;
			trapevnudge(lwpp, forcerun);
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
	} else {
		/*
		 * None of the trap events being posted are of the
		 * "signallable" variety.
		 */
		if (all) {
			/* All LWPs receive the trap event flags */
			lwpp = pp->p_lwpp;
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf |= trap_events;
			UNLOCK(&lwpp->l_mutex, PLHI);
		} else {
			/*
			 * All LWPs except the caller receive the 
			 * trap event flags.
			 */
			ASSERT(pp == u.u_procp);
			lwpp = u.u_lwpp;
		}

		/*
		 * Post the non-signallable trap event flags.
		 */
		last_lwpp = lwpp;
		while ((lwpp = lwpp->l_prev) != last_lwpp) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf |= trap_events;
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
	}
}


/*
 *
 * void trapevunproc(proc_t *pp, u_int trap_events, boolean_t all)
 * 	Remove the given trap event flags for all LWPs in the specified 
 *	process (all == B_TRUE), or from all LWPs in the process except
 *	the calling LWP (all == B_FALSE and pp references the calling
 *	process).
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must be held
 *	by the caller.  It remains held upon return.
 */
void
trapevunproc(proc_t *pp, u_int trap_events, boolean_t all)
{
	register lwp_t *lwpp;
	register lwp_t *last_lwpp;

	trap_events = ~trap_events;
	if (all) {
		/*
		 * ALL LWPs in the process are to have the given
		 * set of trap event flags cancelled.
		 */
		lwpp = pp->p_lwpp;
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= trap_events;
		UNLOCK(&lwpp->l_mutex, PLHI);
	} else {
		/*
		 * All LWPs except the caller have the 
		 * trap event flags cancelled.
		 */
		ASSERT(pp == u.u_procp);
		lwpp = u.u_lwpp;
	}

	last_lwpp = lwpp;
	while ((lwpp = lwpp->l_prev) != last_lwpp) {
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= trap_events;
		UNLOCK(&lwpp->l_mutex, PLHI);
	}
}
