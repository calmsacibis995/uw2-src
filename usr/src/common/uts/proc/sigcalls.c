/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/sigcalls.c	1.27"

#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/signal.h>
#include <proc/siginfo.h>
#include <proc/procset.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <svc/systm.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/var.h>

static k_sigset_t	nullset;		/* All zero */
extern void set_sigreturn(void);
extern int pause(void);

/*
 * We support the old sigaction(2) system call and the new one
 * implemented in ES/MP; act->sa_flags & SA_NSIGACT selects the new
 * style (which passes a handler return address).
 */

/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

/*
 * The sigaction() system call
 */
struct sigactiona {
	int sig;
	struct sigaction *act;
	struct sigaction *oact;
	void (*sigactret)();			/* New for V.4 ES/MP */
};

/*
 *
 * int
 * sigaction(struct sigactiona *uap, rval_t *rvp)
 *	Implement the sigaction(2) system call.  This function allows
 *	the calling LWP to examine and/or specify the action to be
 *	taken on delivery of a specific signal.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, sigaction returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
sigaction(register struct sigactiona *uap, rval_t *rvp)
{
	k_sigset_t heldset;
	struct sigaction act;
	register int sig;
	register proc_t *p;

	ASSERT(getpl() == PLBASE);

	sig = uap->sig;
	if (sig <= 0 || sig > MAXSIG ||
	    (uap->act != NULL && sigismember(&sig_cantmask, sig)))
		return EINVAL;

	/* act and oact might be the same address, so copyin act first. */
	if (uap->act && copyin((void *)uap->act, &act, sizeof act))
		return EFAULT;

	p = u.u_procp;

	if (uap->oact) {
		struct sigaction oact;
		sigstate_t ss;
		uint pflags;		/* process flags */
		boolean_t sigwait;
		/*
		 * Return original signal action for signal.
		 */
		(void)LOCK(&p->p_mutex, PLHI);
		ss = p->p_sigstate[sig - 1];
		pflags = p->p_flag;
		sigwait = p->p_sigwait;
		UNLOCK(&p->p_mutex, PLBASE);

		oact.sa_handler = ss.sst_handler;
		if (oact.sa_handler != SIG_DFL && oact.sa_handler != SIG_IGN) {
			sigktou(&ss.sst_held, &oact.sa_mask);
			oact.sa_flags = ss.sst_cflags;
		} else {
			bzero(&oact.sa_mask, sizeof oact.sa_mask);
			oact.sa_flags = 0;
		}

		switch (sig) {
		case SIGCLD:
			if (pflags & P_NOWAIT)
				oact.sa_flags |= SA_NOCLDWAIT;
			if (!(pflags & P_JCTL))
				oact.sa_flags |= SA_NOCLDSTOP;
			break;
		case SIGWAITING:
			if (sigwait)
				oact.sa_flags |= SA_WAITSIG;
			break;
		}

		if (copyout(&oact, (void *)uap->oact, sizeof oact))
			return EFAULT;
	}

	if (uap->act) {
		sigstate_t *ssp = &p->p_sigstate[sig-1];
		/*
		 * Establish new action for signal.
		 */
		(void)LOCK(&p->p_mutex, PLHI);
		if (act.sa_flags & SA_NSIGACT) {
			/*
			 * ES/MP "new" sigaction (with handler address).
			 */
			ssp->sst_rflags |= SST_NSIGACT;
			p->p_sigactret = uap->sigactret;
		} else
			ssp->sst_rflags &= ~SST_NSIGACT;
		ssp->sst_rflags &= ~SST_OLDSIG;
		sigutok(&act.sa_mask, &heldset);
		setsigact(sig, act.sa_handler, heldset, act.sa_flags);
					/* p_mutex is unlocked on return */
	}

	return 0;
}


struct siga {
	int	signo;
	void	(*fun)();
};

/*
 * int
 * ssig(struct siga *uap, rval_t *rvp)
 *	ssig() is the common entry for signal, sigset, sighold, sigrelse,
 *	sigignore and sigpause.
 *
 * Calling/Exit State:
 *	No locks can be held on entry and none will be held on exit.
 *
 */
int
ssig(struct siga *uap, rval_t *rvp)
{
	int		sig;
	proc_t 		* const p = u.u_procp;		/* calling process */
	lwp_t		* const lwpp = u.u_lwpp;	/* calling LWP */
	void 		(*func)();
	int 		flags;
	k_sigset_t	heldset;
	k_sigset_t	oldset;

	ASSERT(KS_HOLD0LOCKS());

	sig = uap->signo & SIGNO_MASK;

	/*
	 * Validate the signal number specified.  If its value
	 * isn't in the range of valid signal numbers, or the
	 * signal can't be masked, return an error.
	 */
	if (sig <= 0 || sig > MAXSIG || sigismember(&sig_cantmask, sig))
		return EINVAL;

	func = uap->fun;

	switch (uap->signo & ~SIGNO_MASK) {

	case SIGHOLD:	/* sighold */
		/*
		 * Since we are merely reading the held mask of the
		 * context that can only be changed by the context,
		 * we do not hold p_mutex.
		 */
		heldset = lwpp->l_sigheld;
		sigaddset(&heldset, sig);
		lwpsigmask(heldset);
		return 0;

	case SIGRELSE:	/* sigrelse */
		heldset = lwpp->l_sigheld;
		sigdelset(&heldset, sig);
		lwpsigmask(heldset);
		return 0;

	case SIGPAUSE:	/* sigpause */
		heldset = lwpp->l_sigheld;
		sigdelset(&heldset, sig);
		lwpsigmask(heldset);
		return pause();

	case SIGIGNORE:	/* signore */
		func = SIG_IGN;
		flags = 0;
		heldset = lwpp->l_sigheld;
		sigdelset(&heldset, sig);
		lwpsigmask(heldset);
		(void)LOCK(&p->p_mutex, PLHI);
		break;

	case SIGDEFER:		/* sigset */

		if (func == SIG_HOLD) {
			heldset = lwpp->l_sigheld;
			if (sigismember(&heldset, sig))
				rvp->r_val1 = (int) SIG_HOLD;
			else
				rvp->r_val1 = (int)
					p->p_sigstate[sig - 1].sst_handler;
			sigaddset(&heldset, sig);
			lwpsigmask(heldset);
			return 0;
		}

		/*
		 * No locks need be held to initialize the p_sigreturn field
		 * (integer writes are assumed atomic).  Further, there is only
		 * one trampoline function that we need to register.
		 */
		set_sigreturn();
		flags = 0;

		/*
		 * The following is an undocumented feature: If a signal
		 * disposition is being changed via sigset(), the signal will
		 * also be removed from the held mask!
		 */

		heldset = lwpp->l_sigheld;
		oldset = heldset;
		sigdelset(&heldset, sig);
		lwpsigmask(heldset);
		(void)LOCK(&p->p_mutex, PLHI);
		if (sigismember(&oldset, sig))
			rvp->r_val1 = (int) SIG_HOLD;
		else
			rvp->r_val1 = (int)p->p_sigstate[sig - 1].sst_handler;
		break;

	case 0:	/* signal */

		/*
		 * No locks need be held to initialize the p_sigreturn field
		 * (integer writes are assumed atomic).  Further, there is only
		 * one trampoline function that we need to register.
		 */
		set_sigreturn();
		flags = SA_RESETHAND | SA_NODEFER;

		(void)LOCK(&p->p_mutex, PLHI);
		rvp->r_val1 = (int)p->p_sigstate[sig - 1].sst_handler;
		break;

	default:		/* error */
		return EINVAL;
	}

	ASSERT(LOCK_OWNED(&p->p_mutex));		/* p_mutex is held */

	if (sigismember(&sig_stopdefault, sig))
		flags |= SA_RESTART;
	else if (sig == SIGCLD) {
		flags |= SA_NOCLDSTOP;
		if (func == SIG_IGN)
			flags |= SA_NOCLDWAIT;
	}
	p->p_sigstate[sig - 1].sst_rflags = SST_OLDSIG;

	/*
	 * Set the signal disposition for the specified signal.
	 * Setsigact() is called with p_mutex held.
	 * On return p_mutex is released at PLBASE.
	 */
	setsigact(sig, func, nullset, flags);

	/*
	 * If the caller is catching SIGCLD, send a SIGCLD
	 * to the caller if it has any waitable zombies.
	 */
	if (sig == SIGCLD && (func != SIG_IGN && func != SIG_DFL)) {
		proc_t *cp;

		(void)LOCK(&p->p_mutex, PLHI);
		for (cp = p->p_child; cp != NULL; cp = cp->p_nextsib) {
			if (cp->p_nlwp == 0 && cp->p_wcode != 0) {
				(void)sigtoproc_l(p, SIGCLD,(sigqueue_t *)NULL);
				break;
			}
		}
		UNLOCK(&p->p_mutex, PLBASE);
	}

	return 0;
}


/*
 * sigwait() system call.
 */
struct sigwaita {
        const sigset_t *set;
	siginfo_t *info;
	const struct timespec *timeout;
};

/*
 *
 * int
 * sigtimedwait(struct sigwaita *uap, rval_t *rvp)
 *      This is the sigwait system call.
 *
 * Calling/Exit State:
 *      No locks held on entry and none held on return.
 *
 */
int
sigtimedwait(struct sigwaita *uap, rval_t *rvp)
{
	register proc_t *procp;		/* the calling process */
	register lwp_t *lwpp;		/* the calling LWP */
	uchar_t sigvec[MAXSIG+1];	/* list of sigwait(2) signals */
	sigstate_t *ssp;
	sigset_t   uset;	/* sigwait signal set passed from user-level */
	k_sigset_t kset;	/* sigwait signal set (kernel version) */
	k_sigset_t wset;	/* working signal mask */
	k_sigset_t rset;	/* signal mask upon return */
	k_sigset_t oset;	/* original signal mask for abort return */
	sigqueue_t *sqp;
	int retval;		/* return value */
	int sig;
	int i;
	boolean_t blocked;	/* B_TRUE if sigwait(2) has blocked */

	procp = u.u_procp;
	lwpp = u.u_lwpp;
	blocked = B_FALSE;

	/*
	 * Get the set of signals to be waited for.
	 */
	if (copyin((void *)uap->set, &uset, sizeof uset))
		return EFAULT;
	sigutok(&uset, &kset);
	sigdiffset(&kset, &sig_cantmask);

	/*
	 * Determine the vector of signals that we are waiting for
	 * in an easier to use form.
	 */
	wset = kset;
	for (i = 0; !sigisempty(&wset); i++) {
		sigvec[i] = (uchar_t)sigdelnext(&wset);
		ASSERT(sigvec[i] >= 0 && sigvec[i] <= MAXSIG);
	}
	ASSERT(i <= MAXSIG);
	sigvec[i] = 0;			/* Zero terminated list */

	if (i == 0)			/* No signals being sigwaited for */
		return EINVAL;

	/*
	 * Compute the signal mask the LWP should have upon return.
	 */
	rset = oset = lwpp->l_sigheld;
	sigorset(&rset, &kset);

	/*
	 * Mask all maskable signals, except those being sigwait'ed for.
	 */
	sigfillset(&wset);
	sigdiffset(&wset, &sig_cantmask);
	sigdiffset(&wset, &kset);
	(void)LOCK(&procp->p_mutex, PLHI);
	lwpsigmask_l(wset);

again:	if (QUEUEDSIG(lwpp)) {
		switch (issig(&procp->p_mutex)) {

		case ISSIG_NONE:
			/*
			 * It is possible that we are a vfork(2)ed child, in
			 * which case pending advisory job control stop signals
			 * will not be promoted from pending to current.
			 */
			UNLOCK(&lwpp->l_mutex, PLBASE);
			ASSERT(LOCK_OWNED(&procp->p_mutex));
			break;

		case ISSIG_SIGNALLED:
			/*
			 * The LWP has a signal or event to process.
			 * The p_mutex lock is not held.
			 */
			if (lwpp->l_cursig == 0) {
				/*
				 * At the time issig() was invoked, the
				 * only signallable event besides a real
				 * signal was either a rendezvous request,
				 * and/or a destroy request, and/or a
				 * system call abort request.  System call
				 * abort requests however are quietly
				 * ignored, since POSIX does not allow
				 * sigwait(2) to return with EINTR.
				 */
				lwpp->l_trapevf &= ~EVF_PL_SYSABORT;
				if ((lwpp->l_trapevf
				     & (EVF_PL_RENDEZV|EVF_PL_DESTROY)) == 0) {
					/*
					 * EVF_PL_SYSABORT was set, but we just
					 * cleared it above.  Behave as though
					 * ISSIG_STOPPED was returned.
					 */
					(void)LOCK(&procp->p_mutex, PLHI);
					goto again;
				}
				retval = ERESTART;
				if (!blocked) {
					lwpsigmask(oset);
					return retval;
				}
			} else {
				/*
				 * Free any information associated with the
				 * signal (if the signal was pending even
				 * before we began sigwait(2), there could be
				 * associated information.  Also, if the signal
				 * was posted by a debugger, there could be
				 * associated information).
				 */
				sqp = lwpp->l_cursigst.sst_info;
				if (sqp != NULL) {
					siginfo_free(sqp);
					lwpp->l_cursigst.sst_info = NULL;
				}
				rvp->r_val1 = lwpp->l_cursig;
				if (!blocked) {
					/*
					 * One of the signals being sigwait'ed
					 * was already pending.
					 */
					(void)LOCK(&procp->p_mutex, PLHI);
					lwpp->l_cursig = 0;
					lwpsigmask_l(rset);
					UNLOCK(&procp->p_mutex, PLBASE);
					return 0;
				}
				retval = 0;
			}
			i = 0;
			(void)LOCK(&procp->p_mutex, PLHI);
			lwpp->l_cursig = 0;		/* no current signal! */
			while ((sig = sigvec[i++]) != 0) {
				ssp = &procp->p_sigstate[sig-1];
				if (--ssp->sst_swcount == 0) {
					ssp->sst_rflags &= ~SST_SIGWAIT;
					ssp->sst_info = NULL;
				}
			}
			lwpsigmask_l(rset);
			UNLOCK(&procp->p_mutex, PLBASE);
			return retval;

		case ISSIG_STOPPED:
			/*
			 * Stopped.  Loop to check signals again.
			 * The p_mutex lock is not held.
			 */
			(void)LOCK(&procp->p_mutex, PLHI);
			goto again;

		default:
			/* this should never happen. */
			/*
			 *+ The issig() function returned an
			 *+ unexpected status.  This indicates a
			 *+ kernel software problem.
			 */
			cmn_err(CE_PANIC, "unexpected return from issig()");
		}
	}

	/*
	 * None of the signals being sigwait'ed for are previously
	 * pending.  There are also no rendezvous or destroy requests.
	 * The p_mutex lock is held.
	 */
	ASSERT(LOCK_OWNED(&procp->p_mutex));

	if (!blocked) {
		blocked = B_TRUE;
		i = 0;
		while ((sig = sigvec[i++]) != 0) {
			/*
			 * Update the number of LWPs sigwaiting for each
			 * signal.
			 */
			ssp = &procp->p_sigstate[sig-1];
			if (ssp->sst_rflags & SST_SIGWAIT) {
				ssp->sst_swcount++;
			} else {
				/*
				 * NOTE: sst_swcount overlays sst_info.
				 * If the signal somehow became pending,
				 * the siginfo pointed to by sst_info
				 * must be freed.  After SST_SIGWAIT is
				 * set, sst_info has no meaning.
				 */
				if (ssp->sst_info)
					siginfo_free(ssp->sst_info);
				ssp->sst_rflags |= SST_SIGWAIT;
				ssp->sst_swcount = 1;
			}
		}
	}

	lwpp->l_sigwait = 1;

	UNLOCK(&procp->p_mutex, PLBASE);

	(void)pause();

	(void)LOCK(&procp->p_mutex, PLHI);

	lwpp->l_sigwait = 0;
	goto again;
}


/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

/*
 * sigsuspend() system call.
 */
struct sigsuspenda {
	sigset_t *set;
};

/*
 *
 * int
 * sigsuspend(struct suspenda *uap, rval_t *rvp)
 *	This is the sigsuspend system call.
 *
 * Calling/Exit State:
 *	No locks held on entry and none held on return.
 *
 */
/* ARGSUSED */
int
sigsuspend(struct sigsuspenda *uap, rval_t *rvp)
{
	sigset_t 	set;
	k_sigset_t 	kset;
	lwp_t		*lwpp = u.u_lwpp;

	if (copyin((void *)uap->set, &set, sizeof set))
		return EFAULT;

	sigutok(&set, &kset);
	sigdiffset(&kset, &sig_cantmask);
	u.u_sigoldmask = lwpp->l_sigheld;
	u.u_sigflag |= SOMASK;
	lwpsigmask(kset);
	return pause();
}


/*
 * sigaltstack() system call.
 */
struct sigaltstacka {
	struct sigaltstack *ss;
	struct sigaltstack *oss;
};

/*
 *
 * int
 * sigaltstack(struct sigaltstacka *uap, rval_t *rvp)
 *	The sigaltstack() system call.
 *
 * Calling/Exit State:
 *	No locks held on entry and none held on return.
 *
 */
/* ARGSUSED */
int
sigaltstack(struct sigaltstacka *uap, rval_t *rvp)
{
	struct sigaltstack ss;

	/*
	 * User's oss and ss might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->ss) {
		if (u.u_sigaltstack.ss_flags & SS_ONSTACK)
			return EPERM;
		if (copyin((void *)uap->ss, &ss, sizeof ss))
			return EFAULT;
		if (ss.ss_flags & ~SS_DISABLE)
			return EINVAL;
		if (!(ss.ss_flags & SS_DISABLE) && ss.ss_size < MINSIGSTKSZ)
			return ENOMEM;
	}

	if (uap->oss)
		if (copyout(&u.u_sigaltstack, (void *)uap->oss,
			    sizeof u.u_sigaltstack))
			return EFAULT;

	if (uap->ss)
		u.u_sigaltstack = ss;

	return 0;
}


/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

/*
 * sigpending() system call.
 */
struct sigpendinga {
	int flag;
	sigset_t *set;
};

/*
 *
 * int
 * sigpending(struct sigpendinga *uap, rval_t *rvp)
 *	The sigpending() system call.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and no locks held on return.
 *
 * Remarks:
 *	It should be understood that the return value can be stale!
 *
 */
/* ARGSUSED */
int
sigpending(struct sigpendinga *uap, rval_t *rvp)
{
	sigset_t set;
	k_sigset_t kset;

	switch (uap->flag) {
	case 1: /* sigpending */
		(void)LOCK(&u.u_procp->p_mutex, PLHI);
		kset = u.u_procp->p_sigs;
		/*
		 * Remove the signals already accepted (unmasked) by other
		 * LWPs to get the masked, process-level pending set.
		 */
		sigdiffset(&kset, &u.u_procp->p_sigaccept);

		/*
		 * OR in pending LWP instance signals.
		 * (There is no need to OR in pending process instance signals
		 *  accepted by the LWP, since such signals are not masked.)
		 */
		sigorset(&kset, &u.u_lwpp->l_lwpsigs);
		UNLOCK(&u.u_procp->p_mutex, PLBASE);

		/*
		 * We have an interesting semantic issue here.  In the past,
		 * the held signal mask was an attribute of the process.
		 * With LWPs, the held mask is an attribute of the LWP!
		 * We have chosen to return the set of signals pending to
		 * the process held by all LWPs in the process, plus the
		 * set of LWP instance signals masked by the calling LWP.
		 * This interpretation provides the required compatibility
		 * for single-threaded processes, and is the most meaningful
		 * interpretation for multi-threaded process.
		 *
		 * NOTE: l_sigheld is stable without holding p_mutex since
		 *       only the calling LWP can modify it.
		 */
		sigandset(&kset, &u.u_lwpp->l_sigheld);
		break;
	case 2: /* sigfillset */
		sigfillset(&kset);
		break;
	default:
		return EINVAL;
	}

	sigktou(&kset, &set);
	if (copyout(&set, (void *)uap->set, sizeof set))
		return EFAULT;

	return 0;
}


/*
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function.
 */

/*
 * sigprocmask() system call.
 */
struct sigprocmaska {
	int how;
	sigset_t *set;
	sigset_t *oset;
};

/*
 *
 * int
 * sigprocmask(struct sigprocmaska *uap, rval_t *rvp)
 *	The sigprocmask() system call.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none will be held on return.
 *
 */
/* ARGSUSED */
int
sigprocmask(struct sigprocmaska *uap, rval_t *rvp)
{
	k_sigset_t 	kset;
	k_sigset_t	heldset;

	/*
	 * User's oset and set might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->set) {
		sigset_t set;
		if (copyin((void *)uap->set, &set, sizeof set))
			return EFAULT;
		sigutok(&set, &kset);
	}

	if (uap->oset) {
		sigset_t set;
		/*
		 * Since we are merely reading the held mask for the
		 * current context and since this mask can only be
		 * changed by the owning context, we do not acquire
		 * the p_mutex lock.
		 */

		sigktou(&u.u_lwpp->l_sigheld, &set);
		if (copyout(&set, (void *)uap->oset, sizeof set))
			return EFAULT;
	}

	if (uap->set) {
		sigdiffset(&kset, &sig_cantmask);
		switch (uap->how) {
		case SIG_BLOCK:
			sigorset(&kset, &u.u_lwpp->l_sigheld);
			lwpsigmask(kset);
			break;
		case SIG_UNBLOCK:
			heldset = u.u_lwpp->l_sigheld;
			sigdiffset(&heldset, &kset);
			lwpsigmask(heldset);
			break;
		case SIG_SETMASK:
			lwpsigmask(kset);
			break;
		default:
			return EINVAL;
		}
	}

	return 0;
}


/*
 * The sigsendset() system call.
 */
struct sigsenda {
	procset_t *psp;		/* ptr to the process set */
	int	sig;		/* the signal to send */
};

/*
 *
 * int sigsendsys(struct sigsenda *uap, rval_t *rvp)
 *	This function implements the sigsendset(2) system call.
 *
 * Calling/Exit State::
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned (e.g., ESRCH or EPERM).
 *
 */
/* ARGSUSED */
int
sigsendsys(register struct sigsenda *uap, rval_t *rvp)
{
	procset_t set;
	register int sig;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sig = uap->sig;
	if (sig < 0 || sig > MAXSIG)
		return EINVAL;

	if (copyin((void *)uap->psp, &set, sizeof set))
		return EFAULT;

	return sigsendset(&set, sig);
}


/*
 * The kill() system call.
 */
struct killa {
	pid_t   pid;
	int     sig;
};

/*
 *
 * int kill(struct killa *uap, rval_t *rvp)
 *	This function implements the kill(2) system call.
 *
 * Calling/Exit State::
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned (e.g., ESRCH or EPERM).
 *
 * Remarks:
 *	For implementations that don't require binary compatibility, the
 *	kill(2) system call may be made into a library call atop the
 *	sigsend(2) system call.
 *
 */
/* ARGSUSED */
int
kill(register struct killa *uap, rval_t *rvp)
{
	register id_t id;
	register idtype_t idtype;
	register idop_t idop;
	procset_t set;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

/* Enhanced Application Compatibility Support */
#ifdef VIRTUAL_XOUT
	if (VIRTUAL_XOUT && uap->sig == XENIX_SIGPOLL)
		uap->sig = SIGPOLL;
#endif

#ifdef ISC_USES_POSIX
	if (ISC_USES_POSIX)
		switch (uap->sig) {
		case ISC_SIGCONT:
			uap->sig = SIGCONT;
			break;
		case ISC_SIGSTOP:
			uap->sig = SIGSTOP;
			break;
		case ISC_SIGTSTP:
			uap->sig = SIGTSTP;
			break;
		}
#endif
/* End Enhanced Application Compatibility Support */

	if (uap->sig < 0 || uap->sig > MAXSIG) {
		sigsend_t *dummy = NULL;

		ADT_SIGINIT(u.u_lwpp->l_auditp, dummy);
		return EINVAL;
	}

	id = (id_t)uap->pid;
	if (id > 0) {			/* signalling a single process */
		idtype = P_PID;
		idop = POP_AND;
	} else if (id == -1) {		/* signalling everyone we can */
		idtype = P_ALL;		/* id argument is irrelevant */
		idop = POP_OR;		/* faster for dotoprocs() */
		/*
		 * NOTE: No "id" value is set here, because for the
		 *	 P_ALL case, the "id" value is ignored.
		 */
	} else {			/* signalling a process group */
		idtype = P_PGID;
		idop = POP_AND;
		if (id == 0)		/* signalling caller's pgrp */
			id = (id_t)u.u_procp->p_pgid;
		else			/* signalling given pgrp */
			id = -id;
	}

	/*
	 * NOTE: We have consciously chosen to call the sigsendset()
	 *	 interface instead of attempting more direct optimizations
	 *	 here.  We have done this for two reasons:
	 *
	 *	 1) The sigsendset() interface built atop the dotoprocs()
	 *	    kernel primitive will in all cases, do the minimum
	 *	    amount of work necessary to signal a specific process,
	 *	    process group, and all possible processes (the -1
	 *	    case).  Furthermore, the overhead in letting
	 *	    dotoprocs() determine the "shortest path" is very low.
	 *
	 *	 2) By using the sigsendset() interface, it is not
	 *	    necessary to export other elements of the dotoprocs()
	 *	    interface beyond the dotoprocs() function itself.
	 */
	setprocset(&set, idop, idtype, id, P_ALL, P_MYID);
	return (sigsendset(&set, uap->sig));
}


/*
 * The _lwp_kill() system call.
 */
struct lwp_killa {
	lwpid_t	lwpid;
	int     sig;
};

/*
 *
 * The _lwp_kill() system call sends the given signal to the specified
 * LWP in the caller's process.  If the sig argument is zero, then error
 * checking is performed but no signal is sent.
 *
 * Calling/Exit State::
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned:
 *		EINVAL: if sig contains an invalid or unsupported signal
 *			number, or lwpid contains an invalid LWP-ID.
 * 		ESRCH:	if the LWP does not exist in the caller's process.
 *		EPERM:	if the caller is the system init(1) process, and
 *			the signal to send is one of SIKILL, or SIGSTOP.
 */
/* ARGSUSED */
int
_lwp_kill(struct lwp_killa *uap, rval_t *rvp)
{
	register k_lwpid_t lwpid;
	register int sig;
	register sigqueue_t *siginfop;
	proc_t *procp;
	lwp_t *lwpp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sig = uap->sig;
	if (sig < 0 || sig > MAXSIG || uap->lwpid < 0)
		return EINVAL;

	if (uap->lwpid < 0 || uap->lwpid > USHRT_MAX)
		return ESRCH;

	lwpid = uap->lwpid;
	procp = u.u_procp;

	if (sig != 0) {
		if (procp == proc_init && sigismember(&sig_cantmask, sig))
			return EPERM;

		/*
		 * Allocate LWP variant of sigqueue_t object.
		 */
		siginfop = siginfo_get(KM_SLEEP, 0);
		siginfop->sq_info.si_signo = sig;
		siginfop->sq_info.si_code = SI_USER;
		siginfop->sq_info.si_pid = procp->p_pidp->pid_id;
		siginfop->sq_info.si_uid = u.u_lwpp->l_cred->cr_ruid;
	}

	lwpid--;			/* LWP directory is zero-based */
	pl = LOCK(&procp->p_mutex, PLHI);
	if (lwpid >= procp->p_nlwpdir ||
	    (lwpp = procp->p_lwpdir[lwpid]) == NULL ||
	    lwpp->l_stat == SIDL) {
		UNLOCK(&procp->p_mutex, pl);
		if (sig != 0)
			siginfo_free(siginfop);
		return ESRCH;
	}

	if (sig == 0) {			/* only error checking */
		UNLOCK(&procp->p_mutex, pl);
		return 0;
	}

	if (sigtolwp_l(lwpp, sig, siginfop))
		UNLOCK(&procp->p_mutex, pl); /* signal info referenced */
	else {
		/*
		 * Signal information was not referenced.
		 */
		UNLOCK(&procp->p_mutex, pl);
		siginfo_free(siginfop);
	}

	return 0;
}
