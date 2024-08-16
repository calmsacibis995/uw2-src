/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)kern:proc/seize.c	1.8"
#ident	"$Header: $"

/*
 * Seize operations, currently consisting only of vm_seize and vm_unseize.
 */
#include <util/types.h>
#include <util/ksynch.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/disp.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/systm.h>

/*
 * boolean_t vm_seize(procp)
 *	Seize the processor resource from all LWPs in the target process,
 *	except the calling LWP if the calling LWP resides in the target
 *	process.
 *
 * Calling/Exit State:
 *	procp is a pointer to the process to be seized.
 *
 *	Returns:
 *		B_TRUE	if the process was successfully seized.
 *		B_FALSE if the process has already reached the point in
 *			exit processing where it can no longer be seized,
 *			or the calling LWP resided in the target process and
 *			a seize operation was previously in progress.
 *			(Returns B_TRUE to the swapper unless the process
 *			is exiting.)
 *
 *	The caller must hold the p_mutex lock of the process upon entry.
 *	This lock is released upon return at PLBASE.
 *	No other spin locks can be held by the caller, as this function may
 *	block.
 *
 * Remarks:
 *	The semantics of this primitive depend upon whether or not the
 *	calling LWP resides in the target process:
 *	
 *	1) Caller is part of the process being seized.  (The normal VM page
 *	   aging case.)  Semantics are:
 *
 *		if (my process is not currently being seized)
 *			seize all LWPs except me
 *		else
 *			submit self to the seize.
 *			when unseized, return B_FALSE
 *		fi.
 *
 *	   If B_FALSE is returned, then the caller must back out (the page
 *	   aging was performed by some other agent; the swapper or another
 *	   LWP in the caller's process has already done the page aging).
 *
 *	2) Caller is not part of the process being seized.  (The caller is
 *	   the swapper.)  Semantics are:
 *	
 *		Seize the process even if it's currently being seized.  The
 *		swapper may want to swap the entire address space and u-blocks
 *		even if the process is already releasing part of its address
 *		space.
 */
boolean_t
vm_seize(proc_t *procp)
{
	lwp_t *mylwpp;
	lwp_t *lwpp;

	/*
	 * We should hold exactly one lock, the p_mutex of the process to
	 * be seized.
	 */
	ASSERT(KS_HOLD1LOCK());
	ASSERT(LOCK_OWNED(&procp->p_mutex));

	if (procp->p_flag & P_NOSEIZE) {
		/*
		 * The process' address space is being freed in exit, we
		 * cannot seize it.
		 */
		UNLOCK(&procp->p_mutex, PLBASE);
		return (B_FALSE);
	} else if (procp->p_flag & P_SEIZE) {
		/*
		 * A previous caller to vm_seize() has the process seized,
		 * or is seizing the process now.  It is assumed that the
		 * swapper will not issue concurrent seize requests on the
		 * same process, so if we are not in the target process,
		 * block to obtain access to the process (swapper case).
		 * Otherwise (page stealing case), yield to the swapper's
		 * seize request or the seize request of another LWP in the
		 * process.
		 */
		if (procp == u.u_procp) {
			/*
			 * page ageing case; yield
			 */
			UNLOCK(&procp->p_mutex, PLBASE);
                        CL_PREEMPT(u.u_lwpp, u.u_lwpp->l_cllwpp);
			return (B_FALSE);
		}
		/*
		 * We are the swapper.  Queue to get the seized process.
		 */
		SV_WAIT(&procp->p_wait2seize, PRIMEM, &procp->p_mutex);
	} else {
		/*
		 * No previous caller to vm_seize() has the process seized.
		 * Mark the process for seizing and tell every LWP to seize
		 * itself (if we can't seize it trivially).
		 */
		procp->p_flag |= P_SEIZE;
		mylwpp = u.u_lwpp;
		lwpp = procp->p_lwpp;
		/*
		 * It is possible that two contexts may signal the 
		 * completion of the seize operation. This occurs when 
		 * an exiting LWP races with another LWP that is submitting 
		 * itself to the barrier. So, we need to clear the 
		 * event structure prior to initiating the seize
		 * operation.
		 */
		EVENT_CLEAR(&procp->p_seized);
		do {
			/*
			 * NOTE: lwp_create() is interlocked with p_mutex,
			 * and since we hold p_mutex, there is no race with
			 * lwp_create(2).
			 */
			if (lwpp == mylwpp) {
				/*
				 * Do NOT set EVF_PL_SEIZE for ourselves, as
				 * this would make it impossible for us to run
				 * in the future.
				 */
				(void)LOCK(&procp->p_seize_mutex, PLHI);
				procp->p_nseized++;
				UNLOCK(&procp->p_seize_mutex, PLHI);
				continue;
			}
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf |= EVF_PL_SEIZE;
			switch (lwpp->l_stat) {
			case SRUN:
				/*
				 * We need to acquire the run queue lock
				 * both to catch the transition from SRUN
				 * to SONPROC, and to call kpnudge() if the
				 * transition occurs.
				 */
				RUNQUE_LOCK();
				if (lwpp->l_stat == SONPROC) {
					/*
					 * Raced with dispatching engine and
					 * dispatching engine won.
					 */
					kpnudge(PRIMED, lwpp->l_eng);
						/* sets runrun flag, causing */
						/* call to CL_PREEMPT() */
					RUNQUE_UNLOCK();
					break;
				}
				dispdeq(lwpp);
				RUNQUE_UNLOCK();
				/*FALLTHROUGH*/
			case SSLEEP:
			case SIDL:
			case SSTOP:
				(void)LOCK(&procp->p_seize_mutex, PLHI);
				procp->p_nseized++;
				UNLOCK(&procp->p_seize_mutex, PLHI);
				break;
			case SONPROC:
				RUNQUE_LOCK();	/* must hold for kpnudge */
				kpnudge(PRIMED, lwpp->l_eng);
						/* sets runrun flag, causing */
						/* CL_PREEMPT() to be called */
				RUNQUE_UNLOCK();
				break;
			default:
				/*
				 *+ An lwp was found in an unknown state.  This
				 *+ indicates a kernel software problem.
				 */
				cmn_err(CE_PANIC, "vm_seize:  unknown l_stat");
				break;
			}
			UNLOCK(&lwpp->l_mutex, PLHI);
		} while ((lwpp = lwpp->l_next) != NULL);

		/*
		 * Without holding p_seize_mutex (relying on atomic memory
		 * access of ushorts), check to see if every LWP has already
		 * been seized on the optimistic chance that none of the LWPs
		 * was actually connected to a CPU.
		 */
		if (procp->p_nseized == procp->p_nlwp) {
			/*
			 * P_NOSEIZE may be set, but all that means is that
			 * we enqueued ourselves to seize the process BEFORE
			 * the process began exit processing.
			 */
			UNLOCK(&procp->p_mutex, PLBASE);
			return (B_TRUE);
		}

		/*
		 * The process (and every LWP in the process) has been marked
		 * for seizing.  This means that the process cannot exit.  We
		 * can now safely release p_mutex.
		 */
		UNLOCK(&procp->p_mutex, PLBASE);
	}

	EVENT_WAIT(&procp->p_seized, PRIMEM);

	/*
	 * ASSERT:  we were not prematurely awakened.
	 */
	ASSERT(procp->p_nseized == procp->p_nlwp);

	/*
	 * P_NOSEIZE may be set, but all that means is that we enqueued
	 * ourselves to seize the process BEFORE the process began exit
	 * processing.
	 */
	return (B_TRUE);
}

/*
 * void vm_unseize(procp)
 *	Release a process that was previously seized with vm_sieze().
 *
 * Calling/Exit State:
 *	procp is the process to be released.  Returns:  none.
 *
 *	p_mutex of the given process must not be held.  Assumed to be called
 *	at PLBASE, since the caller must tolerate blocking at PLBASE.
 */
void
vm_unseize(proc_t *procp)
{
	lwp_t *lwpp;
	lwp_t *endlwpp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	endlwpp = u.u_lwpp;
	(void)LOCK(&procp->p_mutex, PLHI);
	if (!SV_BLKD(&procp->p_wait2seize)) {
		/*
		 * No other agent in the kernel wants to seize the process.
		 */
		if (endlwpp->l_procp == procp) {
			/*
			 * The caller of vm_unseize() is in the process to
			 * be released.
			 */
			if (procp->p_nlwp == 1) {
				/* only one LWP */
				procp->p_flag &= ~P_SEIZE;
				procp->p_nseized = 0;
				UNLOCK(&procp->p_mutex, PLBASE);
				return;
			}
			lwpp = endlwpp->l_prev;
		} else {
			lwpp = procp->p_lwpp;
			endlwpp = lwpp;
		}
		do {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~EVF_PL_SEIZE;
			if (lwpp->l_stat == SRUN) {
                                lwpp->l_flag |= L_AGEDISP;
                                setrun(lwpp);
                                lwpp->l_flag &= ~L_AGEDISP;
			}
			UNLOCK(&lwpp->l_mutex, PLHI);
		} while ((lwpp = lwpp->l_prev) != endlwpp);
		procp->p_flag &= ~P_SEIZE;

		/*
		 * Even though p_nseized is normally locked by p_seize_mutex,
		 * we hold p_mutex here until p_nseized is set to zero.  This
		 * prevents another seize request from starting with a non-
		 * zero stale p_nseized field.
		 */
		procp->p_nseized = 0;
		UNLOCK(&procp->p_mutex, PLBASE);
	} else {
		/*
		 * Another execution context is waiting to get ahold of the
		 * process in the seized state.  The only way that this can 
		 * occur with the supported usage of vm_seize(), is if the
		 * swapper queues to seize the process after an LWP within
		 * the process has begun seizing or has already seized the
		 * process.
		 *
		 * Become seized ourself, and release the seized process
		 * to the swapper. We assume that in this case, the context
		 * doing the unseize MUST be from within the process.
		 */

		ASSERT(procp == u.u_procp);
		(void)LOCK(&endlwpp->l_mutex, PLHI);
		endlwpp->l_trapevf |= EVF_PL_SEIZE;
		UNLOCK(&endlwpp->l_mutex, PLHI);
		/*
		 * Decrement p_nseized since the CL_PREEMPT() call is going to
		 * see EVF_PL_SEIZE and go through the normal seize algorithm.
		 */
		(void)LOCK(&procp->p_seize_mutex, PLHI);
		procp->p_nseized--;
		UNLOCK(&procp->p_seize_mutex, PLHI);
		/*
		 * Prior to signalling the context that is waiting to seize
		 * us, make sure that the seizing context will wait for this 
		 * context to come to the barrier. Since we have not come 
		 * to the barrier, just clear the event. When we submit 
		 * ourselves to the barrier, we will signal the context 
		 * waiting to seize this process.
		 */
		EVENT_CLEAR(&procp->p_seized);
		SV_SIGNAL(&procp->p_wait2seize, 0);
		UNLOCK(&procp->p_mutex, PLBASE);
		/* become seized */
		CL_PREEMPT(endlwpp, endlwpp->l_cllwpp);
	}
}

/*
 * boolean_t become_seized(lwp_t *)
 *	Check another lwp into the seize.
 *
 * Calling/Exit State:
 *	Returns: B_TRUE if the all the lwp's in the given lwp have checked
 *	into the seize, B_FALSE otherwise.
 *
 * 	Called at PLHI, returns at the same level.
 */
boolean_t
become_seized(lwp_t *lwpp)
{
	proc_t *procp = lwpp->l_procp;
	boolean_t ret;

	(void)LOCK(&procp->p_seize_mutex, PLHI);
	if (++procp->p_nseized == procp->p_nlwp) {
		ret = B_TRUE;
	} else {
		ret = B_FALSE;
	}
	UNLOCK(&procp->p_seize_mutex, PLHI);
	return (ret);
}
