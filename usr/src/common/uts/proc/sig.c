/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/sig.c	1.77"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <mem/kmem.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/resource.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/var.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#endif

/*
 * General comments about synchronization and signals:
 *
 * l_cursig cannot be altered in any way, unless p_mutex is held.
 * This is so that code that cancels pending signals against an LWP
 * (e.g., the cancel_jobctrl() function), can get a stable view of the
 * signal state of each LWP (l_cursig and l_sigs) so that the
 * EVF_PL_SIG flag can be reset as appropriate.
 *
 * It should be noted however that the l_cursig field of a given LWP
 * can only be changed by the LWP, or by a debugger when the LWP is in
 * the stopped state.  Hence, no locking is needed by an LWP to
 * inspect its OWN l_cursig field.
 */

/*
 * Globally visible signal sets:
 */
k_sigset_t sig_fillset;		/* set of valid signals */
k_sigset_t sig_cantmask; 	/* set of signals that cannot be masked */
k_sigset_t sig_ignoredefault; 	/* set of signals that are ignored by default */
k_sigset_t sig_stopdefault;	/* set of signals that stop by default */
k_sigset_t sig_jobcontrol;	/* set of all job control signals */

/*
 * Signal sets private to this module:
 */
STATIC k_sigset_t sig_cantreset;	/* signals that cannot be reset */
STATIC k_sigset_t sig_coredefault;	/* signals that dump core by default */
STATIC k_sigset_t sig_holdvfork;	/* signals to mask during vfork(2) */


event_t	pause_event;			/* event used for pause() primitive */


/*
 *
 * Remove the signal information from the LWP instance signal.
 * The caller must hold the p_mutex lock of the designated LWP.
 * It remains held upon return.
 *
 */
#define RMLWP_SIGINFO(lwpp, sig, sqp) \
{ \
	sigqueue_t **psqp; \
	sigdelset(&(lwpp)->l_sighasinfo, (sig)); \
	psqp = &(lwpp)->l_siginfo; \
	(sqp) = *psqp; \
	while ((sqp)->sq_info.si_signo != (sig)) { \
		psqp = &(sqp)->sq_next; \
		(sqp) = *psqp; \
	} \
	*psqp = (sqp)->sq_next; \
}


/*
 * Remove the specified process instance signal
 * The caller must hold the p_mutex lock of the designated process.
 * It remains held upon return.
 */
#define RMPROC_SIG(p, sig) \
{ \
	sigdelset(&(p)->p_sigs, (sig)); \
	sigdelset(&(p)->p_sigaccept, (sig));  \
}


/* Release signal information (sigqueue_t object). */
#define PROCSIG_RELE(sqp) \
{ \
	FSPIN_LOCK(&(sqp)->sq_mutex); \
	if (--(sqp)->sq_ref == 0) { \
		FSPIN_UNLOCK(&(sqp)->sq_mutex); \
		kmem_free((sqp), sizeof (sigqueue_t)); \
	} else { \
		FSPIN_UNLOCK(&(sqp)->sq_mutex); \
	} \
}


#define tracing(p, sig) \
	(sigismember(&(p)->p_sigtrmask, (sig)))

/* Release the lock passed into issig() function */
#define RELEASE_HELD_LOCK(hlp, pl, p) \
	if ((hlp) != NULL && (hlp) != &(p)->p_mutex) { \
		UNLOCK((hlp), (pl)); \
		(hlp) = NULL; \
	}

/*
 * Release the signal information object associated with the current
 * signal for the calling LWP.  The caller must do the necessary locking
 * depending upon the circumstances.
 */
#define DISCARD_CURSIGINFO(lwpp) \
{ \
	siginfo_free((lwpp)->l_cursigst.sst_info); \
	(lwpp)->l_cursigst.sst_info = NULL; \
}

/*
 * Nudge the given LWP to notice the new unmasked signal posted to it.
 * The p_mutex lock must be held by the caller upon entry.  It remains
 * held upon return.
 */
#define SIGNUDGE(lwpp, sig) \
{ \
	sigaddset(&(lwpp)->l_sigs, (sig));				\
	(void)LOCK(&(lwpp)->l_mutex, PLHI);				\
	(lwpp)->l_trapevf |= EVF_PL_SIG;				\
	trapevnudge((lwpp), (sig) == SIGKILL ? B_TRUE : B_FALSE);	\
	UNLOCK(&(lwpp)->l_mutex, PLHI);					\
}


extern int core(pid_t, struct cred *, rlim_t, int, boolean_t);

/*
 *
 * sigqueue_t *siginfo_get(int km_flag, u_long refcnt)
 * 	Allocate and initialize a sigqueue_t object of the indicated
 * 	signal type (process instance or LWP instance).
 *
 * Calling/Exit State:
 *    Parameters:
 *	"km_flag" specifies whether to block allocating the sigqueue_t
 *	object (KM_SLEEP or KM_NOSLEEP).
 *
 *	"refcnt" identifies the initial reference count for the object
 *	except when the refcnt is zero.  A refcnt of zero indicates
 *	that the object is to be allocated for an LWP instance signal.
 *
 *    Return value:
 *	Returns a pointer to the allocated and initialized sigqueue_t object
 *	upon success.  Otherwise, NULL is returned indicating failure.
 *
 */
sigqueue_t *
siginfo_get(int km_flag, u_long refcnt)
{
	sigqueue_t *sqp;

	sqp = kmem_zalloc(sizeof *sqp, km_flag);
	if (sqp != NULL) {
		if (refcnt != 0) {		/* process instance signal */
			FSPIN_INIT(&sqp->sq_mutex);
			sqp->sq_ref = refcnt;
		} else 				/* LWP instance signal */
			sqp->sq_qlist = 1;
	}
	return sqp;
}


/*
 *
 * void siginfo_free(sigqueue_t *sqp)
 *	Free the given sigqueue_t signal information object previously
 *	allocated by siginfo_get().
 *
 * Calling/Exit State:
 *	No fast spin locks can be held when calling this function.
 *
 * Remarks:
 *	If the sigqueue_t object is for a process instance signal, the
 *	reference count contained in the sigqueue_t object is decremented
 *	appropriately and the object is freed on last reference.  Otherwise,
 *	the signal information object is for an LWP instance signal, and it
 *	is freed directly (LWP instance sigqueue_t objects are not reference
 *	counted).
 *
 */
void
siginfo_free(register sigqueue_t *sqp)
{
	if (sqp->sq_qlist)
		kmem_free(sqp, sizeof *sqp);
	else
		PROCSIG_RELE(sqp);
}


/*
 *
 * void winfo(proc_t *cp, k_siginfo_t *ip, boolean_t waitflag)
 * 	Format siginfo structure for wait system calls.
 *
 * Calling/Exit State:
 *    Parameters:
 *	"waitflag" if B_TRUE, indicates that the child process should
 *	no longer be considered waitable upon completion.  Otherwise,
 *	the wait operation being performed is "non-destructive."
 *
 *    Locking:
 *	The p_mutex lock of the specified child process must be held
 *	by the caller.  The p_mutex lock remains held upon return.
 *
 */
void
winfo(register proc_t *cp, register k_siginfo_t *ip, boolean_t waitflag)
{
	ASSERT(LOCK_OWNED(&cp->p_mutex));

	struct_zero(ip, sizeof *ip);
	ip->si_signo = SIGCLD;
	ip->si_code = cp->p_wcode;
	ip->si_pid = cp->p_pidp->pid_id;
	ip->si_status = cp->p_wdata;
	/*
	 * NOTE: In SVR4ES/MP, the si_stime and si_utime fields have
	 *	 been dropped, and are instead returned as zero.
	 */
	if (waitflag)
		PROC_NOWAIT(cp);
}


/*
 *
 * void sigcld_l(proc_t *p)
 * 	Post a SIGCHLD signal to the parent of the indicated process.
 *	Also, "nudge" the parent if the parent is in one of the wait
 *	system calls.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the indicated process and its parent process
 *	must be held upon entry.  These locks remains held upon return.
 *
 */
void
sigcld_l(register proc_t *p)
{
	register proc_t *pp;

	pp = p->p_parent;

	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(LOCK_OWNED(&pp->p_mutex));

	switch (p->p_wcode) {
	case CLD_EXITED:
	case CLD_DUMPED:
	case CLD_KILLED:
		break;

	case CLD_STOPPED:
	case CLD_CONTINUED:
		if (pp->p_flag & P_JCTL)
			break;
	case CLD_TRAPPED:
		pp->p_flag |= P_CLDEVT;
		if (SV_BLKD(&pp->p_waitsv))
			SV_BROADCAST(&pp->p_waitsv, 0);
		/* FALLTHROUGH */

	default:
		return;
	}


	if (pp->p_nlwp > 0) {	/* parent not a zombie */
		if (!sigismember(&pp->p_sigs, SIGCLD)) {
			/*
			 * SIGCLD signal is not pending against the parent.
			 * Format the child info in the parent's sigcldinfo
			 * buffer.
			 */
			winfo(p, &pp->p_sigcldinfo->sq_info, B_FALSE);
			FSPIN_LOCK(&pp->p_sigcldinfo->sq_mutex);
			pp->p_sigcldinfo->sq_ref++;
			FSPIN_UNLOCK(&pp->p_sigcldinfo->sq_mutex);
			(void)sigtoproc_l(pp, SIGCLD, pp->p_sigcldinfo);
		}
		pp->p_flag |= P_CLDEVT;
		if (SV_BLKD(&pp->p_waitsv))
			SV_BROADCAST(&pp->p_waitsv, 0);
	}
}


/*
 *
 * void sigcld(proc_t *p)
 * 	Post a SIGCHLD signal to the parent of the indicated process.
 *	Also, "nudge" the parent if the parent is in the wait system calls.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the indicated process must be held by the
 *	caller.  The p_mutex lock remains held upon return.
 *
 */
void
sigcld(register proc_t *p)
{
	register proc_t *pp;
	register pl_t pl;

	pp = p->p_parent;
	pl = LOCK_SH(&pp->p_mutex, PLHI);
	sigcld_l(p);
	UNLOCK(&pp->p_mutex, pl);
}


/*
 *
 * STATIC void discard_sig(proc_t *p, int sig, sigqueue_t **sqplistp)
 * 	Discard all pending instances of the specified signal
 * 	against the target process.
 *
 * Calling/Exit State:
 *    Parameters:
 *	"p" is a pointer to the proc structure of the target process.
 *	"sig" is the signal to be discarded.
 *	"sqplistp" is a pointer to a sigqueue_t pointer, allowing a
 *	concatenated list of siginfo structures (to be freed as a result
 *	of discarding the signal) to be constructed.  "*sqplistp" must
 *	either be initialized to NULL by the caller, or be the result of
 *	a previous call to discard_sig().  Upon return, "*sqplistp"
 *	references the list of sigqueue_t structures to be freed.
 *    Locking:
 *	The p_mutex lock of the target process must be held on entry.
 *	The p_mutex lock remains held upon return.
 *
 */
STATIC void
discard_sig(proc_t *p, register int sig, sigqueue_t **sqplistp)
{
	register lwp_t *lwpp;
	register sigqueue_t *sqp;
	sigstate_t *ssp;

	/*
	 * Clear all pending instances of the signal against the individual
	 * LWPs of the process.
	 */
	ASSERT(LOCK_OWNED(&p->p_mutex));

	for (lwpp = p->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
		/*
		 * If the LWP is sigwaiting for the signal, then we must
		 * not discard the LWP instance of the signal (even if the
		 * signal is being sigwaited for, it is by no means
		 * guaranteed that the LWP under scrutiny is sigwaiting
		 * for the signal).
		 */
		if (lwpp->l_sigwait && !sigismember(&lwpp->l_sigheld, sig))
			continue;
		if (sigismember(&lwpp->l_lwpsigs, sig)) {
			sigdelset(&lwpp->l_lwpsigs, sig);
			if (sigismember(&lwpp->l_sighasinfo, sig)) {
				/*
				 * Remove the signal info.  Free the
				 * info structure or build the link
				 * list to be freed later by the caller.
				 */
				RMLWP_SIGINFO(lwpp, sig, sqp);
				sqp->sq_next = *sqplistp;
				*sqplistp = sqp;
			}
		} else if (!sigismember(&lwpp->l_procsigs, sig))
			continue;
		sigdelset(&lwpp->l_procsigs, sig);
		sigdelset(&lwpp->l_sigs, sig);
		if (lwpp->l_cursig == 0 && sigisempty(&lwpp->l_sigs)) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~EVF_PL_SIG;
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
	}

	/*
	 * Clear any pending process instance of the signal,
	 * if the signal is not being sigwaited for.
	 */
	ssp = &p->p_sigstate[sig - 1];
	if (sigismember(&p->p_sigs, sig) && !(ssp->sst_rflags & SST_SIGWAIT)) {
		RMPROC_SIG(p, sig);
		if ((sqp = ssp->sst_info) != NULL) {
			/*
			 * Can't use PROCSIG_RELE(), as we build
			 * the sigqueue_t object list to be discarded
			 * later by the caller.
			 */
			FSPIN_LOCK(&sqp->sq_mutex);
			if (--sqp->sq_ref == 0) {
				FSPIN_UNLOCK(&sqp->sq_mutex);
				sqp->sq_next = *sqplistp;
				*sqplistp = sqp;
			} else
				FSPIN_UNLOCK(&sqp->sq_mutex);
			ssp->sst_info = NULL;
		}
	}
}


/*
 *
 * STATIC void cancel_jobctrl(proc_t *p, int sig)
 * 	Cancel any pending job control signal(s) opposing the job control
 *	signal being posted ('sig') for the indicated process.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held upon entry.
 *	The p_mutex lock remains held upon return.
 *    Return state:
 *	When posting a SIGCONT signal, all LWPs stopped for job control
 *	are continued upon return.  When posting a job control stop signal,
 *	all pending SIGCONT signal instances are discarded.
 */
STATIC void
cancel_jobctrl(register proc_t *p, int sig)
{
	static int stops[] = {SIGSTOP, SIGTSTP, SIGTTOU, SIGTTIN};
	register lwp_t *lwpp;
	register sigqueue_t *sqp;
	sigstate_t *ssp;
	u_int trap_flag;		/* trap event flags to be cleared */

	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(sigismember(&sig_jobcontrol, sig));

	if (sig == SIGCONT) {
		/*
		 * Posting SIGCONT.
		 * Unconditionally discard all pending process instance job
		 * control stop signals (even if there are LWPs presently
		 * sigwaiting for such signals).
		 */
		if (sigmembers(&p->p_sigs, &sig_stopdefault)) {
			int i;

			sigdiffset(&p->p_sigs, &sig_stopdefault);
			sigdiffset(&p->p_sigaccept, &sig_stopdefault);
			for (i = 0; i < sizeof stops/sizeof stops[0]; i++) {
				ssp = &p->p_sigstate[stops[i] - 1];
				if ((ssp->sst_rflags & SST_SIGWAIT) == 0 &&
				    (sqp = ssp->sst_info) != NULL) {
					PROCSIG_RELE(sqp);
					ssp->sst_info = NULL;
				}
			}
		}

		/*
		 * Unconditionally discard all pending LWP instance job control
		 * stop signals (even if there are LWPs sigwaiting for such
		 * signals), as well as any pending job control stop requests
		 * (EVF_PL_JOBSTOP).
		 */
		for (lwpp = p->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
			trap_flag = 0;		/* clear trap event flags */

			if (sigmembers(&lwpp->l_sigs, &sig_stopdefault) ||
			    sigmembers(&lwpp->l_lwpsigs, &sig_stopdefault)) {
				register sigqueue_t **psqp;
				register sigqueue_t *sqp;

				sigdiffset(&lwpp->l_lwpsigs, &sig_stopdefault);
				sigdiffset(&lwpp->l_procsigs, &sig_stopdefault);
				sigdiffset(&lwpp->l_sigs, &sig_stopdefault);
				if (lwpp->l_cursig == 0 &&
				    sigisempty(&lwpp->l_sigs)) {
					trap_flag = EVF_PL_SIG;
				}
				/*
				 * Discard information associated with LWP
				 * instances of the job control stop signal(s).
				 */
				psqp = &lwpp->l_siginfo;
				while (sigmembers(&lwpp->l_sighasinfo,
						  &sig_stopdefault)) {
					sqp = *psqp;
					while (!sigismember(&sig_stopdefault,
					       sqp->sq_info.si_signo)) {
						psqp = &sqp->sq_next;
						sqp = *psqp;
					}
					*psqp = sqp->sq_next;
					sigdelset(&lwpp->l_sighasinfo,
						  sqp->sq_info.si_signo);
					kmem_free(sqp, sizeof *sqp);
				}
			}

			/*
			 * If the LWP is stopped only for a job control signal,
			 * then continue.  The LWP can also be simultaneously
			 * stopped for /proc.
			 */
			if (lwpp->l_stat == SSTOP &&
			    lwpp->l_whystop == PR_JOBCONTROL) {
				/*
				 * An LWP cannot transition to SSTOP or SRUN
				 * without acquiring p_mutex which we currently
				 * hold.
				 */
				(void)LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_flag &= ~L_JOBSTOPPED;
				lwpp->l_trapevf &= ~trap_flag;
				setrun(lwpp);
				UNLOCK(&lwpp->l_mutex, PLHI);
				p->p_nstopped--;
			} else {
				if (lwpp->l_trapevf & EVF_PL_JOBSTOP) {
					/*
					 * The LWP has not yet reacted to the
					 * job control stop request pending
					 * against it.
					 */
					trap_flag |= EVF_PL_JOBSTOP;
				}
				if (trap_flag) {
					(void)LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf &= ~trap_flag;
					UNLOCK(&lwpp->l_mutex, PLHI);
				}
			}
		}

		if (p->p_sigjobstop != 0) {
			/*
		 	 * One or more LWPs in the process are stopped due
			 * to job control.
			 */
			p->p_sigjobstop = 0;
			ASSERT(!p->p_wcode || p->p_wcode == CLD_STOPPED);
			p->p_wcode = CLD_CONTINUED;
			p->p_wdata = SIGCONT;
			sigcld(p);
		}

	} else {
		/*
		 * Posting SIGSTOP, SIGTSTP, SIGTTIN, or SIGTTOU.
		 * Unconditionally clear any pending instance of SIGCONT
		 * against the process (even if there are LWP(s) sigwaiting
		 * for SIGCONT).
		 */
		if (sigismember(&p->p_sigs, SIGCONT)) {
			ssp = &p->p_sigstate[SIGCONT - 1];
			RMPROC_SIG(p, SIGCONT);
			if ((ssp->sst_rflags & SST_SIGWAIT) == 0 &&
			    (sqp = ssp->sst_info) != NULL) {
				PROCSIG_RELE(sqp);
				ssp->sst_info = NULL;
			}
		}

		/*
		 * Unconditionally clear all pending instances of SIGCONT
		 * against the individual LWPs of the process (even if there
		 * LWPs presently sigwaiting for SIGCONT).
		 */
		for (lwpp = p->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
			if (sigismember(&lwpp->l_lwpsigs, SIGCONT)) {
				sigdelset(&lwpp->l_lwpsigs, SIGCONT);
				if (sigismember(&lwpp->l_sighasinfo, SIGCONT)) {
					/*
					 * Remove the signal info.
					 */
					RMLWP_SIGINFO(lwpp, SIGCONT, sqp);
					kmem_free(sqp, sizeof *sqp);
				}
			} else if (!sigismember(&lwpp->l_procsigs, SIGCONT))
				continue;
			sigdelset(&lwpp->l_procsigs, SIGCONT);
			sigdelset(&lwpp->l_sigs, SIGCONT);
			if (lwpp->l_cursig == 0 && sigisempty(&lwpp->l_sigs)) {
				(void)LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_trapevf &= ~EVF_PL_SIG;
				UNLOCK(&lwpp->l_mutex, PLHI);
			}
		}
	}
}


/*
 *
 * int sigtolwp_l(lwp_t *lwpp, int sig, sigqueue_t *sqp)
 * 	Post the given signal (and associated signal information if the
 *	sqp pointer is non-NULL) to the specified LWP.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the process in which the target LWP resides
 *	must be held upon entry.  The p_mutex lock remains held upon return.
 *
 *    Return value:
 *	This function returns 1 if any associated 'sqp' was accepted
 *	by the LWP/process; otherwise, 0 is returned.
 *
 */
int
sigtolwp_l(register lwp_t *lwpp, int sig, sigqueue_t *sqp)
{
	proc_t *p;

	ASSERT(sig > 0 && sig <= MAXSIG);

	/* Do not post the signal if it is already pending to the LWP. */
	if (sigismember(&lwpp->l_lwpsigs, sig))
		return 0;

	p = lwpp->l_procp;
	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * If posting a job control signal, cancel opposing job control
	 * signal(s).
	 */
	if (sigismember(&sig_jobcontrol, sig))
		cancel_jobctrl(p, sig);

	/*
	 * Post the signal if the signal is being sigwaited for by the LWP.
	 * However, don't bother associating any signal information in this
	 * case, as sigwait(2) returns no such information.
	 */
	if (lwpp->l_sigwait && !sigismember(&lwpp->l_sigheld, sig)) {
		/*
		 * The signal is being sigwaited for by the target LWP.
		 * Do not post any associated signal information.
		 */
		sigaddset(&lwpp->l_lwpsigs, sig);
		SIGNUDGE(lwpp, sig);
	} else {
		/*
		 * The signal is not being sigwaited for by the target LWP.
		 * Post the signal if the signal is not ignored or traced.
		 */
		if (!sigismember(&p->p_sigignore, sig) || tracing(p, sig)) {
			/*
			 * The target LWP is not sigwaiting for the
			 * signal, and the signal is not registered
			 * to be (effectively) ignored.
			 */
			sigaddset(&lwpp->l_lwpsigs, sig);
			if (!sigismember(&lwpp->l_sigheld, sig)) {
				SIGNUDGE(lwpp, sig);
			}

			/* Associate signal information */
			if (sqp != NULL) {
				sqp->sq_next = lwpp->l_siginfo;
				lwpp->l_siginfo = sqp;
				sigaddset(&lwpp->l_sighasinfo, sig);
				return 1;
			}
		}
	}
	return 0;
}


/*
 *
 * int sigtolwp(lwp_t *lwpp, int sig, sigqueue_t *sqp)
 * 	Post the given signal (and associated signal information if the
 *	sqp pointer is non-NULL) to the specified LWP.
 *
 * Calling/Exit State:
 *	This function returns 1 if any associated 'sqp' was accepted
 *	by the LWP/process; otherwise, 0 is returned.
 *
 * Remarks:
 *	The function accquires and releases p_mutex lock of the
 *	process containing the target LWP.
 *
 */
int
sigtolwp(lwp_t *lwpp, int sig, sigqueue_t *sqp)
{
	register proc_t *p;
	register pl_t pl;
	register int rval;

	p = lwpp->l_procp;
	pl = LOCK(&p->p_mutex, PLHI);
	rval = sigtolwp_l(lwpp, sig, sqp);
	UNLOCK(&p->p_mutex, pl);
	return rval;
}

/*
 *
 * int sigtoproc_l(proc_t *p, int sig, sigqueue_t *sqp)
 * 	Post the given signal (and associated signal information if the
 *	sqp pointer is non-NULL) to the specified process.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held upon entry.
 *	The p_mutex lock remains held upon return.
 *
 *    Return value:
 *	This function returns 1 if any associated 'sqp' was accepted
 *	by the process; otherwise, 0 is returned.
 *
 */
int
sigtoproc_l(register proc_t *p, int sig, sigqueue_t *sqp)
{
	register lwp_t *lwpp;
	sigstate_t *ssp;
	k_lwpid_t tgt;

	ASSERT(sig > 0 && sig <= MAXSIG);
	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(p->p_nlwp > 0);

	/*
	 * Do not post the signal if it is already pending to the process.
	 */
	if (sigismember(&p->p_sigs, sig))
		return 0;

	/*
	 * If posting a job control signal, cancel opposing job control
	 * signal(s).
	 */
	if (sigismember(&sig_jobcontrol, sig))
		cancel_jobctrl(p, sig);

	/*
	 * Post the signal if the signal is not ignored, sigwaited for,
	 * or is traced.
	 */
	ssp = &p->p_sigstate[sig-1];
	if (!sigismember(&p->p_sigignore, sig) ||
	    (ssp->sst_rflags & SST_SIGWAIT) || tracing(p, sig)) {
		/*
		 * In the checks below, the test for "tgt > p->p_nlwpdir"
		 * and for "(lwpp = p->p_lwpdir[tgt-1]) == NULL" catch
		 * stale state info from exited LWPs.  The test for
		 * "sigismember(&lwpp->l_sigheld, sig)" catches signals
		 * momentarily masked by an LWP.
		 */
		sigaddset(&p->p_sigs, sig);
		if ((tgt = ssp->sst_acceptlwp) == 0 || tgt > p->p_nlwpdir ||
		    (lwpp = p->p_lwpdir[tgt-1]) == NULL ||
		    sigismember(&lwpp->l_sigheld, sig) ||
		    ((ssp->sst_rflags & SST_SIGWAIT) && !lwpp->l_sigwait)) {
			/*
			 * We do not know of an LWP in the process that has
			 * the signal unmasked (or even if there is one), or
			 * else the LWP that we do know of has the signal
			 * unmasked and is not in sigwait(2), but there is some
			 * LWP in the process that is (we must always give the
			 * signal to the sigwaiting LWP).
			 *
			 * Search the LWPs to find an appropriate recipient
			 * LWP for the signal.
			 */
			ssp->sst_acceptlwp = 0;
			lwpp = p->p_lwpp;
			do {
				if (!sigismember(&lwpp->l_sigheld, sig) &&
				    ((ssp->sst_rflags & SST_SIGWAIT) == 0 ||
				     lwpp->l_sigwait)) {
					ssp->sst_acceptlwp = lwpp->l_lwpid;
					break;
				}
			} while ((lwpp = lwpp->l_next) != NULL);
		}
		if (lwpp != NULL) {	/* LWP has the signal unmasked */
			sigaddset(&p->p_sigaccept, sig);
			sigaddset(&lwpp->l_procsigs, sig);
			SIGNUDGE(lwpp, sig);
		}

		/*
		 * Associate signal information.
		 * NOTE: We must not associate signal information if the
		 * signal is being sigwaited for.  Otherwise, we would
		 * clobber sst_swcount (which is overlayed with sst_info).
		 */
		if (sqp != NULL && (ssp->sst_rflags & SST_SIGWAIT) == 0) {
			ssp->sst_info = sqp;
			return 1;
		}
	}
	return 0;
}


/*
 *
 * int sigtoproc(proc_t *p, int sig, sigqueue_t *sqp)
 * 	Post the given signal (and associated signal information if the
 *	sqp pointer is non-NULL) to the specified process.
 *
 * Calling/Exit State:
 *	This function returns 1 if any associated 'sqp' was accepted
 *	by the process; otherwise, 0 is returned.
 *
 * Remarks:
 *	The function accquires and releases p_mutex lock of the
 *	target process.
 *
 */
int
sigtoproc(register proc_t *p, int sig, sigqueue_t *sqp)
{
	register int rval;
	register pl_t pl;

	pl = LOCK(&p->p_mutex, PLHI);
	rval = sigtoproc_l(p, sig, sqp);
	UNLOCK(&p->p_mutex, pl);
	return rval;
}


/*
 *
 * void
 * dbg_sigheld(lwp_t *lwpp, k_sigset_t newmask)
 *	Set the held signal mask of the target LWP to the given set.
 *	This function is a debugger helper function, and is used by
 *	the /proc file system.
 *
 * Calling/Exit State:
 *    Locking requirements:
 *	The p_mutex lock of the calling process must be held by the
 *	caller.  This lock remains held upon return.  Upon return,
 *	the held signal mask of the specified LWP is set accordingly.
 *
 */
void
dbg_sigheld(lwp_t *lwpp,		/* target LWP */
	    k_sigset_t newmask)		/* new l_sigheld */
{
	k_sigset_t held_sigs;

	ASSERT(LOCK_OWNED(&lwpp->l_procp->p_mutex));

	held_sigs = lwpp->l_sigheld;
	sigdiffset(&held_sigs, &newmask);
	if (!sigisempty(&held_sigs)) {
		/*
		 * Signal(s) are being removed from the current
		 * held signals mask.  Promote any pending signal(s)
		 * to immediately pending, and record this LWP as
		 * willing to accept new instances of such signals.
		 */
		unmask_signals(lwpp, held_sigs);
	}
	held_sigs = newmask;
	sigdiffset(&held_sigs, &lwpp->l_sigheld);
	lwpp->l_sigheld = newmask;
	if (!sigisempty(&held_sigs)) {
		/*
		 * Signal(s) are being added to the held signals mask.
		 */
		sigandset(&held_sigs, &lwpp->l_sigs);
		if (!sigisempty(&held_sigs)) {
			/*
			 * Pushed back the held process instance signals
			 * to the process, and/or mask LWP instance signals.
			 */
			mask_signals(lwpp, held_sigs);
		}
	}

	lwpp->l_sigheld = newmask;
}


/*
 *
 * void
 * dbg_sigtrmask(proc_t *p, k_sigset_t newmask)
 *	Set the p_sigtrmask of the target process to the given set.
 *	This function is a debugger helper function, and is used by
 *	the /proc file system.
 *
 * Calling/Exit State:
 *    Locking requirements:
 *	The p_mutex lock of the process containing the target LWP must be
 *	held by the caller.  Upon return, the p_mutex lock is released at
 *	PLBASE.
 *
 */
void
dbg_sigtrmask(proc_t *p,	/* target process */
	      k_sigset_t newmask)	/* new p_sigtrmask */
{
	int sig;
	sigqueue_t *sqp;
	sigqueue_t *sqplist;

	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * Discard all signals that are no longer traced, are ignored,
	 * and are not sigwaited for.
	 */
	sqplist = NULL;
	sigdiffset(&p->p_sigtrmask, &newmask);
	while ((sig = sigdelnext(&p->p_sigtrmask)) != 0) {
		ASSERT(sig > 0 && sig <= MAXSIG);
		if (sigismember(&p->p_sigignore, sig)) {
			discard_sig(p, sig, &sqplist);
		}
	}
	p->p_sigtrmask = newmask;
	UNLOCK(&p->p_mutex, PLBASE);

	/*
	 * Now that we've dropped p_mutex, discard all of the associated
	 * signal information.
	 */
	while (sqplist != NULL) {
		sqp = sqplist->sq_next;
		kmem_free(sqplist, sizeof *sqplist);
		sqplist = sqp;
	}
}


/*
 *
 * void
 * dbg_restop(LWP_t *lwpp)
 *	Change the stop reason of the target LWP from PR_JOBCONTROL or
 *	PR_SUSPENDED to PR_REQUESTED.  This function is a debugger
 *	helper function, and is used by the /proc file system to assume
 *	control of a LWP which is suspended or stopped for job control.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the target LWP must be
 *	held by the caller.  This lock remains held upon return.
 *	lwpp->l_mutex is acquired and released by this function.
 *
 */
void
dbg_restop(lwp_t *lwpp)			/* target LWP */
{
	proc_t *p;			/* process containing the LWP */

	p = lwpp->l_procp;
	ASSERT(p->p_trace);
	ASSERT(LOCK_OWNED(&p->p_mutex));

	(void)LOCK(&lwpp->l_mutex, PLHI);
	ASSERT(lwpp->l_stat == SSTOP);
	ASSERT((lwpp->l_whystop == PR_JOBCONTROL) ||
	       (lwpp->l_whystop == PR_SUSPENDED));

	if (lwpp->l_whystop == PR_JOBCONTROL) {
		ASSERT(lwpp->l_flag & L_JOBSTOPPED);
	} else {
		ASSERT(lwpp->l_flag & L_SUSPENDED);
	}

	/* Register the target LWP as being stopped for /proc. */
	lwpp->l_whystop = PR_REQUESTED;
	p->p_nreqstopped++;
	p->p_nprstopped++;
	lwpp->l_trapevf &= ~EVF_PL_PRSTOP;
	lwpp->l_flag |= L_PRSTOPPED;

	UNLOCK(&lwpp->l_mutex, PLHI);	/* @PLHI for p_mutex */

	prstopped(lwpp);
}


/*
 *
 * void
 * dbg_setrun(lwp_t *lwpp)
 *	Release the target LWP from a /proc stop.  If there are no other
 *	pending reasons for the LWP not to run, change the state of the
 *	LWP from stopped (SSTOP) to runnable (SRUN).  This function is a
 *	debugger helper function, and is used by the /proc file system
 *	to allow a stopped process to resume.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the target LWP must be
 *	held by the caller.  This lock remains held upon return.
 *
 * Remarks:
 *	The need to maintain p_nstopped under p_mutex lock provides the
 *	motivation for this function to be used by /proc.
 *	This function, in concert with the following code fragment in setrun()
 *	ensures that the LWP is not made prematurely runnable.
 *
 *	    if (lwpp->l_stat == SSTOP) {
 *
 *		-- the sending of SIGSTOP (L_JOBSTOPPED)
 *		-- or /proc (L_PRSTOPPED) can prevent an LWP from running.
 *		-- To prevent callers of setrun() in the kernel from
 *		-- inappropriately making a stopped LWP runnable (e.g.,
 *		-- callers from the STREAMS code for instance), both of
 *		-- these flags must be clear for the requested LWP to
 *		-- be made runnable.  Otherwise, the target LWP is to
 *		-- remain stopped.
 *
 *		if (lwpp->l_flag & (L_JOBSTOPPED|L_PRSTOPPED)) {
 *
 *			-- The LWP is stopped for job control
 *			-- or /proc.  Return without disturbing the LWP
 *			-- (l_mutex remains held).
 *
 *			return;
 *		}
 *
 *	    }
 *		:
 *		:
 *
 */
void
dbg_setrun(lwp_t *lwpp)	/* target LWP */
{
	proc_t *p;		/* process containing the LWP */

	p = lwpp->l_procp;
	ASSERT(LOCK_OWNED(&p->p_mutex));

	(void)LOCK(&lwpp->l_mutex, PLHI);
	ASSERT(lwpp->l_stat == SSTOP);

	ASSERT(lwpp->l_flag & L_PRSTOPPED);
	lwpp->l_flag &= ~L_PRSTOPPED;
	p->p_nprstopped--;
	if (lwpp->l_whystop == PR_REQUESTED)
		p->p_nreqstopped--;
	if (lwpp->l_flag & L_SUSPENDED) {
		/*
		 * Register the target LWP as being stopped because
		 * it is suspended.  This only happens if the LWP had
		 * already been suspended at the time of the /proc stop.
		 */
		UNLOCK(&lwpp->l_mutex, PLHI);	/* @PLHI for p_mutex */
		lwpp->l_whystop = PR_SUSPENDED;
	} else if (p->p_sigjobstop != 0) {
		/*
		 * The target LWP was stopped by /proc from some
		 * other reason besides PR_SIGNALLED, and one or
		 * more LWPs in the process are already stopped
		 * exclusively due to a job control signal.
		 *
		 * Since this LWP was already stopped by /proc,
		 * there is no possibility that we are somehow the
		 * "last" LWP in the process to become stopped (we
		 * were already stopped)!.   Hence, no code is needed
		 * to send a SIGCLD signal to the parent.
		 */
		lwpp->l_trapevf &= ~EVF_PL_JOBSTOP;
		lwpp->l_flag |= L_JOBSTOPPED;
		/*
		 * Register the target LWP as being stopped for
		 * job control.
		 */
		UNLOCK(&lwpp->l_mutex, PLHI);	/* @PLHI for p_mutex */
		lwpp->l_whystop = PR_JOBCONTROL;
		lwpp->l_whatstop = p->p_sigjobstop;
	} else {
		setrun(lwpp);
		UNLOCK(&lwpp->l_mutex, PLHI);	/* @PLHI for p_mutex */
		p->p_nstopped--;
		if (p->p_wcode != CLD_CONTINUED)
			PROC_NOWAIT(p);
	}
	/*
	 * If there are now no /proc stopped LWPs, and some (other) LWPs
	 * are waiting to initiate a rendezvous, wake them up.
	 */
	if (p->p_nprstopped == 0 && SV_BLKD(&p->p_stopsv))
		SV_BROADCAST(&p->p_stopsv, 0);
}


/*
 *
 * void
 * dbg_clearlwpsig(lwp_t *lwpp)
 *	Clear the current signal for the target LWP (which must be in the
 *	stopped state).  This function is a debugger helper function, and is
 *	to be used by the /proc file system.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the target LWP must
 *	be held upon entry.  It remains held upon return.
 *	The target LWP must be in the stopped (SSTOP) state.
 *
 */
void
dbg_clearlwpsig(lwp_t *lwpp)
{
	ASSERT(LOCK_OWNED(&lwpp->l_procp->p_mutex));
	ASSERT(lwpp->l_stat == SSTOP);

	lwpp->l_cursig = 0;
	if (lwpp->l_cursigst.sst_info != NULL) {
		DISCARD_CURSIGINFO(lwpp);
	}
	if (sigisempty(&lwpp->l_sigs)) {
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_SIG;
		UNLOCK(&lwpp->l_mutex, PLHI);
	}
}


/*
 *
 * int
 * dbg_setlwpsig(lwp_t *lwpp, int sig, sigqueue_t *siginfop)
 *	Set the current signal of the target LWP to the given signal.
 *	This function is a debugger helper function, and is to be used
 *	by the /proc file system.
 *
 * Calling/Exit State:
 *  The target LWP must be in the stopped (SSTOP) state.
 *    Parameters:
 *	"lwpp" is the target LWP.
 *	"sig" is the signal to be established as the current signal.
 *	"siginfop" is the information to be associated with the signal
 *		(NULL if no information is to be associated with the signal).
 *    Locking requirements:
 *	The p_mutex lock of the process containing the target LWP must
 *	be held upon entry.  It remains held upon return.
 *    Returns:
 *	1: If any associated 'siginfo' was accepted by the LWP (the
 *	   process containing the target LWP is not defaulting the signal,
 *	   requested information for the signal, and the caller supplied
 *	   such information).
 *	0: Otherwise.
 *
 */
int
dbg_setlwpsig(lwp_t *lwpp,		/* target LWP */
	      int sig,			/* new current signal */
	      sigqueue_t *siginfop)	/* info with signal; NULL if none */
{
	register proc_t *p;		/* process containing the target LWP */
	sigstate_t *ssp;

	p = lwpp->l_procp;
	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(lwpp->l_stat == SSTOP);
	ASSERT(sig > 0 && sig <= MAXSIG);

	/*
	 * If SIGKILL is already the current signal, return ZERO
	 * without taking any action.
	 */
	if (lwpp->l_cursig == SIGKILL)
		return 0;

	/*
	 * Set the current LWP signal.
	 */
	ssp = &p->p_sigstate[sig-1];
	lwpp->l_cursig = (u_char)sig;
	lwpp->l_cursigst = *ssp;
	lwpp->l_cursigst.sst_info = NULL;

	/*
	 * An LWP can only transition from the stopped (SSTOP) state to the
	 * runnable (SRUN) state with p_mutex held (and we hold p_mutex).
	 */
	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_trapevf |= EVF_PL_SIG;
	if (sig == SIGKILL) {
		/*
		 * We only setrun() the LWP if a SIGKILL signal
		 * has been sent.  Otherwise, the LWP is not to
		 * be continued by a signal alone.
		 */
		FORCERUN_FROMSTOP(lwpp, p);
		PROC_NOWAIT(p);
	}
	UNLOCK(&lwpp->l_mutex, PLHI);

	if (lwpp->l_sigwait && !sigismember(&lwpp->l_sigheld, sig)) {
		/*
		 * The LWP is sigwaiting for the signal.
		 * Signals that are sigwaited for must always appear as
		 * registered to be caught, with no associated signal
		 * information.
		 */
		lwpp->l_cursigst.sst_handler = (void(*)())issig;
	} else {
		/*
		 * The LWP is not sigwaiting for the signal.
		 * Save any associated signal information now.
		 */
		if (siginfop != NULL) {
			lwpp->l_cursigst.sst_info = siginfop;
			return 1;
		}
	}

	return 0;
}


/*
 *
 * int
 * dbg_setprocsig(proc_t *p, int sig, sigqueue_t *siginfop)
 *	Set the current signal of the target process to the given signal.
 *	This function is a debugger helper function, and is to be used
 *	by the /proc file system.
 *
 * Calling/Exit State:
 *  All LWPs in the target process must be in the stopped (SSTOP) state.
 *    Parameters:
 *	"p" is the target process.
 *	"sig" is the signal to be established as the current signal.
 *	"siginfop" is the information to be associated with the signal
 *		(NULL if no information is to be associated with the signal).
 *    Locking requirements:
 *	The p_mutex lock of the process must be held upon entry.
 *	It remains held upon return.
 *    Returns:
 *	1: If any associated 'siginfo' was accepted by the process (the
 *	   process is not defaulting the signal, requested information for
 *	   the signal, and the caller supplied such information).
 *	0: Otherwise.
 *
 */
int
dbg_setprocsig(register proc_t *p,	/* target process */
	       register int sig,	/* new current signal */
	       sigqueue_t *siginfop)	/* info with signal; NULL if none */
{
	register lwp_t *lwpp;		/* recipient "chosen" LWP */
	sigstate_t *ssp;		/* signal state info ptr */
	k_lwpid_t tgt;			/* LWP-ID of target LWP */

	ASSERT(p->p_nstopped == p->p_nlwp);
	ASSERT(sig > 0 && sig <= MAXSIG);

	/*
	 * Make sure no other LWP in the process has the designated signal
	 * set as its current signal already.  Also determine an LWP that
	 * is not masking the signal (and give preference to sigwaiting LWPs).
	 */
	ssp = &p->p_sigstate[sig-1];
	tgt = ssp->sst_acceptlwp;
	if (tgt != 0) {
		if (tgt > p->p_nlwpdir ||
		    (lwpp = p->p_lwpdir[tgt-1]) == NULL ||
		    sigismember(&lwpp->l_sigheld, sig) ||
		    ((ssp->sst_rflags & SST_SIGWAIT) && !lwpp->l_sigwait)) {
			tgt = 0;	/* no known recipient LWP */
		}
	}
	lwpp = p->p_lwpp;
	for (;;) {
		if (lwpp->l_cursig == sig) {
			/*
			 * When setting the "current signal of the
			 * process," it is not possible to establish
			 * the signal as the current signal for more
			 * than one LWP in the process.
			 */
			return 0;
		}
		if (tgt == 0) {		/* no good target yet */
			if (!sigismember(&lwpp->l_sigheld, sig) &&
			    ((ssp->sst_rflags & SST_SIGWAIT) == 0 ||
			     lwpp->l_sigwait)) {
				ssp->sst_acceptlwp = tgt = lwpp->l_lwpid;
			}
		}
		if ((lwpp = lwpp->l_next) == NULL) {
			/*
			 * All LWPs have been examined.
			 */
			if (tgt) {
				/*
				 * We take advantage of the fact that if
				 * p_nlwpdir == 1, that p_lwpdir points to
				 * the p_lwpp field....
				 */
				lwpp = p->p_lwpdir[tgt-1];
			} else {
				/*
				 * Unlike sigtoproc(), if no LWP has the
				 * signal unmasked, we pick the first one
				 * in the process arbitrarily.  This is
				 * because the debuggers want to be able to
				 * force a process to take a signal (as long
				 * the signal is not ignored).
				 */
				lwpp = p->p_lwpp;
			}
			break;
		}
	}

	/*
	 * Set the current signal of the selected LWP.
	 */
	return dbg_setlwpsig(lwpp, sig, siginfop);
}


/*
 *
 * void dbg_unkilllwp(lwp_t *lwpp, int sig)
 *	Remove a pending signal from the target LWP.
 *	This function is a debugger helper function, and is to be used
 *	by the /proc file system.
 *
 * Calling/Exit State:
 *    Parameters:
 *	"lwpp" is the target LWP.
 *	"sig" is the signal to be removed.
 *    Locking requirements:
 *	The p_mutex lock of the LWP's process must be held upon entry.
 *	It remains held upon return.
 *
 */
void
dbg_unkilllwp(lwp_t *lwpp, int sig)
{
	proc_t *p = lwpp->l_procp;

	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(sig > 0 && sig <= MAXSIG && sig != SIGKILL);

	if (sigismember(&lwpp->l_procsigs, sig)) {
		sigdelset(&lwpp->l_procsigs, sig);
		RMPROC_SIG(p, sig);
		if (!sigismember(&lwpp->l_lwpsigs, sig))
			sigdelset(&lwpp->l_sigs, sig);
		if (!(p->p_sigstate[sig-1].sst_rflags & SST_SIGWAIT) &&
		    p->p_sigstate[sig-1].sst_info) {
			PROCSIG_RELE(p->p_sigstate[sig-1].sst_info);
			p->p_sigstate[sig-1].sst_info = NULL;
		}
	} else {
		sigdelset(&lwpp->l_lwpsigs, sig);
		sigdelset(&lwpp->l_sigs, sig);
		if (sigismember(&lwpp->l_sighasinfo, sig)) {
			sigqueue_t *sqp;
			RMLWP_SIGINFO(lwpp, sig, sqp);
			kmem_free(sqp, sizeof *sqp);
		}
	}
	if (sigisempty(&lwpp->l_sigs) &&
	    lwpp->l_cursig == 0) {
		pl_t o = LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_SIG;
		UNLOCK(&lwpp->l_mutex, o);
	}
}


/*
 *
 * void dbg_unkillproc(proc_t *p, int sig)
 *	Remove a pending signal from the target process.
 *	This function is a debugger helper function, and is to be used
 *	by the /proc file system.
 *
 * Calling/Exit State:
 *    Parameters:
 *	"p" is the target process.
 *	"sig" is the signal to be removed.
 *    Locking requirements:
 *	The p_mutex lock of the process must be held upon entry.
 *	It remains held upon return.
 *
 */
void
dbg_unkillproc(proc_t *p, int sig)
{
	lwp_t *lwpp;

	ASSERT(LOCK_OWNED(&p->p_mutex));
	ASSERT(sig > 0 && sig <= MAXSIG && sig != SIGKILL);

	/* Check for process instance signal first. */
	if (sigismember(&p->p_sigs, sig)) {
		sigdelset(&p->p_sigs, sig);
		if (!(p->p_sigstate[sig-1].sst_rflags & SST_SIGWAIT) &&
		    p->p_sigstate[sig-1].sst_info) {
			PROCSIG_RELE(p->p_sigstate[sig-1].sst_info);
			p->p_sigstate[sig-1].sst_info = NULL;
		}
		if (sigismember(&p->p_sigaccept, sig)) {
			sigdelset(&p->p_sigaccept, sig);
			for (lwpp = p->p_lwpp; lwpp; lwpp = lwpp->l_next)
				if (sigismember(&lwpp->l_procsigs, sig)) {
					sigdelset(&lwpp->l_procsigs, sig);
					if (!sigismember(&lwpp->l_lwpsigs,
							 sig)) {
						sigdelset(&lwpp->l_sigs, sig);
						if (sigisempty(&lwpp->l_sigs) &&
						    lwpp->l_cursig == 0) {
					pl_t o = LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf &= ~EVF_PL_SIG;
					UNLOCK(&lwpp->l_mutex, o);
						}
					}
				}
		}
		return;
	}

	/* Not found at process level, so choose an LWP */
	/* and check for LWP instance signal. */
	if (lwpp = prchoose(p))
		dbg_unkilllwp(lwpp, sig);
}


/*
 *
 * void discard_lwpsigs(lwp_t *lwpp)
 * 	Discard all current and pending signals for the specified LWP.
 *
 * Calling/Exit State:
 *	For the _lwp_exit(2) case, the specified LWP must have
 *	completely removed itself from the process in which it was
 *	formerly a member.  For the _exit(2) case, the LWP is simply
 *	the last surviving LWP in the process and has previously
 *	masked _all_ signals.
 *
 */
void
discard_lwpsigs(lwp_t *lwpp)
{
	register sigqueue_t *nextsqp;
	register sigqueue_t *sqp;

	sigemptyset(&lwpp->l_sigs);
	sigemptyset(&lwpp->l_lwpsigs);
	sigemptyset(&lwpp->l_procsigs);
	sigemptyset(&lwpp->l_sighasinfo);
	if (lwpp->l_cursig) {
		lwpp->l_cursig = 0;
		if (lwpp->l_cursigst.sst_info != NULL)
			DISCARD_CURSIGINFO(lwpp);	/* free up siginfo */
	}

	/*
	 * Free up all information for LWP signal instances.
	 */
	sqp = lwpp->l_siginfo;
	while (sqp) {
		nextsqp = sqp->sq_next;
		kmem_free(sqp, sizeof *sqp);
		sqp = nextsqp;
	}
	lwpp->l_siginfo = NULL;
}


/*
 *
 * void discard_procsigs()
 * 	Discard all pending signals for the calling process.
 *
 * Calling/Exit State:
 *	This function is called from exit.  The calling process must
 *	have set the action of all signals to ignore, and have
 *	cleared the signal tracing mask.
 *
 */
void
discard_procsigs()
{
	register sigstate_t *ssp;
	register sigqueue_t *sqp;
	register int i;
	proc_t *p;

	/*
	 * Free up all information for process signal instances.
	 */
	p = u.u_procp;
	sigemptyset(&p->p_sigs);
	sigemptyset(&p->p_sigtrmask);
	ssp = p->p_sigstate;
	for (i = 1; i <= MAXSIG; i++, ssp++) {
		ASSERT ((ssp->sst_rflags & SST_SIGWAIT) == 0);
		if ((sqp = ssp->sst_info) != NULL) {
			PROCSIG_RELE(sqp);
			ssp->sst_info = NULL;
		}
	}
}


/*
 *
 * void mask_signals(lwp_t *lwpp, k_sigset_t held_sigs)
 *	Mask the given set of signals for the specified LWP.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the LWP must be held
 *	upon entry.  The p_mutex lock remains held upon return.
 *
 * Description:
 *	LWP instance signals are masked by clearing the pending bits in
 *	l_sigs.  Process instance signals selected by the mask as accepted
 *	by the LWP, are pushed back to the process level.
 *
 *	This function is called whenever one or more signals become masked
 *	by the LWP or an exiting LWP has process instance signals pending.
 *	This function does not alter the held signal mask of the LWP.
 *
 * Remarks:
 *	This function accquires and releases the l_mutex lock of the
 *	specified LWP.
 *
 */
void
mask_signals(lwp_t *lwpp, k_sigset_t held_sigs)
{
	register proc_t *p;
	register sigstate_t *ssp;
	register k_lwpid_t lwpid;
	register int sig;
	k_lwpid_t tgt;

	p = lwpp->l_procp;

	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * Remove the signals to be held from the immediately pending
	 * set of signals for the LWP, and update EVF_PL_SIG accordingly.
	 */
	sigdiffset(&lwpp->l_sigs, &held_sigs);
	if (sigisempty(&lwpp->l_sigs)) {
		ASSERT((lwpp->l_trapevf & EVF_PL_SIG) != 0);
		ASSERT(lwpp->l_cursig == 0);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_SIG;
		UNLOCK(&lwpp->l_mutex, PLHI);
	}
	if (!sigmembers(&held_sigs, &lwpp->l_procsigs)) {
		/*
		 * No process instance signals have been masked, so there is
		 * no more work to do.
		 */
		return;
	}

	/*
	 * Remove the newly masked process instance signals from
	 * the LWP, and clear the indication that the signals
	 * were accepted by an LWP in the process.
	 */
	sigandset(&held_sigs, &lwpp->l_procsigs);
	sigdiffset(&lwpp->l_procsigs, &held_sigs);
	sigdiffset(&p->p_sigaccept, &held_sigs);

	/*
	 * The implicit assumption here is that only a small number of
	 * signals are being pushed back to the process level.
	 * (Therefore, it's on average cheaper to do the sigisempty check
	 * separate from sigdelnext.)
	 */
	lwpid = lwpp->l_lwpid;
	while (!sigisempty(&held_sigs)) {
		sig = sigdelnext(&held_sigs);
		ASSERT(sig > 0 && sig <= MAXSIG);
		ssp = &p->p_sigstate[sig-1];
		/*
		 * Push this signal to a sigwaiting LWP if there is one,
		 * otherwise to any LWP that is not masking the signal.
		 */
		if ((tgt = ssp->sst_acceptlwp) != 0 && tgt != lwpid &&
		    tgt <= p->p_nlwpdir &&
		    (lwpp = p->p_lwpdir[tgt-1]) != NULL &&
		    !sigismember(&lwpp->l_sigheld, sig) &&
		    ((ssp->sst_rflags & SST_SIGWAIT) == 0 || lwpp->l_sigwait))
			/* Another LWP is accepting the signal. */
			sigaddset(&p->p_sigaccept, sig);
		else {
			/*
			 * Either the calling LWP was accepting the signal,
			 * or we don't know if any LWP is accepting the signal,
			 */
			ssp->sst_acceptlwp = 0;
			lwpp = p->p_lwpp;
			do {
				if (!sigismember(&lwpp->l_sigheld, sig) &&
				    ((ssp->sst_rflags & SST_SIGWAIT) == 0 ||
				     lwpp->l_sigwait)) {
					ssp->sst_acceptlwp = lwpp->l_lwpid;
					sigaddset(&p->p_sigaccept, sig);
					break;
				}
			} while ((lwpp = lwpp->l_next) != NULL);
		}
		if (lwpp != NULL) {
			sigaddset(&lwpp->l_procsigs, sig);
			SIGNUDGE(lwpp, sig);
		}
	}
}


/*
 *
 * void unmask_signals(lwp_t *lwpp, k_sigset_t held_sigs)
 *	Unmask the given set of signals for the specified LWP.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the LWP must be held
 *	upon entry.  The p_mutex lock remains held upon return.
 *
 * Description:
 *	Accept set of specified process instance signals(held_sigs) that
 *	are not accepted by another LWP in the process.  In addition, any
 *	LWP instance signals that have been unmasked are added to the
 *	immediately pending set of signals active for the LWP.
 *
 *	This function is called whenever one or more signals become
 *	unmasked by the LWP.  This function does not alter the held
 *	signal mask of the LWP.
 *
 * Remarks:
 *	This function accquires and releases the l_mutex lock of the
 *	specified LWP.
 *
 */
void
unmask_signals(lwp_t *lwpp, k_sigset_t held_sigs)
{
	register proc_t *p;
	register int sig;
	k_sigset_t accept_sigs;

	p = lwpp->l_procp;

	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * If the set of signals being unmasked by the LWP includes pending
	 * process instance signals, then accept these pending signals.
	 */
	if (sigmembers(&p->p_sigs, &held_sigs)) {
		accept_sigs = held_sigs;
		sigandset(&accept_sigs, &p->p_sigs);
		sigdiffset(&accept_sigs, &p->p_sigaccept);
		if (!sigisempty(&accept_sigs)) {
			/*
			 * Accept pending unaccepted process instance signal(s).
			 */
			sigorset(&p->p_sigaccept, &accept_sigs);
			sigorset(&lwpp->l_procsigs, &accept_sigs);
			sigorset(&lwpp->l_sigs, &accept_sigs);
			if ((lwpp->l_trapevf & EVF_PL_SIG) == 0) {
				(void)LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_trapevf |= EVF_PL_SIG;
				UNLOCK(&lwpp->l_mutex, PLHI);
			}
		}
	}

	/*
	 * If the set of signals being unmasked by the LWP includes
	 * pending LWP instance signals, then add these signals to
	 * the immediately pending active signal set for the LWP.
	 */
	if (sigmembers(&lwpp->l_lwpsigs, &held_sigs)) {
		/*
		 * Pending LWP instance signals are being unmasked.
		 */
		accept_sigs = held_sigs;
		sigandset(&accept_sigs, &lwpp->l_lwpsigs);
		sigorset(&lwpp->l_sigs, &accept_sigs);
		if ((lwpp->l_trapevf & EVF_PL_SIG) == 0) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf |= EVF_PL_SIG;
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
	}

	/*
	 * Update the LWP signal acceptance information.
	 */
	while ((sig = sigdelnext(&held_sigs)) != 0) {
		/*
		 * Make ourselves known as willing to accept the process
		 * instance signal.
		 */
		ASSERT(sig > 0 && sig <= MAXSIG);
		p->p_sigstate[sig-1].sst_acceptlwp = lwpp->l_lwpid;
	}
}


/*
 *
 * void lwpsigmask_l(k_sigset_t mask)
 *	Set the held signal mask of the calling LWP to the given set.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held by the
 *	caller.  This lock remains held upon return.  Upon return,
 *	the held signal mask of the calling LWP is set accordingly.
 *
 */
void
lwpsigmask_l(k_sigset_t mask)
{
	k_sigset_t held_sigs;
	register lwp_t *lwpp;

	lwpp = u.u_lwpp;
	held_sigs = lwpp->l_sigheld;
	sigdiffset(&held_sigs, &mask);
	if (!sigisempty(&held_sigs)) {
		/*
		 * Signal(s) are being removed from the current
		 * held signals mask.  Promote any pending signal(s)
		 * to immediately pending, and record this LWP as
		 * willing to accept new instances of such signals.
		 */
		unmask_signals(lwpp, held_sigs);
	}
	held_sigs = mask;
	sigdiffset(&held_sigs, &lwpp->l_sigheld);
	lwpp->l_sigheld = mask;
	if (!sigisempty(&held_sigs)) {
		/*
		 * Signal(s) are being added to the held signals mask.
		 */
		sigandset(&held_sigs, &lwpp->l_sigs);
		if (!sigisempty(&held_sigs)) {
			/*
			 * Pushed back the held process instance signals
			 * to the process, and/or mask LWP instance signals.
			 */
			mask_signals(lwpp, held_sigs);
		}
	}
}


/*
 *
 * void lwpsigmask(k_sigset_t mask)
 * 	Set the held signal mask of the calling LWP to the given set.
 *
 * Calling/Exit State:
 *	Upon return, the held signal mask of the calling LWP is
 *	set accordingly.
 *
 * Remarks:
 *	This function accquires and releases the p_mutex lock of
 *	the calling process.
 *
 */
void
lwpsigmask(k_sigset_t mask)
{
	proc_t *p;
	pl_t pl;

	p = u.u_procp;
	pl = LOCK(&p->p_mutex, PLHI);
	lwpsigmask_l(mask);
	UNLOCK(&p->p_mutex, pl);
}


/*
 *
 * stopret_t stop(int why, int what)
 *	Put the calling LWP into the stopped state and notify tracers.
 *
 * Calling/Exit State:
 *   Locking:
 *	The p_mutex lock of the process containing the calling LWP must be
 *	held upon entry.  The p_mutex lock is released upon return.
 *	The l_mutex lock is acquired, and is held on return of STOP_SUCCESS.
 *
 *   Return value:
 *    STOP_DESTROY:
 *	The LWP cannot stop because one of the following reasons:
 *	     1.	There is a pending or current SIGKILL for the LWP.
 *	     2.	The EVF_PL_DESTROY flag for the LWP is set (indicating
 *		that the LWP should be destroyed, and possibly the
 *		entire process).
 *	The p_mutex lock is released upon return.
 *
 *    STOP_SUCCESS:
 *	The LWP is stopped.  The p_mutex lock is released upon return.
 *	The l_mutex lock of the calling LWP is returned as held.
 *
 *    STOP_FAILED:
 *	The LWP cannot stop because there is a pending rendezvous request.
 *
 */
stopret_t
stop(int why,			/* why LWP is stopped (e.g., PR_JOBSTOPPED) */
     int what)			/* what caused stop (e.g., signal number) */
{
	register lwp_t *lwpp;
	register proc_t *p;

	lwpp = u.u_lwpp;
	p = u.u_procp;

	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * If this LWP is stopping for its own reasons, but some other
	 * LWP has already caused a "synchronous stop" request, we must
	 * first respect the "synchronous stop" by stopping
	 * PR_REQUESTED.  When the synchronous stop is released, we may
	 * proceed with our own stop.
	 */
	while (why != PR_REQUESTED &&
	       (lwpp->l_trapevf & EVF_PL_PRSTOP) &&
	       !(p->p_flag & P_PRASYNC)) {
		stopret_t s = stop(PR_REQUESTED, 0);
		if (s != STOP_SUCCESS)
			return s;
		ASSERT(LOCK_OWNED(&lwpp->l_mutex));
		swtch(lwpp);		/* releases l_mutex */
		(void)LOCK(&p->p_mutex, PLHI);
	}

	/*
	 * If SIGKILL or EVF_PL_DESTROY are pending for the LWP, don't stop.
	 */
	if (lwpp->l_cursig == SIGKILL || sigismember(&lwpp->l_sigs, SIGKILL) ||
	    (lwpp->l_trapevf & EVF_PL_DESTROY)) {
		if ((lwpp->l_trapevf & EVF_PL_DESTROY) == 0) {
			/*
			 * Destroy all other LWPs, and set the current signal
			 * of the LWP to SIGKILL.
			 */
			post_destroy(B_TRUE, 0);
			lwpp->l_cursig = SIGKILL;
			UNLOCK(&p->p_mutex, PLBASE);
			/*
			 * An LWP is the only agent to change l_cursigst,
			 * except /proc when the LWP is in the SSTOP state.
			 */
			if (lwpp->l_cursigst.sst_info != NULL)
				DISCARD_CURSIGINFO(lwpp);
			lwpp->l_cursigst.sst_handler = SIG_DFL;
		} else
			UNLOCK(&p->p_mutex, PLBASE);
		return STOP_DESTROY;
	}

	/*
	 * If rendezvous request is pending and why is PR_SUSPENDED; fail
	 * the stop operation.  This is because if we allow the context to
	 * suspend itself and if a rendezvous is in progress, the process
	 * will essentially deadlock with itself.
	 */

	if ((lwpp->l_trapevf & EVF_PL_RENDEZV) && (why == PR_SUSPENDED)) {
		UNLOCK(&p->p_mutex, PLBASE);
		return STOP_FAILED;
	}

	/*
	 * Perform work that must be accomplished while holding p_mutex,
	 * but without holding l_mutex.
	 */
	if (why == PR_JOBCONTROL && p->p_sigjobstop == 0) {
		/* First lwp to stop */
		ASSERT(lwpp->l_cursig == what &&
			sigismember(&sig_stopdefault, what));

		lwpp->l_cursig = 0;
		if (lwpp->l_cursigst.sst_info != NULL)
			DISCARD_CURSIGINFO(lwpp);
		/*
		 * Record the signal that is causing the job control
		 * stop, and notify all other LWPs in the process.
		 */
		p->p_sigjobstop = (u_char) what;
		trapevproc(p, EVF_PL_JOBSTOP, B_FALSE);

		/*
		 * Must abort any on-going rendezvous.  Otherwise,
		 * deadlock could occur.  When all of the LWPs
		 * are released from the job control stop,
		 * the rendezvous will be reentered.
		 */
		abort_rendezvous();
	}

	lwpp->l_whystop = (short)why;		/* Record why and what under */
	lwpp->l_whatstop = (short)what;		/* p_mutex, but not l_mutex. */
	p->p_nstopped++;			/* one more LWP is stopped */

	if (p->p_nstopped == p->p_nlwp && p->p_sigjobstop != 0) {
		/*
		 * When all LWPs in the process are stopped, and any are
		 * stopped for job control, then wait/waitid/waitpid
		 * will report it as stopped for job control.  This prevents
		 * them from deadlocking with /proc due to some of the LWPs
		 * being stopped by /proc, and some being stopped for job
		 * control, while both are waiting for the entire process
		 * to become stopped.
		 */
		p->p_wcode = CLD_STOPPED;
		p->p_wdata = p->p_sigjobstop;
		sigcld(p);
	}

	/*
	 * Some things have to be done before l_mutex is acquired, and
	 * some things afterward.
	 */
	switch (why) {
	case PR_SUSPENDED:
		/*
		 * If we are going to suspend ourselves, wakeup all the
	 	 * waiters.  Note that we are presently holding p_mutex
		 * and since all contexts that may decide to wait for an
		 * lwp to suspend do so holding p_mutex, they will not
		 * miss the wakeup.
	 	 */
		if (SV_BLKD(&p->p_suspsv))
			SV_BROADCAST(&p->p_suspsv, 0);
		break;

	case PR_REQUESTED:
		p->p_nreqstopped++;
		/* FALLTHROUGH */
	case PR_SIGNALLED:
	case PR_SYSENTRY:
	case PR_SYSEXIT:
	case PR_FAULTED:
		p->p_nprstopped++;
		prstopped(lwpp);
		break;

	default:
		break;
	}

	/*
	 * Hold both l_mutex and p_mutex during the transition from SONPROC
	 * to SSTOP.
	 */
	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SSTOP;

	switch (why) {
	case PR_SUSPENDED:
		lwpp->l_trapevf &= ~EVF_PL_SUSPEND;
		lwpp->l_flag |= (L_SUSPENDED | L_NWAKE);
		ASSERT(!(lwpp->l_trapevf & EVF_PL_PRSTOP));
		break;
	case PR_JOBCONTROL:
		lwpp->l_trapevf &= ~EVF_PL_JOBSTOP;
		lwpp->l_flag |= L_JOBSTOPPED;
		break;

	case PR_REQUESTED:
	case PR_SIGNALLED:
	case PR_SYSENTRY:
	case PR_SYSEXIT:
	case PR_FAULTED:
		lwpp->l_trapevf &= ~EVF_PL_PRSTOP;
		lwpp->l_flag |= L_PRSTOPPED;
		break;

	default:
		/*
		 *+ An invalid "why" argument was given to the stop()
		 *+ function.
		 */
		cmn_err(CE_PANIC, "Invalid 'why' argument to stop().");
	}

	UNLOCK(&p->p_mutex, PLHI);		/* keep PLHI for l_mutex */
	CL_STOP(lwpp, lwpp->l_cllwpp);
	return STOP_SUCCESS;
}


/*
 *
 * issigret_t issig(lock_t *held_lock)
 *	Determine if the calling LWP has unheld pending signals to be
 *	acted upon.  Requests to rendezvous for forkall(2) system calls,
 *	LWP destruction requests, and system call cancellation are also
 *	handled as pseudo-signals.
 *
 *	Job control stops and /proc requested stops are handled
 *	internally by this function in concert with the stop() function.
 *
 * Calling/Exit State:
 *   Locking:
 *	Neither the l_mutex or p_mutex lock of the respective calling LWP
 *	and process can be held upon entry.  The only possible exception
 *	to this rule occurs when the "held lock" parameter is the p_mutex
 *	lock of the process containing the calling LWP.
 *
 *	Upon return, the l_mutex of the calling LWP, and the passed in held
 *	lock (if there is one), are returned held only if ISSIG_NONE is
 *	returned.  In all other cases, the held lock is released.
 *
 *   Return values:
 *	ISSIG_NONE:
 *		No signals exist, and no attempt was made to abort the
 *		current system call.
 *	ISSIG_SIGNALLED:
 *		The LWP has a signal to process, the current system call
 *		has been aborted, the calling LWP is to be destroyed, or
 *		the calling LWP must rendezvous.  In addition, the LWP may
 *		have entered the stopped state for job control or /proc
 *		tracing.
 *	ISSIG_STOPPED:
 *		The LWP was stopped by job control or a /proc requested
 *		stop only.
 *
 * Description:
 *	This is asked at least once each time a process enters the system
 *	using the following sequence:
 *
 *		if (QUEUEDSIG(lwpp)) {
 *			if (lwpp->l_cursig != 0) {
 *				psig();
 *			} else {
 *				switch (issig((lock_t *)NULL)) {
 *				case ISSIG_NONE:
 *					UNLOCK(&lwpp->l_mutex, PLBASE);
 *					break;
 *				case ISSIG_SIGNALLED:
 *					psig();
 *					break;
 *				default:	-- ISSIG_STOPPED
 *					break;
 *				}
 *			}
 *		}
 *
 *
 *	When called from any of the kernel synchronization primitives
 *	before blocking on the synchronization object, the following
 *	sequence is used:
 *
 *		pl = LOCK(&lwpp->l_mutex, PLHI);
 *		if (QUEUEDSIG(lwpp)) {
 *
 *			-- We have a signal to process (though
 *			-- a job control signal could be cancelled
 *			-- or signal registration can be changed
 *			-- by the time we get around to calling
 *			-- issig() below).
 *
 *			UNLOCK(&lwpp->l_mutex, pl);
 *			switch (issig(held_lock)) {
 *			case ISSIG_NONE:
 *
 *				-- A job control stop signal was
 *				-- cancelled by a defaulted or ignored
 *				-- SIGCONT signal, or a caught SIGCONT
 *				-- signal was cancelled by an ignored
 *				-- or held job control stop signal or
 *				-- signal registration is changed to
 *				-- ignored.  In any case, l_mutex is
 *				-- held, and p_mutex is dropped.
 *				break;
 *
 *			case ISSIG_SIGNALLED:	-- all locks dropped
 *				return EINTR;
 *
 *			case ISSIG_STOPPED:	-- all locks dropped
 *
 *				-- If a condition, return, else loop
 *				-- to try again within sync primitive;
 *
 *				return or loop within sync primitive;
 *			}
 *		}
 *
 *		-- l_mutex is held.  No signals.  Try for sync object.
 *
 *
 *	When called from the kernel synchronization primitives after
 *	having blocked on the synchronization object (made runnable either
 *	by a signal or because the synchronization object was granted), the
 *	following sequence is used:
 *
 *		-- Blocked on the synchronization object
 *		swtch();
 *
 *		if (lwpp->l_flag & L_SIGWOKE) {
 *			switch (issig((lock_t *)NULL)) {
 *			case ISSIG_NONE:	--  l_mutex is held.
 *				UNLOCK(&l_mutex, spl_held_lock);
 *
 *				-- If a condition, return, else loop again.
 *				return or loop within sync primitive;
 *
 *			case ISSIG_SIGNALLED:	-- all locks dropped
 *				return EINTR;
 *
 *			case ISSIG_STOPPED:	-- all locks dropped
 *				-- If a condition, return, else loop again.
 *				return or loop within sync primitive;
 *			}
 *		}
 *
 *
 *	A signal does not do anything directly to an LWP, instead it
 *	sets a flag that asks the LWP to do something to itself.  If
 *	the signal is defaulted, then the entire process is affected.
 *
 *	If an unheld signal is pending, then the signal to process is
 *	placed in l_cursig, along with all disposition information for
 *	the signal which is placed in l_cursigst.
 *
 */
issigret_t
issig(lock_t *held_lock)
{
	k_sigset_t pending_sigs;	/* pending signal set for LWP */
	register lwp_t *lwpp;		/* calling LWP */
	register proc_t *p;		/* calling process */
	int sig;			/* current signal */
	sigqueue_t *sqp;		/* pointer to signal info */
	issigret_t rval;		/* return value */
	void (*handler)();		/* handler for posted signal */

	/*
	 * If l_cursig is set, then we must not allow ourselves to
	 * become stopped by a debugger to set a new current signal.
	 */
	lwpp = u.u_lwpp;
	if (lwpp->l_cursig != 0) {
		if (held_lock != NULL)
			UNLOCK(held_lock, PLBASE);
		return ISSIG_SIGNALLED;
	}

	p = u.u_procp;
	if (held_lock != &p->p_mutex)
		(void)LOCK(&p->p_mutex, PLHI);

	/*
	 * If the LWP is to be destroyed return as though signalled.
	 */
	if (lwpp->l_trapevf & EVF_PL_DESTROY) {
		RELEASE_HELD_LOCK(held_lock, PLHI, p);
		UNLOCK(&p->p_mutex, PLBASE);
		return ISSIG_SIGNALLED;
	}

	rval = ISSIG_NONE;

loop:

	while (lwpp->l_trapevf & EVF_PL_PRSTOP) {
		/*
		 * Stop for /proc request.
		 */
		RELEASE_HELD_LOCK(held_lock, PLHI, p);
		if (stop(PR_REQUESTED, 0) == STOP_DESTROY)
			return ISSIG_SIGNALLED;
		/*
		 * The LWP was stopped.  p_mutex is dropped, l_mutex is held.
		 */
		rval = ISSIG_STOPPED;
		ASSERT(LOCK_OWNED(&lwpp->l_mutex));
		swtch(lwpp);		/* releases l_mutex */

		(void)LOCK(&p->p_mutex, PLHI);
		/*
		 * The debugger always wins job control state.
		 */
		if ((sig = lwpp->l_cursig) != 0 &&
		    sigismember(&sig_jobcontrol, sig))
			cancel_jobctrl(p, sig);
	}

	if (lwpp->l_trapevf & EVF_PL_SUSPEND) {
		/* Suspend the current context */
		RELEASE_HELD_LOCK(held_lock, PLHI, p);
		if (stop(PR_SUSPENDED, 0) != STOP_SUCCESS)
			/*
			 * There is a pending rendezvous request or a
			 * destroy request return as if signalled.  Let
			 * psig() sort all this out.
			 */
			return ISSIG_SIGNALLED;

		/*
		 * The lwp will be suspended. p_mutex dropped, l_mutex held.
		 */
		rval = ISSIG_STOPPED;
		ASSERT(LOCK_OWNED(&lwpp->l_mutex));
		swtch(lwpp);
		(void)LOCK(&p->p_mutex, PLHI);
	}

	/*
	 * If another LWP in the process has taken delivery of a defaulted
	 * job control stop signal, then all other LWPs must become stopped
	 * by the same signal (in the absence of other stop requests).  Such
	 * a state overrides the signal in l_cursig, as set by a debugger.
	 * When the process is continued (via SIGCONT), the LWP will then
	 * act upon the signal set by the debugger.
	 */
	if ((sig = p->p_sigjobstop) == 0 && (sig = lwpp->l_cursig) == 0)
		goto next_sig;
	handler = lwpp->l_cursigst.sst_handler;
	if ((handler == SIG_DFL && !sigismember(&sig_ignoredefault, sig)) ||
	    p->p_sigjobstop) {
		if (!sigismember(&sig_jobcontrol, sig)) {    /* fatal sig */
			RELEASE_HELD_LOCK(held_lock, PLHI, p);
			/*
			 * Destroy all of the other LWPs in the
			 * process, and return with all signal
			 * information intact.
			 */
			if ((lwpp->l_trapevf & EVF_PL_DESTROY) == 0) {
				/*
				 * Won any race with another LWP released
				 * from the stopped state that also has a
				 * fatal signal pending.  Destroy all of the
				 * other LWPs in the process, and return with
				 * all signal information intact.
				 */
			    	if (sigismember(&sig_coredefault, sig)) {
					/*
					 * rendezvous all LWPs in the process
					 * first to generate the core file.
					 */
					post_destroy(B_TRUE, EVF_PL_RENDEZV);
				} else
					post_destroy(B_TRUE, 0);
			}
			UNLOCK(&p->p_mutex, PLBASE);
			return ISSIG_SIGNALLED;
		}
		/*
		 * The signal is a job control signal.
		 */
		if (sig == SIGCONT ||
		    (sig != SIGSTOP && (p->p_flag & P_PGORPH))) {
			/*
			 * The defaulted signal does not result in a job
			 * control stop.  Discard the current signal.
			 */
			lwpp->l_cursig = 0;
			if (lwpp->l_cursigst.sst_info != NULL)
				DISCARD_CURSIGINFO(lwpp);
		} else {

			/* The LWP is to be stopped or destroyed */

			RELEASE_HELD_LOCK(held_lock, PLHI, p);

			if (stop(PR_JOBCONTROL, sig) == STOP_DESTROY)
				return ISSIG_SIGNALLED;
			/* p_mutex is dropped, l_mutex is held */
			rval = ISSIG_STOPPED;
			ASSERT(LOCK_OWNED(&lwpp->l_mutex));
			swtch(lwpp);	/* releases l_mutex */

			(void)LOCK(&p->p_mutex, PLHI);
			ASSERT((lwpp->l_trapevf & EVF_PL_SYSABORT) == 0);
			/*
			 * Loop back to allow /proc stop requests.
			 */
			goto loop;
		}
	} else if (handler == SIG_IGN ||
		   (handler == SIG_DFL &&
		    sigismember(&sig_ignoredefault, sig))) {
		/*
		 * This case can only happen when a debugger deposits
		 * a signal and associated information directly
		 * into l_cursig and l_cursigst, respectively.
		 */
		lwpp->l_cursig = 0;
		if (lwpp->l_cursigst.sst_info != NULL)
			DISCARD_CURSIGINFO(lwpp);
	} else {	/* The signal is being caught or sigwaited for */
		RELEASE_HELD_LOCK(held_lock, PLHI, p);
		UNLOCK(&p->p_mutex, PLBASE);
		return ISSIG_SIGNALLED;
	}

next_sig:
	/*
	 * There is no current signal.
	 */
	if (sigismember(&lwpp->l_sigs, SIGKILL)) {
		sig = SIGKILL;
	} else {
		pending_sigs = lwpp->l_sigs;
		if (p->p_flag & P_VFORK)
			sigdiffset(&pending_sigs, &sig_holdvfork);
		sig = signext(&pending_sigs);
		ASSERT(sig >= 0 && sig <= MAXSIG);
		if (sig == 0) {
			/*  There are no unmasked pending signals. */
			if (lwpp->l_trapevf &
			    (EVF_PL_RENDEZV|EVF_PL_SYSABORT|EVF_PL_DESTROY)) {
				/*
				 * A rendezvous, destroy, or abort-system call
				 * request is pending.  Behave as though the
				 * LWP was signalled.
				 */
				RELEASE_HELD_LOCK(held_lock, PLHI, p);
				rval = ISSIG_SIGNALLED;
			}
			/*
			 * Check to see if we need to clear the EVF_PL_SIG
			 * flag.  We clear the flag if no signals that can be
			 * acted upon are pending.  Hence, if this is a
			 * vforked child and the only signals pending are
			 * the signals that this process should not see,
			 * we clear the EVF_PL_SIG flag.  Note that the
			 * flag will be set if the process execs and these
			 * signals are pending.
			 */
			if ((lwpp->l_trapevf & EVF_PL_SIG) &&
			     sigisempty(&pending_sigs)) {
				(void)LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_trapevf &= ~EVF_PL_SIG;
				if (rval != ISSIG_NONE)
					UNLOCK(&lwpp->l_mutex, PLHI);
			} else if (rval == ISSIG_NONE)
				(void)LOCK(&lwpp->l_mutex, PLHI);
			ASSERT(LOCK_OWNED(&p->p_mutex));
			if (rval != ISSIG_NONE)
				UNLOCK(&p->p_mutex, PLBASE);
			else if (held_lock != &p->p_mutex)
				UNLOCK(&p->p_mutex, PLHI);
			return rval;
		}
/* Enhanced Application Compatibility Support */
#ifdef VIRTUAL_XOUT
		if (VIRTUAL_XOUT && sig == SIGPOLL)
			sig = XENIX_SIGPOLL;
#endif
/* End Enhanced Application Compatibility Support */
	}

	/*
	 * There are pending signal(s) to be dealt with.
	 * Therefore, drop the held lock now.
	 */
	RELEASE_HELD_LOCK(held_lock, PLHI, p);

	/*
	 * Take the signal.  It is an error to get here and have signal sig
	 * being ignored, unless such signals are being traced or sigwaited
	 * for.  Remove the signal from various masks.
	 */
	ASSERT(sig != 0);

	if (sigismember(&lwpp->l_procsigs, sig)) {
		sigdelset(&lwpp->l_procsigs, sig);	/* promote proc sig */
		RMPROC_SIG(p, sig);
		if (!sigismember(&lwpp->l_lwpsigs, sig))
			sigdelset(&lwpp->l_sigs, sig);
		if (lwpp->l_sigwait) {
			/*
			 * If the signal is sigwaited for, then the signal must
			 * always appear as caught.  Also, if l_sigwait is set,
			 * then there is NO associated signal information, as
			 * by the time l_sigwait is set, any such information
			 * will have been discarded and overlayed with the
			 * sst_swcount field (see sigwait(2)).
			 */
			lwpp->l_cursigst.sst_handler = (void(*)())issig;
			lwpp->l_cursigst.sst_info = NULL;
		} else {
			lwpp->l_cursigst = p->p_sigstate[sig-1];
			p->p_sigstate[sig-1].sst_info = NULL;
		}
	} else {
		sigdelset(&lwpp->l_lwpsigs, sig);
		sigdelset(&lwpp->l_sigs, sig);
		/*
		 * Dequeue any associated signal information.
		 */
		sqp = NULL;
		if (sigismember(&lwpp->l_sighasinfo, sig))
			RMLWP_SIGINFO(lwpp, sig, sqp);
		lwpp->l_cursigst = p->p_sigstate[sig-1];
		lwpp->l_cursigst.sst_info = sqp;
		if (lwpp->l_sigwait) {
			/*
			 * If the LWP instance signal was posted in advance
			 * of the LWP calling sigwait(2), there may be signal
			 * information to pick up.  However, we must still
			 * make sure the signal appears as caught.
			 */
			lwpp->l_cursigst.sst_handler = (void(*)())issig;
		}
	}

	/*
	 * Establish current signal.
	 */
	lwpp->l_cursig = (u_char) sig;

	/*
	 * If tracing, stop.
	 */
	if (tracing(p, sig)) {
		if (stop(PR_SIGNALLED, sig) == STOP_DESTROY)
			return ISSIG_SIGNALLED;

		/* l_mutex is held */
		rval = ISSIG_STOPPED;
		ASSERT(LOCK_OWNED(&lwpp->l_mutex));
		swtch(lwpp);	/* releases l_mutex */

		(void)LOCK(&p->p_mutex, PLHI);
		/*
		 * Debugger always wins job control state.
		 */
		sig = lwpp->l_cursig;
		if (sigismember(&sig_jobcontrol, sig))
			cancel_jobctrl(p, sig);
	}

	/*
	 * Loop around to check for requested stop before
	 * performing the usual current-signal actions.
	 */
	goto loop;
}


/*
 *
 * void setsigact(int sig, void (*disp()), k_sigset_t mask, uint flags)
 * 	Set the signal action of the given signal according to the
 * 	passed-in disposition.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held.
 *	Upon return, the p_mutex lock is dropped, and the pl level
 *	is to PLBASE.
 *
 */
void
setsigact(int sig, void (*disp)(), k_sigset_t mask, uint flags)
{
	register proc_t *p;
	register sigstate_t *ssp;
	sigqueue_t *sqp, *nextsqp;

	ASSERT(sig > 0 && sig <= MAXSIG);

	p = u.u_procp;
	sqp = NULL;
	ssp = &p->p_sigstate[sig - 1];
	ssp->sst_handler = disp;
	if (disp == SIG_DFL) {
		if (sigismember(&sig_ignoredefault, sig)) {
			sigaddset(&p->p_sigignore, sig);
			discard_sig(p, sig, &sqp);
		} else
			sigdelset(&p->p_sigignore, sig);
		ssp->sst_cflags = 0;
	} else if (disp == SIG_IGN) {
		sigaddset(&p->p_sigignore, sig);
		discard_sig(p, sig, &sqp);
		ssp->sst_cflags = 0;
	} else {				/* catching signal */
		uchar_t cflags = (uchar_t)flags;
		sigdelset(&p->p_sigignore, sig);
		sigdiffset(&mask, &sig_cantmask);
		ssp->sst_held = mask;
		cflags &= (SA_ONSTACK|SA_RESETHAND|SA_RESTART|
			   SA_SIGINFO|SA_NODEFER);
		if (sigismember(&sig_cantreset, sig))
			cflags &= ~SA_RESETHAND;
		ssp->sst_cflags = cflags;
	}

	switch (sig) {
	case SIGCLD:
		p->p_flag &= ~(P_NOWAIT|P_JCTL);	/* start clean */
		if (!(flags & SA_NOCLDSTOP))
			p->p_flag |= P_JCTL;
		if (flags & SA_NOCLDWAIT) {
			register proc_t *cp;

			p->p_flag |= P_NOWAIT;
			for (cp = p->p_child; cp != NULL; cp = cp->p_nextsib) {
				if (cp->p_nlwp == 0 && cp->p_wcode != 0) {
					/*
					 * We've found an unwaited for zombie
					 * that exited previous to our lack
					 * of interest in the matter.
					 * Add it to the list of zombies which
					 * are disposed of by freeprocs().
					 */
					cp->p_nextzomb = p->p_zombies;
					p->p_zombies = cp;
					PROC_NOWAIT(cp);
				}
			}
			if ((p->p_flag & P_CLDWAIT) == 0 &&
			    (p->p_zombies != NULL)) {
				/*
				 * No other LWP in the process is in:
				 * _exit(2)/wait(2)/waitid(2)/waitpid(2).
				 */
				freeprocs();  /* releases p_mutex */
			} else {
				/*
				 * Let _exit(2)/wait(2)/waitid(2)/waitpid(2)
				 * clean up the zombies if any.
				 */
				UNLOCK(&p->p_mutex, PLBASE);
			}
		} else
			UNLOCK(&p->p_mutex, PLBASE);
		break;
	case SIGWAITING:
		if (flags & SA_WAITSIG)
			p->p_sigwait = B_TRUE;
		UNLOCK(&p->p_mutex, PLBASE);
		/* Note that p_sigwait is not reset by clearing SA_WAITSIG. */
		break;
	default:
		UNLOCK(&p->p_mutex, PLBASE);
		break;
	}

	/*
	 * No spin locks are held.
	 * Free up signal information structures if we have any.
	 */
	while (sqp != NULL) {
		nextsqp = sqp->sq_next;
		kmem_free(sqp, sizeof *sqp);
		sqp = nextsqp;
	}
}


/*
 *
 * void psig(void)
 * 	Perform the action specified by the current signal, and/or deal
 *	with rendezvous and destroy requests.
 *
 * Calling/Exit State:
 * 	The signal bit for the calling LWP/process has already been cleared by
 *	issig(), the current signal number and all disposition information and
 *	associated information are respectively recorded in lwpp->l_cursig and
 *	lwpp->l_cursigst, if the calling LWP has a current signal.
 *	The function must be called at PLBASE.
 *
 * Remarks:
 *	While examining the l_trapevf field for EVF_PL_RENDEZV and
 *	EVF_PL_DESTROY, locks are not held.  The race
 *	with this field being set is caught by the fact that the calling LWP
 *	is SONPROC and therefore will be "nudged" (l.runrun flag will be set).
 *	The l.runrun flag forces the LWP to return to trap processing to
 *	retest for these condition before returning to user-mode.
 */
void
psig(void)
{
	extern boolean_t sendsig(int, sigqueue_t *sqp);
					/* machine-dependent send signal fn */

	k_sigset_t held_sigs;
	register proc_t *p;
	register lwp_t *lwpp;
	register int sig;
	sigqueue_t *sqp;
	boolean_t rval;
	boolean_t destroyed;
	register int exitcode;

	exitcode = CLD_KILLED;
	lwpp = u.u_lwpp;
	p = u.u_procp;


	/*
	 * issig() guarantees that SIGKILL will be chosen before all
	 * other pending signals if SIGKILL is pending to the process
	 * or LWP.
	 */

loop:
	/*
	 * A rendezvous request for forkall(2) or
	 * core dump generation (EVF_PL_RENDEZV), or a destroy request
	 * due to fatal signal delivery, _exit(2), or exec(2)
	 * (EVF_PL_DESTROY) are handled here.
	 */
	if (lwpp->l_trapevf & (EVF_PL_RENDEZV|EVF_PL_DESTROY)) {

		/*
		 * Check for rendezvous requests first to allow
		 * all of the LWPs except the LWP that took
		 * delivery of the fatal signal to rendezvous to
		 * generate core dumps.
		 */
		if (lwpp->l_trapevf & EVF_PL_RENDEZV) {
			/*
			 * The errno is set to ERESTART in systrap() when
			 * the system call returned EINTR and no signal
			 * exists for the LWP.  This implies that the EINTR
			 * was returned due to EVF_PL_RENDEZV synchronization
			 * request, or by an EVF_PL_DESTROY request.  In the
			 * latter case however, the LWP will be destroyed, so
			 * the setting of errno to  ERESTART does not matter.
			 *
			 * Wakeup the requesting LWP(s) if we are the
			 * last LWP to enter the rendezvous.  Block on
			 * p_rendezvoused.
			 */
			(void)LOCK(&p->p_mutex, PLHI);
			if (lwpp->l_trapevf & EVF_PL_RENDEZV) {
				/* Did not race with destroy */
				if (++p->p_nrendezvoused == p->p_nlwp - 1)
					SV_SIGNAL(&p->p_rendezvous, 0);

				SV_WAIT(&p->p_rendezvoused, PRIMED,
					&p->p_mutex);
				/*
				 * We may have been suspended but forced to
				 * run just to complete the rendezvous.
				 */
				if ((lwpp->l_flag & L_SUSPENDED) ||
				    (lwpp->l_trapevf & EVF_PL_SUSPEND)) {
					(void) LOCK(&p->p_mutex, PLHI);
					(void) LOCK(&lwpp->l_mutex, PLHI);
					/*
					 * Now that we hold l_mutex
					 * check again.
					 */
					if ((lwpp->l_flag & L_SUSPENDED) ||
					    (lwpp->l_trapevf &
				             EVF_PL_SUSPEND)) {

						lwpp->l_trapevf |=
						EVF_PL_SUSPEND;
						UNLOCK(&lwpp->l_mutex, PLHI);
						if (stop(PR_SUSPENDED, 0)
						    != STOP_SUCCESS)
							/*
						 	 * Either a rendezvous
						 	 * or a destroy
							 * request is
						 	 * pending.  Loop back
						 	 * and do the "right"
							 * thing.
						 	 * Note that p_mutex is
						 	 * dropped.
						 	 */
							goto loop;
						/*
					 	 * The LWP was stopped. p_mutex
					 	 * is released and
						 * l_mutex is held.
					 	 * Give up the processor.
					 	 */
						swtch(lwpp);
					} else {
						/*
						 * We raced against an
						 * _lwp_continue() resquest.
						 */
						UNLOCK(&lwpp->l_mutex, PLHI);
						UNLOCK(&p->p_mutex, PLBASE);
					}
				}
			} else {
				/*
				 * A destroy request has cleared the
				 * EVF_PL_RENDEZV flag.
				 */
				UNLOCK(&p->p_mutex, PLBASE);
			}
		}
		if (lwpp->l_trapevf & EVF_PL_DESTROY) {

			/* LWP is requested to be destroyed */
			lwp_exit();		/* no return */

			/* NOTREACHED */
		}

		if (lwpp->l_trapevf & (EVF_PL_JOBSTOP|EVF_PL_PRSTOP)) {
			/*
			 * The rendezvous was aborted by a job control or a
			 * /proc stop request.
			 */
			if (lwpp->l_cursig == 0 &&
			    issig((lock_t *)NULL) == ISSIG_SIGNALLED) {
				/* rendezvous, destroy request, or a signal. */
				goto loop;	/* start over */
			}
		}

	}

	/*
	 * Handle the current signal (if there is one).
	 */
	if ((sig = lwpp->l_cursig) == 0)
		return;				/* All done */

	/*
	 * There is a current signal to handle.
	 * For fatal signals, all other LWPs except the LWP that
	 * took delivery have been destroyed above.
	 */

	ASSERT(lwpp->l_cursigst.sst_handler != SIG_IGN);

	/* Exit immediately on a ptrace exit request. */
	if (p->p_flag & P_TRC && sigismember(&p->p_sigs, SIGKILL))
		exit(exitcode, SIGKILL);	/* no return */

	if (lwpp->l_cursigst.sst_handler != SIG_DFL) {

		/*
		 * The signal is being caught.
		 */
		if (u.u_sigflag & SOMASK)
			u.u_sigflag &= ~SOMASK;
		else
			u.u_sigoldmask = lwpp->l_sigheld;

		/*
		 * Clear the current signal, and adjust the held signal
		 * mask as necessary.
		 */
		sqp = lwpp->l_cursigst.sst_info;
		(void)LOCK(&p->p_mutex, PLHI);
		/*
		 * l_cursig is cleared only while holding p_mutex to avoid
		 * races with code that cancel signals (e.g., cancel_jobctrl())
		 * that must have a stable view of l_cursig while holding
		 * p_mutex.
		 */
		lwpp->l_cursig = 0;
		lwpp->l_cursigst.sst_info = NULL;
		sigorset(&lwpp->l_sigheld, &lwpp->l_cursigst.sst_held);
		if ((lwpp->l_cursigst.sst_cflags & SA_NODEFER) == 0)
			sigaddset(&lwpp->l_sigheld, sig);
		if (sigmembers(&lwpp->l_sigheld, &lwpp->l_sigs)) {
			/*
			 * There exist held process instance signals
			 * to be pushed back to the process, and/or
			 * LWP instance signals to be masked.
			 */
			held_sigs = lwpp->l_sigheld;
			sigandset(&held_sigs, &lwpp->l_sigs);
			mask_signals(lwpp, held_sigs);
		} else if (sigisempty(&lwpp->l_sigs)) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~EVF_PL_SIG;
			UNLOCK(&lwpp->l_mutex, PLHI);
		}

		/*
		 * Reset handler as needed.
		 */
		if (lwpp->l_cursigst.sst_cflags & SA_RESETHAND) {
			k_sigset_t held;
			sigemptyset(&held);
			setsigact(sig, SIG_DFL, held, 0);
			/* p_mutex is returned unlocked */
		} else
			UNLOCK(&p->p_mutex, PLBASE);

		/*
		 * Send the signal.
		 */
		if (sqp != NULL && (lwpp->l_cursigst.sst_cflags & SA_SIGINFO)) {
			rval = sendsig(sig, sqp);
		} else
			rval = sendsig(sig, (sigqueue_t *)NULL);

		if (sqp != NULL)
			siginfo_free(sqp);

		if (rval)
			return;

		/*
		 * We were unable to deliver the signal.
		 * Destroy the process with SIGSEGV.
		 */
		sig = SIGSEGV;
		destroyed = B_FALSE;
	} else
		destroyed = B_TRUE;

	/*
	 * The signal is defaulted or undeliverable,
	 * generate the core file if necessary.
	 */
	if (sigismember(&sig_coredefault, sig)) {
	 	/* Dump core. */
		if (core(p->p_pidp->pid_id, lwpp->l_cred,
			 u.u_rlimits->rl_limits[RLIMIT_CORE].rlim_cur,
			 sig, destroyed) == 0) {
			exitcode = CLD_DUMPED;
		}
	}

#ifdef ISC_USES_POSIX
	if (ISC_USES_POSIX)
		switch (sig) {
		case SIGCONT:
			sig = ISC_SIGCONT;
			break;
		case SIGSTOP:
			sig = ISC_SIGSTOP;
			break;
		case SIGTSTP:
			sig = ISC_SIGTSTP;
			break;
		}
#endif

	exit(exitcode, sig);		/* no return */
}


/*
 *
 * void sigsendinit(sigsend_t *sigsendp,
 *		    ulong_t init_count, int sig, boolean_t checkperm)
 *	Allocate a process instance signal information object with an
 *	initial reference count of (init_count), and attach it to the given
 *	sigsend_t object (sigsendp) via ss_sqp.  Also initialize the
 *	sigsend_t object (sigsendp) with the signal being sent (sig).
 *	Also set up audit data structure.
 *
 * Calling/Exit State:
 *    Locking:
 *	No locks should be held when calling this function.
 *    Parameters:
 *	sigsendp references the sigsend_t object to be initialized.
 *	init_count defines the initial reference count on the process
 *	      instance signal object allocated, initialized, and
 *	      attached to the sigsend_t object.
 *	checkperm if B_TRUE indicates that permissions must be
 *	      checked when signalling the selected processes.
 *
 * Remarks:
 *	This function in combination with sigsenddone() and sigsendproc(),
 *	is used by the sigsendset() kernel primitive, as well as by /proc.
 *
 */
void
sigsendinit(register sigsend_t *sigsendp,
	    ulong_t init_count, int sig, boolean_t checkperm)
{
	register sigqueue_t *sqp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sqp = siginfo_get(KM_SLEEP, init_count);
	sqp->sq_info.si_signo = sig;
	sqp->sq_info.si_code = SI_USER;
	sqp->sq_info.si_pid = u.u_procp->p_pidp->pid_id;
	sqp->sq_info.si_uid = u.u_lwpp->l_cred->cr_ruid;
	sigsendp->ss_sig = sig;
	sigsendp->ss_sqp = sqp;
	sigsendp->ss_checkperm = checkperm;
	sigsendp->ss_perm = B_FALSE;
	sigsendp->ss_count = 0; /* #of procs that accepted sigqueue info */
	sigsendp->ss_pidlistp = NULL;
	ADT_SIGINIT(u.u_lwpp->l_auditp, sigsendp);
}


/*
 *
 * int sigsendproc(proc_t *pp, void *vpv)
 *	Post the signal described in the sigsend_t object (aliased by vpv)
 *	to the non-zombie, non-system process (pp).
 *
 * Calling/Exit State:
 *    Parameters:
 *	A sigqueue_t structure containing information to be sent with
 *	the signal in ((sigsend_t *)vpv)->ss_sig must be referenced from
 *	the sigsend_t object by ((sigsend_t *)vpv)->ss_sqp field.
 *    Locking:
 *	The p_mutex lock of the target process must be held upon entry.
 *	This lock remains held upon return.
 *    Return value:
 *	EPERM if an attempt is made to signal init with an uncatchable signal.
 *	ESRCH if the process should not be recognized as existing for covert
 *	  channel reasons.  When ESRCH is returned, the caller must
 *	  pretend that the process is not in the selected process set
 *	  when this function is called via dotoprocs().
 *	Zero otherwise, indicating success.
 *
 * Remarks:
 *	This function in combination with sigsendinit() and sigsenddone(),
 *	is used by the sigsendset() kernel primitive, as well as by /proc
 *	to send a signal to the given process.
 *
 */
int
sigsendproc(register proc_t *pp, void *vpv)
{
	sigsend_t *pv;
	lwp_t *lwpp = u.u_lwpp;
	cred_t *lcrp = lwpp->l_cred;
	cred_t *pcrp = pp->p_cred;

	ASSERT(LOCK_OWNED(&pp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0);

	pv = (sigsend_t *)vpv;
	
	ADT_SIG_GPID(pv, pp);
	if (pp == proc_init &&
	    pv->ss_sig && sigismember(&sig_cantmask, pv->ss_sig))
		return EPERM;

	if (MAC_ACCESS(MACEQUAL, lcrp->cr_lid, pcrp->cr_lid) &&
	    pm_denied(lcrp, P_MACWRITE) && pm_denied(lcrp, P_COMPAT) &&
	    pm_denied(pcrp, P_COMPAT)) {
		if (MAC_ACCESS(MACDOM, lcrp->cr_lid, pcrp->cr_lid)) {
			return ESRCH;	/* don't make process visible */
		} else
			return 0;	/* normal error, pv->ss_perm not set */
	}

#ifdef CC_PARTIAL
	MAC_ASSERT(pp, MAC_SAME);
#endif

	/*
	 * Neither the p_mutex nor the session lock of the calling process/LWP
	 * is held.  Therefore, it is possible that one of the LWPs in the
	 * calling process can perform a setsid(2) or setpgrp(2) system call,
	 * and change the calling process' p_sid field.
	 *
	 * This is of no interest, except in the case of the SIGCONT signal,
	 * which allows the signalling of a process so long as the target
	 * process is in the same session as the caller even when the
	 * permissions mechanisms would otherwise prevent this from happening
	 * (See IEEE POSIX 1003.1).  In this case, a racing setsid(2) or
	 * setpgrp(2) operation by another LWP in the calling process can
	 * cause some process that would have otherwise been signalled, to
	 * not receive the signal.
	 *
	 * This does not create any security problems, as the only result
	 * of the race is to PREVENT processes from being signalled that would
	 * have otherwise received the signal.
	 */
	if (!pv->ss_checkperm || hasprocperm(pcrp, lcrp)
	  || (pv->ss_sig == SIGCONT && pp->p_sid == u.u_procp->p_sid)) {
		pv->ss_perm = B_TRUE;      /* we have permission! */
		if (pv->ss_sig) 
			pv->ss_count += sigtoproc_l(pp, pv->ss_sig, pv->ss_sqp);
	
	}
	return 0;
}


/*
 *
 * void sigsenddone(sigsend_t *sigsendp, ulong_t init_refcount)
 *	Release the remaining unused references against the signal information
 *	object previously allocated by sigsendinit().
 *	Also perform auditing.
 *
 * Calling/Exit State:
 *    Locking:
 *	No locks are acquired or required to be held when calling this
 *	function.
 *    Parameters:
 *	The 'sigsendp' pointer identifies the sigsend_t object previously
 *	initialized by sigsendinit().  The 'init_refcount' parameter
 *	specifies the siginfo_t reference count originally given to
 *	sigsendinit().
 *
 * Remarks:
 *	This function in combination with sigsendinit() and sigsendproc(),
 *	is used by the sigsendset() kernel primitive, as well as by /proc.
 *
 */
void
sigsenddone(sigsend_t *sigsendp, ulong_t init_refcount)
{
	register sigqueue_t *sqp;

	sqp = sigsendp->ss_sqp;

	FSPIN_LOCK(&sqp->sq_mutex);
	sqp->sq_ref -= (init_refcount - sigsendp->ss_count);
	ASSERT(sqp->sq_ref >= 0);
	if (sqp->sq_ref == 0) {
		FSPIN_UNLOCK(&sqp->sq_mutex);
		kmem_free(sqp, sizeof *sqp);
	} else {
		FSPIN_UNLOCK(&sqp->sq_mutex);
	}
}


/*
 *
 * int sigsendset(procset_t *psp, int sig)
 *	Send the signal (sig) to the given process set (psp).
 *
 * Calling/Exit State::
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	The signal specified in (sig) is assumed to be a valid signal or zero.
 *	The procset (psp) is not assumed to be valid.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned (e.g., ESRCH or EPERM).
 *
 */
int
sigsendset(register procset_t *psp, register int sig)
{
	sigsend_t sigsendv;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(sig >= 0 && sig <= MAXSIG);

	/*
	 * Initialize the sigsend_t object (sigsendv) with an associated
	 * signal information sigqueue_t object (sigsendv.ss_sqp) with an
	 * initial process reference count of v.v_proc+1.  Also, in this
	 * usage, permissions to post the signal to the target process are
	 * to be checked.
	 */
	sigsendinit(&sigsendv, (ulong_t)v.v_proc + 1UL, sig, B_TRUE);
	error = dotoprocs(psp, B_TRUE, sigsendproc, &sigsendv);

	/*
	 * Release remaining unused references against the signal information
	 * object sent with the signal.
	 */
	sigsenddone(&sigsendv, (ulong_t)v.v_proc + 1UL);

	if (error == 0 && !sigsendv.ss_perm) {
		/*
		 * If we were killing a pgrp, and no processes were found,
		 * but the pgrp does exist, return success.
		 */
		if (psp->p_lidtype == P_PGID && psp->p_op == POP_AND &&
		    pgfind((pid_t)psp->p_lid))
			return 0;
		error = EPERM;
	}
	return error;
}


/*
 *
 * void sigfork(proc_t *cp, int cond)
 *	Initialize the process-level signal state information of the given
 *	process for a fork.
 *
 * Calling/Exit State:
 *    Parameters:
 *	"cond" identifies the type of fork operation being performed.
 *
 *    Locking:
 *	The calling process must hold its own p_mutex lock upon entry,
 *	if the operation being performed is a fork1(2) operation and
 *	the calling process has multiple LWPs.  The p_mutex lock remains
 *	held upon exit.  For forkall(2) and vfork(2) operations, the
 *	p_mutex lock need not be held, as no LWP races exist in this
 *	latter case.
 *
 *    Return state:
 *	Upon return, all signal registration state information has been
 *	inherited from the parent process by the child.
 *
 */
void
sigfork(register proc_t *cp, int cond)
{
	register proc_t *pp;		/* parent process */
	register sigstate_t *cssp;	/* sigstate array ptr for child */
	register sigstate_t *pssp;	/* sigstate array ptr for parent */
	int i;

	pp = u.u_procp;
	cp->p_flag |= (pp->p_flag & (P_JCTL | P_NOWAIT));
	if (cond & (NP_SYSPROC|NP_INIT)) {
		if (cond & NP_INIT) {
			/*
			 * Allocate the sigqueue_t structure for init(1)
			 * immediately so that if any system processes are
			 * orphaned to init(1), that we have the structure
			 * as needed....
			 */
			cp->p_sigcldinfo = siginfo_get(KM_NOSLEEP, 1);
			if (cp->p_sigcldinfo == NULL) {
				/*
				 *+ Unable to allocate the sigqueue_t structure
				 *+ for the system init(1) process.  Reconfigure
				 *+ the system to a smaller configuration.
				 */
				cmn_err(CE_PANIC,
				   "Unable to allocate sigqueue_t for init(1)");
			}
		}
	} else {			/* an ordinary fork(2) operation */
		if ((pp->p_flag & (P_PROCTR|P_PRFORK)) == (P_PROCTR|P_PRFORK)) {
			cp->p_sigtrmask = pp->p_sigtrmask;
		}
	}
	cp->p_sigignore = pp->p_sigignore;
	cp->p_sigreturn = pp->p_sigreturn;
	cp->p_sigactret = pp->p_sigactret;
	/*
	 * As we copy the registrations, we must be careful to not carry
	 * over pointers to associated sigqueue_t structures from the parent
	 * (since no pending signals are inherited by the child).  Also, for
	 * the fork1(2) case, we must also make sure that all of the
	 * sst_acceptlwp fields reference the surviving LWP in the child,
	 * and not some other (non-existent) LWP.
	 */
	cssp = cp->p_sigstate;
	pssp = pp->p_sigstate;
	for (i = 1; i <= MAXSIG; i++, cssp++, pssp++) {
		/*
		 * Since sst_acceptlwp is advisory, we can let the signals
		 * code sort it out without bothering here.  However, we must
		 * make sure that no LWPs are recorded as sigwaiting for the
		 * signal (as would be possible for a fork(2) operation).
		 */
		*cssp = *pssp;
		cssp->sst_rflags &= ~SST_SIGWAIT;
		cssp->sst_info = NULL;
	}
}


/*
 *
 * void sigexec(void)
 *	Initialize the signal state information of the calling process
 *	for an exec system call.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process must be held at entry.
 *	The p_mutex lock remains held at exit.  This function is called
 *	from exec.  The process must be single threaded.
 */
void
sigexec(void)
{
	sigstate_t *ssp;
	proc_t *p;
	lwp_t *lwpp;
	int i;

	p = u.u_procp;
	lwpp = u.u_lwpp;

	ASSERT(p->p_nlwp == 1);
	ASSERT(LOCK_OWNED(&p->p_mutex));

	/* Disable alternate stack */
	u.u_sigaltstack.ss_sp = 0;
	u.u_sigaltstack.ss_size = 0;
	u.u_sigaltstack.ss_flags = SS_DISABLE;

	/*
	 * Any pending signals remain held, so don't clear any held
	 * signals.  The action for a caught signal is set to SIG_DFL,
	 * the associated siginfo structure (if any) is discarded only
	 * if the signal is ignored on default.  If the signal is
	 * not ignored on default, it may be pending (and held), in
	 * which case the siginfo structure must be retained.
	 */
	for (i = 1, ssp = p->p_sigstate; i <= MAXSIG; i++, ssp++) {
		ASSERT((ssp->sst_rflags & SST_SIGWAIT) == 0);
		ssp->sst_cflags &= ~(SA_ONSTACK|SA_RESETHAND|SA_NODEFER);
		if (ssp->sst_handler != SIG_DFL &&
		    ssp->sst_handler != SIG_IGN) {
			ssp->sst_cflags = 0;
			ssp->sst_rflags = 0;
			ssp->sst_handler = SIG_DFL;
			sigemptyset(&ssp->sst_held);
			if (sigismember(&sig_ignoredefault, i)) {
				sigqueue_t *sqp;
				if ((sqp = ssp->sst_info) != NULL) {
					PROCSIG_RELE(sqp);
					ssp->sst_info = NULL;
				}
				if (sigismember(&lwpp->l_sighasinfo, i)) {
					RMLWP_SIGINFO(lwpp, i, sqp);
					kmem_free(sqp, sizeof *sqp);
				}
			}
		}
	}
	sigorset(&p->p_sigignore, &sig_ignoredefault);
	sigdiffset(&p->p_sigs, &sig_ignoredefault);
	sigdiffset(&lwpp->l_sigs, &sig_ignoredefault);
	sigdiffset(&lwpp->l_lwpsigs, &sig_ignoredefault);
	sigdiffset(&lwpp->l_procsigs, &sig_ignoredefault);
	if ((lwpp->l_trapevf & EVF_PL_SIG) && sigisempty(&lwpp->l_sigs)) {
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_SIG;
		UNLOCK(&lwpp->l_mutex, PLHI);
	}

	p->p_sigreturn = NULL;
	p->p_sigactret = NULL;
	p->p_flag &= ~(P_NOWAIT|P_JCTL);
	/*
	 * Since the execed process may not know about SIGWAITING,
	 * disable SIGWAITING.
	 */
	p->p_sigwait = B_FALSE;
}


/*
 *
 * void sig_init(proc_t *p, lwp_t *lwpp)
 *	Initialize the signals subsystem, and the signal state of
 *	process 0.
 *
 * Calling/Exit State:
 *	No special constraints.
 *
 * Remarks:
 *	The various sig_XXXX signal sets are initialized at boot-time
 *	rather than statically, to allow the correct initialization of
 *	multi-word signal sets.  This function is called from p0init(),
 *	and hence also initializes the signal state of process 0.
 *
 */
void
sig_init(proc_t *p, lwp_t *lwpp)
{
	register int i;

	/*
	 * Initialize signal masks.
	 */
	for (i = 1; i <= MAXSIG; i++)
		sigaddset(&sig_fillset, i);

	sigaddset(&sig_cantmask, SIGKILL);
	sigaddset(&sig_cantmask, SIGSTOP);

	sigaddset(&sig_cantreset, SIGILL);
	sigaddset(&sig_cantreset, SIGTRAP);
	sigaddset(&sig_cantreset, SIGPWR);

	sigaddset(&sig_ignoredefault, SIGCONT);
	sigaddset(&sig_ignoredefault, SIGCLD);
	sigaddset(&sig_ignoredefault, SIGPWR);
	sigaddset(&sig_ignoredefault, SIGWINCH);
	sigaddset(&sig_ignoredefault, SIGURG);
	sigaddset(&sig_ignoredefault, SIGWAITING);
	sigaddset(&sig_ignoredefault, SIGLWP);
	sigaddset(&sig_ignoredefault, SIGAIO);

	sigaddset(&sig_stopdefault, SIGSTOP);
	sigaddset(&sig_stopdefault, SIGTSTP);
	sigaddset(&sig_stopdefault, SIGTTOU);
	sigaddset(&sig_stopdefault, SIGTTIN);

	sig_jobcontrol = sig_stopdefault;
	sigaddset(&sig_jobcontrol, SIGCONT);

	sigaddset(&sig_coredefault, SIGQUIT);
	sigaddset(&sig_coredefault, SIGILL);
	sigaddset(&sig_coredefault, SIGTRAP);
	sigaddset(&sig_coredefault, SIGIOT);
	sigaddset(&sig_coredefault, SIGEMT);
	sigaddset(&sig_coredefault, SIGFPE);
	sigaddset(&sig_coredefault, SIGBUS);
	sigaddset(&sig_coredefault, SIGSEGV);
	sigaddset(&sig_coredefault, SIGSYS);
	sigaddset(&sig_coredefault, SIGXCPU);
	sigaddset(&sig_coredefault, SIGXFSZ);

	sigaddset(&sig_holdvfork, SIGTTOU);
	sigaddset(&sig_holdvfork, SIGTTIN);
	sigaddset(&sig_holdvfork, SIGTSTP);

	/*
	 * Initialize the event used by the pause() primitive.
	 */
	EVENT_INIT(&pause_event);

	/*
	 * Initialize process 0 signal state.
	 */
	sigemptyset(&p->p_sigs);
	sigemptyset(&p->p_sigaccept);
	sigemptyset(&p->p_sigtrmask);

	p->p_sigcldinfo = NULL;
	for (i = 1; i <= MAXSIG; i++)
		p->p_sigstate[i-1].sst_handler = SIG_DFL;
	p->p_flag &= ~P_NOWAIT;
	p->p_flag |= P_JCTL;
	p->p_sigignore = sig_ignoredefault;

	lwpp->l_whystop = 0;
	lwpp->l_whatstop = 0;
	lwpp->l_cursig = 0;
	sigemptyset(&lwpp->l_sigs);
	sigemptyset(&lwpp->l_lwpsigs);
	sigemptyset(&lwpp->l_procsigs);
	sigemptyset(&lwpp->l_sigheld);
	sigemptyset(&lwpp->l_sighasinfo);
	lwpp->l_siginfo = NULL;
}
