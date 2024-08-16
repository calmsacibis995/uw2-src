/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/rendez.c	1.25"
#ident	"$Header: $"

#include <fs/procfs/prsystm.h>
#include <mem/kmem.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/bitmasks.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>

extern void destroy_proc_f(void);

/*
 *
 * boolean_t rendezvous(void)
 *	Cause all LWPs in the calling process to rendezvous.  No writer
 *	operations against the process must be performed while the LWPs
 *	in the process are rendezvoused.
 *
 * Calling/Exit State:
 *    Calling state:
 *	No locks can be held by the caller upon entry.  None are held
 *	upon return.
 *
 *    Return value:
 *	B_TRUE iff all LWPs for the calling process have rendezvoused.
 *	B_FALSE otherwise indicating one of:
 *	     1) Another LWP in the process has requested that the
 *		calling LWP be destroyed (e.g. due to exec(2), _exit(2),
 *		_lwp_exit(2) when the last LWP in the process is
 *		exiting, or fatal-signal activity).
 *	     2) A job control and/or /proc requested stop is already
 *		pending against the calling LWP.
 *	     3)	A rendezvous() is already in progress as requested by
 *		another LWP in the process.
 *
 * Description:
 *	Only one caller of the rendezvous() function is allowed
 *	to succeed at a time.  The only exception occurs when a
 *	rendezvous has been started by an LWP, and another LWP in
 *	the process accepts delivery of a fatal signal requiring a
 *	core dump.  In this case, the first LWP returns B_FALSE from
 *	the rendezvous() call, while the LWP that accepted the fatal
 *	core dumping signal will eventually call core(), which will
 *	in turn invoke rendezvous(), which will return B_TRUE at that
 *	time.
 *
 *	Once all LWPs in the process have rendezvoused, the process
 *	is best described as being in a state of stasis (i.e., all
 *	LWPs within the process are suspended from execution, except
 *	for the single LWP that requested the rendezvous.  During
 *	the time of the rendezvous, it is illegal for any caller
 *	of rendezvous() to perform destructive operations against
 *	the process.
 *
 *	When the operation to be performed while the LWPs in the
 *	process are rendezvoused is complete, the release_rendezvous()
 *	primitive must be invoked to continue the LWPs in the process.
 *
 * Remarks:
 *	Example uses:
 *	   From core():
 *
 *		-- Whenever issig() encounters a fatal signal requiring a
 *		-- core dump, the EVF_PL_DESTROY and EVF_PL_RENDEZV trap
 *		-- event flags are immediately broadcast to all other LWPs
 *		-- in the process.  The P_DESTROY flag is also set to
 *		-- indicate that the process is being destroyed (used to
 *		-- prevent racing destroy requests, and attempts to acquire
 *		-- the process reader/writer lock via p_procrdwr()).
 *
 *		-- The checks incorporated into rendezvous() for the
 *		-- EVF_PL_DESTROY trap event flag (preventing any other
 *		-- LWPs from performing a destroy or rendezvous operation
 *		-- on the process), guarantee that the call below to
 *		-- rendezvous() will be successful.  (Remember: the LWP
 *		-- that took delivery of the fatal signal will NOT have
 *		-- EVF_PL_DESTROY set, so it is allowed to call rendezvous().)
 *
 *		-- Lastly, even though we know the rendezvous request will
 *		-- succeed in core(), we still need to call rendezvous() to
 *		-- wait for all of the other LWPs to come to the rendezvous
 *		-- point.
 *
 *		if (!rendezvous()) {		-- should not fail!
 *			cmn_err(CE_PANIC, ".....");
 *		}
 *
 *		-- All of the LWPs in the process have rendezvoused.
 *		-- Acquire the process reader/writer lock as a reader
 *		-- lock.  This blocks destructive writer operations
 *		-- against the process from LWPs outside of the process
 *		-- (e.g., writes to the process address space via
 *		-- /proc).
 *
 *		RWSLEEP_RDLOCK(&procp->p_rdwrlock, PRIMED);
 *
 *		<Produce the core file>
 *
 *		RWSLEEP_UNLOCK(&procp->p_rdwrlock);
 *
 *		release_rendezvous();
 *
 *	   From forkall(2):
 *		if (!rendezvous()) {
 *
 *			-- The calling LWP is being destroyed.  Alternatively,
 *			-- there exists a pending job control stop, pending
 *			-- /proc stop, or previous rendezvous request against
 *			-- the calling LWP, and the process is not being
 *			-- destroyed.
 *
 *			-- Return from the system call, letting systrap() deal
 *			-- with the various requests.  If the LWP is not to be
 *			-- destroyed, the forkall(2) system call will be
 *			-- restarted.
 *
 *			return (ERESTART);
 *		}
 *
 *		-- All of the LWPs in the process have rendezvoused.
 *		-- Acquire the process reader/writer lock as a reader
 *		-- lock.  This blocks destructive writer operations
 *		-- against the process from LWPs outside of the process
 *		-- (e.g., writes to the process address space via
 *		-- /proc).
 *
 *		RWSLEEP_RDLOCK(&procp->p_rdwrlock, PRIMED);
 *
 *		<Do forkall(2) operation>
 *
 *		RWSLEEP_UNLOCK(&procp->p_rdwrlock);
 *
 *		release_rendezvous();
 *
 */
boolean_t
rendezvous(void)
{
	proc_t *procp;		/* calling process */
	lwp_t *lwpp;		/* calling LWP */

	ASSERT(KS_HOLD0LOCKS());

	lwpp = u.u_lwpp;
	procp = u.u_procp;
	if (procp->p_nlwp > 1) {
		/*
		 * Return B_FALSE if the calling LWP has been marked for
		 * destruction.  Also return B_FALSE if there are pending
		 * job control or /proc stops or there exists a pending
		 * rendezvous request originated by another LWP in the
		 * process, and the process is not being destroyed.
		 * The check for destruction is specific to the calling
		 * LWP, which allows the fatal signal core dump generation
		 * code to work properly).
		 *
		 * The verification that the process is not being destroyed
		 * when deferring to pending job control stops, /proc stops,
		 * or a rendezvous request by another LWP in the process,
		 * is also to allow the core dump code to work properly.
		 * Consider that if the EVF_PL_DESTROY flag is not set for
		 * the calling LWP but P_DESTROY is set, then the calling LWP
		 * is the originator of the destructive operation against the
		 * process, and must be allowed to complete the rendezvous.
		 */
		(void)LOCK(&procp->p_mutex, PLHI);
		if ((lwpp->l_trapevf & EVF_PL_DESTROY) ||
		    ((lwpp->l_trapevf &
		      (EVF_PL_JOBSTOP|EVF_PL_PRSTOP|EVF_PL_RENDEZV)) &&
		     !(procp->p_flag & P_DESTROY))) {
			UNLOCK(&procp->p_mutex, PLBASE);
			return (B_FALSE);
		}

		/*
		 * If any LWPs in the process are stopped for /proc, we
		 * must not begin a rendezvous now.  Try again later.
		 */
		if (procp->p_nprstopped > 0) {
			SV_WAIT_SIG(&procp->p_stopsv, PRIMED,
				    &procp->p_mutex);
			return (B_FALSE);
		}

		/*
		 * Set the EVF_PL_RENDEZV flag for all other LWPs in the
		 * process.  The caller could be calling from core(), in
		 * which case the EVF_PL_RENDEZV flag will have already
		 * been set by issig().
		 */
		if (!(lwpp->l_prev->l_trapevf & EVF_PL_RENDEZV)) {
			trapevproc(procp, EVF_PL_RENDEZV, B_FALSE);
		}

		/*
		 * Wait if necessary for all of the other LWPs in the process
		 * to rendezvous.  Note however that we do NOT sleep
		 * interruptibly, but by special arrangement with issig() and
		 * stop(), job control signals and fatal signals promoted to
		 * current from pending will cause us to be continued.
		 * All other signals however will not interfere.
		 */
		while (procp->p_nrendezvoused + 1U < procp->p_nlwp) {
			SV_WAIT(&procp->p_rendezvous, PRIMED,
				&procp->p_mutex);

			/*
			 * We were awakened because:
			 *   1. Some other LWP requested that all other LWPs
			 *	in the process be destroyed, --and/or--
			 *   2.	A job control or /proc requested stop has
			 *	just been posted, --and/or--
			 *   3.	All LWPs in the process have rendezvoused,
			 *
			 * Note that in the fatal signal case requiring a core
			 * dump, a rendezvous request will still be in effect,
			 * but by another LWP in the process.  If a fatal
			 * signal was encountered not requiring a core dump,
			 * then the rendezvous flags will have been cleared.
			 */
			(void)LOCK(&procp->p_mutex, PLHI);
			if ((lwpp->l_trapevf & EVF_PL_DESTROY) ||
			    ((lwpp->l_trapevf & (EVF_PL_JOBSTOP|EVF_PL_PRSTOP))
			     && !(procp->p_flag & P_DESTROY))) {
				/*
				 * Let systrap() sort it all out.
				 */
				UNLOCK(&procp->p_mutex, PLBASE);
				return (B_FALSE);
			}
		}
		UNLOCK(&procp->p_mutex, PLBASE);
	}
	return (B_TRUE);
}


/*
 *
 * void release_rendezvous(void)
 *	Release the LWPs in the process from the rendezvous request
 *	originated by the calling LWP.
 *
 * Calling/Exit State:
 *	The process must have previously been in the rendezvous state.
 *	Upon return, the rendezvous is released.
 *
 * Remarks:
 *	This function is the companion function to the rendezvous()
 *	primitive.  For every successful call to rendezvous(),
 *	there must be a corresponding call to release_rendezvous().
 *
 *	See also the Description and Remarks sections of the rendezvous()
 *	primitive above.
 *
 */
void
release_rendezvous(void)
{
	lwp_t *nextlwpp;
	lwp_t *lwpp;
	proc_t *procp;

	procp = u.u_procp;
	if (procp->p_nlwp > 1) {
		(void)LOCK(&procp->p_mutex, PLHI);
		if (procp->p_nrendezvoused + 1U != procp->p_nlwp) {
			/*
			 *+ A call to release_rendezvous() was made when
			 *+ not all LWPs in the process were rendezvoused.
			 */
			cmn_err(CE_PANIC,
			"Attempt to release process from partial rendezvous");
		}

		/*
		 * Release all LWPs in the process from the rendezvous.
		 */
		lwpp = u.u_lwpp;
		nextlwpp = lwpp->l_prev;
		do {
			(void)LOCK(&nextlwpp->l_mutex, PLHI);
			nextlwpp->l_trapevf &= ~EVF_PL_RENDEZV;
			UNLOCK(&nextlwpp->l_mutex, PLHI);
		} while ((nextlwpp = nextlwpp->l_prev) != lwpp);
		procp->p_nrendezvoused = 0;
		SV_BROADCAST(&procp->p_rendezvoused, 0);
		UNLOCK(&procp->p_mutex, PLBASE);
	}
}


/*
 *
 * boolean_t destroy_proc(boolean_t all)
 *	Cause all LWPs in the calling process (with the exception of the
 *	calling LWP if "all" is B_FALSE) to be destroyed.
 *
 * Calling/Exit State:
 *    Parameters:
 *	If the boolean "all" parameter is B_FALSE, then the calling
 *	LWP is not going to exit (i.e., destroy_proc() is being called
 *	for an exec(2) request).
 *
 *	If the boolean "all" parameter is B_TRUE, then the calling
 *	LWP is calling destroy_proc() due to the execution of an
 *	_exit(2) system call, or because a fatal signal was delivered.
 *	All subsequent requests to obtain the process reader/writer
 *	lock via p_procrdwr() will fail.  Also, any partially complete
 *	rendezvous requests originated via rendezvous() will be aborted.
 *
 *    Locking:
 *	No locks can be held by the caller.
 *
 *    Return value:
 *	B_TRUE iff all other LWPs in the calling process have been destroyed.
 *	B_FALSE otherwise.  When B_FALSE is returned, another LWP in the
 *		process has requested that the calling LWP be destroyed
 *		(e.g. due to exec(2), _exit(2), or fatal-signal activity).
 *		It is the responsibility of the caller to properly fail
 *		the attempted operation when B_FALSE is returned.
 *
 * Remarks:
 *	Only one caller of destroy_proc() can succeed.  Any subsequent
 *	(racing) destroy requests by the other LWPs in the process
 *	will fail (they are to be destroyed).  In addition, all
 *	on-going activity upon the process with the process
 *	reader/writer lock held by p_procrdwr() will be allowed to
 *	complete.
 *
 *	It is illegal to invoke destroy_proc() when the LWPs of the
 *	process are held in rendezvous via rendezvous().
 *
 *	Example uses:
 *	   From exit():
 *		if (!destroy_proc(B_TRUE)) {
 *
 *			-- Another destructive operation is in progress.
 *			-- Simply exit as _lwp_exit(2) (note that lwp_rexit()
 *			-- honors rendezvous requests: e.g., core-dump).
 *
 *			lwp_exit(0);	-- no return
 *		}
 *
 *		<Do exit>	-- no return.
 *
 *	   From exec():
 *		if (!destroy_proc(B_FALSE)) {
 *
 *			-- Another destructive operation is in progress.
 *			-- The exec(2) system call must fail.
 *
 *			-- Release any resources (e.g., vnodes, etc.).
 *
 *			lwp_exit();
 *
 *		}
 *
 *		-- Acquire the process reader/writer lock as a writer lock.
 *		-- This blocks subsequent readers and writers from accessing
 *		-- the process address space via /proc.  However, all readers
 *		-- and writers presently doing so, will be able complete.  When
 *		-- the exec(2) operation is complete, we must release the
 *		-- reader/writer lock.
 *
 *		RWSLEEP_WRLOCK(&procp->p_rdwrlock, PRIMED, NULL);
 *
 *		-- The process reader/writer lock is held as a writer lock.
 *
 *		<Do exec operation>
 *
 *		-- Release the process reader/writer lock.
 *
 *		RWSLEEP_UNLOCK(&procp->p_rdwrlock);
 *
 */
boolean_t
destroy_proc(boolean_t all)	    /* B_TRUE iff all LWPs will be destroyed */
{
	proc_t *procp;			/* u.u_procp */
	lwp_t *lwpp;			/* u.u_lwpp */
	lwp_t **original_lwpdir;	/* original LWP directory */
	size_t original_nlwpdir;	/* original size of LWP directory */
	uint_t *original_lwpidmap;	/* original LWP-ID bitmap */
	k_lwpid_t original_lwpid;
	uint_t oldsmallmap;		
	boolean_t was_mt = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());

	lwpp = u.u_lwpp;
	procp = u.u_procp;

	if (procp->p_nrendezvoused > 0 &&
	    procp->p_nrendezvoused + 1U >= procp->p_nlwp) {
		/*
		 *+ The kernel destroy_proc() primitive was invoked to destroy
		 *+ all other LWPs in the process when all LWPs in the
		 *+ process were held in rendezvous by the kernel rendezvous()
		 *+ primitive.
		 */
		cmn_err(CE_PANIC,
			"Illegal call to destroy_proc() while in rendezvous");
	}

	(void)LOCK(&procp->p_mutex, PLHI);
	if ((lwpp->l_trapevf & EVF_PL_DESTROY) != 0 && procp->p_nlwp > 1) {
		/*
		 * The process has multiple LWPs, and another LWP in the
		 * process has requested that the calling LWP be destroyed
		 * (we are not the first LWP in the process to request the
		 * destruction of all other LWPs in the process).
		 *
		 * NOTE: The more elaborate check allowing a single-threaded
		 *	 process with a single-LWP to continue destroy_proc()
		 *	 execution--even when EVF_PL_DESTROY is already set,
		 *	 allows the proper exit of single-threaded processes
		 *	 exiting via _lwp_exit(2).
		 */
		UNLOCK(&procp->p_mutex, PLBASE);
		return (B_FALSE);
	}

	if (!(procp->p_flag & P_DESTROY)) {
		/*
		 * This particular destroy request is not due to _exit(2)
		 * being invoked AFTER a fatal signal has been processed
		 * by psig().  In that case, P_DESTROY would have already
		 * been set since issig() calls post_destroy(B_TRUE, ...)
		 * when a fatal signal is encountered.  The P_DESTROY check
		 * above thus allows us to avoid posting the EVF_PL_DESTROY
		 * trap event flag twice.
		 *
		 * Post the destroy request.
		 */
		post_destroy(all, 0);
	}

	if (all) {
		/*
		 * The entire process is being destroyed.
		 * wait for all on-going operations against the calling
		 * process and LWP via /proc to complete.  (All subsequent
		 * attempts by /proc to access the calling process and LWP
		 * were prevented by post_destroy().)
		 */
		if (procp->p_trace != NULL)
			prrdwrwait(procp->p_trace);
		if (lwpp->l_trace != NULL)
			prrdwrwait(lwpp->l_trace);
	}

	/*
	 * Wait for all other non-zombie LWPs to be destroyed.
	 */
	if (!SINGLE_THREADED()) {
		SV_WAIT(&procp->p_destroy, PRIMED, &procp->p_mutex);
		if (procp->p_nlwp != 1) {
			/*
			 *+ An LWP using the destroy_proc() primitive
			 *+ was blocked on the p_destroy synchronization
			 *+ variable, and was awakened prematurely.
			 */
			cmn_err(CE_PANIC,
				"Early wakeup for process destroy operation");
		}
		(void)LOCK(&procp->p_mutex, PLHI);
	}

	/* Do family-specific process teardown. */
	destroy_proc_f();

	if (WAS_MT()) {
		was_mt = B_TRUE;
		original_lwpdir = procp->p_lwpdir;
		original_nlwpdir = procp->p_nlwpdir;
		if (original_nlwpdir <= NBITPW) {
			oldsmallmap = procp->p_small_lwpidmap;
			original_lwpidmap = &oldsmallmap;
		} else
			original_lwpidmap = procp->p_large_lwpidmap;
		original_lwpid = lwpp->l_lwpid;
	}

	if (all) {
		/*
		 * Being called for exit:
		 * Record the LWP directory as being non-existent to prevent
		 * outside agents from referring to the calling LWP via
		 * p_lwpdir[].  This also serves as a flag so that dotolwps
		 * operations don't find this last LWP.
		 */
		procp->p_nlwpdir = 0;
		procp->p_lwpdir = &procp->p_lwpp;
		BITMASK1_CLRALL(&procp->p_small_lwpidmap);	/* No LWP-IDs */
	} else {
		/*
		 * Being called for exec:
		 * If the calling LWP is not open via /proc, and the number
		 * of LWP directory elements is > 1, then shrink the LWP
		 * directory and LWP-ID map down to a single element, thus
		 * recording the remaining LWP as having an LWP-ID of 1,
		 * and saving space.
		 */
		if (lwpp->l_trace == NULL && procp->p_nlwpdir > 1) {
			lwpp->l_lwpid = 1;		/* reset ID */
			procp->p_nlwpdir = 1;
			procp->p_lwpdir = &procp->p_lwpp;
			/* Set ID map for the single LWP (ID 1 == bit 0) */
			BITMASK1_INIT(&procp->p_small_lwpidmap, 0);
		}
	}

	UNLOCK(&procp->p_mutex, PLBASE);

	/*
	 * Now that all other non-zombie LWPs have been destroyed, reap any
 	 * zombie LWPs that exited prior to destroy_proc() being called.
	 * NOTE: We've already dropped p_mutex on the assumption (which is
	 *	 very likely), that we have no zombie LWPs to free up now.
	 *	 Do not free up our original lwpid.
	 */
	if (was_mt) {
		int     idx;
		int	base_lwpid;
		uint_t	lwpidmap_word;
		uint_t	nlwpidmap_words;
		uint_t	*lwpidmap;

		base_lwpid = 1;
		lwpidmap = original_lwpidmap;
		nlwpidmap_words = BITMASK_NWORDS(original_nlwpdir);
		do {
			lwpidmap_word = *lwpidmap++;
			while ((idx = BITMASK1_FFSCLR(&lwpidmap_word)) != -1) {
				if (base_lwpid + idx != original_lwpid) {
					/*
					 * Reap the unreaped zombie:
					 */
					(void)LOCK(&procp->p_mutex, PLHI);
					freelwp(procp);
					UNLOCK(&procp->p_mutex, PLBASE);
				}
			}
			base_lwpid += NBITPW;
		} while (--nlwpidmap_words != 0);

		/*
		 * If the LWP directory has been shrunk by our earlier efforts,
		 * then discard the original LWP directory, and LWP-ID map
		 * directory as well if it was a "large" map.
		 */
		if (procp->p_nlwpdir <= 1) {
			ASSERT(original_lwpdir != &procp->p_lwpp);
			kmem_free(original_lwpdir,
				  original_nlwpdir * sizeof(lwp_t *));
			if (original_nlwpdir > NBITPW) {
				nlwpidmap_words =
					BITMASK_NWORDS(original_nlwpdir);
				kmem_free(original_lwpidmap,
					  nlwpidmap_words * sizeof(uint_t));
			}
		} else {
			/*
			 * We are the one remaining LWP; clear all other bits.
			 */
			nlwpidmap_words = BITMASK_NWORDS(original_nlwpdir);
			BITMASKN_INIT(original_lwpidmap, nlwpidmap_words,
				      original_lwpid - 1);
		}
	}

	return (B_TRUE);
}


/*
 *
 * void post_destroy(boolean_t all, u_int trap_events)	
 * 	Post the EVF_PL_DESTROY trap event flag to all LWPs in the 
 *	calling process if "all" is B_TRUE, or to all LWPs in the
 *	process with the exception of the calling LWP if "all" is
 *	B_FALSE.  In addition, any additional trap event flags specified
 *	in "trap_events" are also posted to the recipient LWPs.
 *
 * Calling/Exit State:
 *    Locking requirements:
 *	The p_mutex lock of the calling process must be held upon entry.
 *	It remains held upon return.
 * 
 * Remarks:
 * 	This function is used internally by the destroy_proc() primitive,
 * 	and is also used from issig() when a fatal signal is encountered.
 *
 *	Any partially complete rendezvous operation will be aborted,
 *	unless EVF_PL_RENDEZV is specified in (trap_events) upon
 *	completion.
 *
 *	The EVF_PL_DESTROY flag must NOT be specified in trap_events.
 *
 */
void
post_destroy(boolean_t all, 	/* B_TRUE iff all LWPs will be destroyed */
	     u_int trap_events)	/* additional trap events */
{
	proc_t *procp;
	lwp_t *lwpp;
	lwp_t *walklwpp;

	/*
	 * At least in debug mode, make sure that the EVF_PL_DESTROY flag
	 * is not set in trap_events, as in the case of taking over a
	 * rendezvous, this could cause the destroy flag to be incorrectly
	 * set for the caller.
	 */
	ASSERT(!(trap_events & EVF_PL_DESTROY));

	lwpp = u.u_lwpp;
	procp = u.u_procp;

	ASSERT(LOCK_OWNED(&procp->p_mutex));

	if (procp->p_nrendezvoused > 0 &&
	    procp->p_nrendezvoused + 1U >= procp->p_nlwp) {
		/*
		 *+ The kernel post_destroy() primitive was invoked to
		 *+ destroy all other LWPs in the process when all LWPs
		 *+ in the process were held in rendezvous by the kernel
		 *+ rendezvous() primitive.
		 */
		cmn_err(CE_PANIC,
			"Illegal call to post_destroy() while in rendezvous");
	}

	/*
	 * If "all" is set, then set the P_DESTROY flag, and also prevent
	 * any future acquisition of the process reader/writer lock via
	 * pr_p_rdwr() from completing successfully.
	 */
	if (all) {
		procp->p_flag |= P_DESTROY;
		if (procp->p_trace != NULL)
			prdestroy(procp->p_trace);
		if (lwpp->l_trace != NULL)
			prdestroy(lwpp->l_trace);
	}

	/*
	 * Loop over all of the other LWPs in the process to revoke access
	 * via /proc.
	 */
	walklwpp = lwpp;
	while ((walklwpp = walklwpp->l_prev) != lwpp) {
		if (walklwpp->l_trace != NULL)
			prdestroy(walklwpp->l_trace);
	}

	/*
	 * Post the destroy trap event.  Also, unless "trap_events"
	 * specifies EVF_PL_RENDEZV, any on-going rendezvous request
	 * will be cancelled.  Lastly, posting the EVF_PL_DESTROY trap
	 * event will prevent any future p_procrdwr() invocation from
	 * completing successfully when called from any other LWP in
	 * the process.
	 */
	if (!SINGLE_THREADED()) {
		trapevproc(procp, trap_events | EVF_PL_DESTROY, B_FALSE);

		/*
	 	 * Since post_destroy() is being called, that guarantees that
	 	 * any in-progress rendezvous attempt via rendezvous() is
	 	 * only partially complete (we, the calling LWP have not
	 	 * rendezvoused).  If trap_events does 
		 * not specify EVF_PL_RENDEZV,
	 	 * then abort any on-going rendezvous attempt.
	 	 */
		if (!(trap_events & EVF_PL_RENDEZV)) {
			abort_rendezvous();
		} else {
			/*
		 	 * Any in-progress rendezvous is to continue.
		 	 * However, the calling LWP must "take-over" 
			 * the rendezvous.
		 	 */
			if (lwpp->l_trapevf & EVF_PL_RENDEZV) {
				LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_trapevf = 
				(lwpp->l_trapevf & ~EVF_PL_RENDEZV)
				| (trap_events & ~EVF_PL_RENDEZV);
				UNLOCK(&lwpp->l_mutex, PLHI);
				/*
				 * Wakeup the context that may be
				 * waiting for the previous rendezvous
				 * operation to complete. Note that
				 * we want to take-over the rendezvous!
				 */
				if (SV_BLKD(&procp->p_rendezvous)) {
					SV_BROADCAST(&procp->p_rendezvous, 0);
				}
			}
		}
	}
}


/*
 *
 * void abort_rendezvous(void)
 *	Abort the in-progress rendezvous operation.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held upon entry.
 *	It remains held upon return.
 *
 * Remarks:
 *	This function is used internally by the destroy_proc() primitive, and
 *	is also used from stop() when a defaulted job control stop signal is
 *	encountered and from prstopped() when an LWP stops for /proc.
 *
 */
void
abort_rendezvous(void)
{
	lwp_t *nextlwpp;
	lwp_t *lwpp;
	proc_t *procp;

	procp = u.u_procp;

	ASSERT(LOCK_OWNED(&procp->p_mutex));

	lwpp = u.u_lwpp;
	nextlwpp = lwpp->l_prev;
	do {
		if (nextlwpp->l_trapevf & EVF_PL_RENDEZV) {
			(void)LOCK(&nextlwpp->l_mutex, PLHI);
			nextlwpp->l_trapevf &= ~EVF_PL_RENDEZV;
			UNLOCK(&nextlwpp->l_mutex, PLHI);
		}
	} while ((nextlwpp = nextlwpp->l_prev) != lwpp);
	procp->p_nrendezvoused = 0;

	if (SV_BLKD(&procp->p_rendezvoused)) {
		/*
		 * Wakeup all rendezvoused LWPs blocked on
		 * p_rendezvoused, waiting for the LWP that
		 * requested the rendezvous to invoke the
		 * release_rendezvous() primitive.
		 */
		SV_BROADCAST(&procp->p_rendezvoused, 0);
	}

	/*
	 * Wakeup the LWP blocked on p_rendezvous waiting for
	 * all other LWPs in the process to rendezvous.  Any
	 * such waiter when awakened will see the destroy request,
	 * and act accordingly.
	 */
	if (SV_BLKD(&procp->p_rendezvous)) {
		SV_BROADCAST(&procp->p_rendezvous, 0);
	}
}
