/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/sig.c	1.4.11.40"

#include <dlfcn.h>
#include <fcntl.h>
#include <ucontext.h>
#include <libthread.h>
#include <trace.h>
#include <sys/reg.h>

#if !defined(ABI) && defined(__STDC__)

#pragma weak sigaction = _sigaction
#pragma weak sigignore = _sigignore
#pragma weak sighold = _sighold
#pragma weak sigrelse = _sigrelse
#pragma weak sigpause = _sigpause
#pragma weak signal = _signal
#pragma weak sigset = _sigset
#pragma weak sigpending = _sigpending
#pragma weak sigsuspend = _sigsuspend
#pragma weak sigwait = _sigwait
#pragma weak sigprocmask = _sigprocmask
#pragma weak setcontext = _setcontext

#endif /*  !defined(ABI) && defined(__STDC__) */

void __thr_siglwphndlr(int , siginfo_t *, ucontext_t *); /* SIGLWP handler */
void _thr_sigacthandler(int , siginfo_t *, ucontext_t *, void (*)()); /* global signal handler */
void __thr_sigwaitinghndlr(); /* SIGWAITING handler */

void _setcontext(ucontext_t *);

STATIC int _thr_sigsetmask(int how, const sigset_t *set, sigset_t *oset,
			   boolean_t lwpmask, thread_desc_t *tp);

/*
 * When a user attempts to change signal disposition for SIGLWP and SIGWAITING
 * the thread library traps it and saves the user specified disposition in
 * _thr_siglwp_handler and _thr_sigwaiting_handler respectively.
 * The actual disposition for those signals is not changed.
 * This sort of `cheating` ensures that the user cannot change signal disposition 
 * for SIGLWP and SIGWAITING and, at the same time, allows applications to 
 * e.g., execute in a loop sigaction for all signals without failing.
 *
 */

void (*_thr_siglwp_handler)() = SIG_DFL;
void (*_thr_sigwaiting_handler)() = SIG_DFL;

/*
 * _thr_siguinfo[] is the library cache of the signal masks and flags for
 * the user-defined signal dispositions. The _thr_siguinfo is protected by
 * _thr_siguinfolock.
 */
struct sigaction _thr_siguinfo[MAXSIG + 1];
mutex_t _thr_siguinfolock;
boolean_t _thr_sigwaitingset = B_FALSE;	/* tells when SIGWAITING is enabled */

static sigset_t _thr_sig_cantmask;	/* signals that cannot be masked */
static sigset_t _thr_sig_cantreset;	/* signals that cannot be reset */
sigset_t _thr_sig_programerror;		/* signals caused by program error */
sigset_t _thr_sig_allmask;		/* mask of all signals  */

/*
 * static int
 * _thr_signext(sigset_t *s)
 *      "Find Last Set" for one-word bit array.
 *
 * Calling/Exit State:
 *      Returns the bit number of the highest set bit; if none, returns 0.
 *
 * Description:
 *
 */

static int
_thr_signext(sigset_t *s)
{
        int i, sig;

        /* currently, sizeof(s->sa_sigbits)/sizeof(int) should = 4 */
        for (i = 0; i < sizeof(s->sa_sigbits)/sizeof(int); i++) {
                if (sig = _thr_hibit(s->sa_sigbits[i]))
                        return (i*BPW + sig);
        }
        return (0);
}

/*
 * void
 * _thr_sigt0init(thread_desc_t *tp)
 *
 * This function is called by _thr_t0init before main() starts. It initializes
 * premortial thread's signal mask and the disposition for SIGLWP and
 * SIGWAITING.  
 * Parameter/Calling State:
 *      No locks need to be held on entry; 
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *      This function returns initialized premortial thread's signal mask, and
 *	initialized disposition for SIGLWP and SIGWAITING.
 */
void
_thr_sigt0init(thread_desc_t *tp)
{
	struct sigaction sigact;

	_THR_MUTEX_INIT(&_thr_siguinfolock, USYNC_PROCESS, 0);
	sigaddset(&_thr_sig_cantmask, SIGKILL);
	sigaddset(&_thr_sig_cantmask, SIGSTOP);
	sigaddset(&_thr_sig_cantmask, SIGWAITING);
	sigaddset(&_thr_sig_cantmask, SIGLWP);
	sigaddset(&_thr_sig_cantreset, SIGILL);
	sigaddset(&_thr_sig_cantreset, SIGTRAP);
	sigaddset(&_thr_sig_cantreset, SIGPWR);
	sigaddset(&_thr_sig_cantreset, SIGLWP);
	sigaddset(&_thr_sig_cantreset, SIGWAITING);
	sigaddset(&_thr_sig_programerror, SIGILL);
	sigaddset(&_thr_sig_programerror, SIGFPE);
	sigaddset(&_thr_sig_programerror, SIGBUS);
	sigaddset(&_thr_sig_programerror, SIGSEGV);
	sigaddset(&_thr_sig_programerror, SIGSYS);
	sigaddset(&_thr_sig_programerror, SIGABRT);

	/*
	 * Initialize a mask blocking all signals.
	 */

	sigfillset(&_thr_sig_allmask);
	_thr_sigdiffset(&_thr_sig_allmask, &_thr_sig_programerror);

	/*
	 * Initialize primordial thread's and its LWP signal mask.
	 * Ensure that neither SIGLWP nor SIGWAITING is being masked.
	 */

	(*_sys_sigprocmask)(0, NULL, &tp->t_hold);
	
	if (sigismember(&tp->t_hold, SIGLWP) ||
	    sigismember(&tp->t_hold, SIGWAITING)) {
		sigdelset(&tp->t_hold, SIGLWP);
		sigdelset(&tp->t_hold, SIGWAITING);
		(*_sys_sigprocmask)(SIG_SETMASK, &tp->t_hold, NULL);
	}

	/* 
	 * establish disposition of SIGLWP and SIGWAITING
	 * We need to enable SIGLWP via _thr_sigaction, and
	 * cache sa_mask and sa_flags for SIGWAITING.
	 * The latter is being done here directly by indexing into 
	 * _thr_siguinfo[] array as: (1) _thr_sigaction does not allow to 
	 * do that; (2) subsequent enabling/disabling of SIGWAITING is
	 * being done directly via _sys__sigaction() system call.
	 */
	
	sigact.sa_handler = __thr_siglwphndlr;	
	sigfillset(&sigact.sa_mask);
	_thr_sigdiffset(&sigact.sa_mask, &_thr_sig_programerror);
	sigact.sa_flags = SA_RESTART | SA_NSIGACT;
	_thr_siguinfo[SIGLWP].sa_mask = sigact.sa_mask;
	_thr_siguinfo[SIGLWP].sa_flags = sigact.sa_flags;
	_thr_siguinfo[SIGLWP].sa_handler = __thr_siglwphndlr;

	if((*_sys__sigaction)(SIGLWP, &sigact, NULL, _thr_sigacthandler) != 0) {
		_thr_panic("_thr_t0init: SIGLWP sigaction failed\n");
	}
	_thr_siguinfo[SIGWAITING].sa_mask = sigact.sa_mask;
	_thr_siguinfo[SIGWAITING].sa_flags = SA_WAITSIG|SA_RESTART|SA_NSIGACT;
	_thr_siguinfo[SIGWAITING].sa_handler = __thr_sigwaitinghndlr;
}

/*
 * void
 * _thr_sigacthandler(int sig, siginfo_t *sip, ucontext_t *uap,
 *		      void (*handler)())
 *
 *	_thr_sigacthandler() is a common signal handler of a process.
 *	When it runs all signals are masked.
 *	If the common signal handler is called while the thread is in a
 *	critical section, the signal and its state are saved in the
 *	thread's descriptor, and the common signal handler returns 
 *	to the interrupt point via setcontext(2) system function.
 *
 *	If the common signal handler is called while the thread is not in a
 *	critical section, and the thread does not block the signal, the
 *	common signal handler calls a user defined signal handler.
 *	If the thread blocks the signal, the common signal handler either
 *	posts the signal to the thread if the signal is directed to the LWP,
 *	or sends the signal back to the process if the signal was 
 *	directed to the process.
 *	
 * Parameter/Calling State:
 *	On entry, all signals are masked and no locks are held.
 *	During processing, _thr_siguinfolock is acquired.
 *	Thread lock may also be acquired.
 * Return Values/Exit State:
 *	On exit, all signals are masked, and no locks are held,
 *	except in the case where the thread must suspending or
 *	preempt itself as a consequence of receiving SIGLWP.
 *
 */

/* ARGSUSED */
void
_thr_sigacthandler(int sig, siginfo_t *sip, ucontext_t *uap, void (*handler)())
{
	thread_desc_t *tp = curthread;
	sigset_t handler_mask;
	void (*__user_handler)();
	struct sigaction tmpact;
	int rval;

	PRINTF2(
 " *** GLOBAL SIGNAL HANDLER _thr_sigacthandler -- signal = %d, thread = %d\n",
		sig, tp->t_tid); 
	PRINTF2("_thr_sigacthandler: tp->t_hold = 0x%x and 0x%x\n",
		tp->t_hold.sa_sigbits[0], tp->t_hold.sa_sigbits[1]); 

	ASSERT(tp->t_tid != -1);

	if (tp->t_nosig > 0) {

		 /* the thread is in a critical section */

		if (sig == SIGLWP && !PREEMPTED(tp) &&
		    tp->t_suspend != TSUSP_PENDING) {
			/* SIGLWP can only be sent by the Threads Library
			 * to request thread's suspension or preemption.
			 * All other occurrences of SIGLWP (e.g., sending
			 * SIGLWP to a process via kill) are ignored by 
			 * the Threads Library.
			 */
			(*_sys_setcontext)(uap);
			/* NOTREACHED */
		}
		
		/*
		 * If an async signal comes when a thread is in a critical
		 * section we have a serious problem in the library and what do
		 * we do? we panic.
		 */

		if (sigismember(&_thr_sig_programerror, sig)) {
			_thr_panic(
		    "_thr_sigacthandler: fault 1 inside a critical section"); 
		}

		/*
		 * If SA_RESETHAND is set then we need to reset signal
		 * disposition back to original.
		 */

		if (_thr_siguinfo[sig].sa_flags & SA_RESETHAND) {
			tmpact.sa_flags = _thr_siguinfo[sig].sa_flags;
			tmpact.sa_handler = _thr_siguinfo[sig].sa_handler;
			sigfillset(&tmpact.sa_mask);
			_thr_sigdiffset(&tmpact.sa_mask,
					&_thr_sig_programerror);
			rval = (*_sys__sigaction)(sig, &tmpact, NULL,
						  _thr_sigacthandler);
			if (rval != 0) {
				_thr_panic(
		      "_thr_sigacthandler: fault 2 inside a critical section"); 
			}
		}
			
		/*
		 * Save signal and current thread's mask in the thread
		 * descriptor.  The signal will be disposed and the mask
		 * restored at appropriate time.
		 */
		
		tp->t_oldmask = tp->t_hold;
		tp->t_sig = (char) sig;
		sigfillset(&tp->t_hold);
		_thr_sigdiffset(&tp->t_hold, &_thr_sig_programerror);
		uap->uc_sigmask = tp->t_hold;

		PRINTF3(" *** _thr_sigacthandler: sig = %d tid = %d,  curthread = 0x%x IS IN CRITICAL SECTION\n", sig, tp->t_tid, tp); 

		/*
		 * At this point we'll return to the interrupt point.
		 * Note that setcontext() will block all signals for this lwp.
		 */

		(*_sys_setcontext)(uap);
		/* NOTREACHED */
		
	} else { /* thread is not in a critical section */
		int	signo;
		sigset_t psig;
		/*
		 * A signal is result of a thr_kill() when the
		 * received signal is in the thread's set of pending
		 * signals as represented by t_psig.
		 * Note since the global signal handler runs with all signals
		 * masked we can acquire thread lock without calling sigoff.
		 * Also note although lwp for global handler runs with all
		 * signals masked, another thread can still send us a signal
		 * via thr_kill() before the LOCK_THREAD below is done, and it
		 * will also do lwp_kill since we are TS_ONPROC.
		 */

		LOCK_THREAD(tp);

		PRINTF3(" *** _thr_sigacthandler: sig = %d thread ID = %d,  curthread = 0x%x IS NOT IN A CRITICAL SECTION\n", sig, tp->t_tid, tp); 

		/*
		 * Since the global signal handler runs with all signals masked
		 * the mask must be adjusted to that specified for the signal
		 * handler in sigaction and cached by the thread library in the
		 * _thr_siguinfo before the user-defined handler can be
		 * executed.
		 */

		_THR_MUTEX_LOCK(&_thr_siguinfolock);
		tp->t_hold = _thr_siguinfo[sig].sa_mask;
		handler_mask = _thr_siguinfo[sig].sa_mask;
		__user_handler = _thr_siguinfo[sig].sa_handler;
		_THR_MUTEX_UNLOCK(&_thr_siguinfolock);

                /* Handle any requests from a debugger to cancel
                 * a pending thr_kill signal - we do not cancel
                 * current signal, here: it's too late.
                 * We do this in the thread library to make sure our
                 * data structures stay sane
                 */

		if (_thr_debug.thr_debug_on) {
			tp->t_dbg_busy = 1;
			if (tp->t_dbg_cancel) {

			_thr_sigdiffset(&tp->t_psig, &tp->t_dbg_set);
			sigemptyset(&tp->t_dbg_set);
			tp->t_dbg_cancel = 0;
			tp->t_dbg_busy = 0;
			_thr_debug_notify(tp, tc_cancel_complete);
			} else {
				tp->t_dbg_busy = 0;
			}
		}
		/*
		 * must send signal to tp if i) a signal was set in t_psig from
		 * a thr_kill before thread was locked (above) and ii) it's not
		 * masked by the handler mask (t_hold).
		 */
		if (_thr_sigisset(&tp->t_psig, &tp->t_hold) != 0) {
			psig = tp->t_psig;
			while (signo = _thr_signext(&psig)) {
				sigdelset(&psig, signo);
				if (!sigismember(&tp->t_hold, signo)) {
					sigdelset(&tp->t_psig, signo);
					if (_lwp_kill(LWPID(tp), signo) < 0)
               		                 	_thr_panic("_thr_sigacthandler: _lwp_kill");
				}
			}
		}
		UNLOCK_THREAD(tp);

		(*_sys_sigprocmask)(SIG_SETMASK, &handler_mask, NULL);

		PRINTF3(" *** _thr_sigacthandler: calling user handler: sig = %d, sip=0x%x, uap= 0x%x\n", sig, sip, uap);

		PRINTF2("tp->t_hold = 0x%x and 0x%x\n",
			tp->t_hold.sa_sigbits[0], tp->t_hold.sa_sigbits[1]); 

		if( sig != SIGLWP) {
			(*__user_handler)(sig, sip, uap);
		} else {
			/*
			 * Restore the original thread descriptor
			 * mask.
			 */
			(*_sys_sigprocmask)(SIG_SETMASK, &tp->t_hold, NULL);
			__thr_siglwphndlr(sig, sip, uap);
		}
		_setcontext(uap);
		/* NOTREACHED */		
	}
}

/*
 * 
 * int
 * _thr_sigaction(int sig, const struct sigaction *nactp,
 *		  struct sigaction *oactp)
 *
 * 	_thr_sigaction() is an internal Threads Library function to examine
 *	and/or specify the action to be taken on delivery of a specific
 *	signal.
 *	The function is used to register the common signal handler 
 *	as well as, a user signal handler.
 *	_thr_sigaction() acquires _thr_siguinfolock mutex and 
 *	caches sa_mask and sa_flags in a user-level table _thr_siguinfo.
 *	The function calls siguaction(2) system call to register the
 *	global signal handler in the kernel.
 *	The global signal handler is registered to block all signals.
 *	Before exit, _thr_sigaction() unlocks _thr_siguifolock.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and no locks are held.
 *	During processing, _thr_siguifolock is acquired.
 *
 * Return Values/Exit State:
 *	On exit, signal handlers remain disabled, and no locks are held.
 */

int
_thr_sigaction(int sig, const struct sigaction *nactp, struct sigaction *oactp)
{
	struct sigaction tmpact;
	register struct sigaction *tmpactp;
	int rval = 0;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(sig > 0 && sig <= MAXSIG);

	_THR_MUTEX_LOCK(&_thr_siguinfolock);
	if (nactp != NULL) {
		tmpact = *nactp;
		tmpactp = &tmpact;

		/*
		 * we don't use user-supplied handlers for SIGLWP or SIGWAITING
		 * so, save user-supplied handler; then set to use our handlers
		 * instead.
		 */
		/* 
		 * user-supplied handler had been saved in _thr_siglwp_handler
		 * for SIGLWP or _thr_sigwaiting_handler for SIGWAITING. Just
		 * return them in oactp
		 */
		if (sig == SIGLWP) {
			_thr_siglwp_handler = tmpactp->sa_handler;
			goto out;
		} else if (sig == SIGWAITING) {
			_thr_sigwaiting_handler = tmpactp->sa_handler;
			goto out;
		} 
		if (tmpactp->sa_handler != SIG_IGN &&
		    tmpactp->sa_handler != SIG_DFL) { 

		/* sig is NOT SIGLWP and NOT SIGWAITING */
	
			tmpactp->sa_flags |= SA_SIGINFO; 
			tmpactp->sa_flags |= SA_NSIGACT; 

			/*
			 * All signals are masked when the kernel
			 * dispatches a signal to the common signal
			 * handler. This enables the threads library
			 * to disable signals without having to do
			 * a system function. The signal is delivered to
			 * the LWP and set pending on the thread.
			 * Signals remain disabled until the thread
			 * restores its t_nosig flag to zero.
			 */

			_thr_sigdiffset(&tmpactp->sa_mask, &_thr_sig_cantmask);
		
			if (sigismember(&_thr_sig_cantreset, sig)) {
				tmpactp->sa_flags &= ~SA_RESETHAND;
			}

			if ((tmpactp->sa_flags & SA_NODEFER) == 0) {
				sigaddset(&tmpactp->sa_mask, sig);
			}		
			if (tmpactp->sa_flags & SA_RESETHAND) {
				sigdelset(&tmpactp->sa_mask, sig);
			}	
			_thr_siguinfo[sig].sa_handler = tmpactp->sa_handler;
			_thr_siguinfo[sig].sa_mask = tmpactp->sa_mask;
			_thr_siguinfo[sig].sa_flags = tmpactp->sa_flags;
			/*
			 * When the common signal handler runs
			 * with all signals masked, a thread needs
			 * only one siginfo.
			 */
			
			 sigfillset(&tmpactp->sa_mask);
			 _thr_sigdiffset(&tmpactp->sa_mask,
					 &_thr_sig_programerror);
		}
	} else {
		tmpactp = NULL;
	}
	rval = (*_sys__sigaction)(sig, tmpactp, oactp, _thr_sigacthandler);
	if (rval != 0 && tmpactp != NULL) {
		sigemptyset(&_thr_siguinfo[sig].sa_mask);
		_thr_siguinfo[sig].sa_flags = 0;
		_thr_siguinfo[sig].sa_handler = NULL;
	}
out:	if (oactp != NULL) {
		if(sig == SIGLWP) {
			oactp->sa_handler = _thr_siglwp_handler;
		} else if(sig == SIGWAITING) {
			oactp->sa_handler = _thr_sigwaiting_handler;
		}
	}
	_THR_MUTEX_UNLOCK(&_thr_siguinfolock);
	return (rval);
}

/*
 * 
 * int
 * sigaction(int sig, const struct sigaction *nactp, struct sigaction *oactp)
 *
 *	sigaction() is a wrapper of the sigaction(2) system 
 *	call to examine and/or specify the action to be taken on 
 *	delivery of a specific signal.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, _thr_siguinfolock may be acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 */

int
_sigaction(int sig, const struct sigaction *nactp, struct sigaction *oactp)
{
	thread_desc_t *tp = curthread;
	int rval;

	if (sig <= 0 || sig > MAXSIG) {
		errno = EINVAL;
		return (-1);
	}
	PRINTF2(" *** sigaction: sig = %d thread id = %d\n", sig,tp->t_tid);
	_thr_sigoff(tp);

	rval = _thr_sigaction(sig, nactp, oactp);
	_thr_sigon(tp);

	return (rval);
}

/*
 * void
 * __thr_siglwphndlr(int sig, siginfo_t *sip, ucontext_t *uap)
 *
 *	SIGLWP is intended for the Threads Library use only.
 *	The Threads Library does not allow applications to 
 *	block, reset disposition, or send SIGLWP,
 *	The Threads Library uses SIGLWP to suspend or preempt a thread.
 *	__thr_siglwphndlr() is the SIGLWP signal handler set by the
 *	Threads Library.
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, the thread lock of the thread being suspended
 * 	is acquired and released. In addition, the sync lock of the t_join
 *	condition variable and the thread lock of any threads awakened
 *	may be acquired indirectly via the call to cond_broadcast.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 */
/* ARGSUSED */
void
__thr_siglwphndlr(int sig, siginfo_t *sip, ucontext_t *uap)
{
	int ostate;
	thread_desc_t *tp = curthread;
	int save_errno = errno;

	ASSERT(tp->t_tid != -1);

	_thr_sigoff(tp);

	if (tp->t_suspend == TSUSP_PENDING) {
		LOCK_THREAD(tp);
		/*
		 * make sure the suspend is still pending after
		 * the thread lock is acquired
		 */
		if (tp->t_suspend == TSUSP_PENDING) {
			ostate = tp->t_state;
			tp->t_state = TS_SUSPENDED;
			tp->t_suspend = TSUSP_CLEAR;
			_THR_COND_BROADCAST(&tp->t_join);
			if (ISBOUND(tp)) {
				if (_thr_debug.thr_debug_on) { 
					_thr_debug_notify(tp, 
					    tc_thread_suspend);
				}
				/*
				 * SUSPEND_BOUND_THREAD doesn't release the
				 * thread lock so we must do it explictly
				 * afterwards.
				 */
				SUSPEND_BOUND_THREAD(tp);
				UNLOCK_THREAD(tp);
			} else {
				/*
				 * TS_ONPROC is a very vague state.
				 * One can't tell the difference
				 * between a MUX thread that is
				 * actually running versus a MUX
				 * thread that is sleeping on an
				 * LWP sync primitive. Since we
				 * are going to switch off the lwp
				 * we don't want to leave it
				 * half blocked in the kernel.
				 * If it is not on a kernel sync queue
				 * then there is no problem, it does
				 * however cost us a system call.
				 */
				if (ostate == TS_ONPROC) {
					cancelblock();
				}
				_thr_swtch(1, tp);
			}
		} else {
			UNLOCK_THREAD(tp);
		}
	} 
	if (PREEMPTED(tp)) {
		LOCK_THREAD(tp);
		ASSERT(PREEMPTED(tp));
		PREEMPT_SELF(tp);
	}
	_thr_sigon(tp);
	errno = save_errno;
}

/*
 * void
 * _thr_sigx(thread_desc_t *tp, boolean_t lwpmask)
 *
 *	_thr_sigx() is an internal library function called by _thr_sigon()
 *	on exit from a critical section.
 *	When a signal arrives at a critical section, the signal and its
 *	context are saved, and all signals are blocked until the thread 
 *	exits the critical section, and the _thr_sigx() is called.
 *	Thus, there can only be one signal ever pending while in a critical
 *	section.
 *	_thr_sigx() retrieves the signal information, adjusts signal
 *	mask as requested by the user-specified handler and executes the
 *	handler.
 *	Before return, _thr_sigx() restores thread's and LWP's
 *	signal masks to their original values.
 *	
 * Parameter/Calling State:
 *	On entry, all signals are masked and no locks are held.
 *	During processing, the calling thread lock and _thr_siguinfolock
 *	locks are acquired.
 *
 * Return Values/Exit State:
 *	On exit, signals are unmasked and no locks are held.
 */

void
_thr_sigx(thread_desc_t *tp, boolean_t lwpmask)
{
	int  sigx;
	int signo;
	sigset_t psig;

	/*
	 * signal could not have been masked in t_oldmask because if it had
	 * been, then it would not have been stored in t_sig.
	 */

	PRINTF2("_thr_sigx: t_tid = %d, t_sig = %d\n", tp->t_tid, tp->t_sig);

	ASSERT(!sigismember(&tp->t_oldmask, tp->t_sig));

	sigx = tp->t_sig;
	tp->t_sig = 0;
	tp->t_hold = tp->t_oldmask;
	ASSERT(tp->t_state == TS_ONPROC);

	/* process cancel requests from a debugger before
	 * attempting to send pending signals
	 */
	if (_thr_debug.thr_debug_on) {
		tp->t_dbg_busy = 1;
		if (tp->t_dbg_cancel) {
			_thr_sigdiffset(&tp->t_psig, &tp->t_dbg_set);
			if (sigismember(&tp->t_dbg_set, sigx))
				sigx = 0;
			sigemptyset(&tp->t_dbg_set);
			tp->t_dbg_cancel = 0;
			tp->t_dbg_busy = 0;
			_thr_debug_notify(tp, tc_cancel_complete);
		}
		else
			tp->t_dbg_busy = 0;
	}
	if (sigx) { /* might have been cleared for debugger, above */
		_lwp_kill(LWPID(tp), sigx);
	}
	
	/*
	 * We can lock thread descriptor here even though we have sigon
	 * since all signals are blocked
	 */
	LOCK_THREAD(tp);
	if (_thr_sigisset(&tp->t_psig, &tp->t_hold) != 0) {
		psig = tp->t_psig;

		/* process all signals pending for the thread (in t_psig) */
		while (signo = _thr_signext(&psig)) {
			sigdelset(&psig, signo);
			if (!sigismember(&tp->t_hold, signo)) {
				PRINTF4(" ------ in _thr_sigx tid: %d,  signo: %d, t_set : 0x%x 0x%x\n-------will call _lwp_kill --------", tp->t_tid, signo,
				&tp->t_hold.sa_sigbits[0], &tp->t_hold.sa_sigbits[1]);
				sigdelset(&tp->t_psig, signo);
				if (_lwp_kill(LWPID(tp), signo) < 0)
					_thr_panic("_thr_sigx: _lwp_kill");
			}
		}
	}
	UNLOCK_THREAD(tp);
	if (lwpmask == B_TRUE) {
		(*_sys_sigprocmask)(SIG_SETMASK, &tp->t_hold, NULL);
	}	
	PRINTF2("_thr_sigx: tp signal mask after calling _sys_sigprocmask() is 0x%x and 0x%x\n", tp->t_hold.sa_sigbits[0], tp->t_hold.sa_sigbits[1]);
}

/*
 * int
 * _thr_sigsend(thread_desc_t *tp, int sig)
 *
 *	_thr_sigsend() is internal library function to send a signal,
 *	sig, to a thread tp. If the state of the target thread is TS_SLEEP
 *	the _thr_activate_lwp() is being called.
 *
 * Parameter/Calling State:
 *	On entry, target thread lock is held and signal handlers are
 *	disabled.
 *
 * Return Values/Exit State:
 *	On exit, all signal handlers remain enabled and the target
 *	thread lock is still held.
 *	INVALID_PRIO: caller need not call _thr_activate_lwp().
 *	any other value: caller should call _thr_activate_lwp().
 */

/* ARGSUSED */
int
_thr_sigsend(thread_desc_t *tp, int sig)
{
	int rval = INVALID_PRIO;

	/* thread lock is acquired by the caller */

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(tp != NULL);
	ASSERT(IS_THREAD_LOCKED(tp));
	ASSERT(tp->t_tid != -1);
	ASSERT(sig > 0 && sig < MAXSIG);
	ASSERT(tp->t_state != TS_ZOMBIE);

	PRINTF3(" *** _thr_sigsend: thread = %d sending signal = %d to thread = %d\n",curthread->t_tid, sig, tp->t_tid);

	if (sig) {
		if (sigismember(&tp->t_psig, sig)) {
			ASSERT(!_thr_sigisempty(&tp->t_psig));
			/*
			 * are we assuming we don't need to queue the signals??
			 */
			return (rval);
		}
		sigaddset(&tp->t_psig, sig);
		if (sigismember(&tp->t_hold, sig)) {
                        if (tp->t_state == TS_SLEEP && tp->t_sig) {
                                if (!sigismember(&tp->t_oldmask, sig)) {
                                        rval = _thr_setrun(tp);
                                }
                        }
			return (rval);
		}
		if ((tp->t_state == TS_ONPROC) && !PREEMPTED(tp)) {
			PRINTF3(" *** _thr_sigsend: calling _lwp_kill() -- signal = %d  thread = %d lwp = %d ....\n", sig,
			    tp->t_tid, LWPID(tp));
			sigdelset(&tp->t_psig, sig);
			_lwp_kill(LWPID(tp), sig);
		} else if (tp->t_state == TS_SLEEP) {
			PRINTF(" *** _thr_sigsend calling _thr_setrun.............\n");
			rval = _thr_setrun(tp);
		}

	}
	return (rval);
}

/*
 * int
 *  sigprocmask(int how, const sigset_t *nsetp, sigset_t *osetp)
 *
 *	sigprocmask() is a wrapper of the sigprocmask(2)
 *	system function to set the thread signal mask.
 *	The function calls internal library function _thr_sigsetmask()
 *	to do the actual work.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and the calling
 *	thread lock is acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigprocmask(int how, const sigset_t *nsetp, sigset_t *osetp)
{
	thread_desc_t *tp = curthread;

	int rval;
	
	THR_SIGMASK_OFF(tp);
	rval = _thr_sigsetmask(how, nsetp, osetp, B_TRUE, tp);
	if (nsetp == NULL && osetp != NULL) {
		(*_sys_sigprocmask)(SIG_SETMASK, osetp, NULL);
	}
	UNLOCK_THREAD(tp);
	_thr_sigon(tp);	
	if (rval != 0) {
		errno = rval;
		return (-1);
	}
	return (0);
}

/*
 * void
 * setcontext(ucontext_t *uc)
 *
 *	setcontext() is a wrapper of the setcontext(2)
 *	system call to set the current context.
 *	The function sets thread signal mask and calls setcontext(2)
 *	system function to do the actual work.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, no locks are held.
 *
 * Return Values/Exit State:
 *	Does not return.
 */

void
_setcontext(ucontext_t *uc)
{
	thread_desc_t *tp = curthread;
	int signo;
	sigset_t psig;

	THR_SIGMASK_OFF(tp);
	tp->t_hold = uc->uc_sigmask;
	/*
	 * re-send those signals that were masked by t_hold but unmasked by
	 * uc_sigmask, in case they had come in during the handler when
	 * t_hold was in effect.
	 */

        /* handle any requests from a debugger to cancel
         * a pending thr_kill signal
         * we do this in the thread library to make sure our
         * data structures stay sane
         */
        if (_thr_debug.thr_debug_on) {
                tp->t_dbg_busy = 1;
                if (tp->t_dbg_cancel) {
                        _thr_sigdiffset(&tp->t_psig, &tp->t_dbg_set);
                        sigemptyset(&tp->t_dbg_set);
                        tp->t_dbg_cancel = 0;
                        tp->t_dbg_busy = 0;
                        _thr_debug_notify(tp, tc_cancel_complete);
                } else {
                        tp->t_dbg_busy = 0;
		}
        }

	if (_thr_sigisset(&tp->t_psig, &tp->t_hold) != 0) {

		psig = tp->t_psig;
		while (signo = _thr_signext(&psig)) {
			sigdelset(&psig, signo);
			if (!sigismember(&tp->t_hold, signo)) {
				sigdelset(&tp->t_psig, signo);
				if (_lwp_kill(LWPID(tp), signo) < 0)
               	                	_thr_panic("_setcontext: _lwp_kill");
			}
		}
	}
	UNLOCK_THREAD(tp);
	_thr_sigon(tp);

	PRINTF3("setcontext: tp->t_tid = %d, tp->t_hold = 0x%x and 0x%x\n", 
	tp->t_tid, tp->t_hold.sa_sigbits[0], tp->t_hold.sa_sigbits[1]);
	(*_sys_setcontext)(uc);
	/*NOTREACHED*/
}

/*
 * int
 * sighold(int sig)
 *
 *	sighold() is a wrapper of the sighold(2) system
 *	call to add a signal to the calling thread's signal mask.
 *	The function calls the library wrapper of sigprocmask()
 *	to do the actual job.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled via sigprocmask()
 *	and the calling thread lock is acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sighold(int sig)
{
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, sig);
	return (_sigprocmask(SIG_BLOCK, &nset, NULL));
}

/*
 * int
 * sigrelse(int sig)
 *
 *	sigrelse() is a wrapper of the sigrelse(2) system
 *	call to remove a signal from the calling thread's signal mask.
 *	The function calls the library wrapper of sigprocmask()
 *	to do the actual job.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled via sigprocmask()
 *	and the calling thread lock is acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigrelse(int sig)
{
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, sig);
	return (_sigprocmask(SIG_UNBLOCK, &nset, NULL));
}

/*
 * void
 *  (*signal(int sig, void (*disp)(int)))(int)
 *
 *	signal() is a wrapper of the signal(2) system
 *	call to modify signal dispositions.
 *	The function calls _thr_sigaction()
 *	to do the actual job.
 *	signal() returns the old signal disposition.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled via
 *	_thr_sigaction() and the _thr_siguinfolock is acquired.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

void 
(*_signal(int sig, void (*disp)(int)))(int)
{
	thread_desc_t *tp = curthread;
	struct sigaction nact, oact;
	int rval;
	
	nact.sa_handler = disp;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = SA_RESETHAND | SA_NSIGACT;
	_thr_sigoff(tp);
	rval = _thr_sigaction(sig, &nact, &oact);
	_thr_sigon(tp);
	if (rval == -1) {
		return (SIG_ERR);
	}
	return (oact.sa_handler);
}

/*
 * void
 * (*sigset(int sig, void (*disp)(int)))(int)
 *
 *	sigset() is a wrapper of the sigset(2) system
 *	call to modify signal dispositions.
 *	The function calls the library wrapper of sigprocmask()
 *	and _thr_sigaction() to do the actual job.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and via
 *	_thr_sigaction() the _thr_siguinfolock and the calling 
 *	thread lock is acquired.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

void 
(*_sigset(int sig, void (*disp)(int)))(int)
{
	thread_desc_t *tp = curthread;
	struct sigaction nact, oact;
	sigset_t nset, oset;
	int rval;

	if (disp == SIG_HOLD) {
		sigemptyset(&nset);
		sigaddset(&nset, sig);
		rval = _sigprocmask(SIG_BLOCK, &nset, &oset);
		if (rval != 0) {
			return (SIG_ERR);
		}
		if (sigismember(&oset, sig)) {
			return (SIG_HOLD);
		}
		_thr_sigoff(tp);
		_thr_sigaction(sig, NULL, &oact);
		_thr_sigon(tp);
		return (oact.sa_handler);
	}
	nact.sa_handler = disp;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = SA_NSIGACT;
	_thr_sigoff(tp);
	rval = _thr_sigaction(sig, &nact, &oact);
	if (rval != 0) {
		_thr_sigon(tp);
		return (SIG_ERR);
	}
	_thr_sigon(tp);
	THR_SIGMASK_OFF(tp);

	_thr_sigsetmask(0, NULL, &oset, B_TRUE, tp);
	(*_sys_sigprocmask)(SIG_SETMASK, &oset, NULL);

	UNLOCK_THREAD(tp);
	_thr_sigon(tp);
	if (sigismember(&oset, sig)) {
		return (SIG_HOLD);
	}
	return (oact.sa_handler);
}

/*
 * int
 * sigignore(int sig)
 *
 *	sigignore() is a wrapper of the sigignore(2) system
 *	call to set the disposition of a signal to SIG_IGN.
 *	The function initializes sigaction structure and calls the
 *	library wrapper of the sigaction() system function to do the 
 *	actual job.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and via 
 *	sigaction() the _thr_siguinfolock lock is acquired.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigignore(int sig)
{
	struct sigaction nact;

	nact.sa_handler = SIG_IGN;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = SA_NSIGACT;
	return (_sigaction(sig, &nact, NULL));
}

/*
 * int
 * sigsuspend(const sigset_t *set)
 *	sigsuspend() is a wrapper of the sigsuspend(2) system
 *	call to replace thread's signal mask with the signal mask provided 
 *	and suspend the calling thread until a signal is delivered whose
 *	action is either to execute a signal caching function or to 
 *	terminate the process.
 *
 *	The function calls thr_sigsetmask() to change thread's
 *	signal mask and sigsuspend(2) actual system function to suspend
 *	the thread.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During the call to thr_sigsetmask() signal handlers are disabled 
 *	and thread lock	is held.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigsuspend(const sigset_t *set)
{
        thread_desc_t   *tp = curthread;
	sigset_t oldset;

	THR_SIGMASK_OFF(tp);
#ifdef DEBUG
	ASSERT(!_thr_sigsetmask(SIG_SETMASK, set, &oldset, B_FALSE, tp));
#else
	(void)_thr_sigsetmask(SIG_SETMASK, set, &oldset, B_FALSE, tp);
#endif
	UNLOCK_THREAD(tp); 
	_thr_tsigon(tp); 

	(*_sys_sigsuspend)(set);

        _thr_sigoff(tp);
        LOCK_THREAD(tp);
#ifdef DEBUG
	ASSERT(!_thr_sigsetmask(SIG_SETMASK, &oldset, NULL, B_TRUE, tp));
#else
	(void)_thr_sigsetmask(SIG_SETMASK, &oldset, NULL, B_TRUE, tp);
#endif
        UNLOCK_THREAD(tp);
        _thr_sigon(tp);
	ASSERT(!_thr_sigcmpset(&tp->t_hold, &oldset));
	errno = EINTR;
	return (-1);
}

/*
 * int
 * sigpause(int sig)
 *
 *	sigpause() is a wrapper of the sigpause(2) system
 *	call to remove a signal from the calling thread's
 *	signal mask and suspend the thread until a signal is received.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During the call to thr_sigsetmask(), signal handlers are disabled
 *	and thread lock	is held.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigpause(int sig)
{
        thread_desc_t   *tp = curthread;
	sigset_t set;
	sigset_t oldset;
	
	THR_SIGMASK_OFF(tp);
        (void)_thr_sigsetmask(0, NULL, &set, B_FALSE, tp);

	oldset = set;
	sigdelset(&set, sig);
	
	(void)_thr_sigsetmask(SIG_SETMASK, &set, NULL, B_FALSE, tp);
        UNLOCK_THREAD(tp);
        _thr_tsigon(tp);
	(*_sys_sigprocmask)(SIG_SETMASK, &oldset, NULL);
#ifdef DEBUG
	ASSERT((*_sys_sigpause)(sig) == -1);
#else
	(void)(*_sys_sigpause)(sig);
#endif
	ASSERT(errno == EINTR || errno == EINVAL);
	return (-1);
}

/*
 * int
 * sigpending(sigset_t *set)
 *
 *	sigpending() is a wrapper of the sigpending(2) system
 *	call to examine signals that are blocked and pending.
 *	The function disables signal handlers, locks the calling thread's
 *	lock, and calculates signals that are blocked and pending.
 *	Before return, the function unlocks the calling thread's lock
 *	and enables signal handlers.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled, the calling thread
 *	lock and _thr_siguinfolock are required.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigpending(sigset_t *set)
{
	thread_desc_t *tp = curthread;
	sigset_t _thr_procpsig;
	int rval;

	_thr_sigoff(tp);
	LOCK_THREAD(tp);
	rval = (*_sys_sigpending)(&_thr_procpsig);
	
	if (rval != 0) {
		UNLOCK_THREAD(tp);
		_thr_sigon(tp);
		return (-1);
	}
	
	sigemptyset(set);
	_thr_sigorset(set, &tp->t_hold);
	_thr_sigandset(set, &tp->t_psig);
	_thr_sigorset(set, &_thr_procpsig);
	UNLOCK_THREAD(tp);
	_thr_sigon(tp);
	return (0);
}

/*
 * int
 * sigwait(sigset_t *set)
 *
 *	sigwait() is a wrapper of the sigwait(2) system
 *	call used to wait for a delivery of a signal to the calling thread.
 *	The function sets thread's signal mask via _thr_sigsetmask(), and
 *	calls sigwait() system function.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	i signal handlers are disabled, the calling thread
 *	lock is locked.
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *
 */

int
_sigwait(sigset_t *set)
{
	thread_desc_t *tp = curthread;
	int sig;
	sigset_t waitset;/* mask for thread based on set */
	sigset_t oldset;/* original signal mask for abort return */
	sigset_t rset;	/* signal mask upon return */

	/* first block those signals not specified by user */
	sigfillset(&waitset);
	/* now those that the user is interested in */
	_thr_sigdiffset(&waitset, set);

	THR_SIGMASK_OFF(tp);
	_thr_sigsetmask(SIG_SETMASK, &waitset, &oldset, B_FALSE, tp);
	sigaddset(&tp->t_hold, SIGLWP);
        UNLOCK_THREAD(tp);
        _thr_tsigon(tp);

	rset = oldset;
	_thr_sigorset(&rset, set);

	PRINTF2("sigwait wrapper: waitset: 0x%x and 0x%x\n", 
		waitset.sa_sigbits[0], waitset.sa_sigbits[1]);
	sig = (*_sys_sigwait)(set);
	PRINTF1("sigwait wrapper: _sys_sigwait return value: %d\n", sig);
	/*
	 * if _sys_sigwait() returned EINVAL, we restore the original mask
	 * for the thread (kernel handles the lwp mask in the same manner).
	 * if it was successful, then we SETMASK the original plus the argument
	 * to sigwait().
	 */
	_thr_sigoff(tp);
	LOCK_THREAD(tp);
	if (sig == EINVAL)
        	_thr_sigsetmask(SIG_SETMASK, &oldset, NULL, B_TRUE, tp);
	else
        	_thr_sigsetmask(SIG_SETMASK, &rset, NULL, B_TRUE, tp);
        UNLOCK_THREAD(tp);
        _thr_sigon(tp);
	return (sig);
}

/*
 * int
 * _thr_sigsetmask(int how, const sigset_t *set, sigset_t *oset,
 *		   boolean_t lwpmask, thread_desc_t *tp)
 *
 *	_thr_sigsetmask() is a Threads Library function that allows a thread
 *	to change or examine its own signal mask, i.e., t_hold.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled;
 *	these conditions remain the same on exit.
 *
 * Return Values/Exit State:
 *	On exit, signal handlers are disabled, and the calling thread's lock
 *	is held.  Returns an error code.
 */

/* ARGSUSED */
STATIC int
_thr_sigsetmask(int how, const sigset_t *set, sigset_t *oset,
		boolean_t lwpmask, thread_desc_t *tp)
{
	sigset_t tmpset, psig;
	sigset_t *t_set;
	int signo;
        ASSERT(THR_ISSIGOFF(tp));
	if (oset != NULL) {
		*oset = tp->t_hold;
	}
	if (set == NULL)
		return (0);
	tmpset = *set;
	_thr_sigdiffset(&tmpset, &_thr_sig_cantmask);
	t_set = &tp->t_hold;

	switch(how) {
	case SIG_BLOCK:
		_thr_sigorset(t_set, &tmpset);
		break;
	case SIG_UNBLOCK:
		_thr_sigdiffset(t_set, &tmpset);
		break;
	case SIG_SETMASK:
		*t_set = tmpset;
		break;
	default:
		return(EINVAL);
	}
	/*
	 * if a pending signal (in tp->t_psig) is unblocked (ie not in t_set)
	 * as a result, then we send it to the tp.
	 */
	/* handle any requests from a debugger to cancel
	 * a pending thr_kill signal
	 * we do this in the thread library to make sure our
	 * data structures stay sane
	 */
	if (_thr_debug.thr_debug_on) {
		tp->t_dbg_busy = 1;
		if (tp->t_dbg_cancel) {
			_thr_sigdiffset(&tp->t_psig, &tp->t_dbg_set);
			sigemptyset(&tp->t_dbg_set);
			tp->t_dbg_cancel = 0;
			tp->t_dbg_busy = 0;
			_thr_debug_notify(tp, tc_cancel_complete);
		}
		else
			tp->t_dbg_busy = 0;
	}

        if (_thr_sigisset(&tp->t_psig, t_set) != 0) {
		psig = tp->t_psig;
		while (signo = _thr_signext(&psig)) {
			sigdelset(&psig, signo);
			if (!sigismember(t_set, signo)) {
				sigdelset(&tp->t_psig, signo);
				if (_lwp_kill(LWPID(tp), signo) < 0)
					/*
					 * Is this a legit panic?  Maybe,
					 * because we've lost atomicity by
					 * now.  It would be better to try to
					 * recover.
					 */
               	                	_thr_panic("_thr_sigsetmask: _lwp_kill");
			}
		}
	}
	if (lwpmask == B_TRUE) {
		(*_sys_sigprocmask)(SIG_SETMASK, t_set, NULL);
	} 
	return(0);
}

/*
 * int
 * thr_sigsetmask(int how, const sigset_t *set, sigset_t *oset)
 *
 *	thr_sigsetmask() allows a thread to change or examine its own signal
 *	mask by calling _thr_sigsetmask().
 *
 * Parameter/Calling State:
 *	On entry, no signals are blocked and no locks are held.
 *	During the call to thr_sigsetmask(), signal handlers are disabled
 *	and calling thread's lock is acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held and no signals are blocked.
 */

/* ARGSUSED */

int
thr_sigsetmask(int how, const sigset_t *set, sigset_t *oset)
{
	thread_desc_t	*tp = curthread;
	int rval;

	ASSERT(THR_ISSIGON(tp));

	THR_SIGMASK_OFF(tp);
	rval = _thr_sigsetmask(how, set, oset, B_TRUE, tp);
	if (set == NULL && oset != NULL) {
		(*_sys_sigprocmask)(SIG_SETMASK, oset, NULL);
	}
	UNLOCK_THREAD(tp);
	_thr_sigon(tp);
	ASSERT(THR_ISSIGON(tp));

	TRACE4(tp, TR_CAT_THREAD, TR_EV_THR_SIGSETMASK, TR_CALL_ONLY,
	   how, set, oset, rval);
	return(rval);
}

/*
 * int
 * thr_kill(thread_t tid, int sig)
 *
 *	thr_kill() sends a signal sig to a thread tid by calling
 *	_lwp_kill(), or stores the signal in t_psig until it can be sent later
 *	by _thr_resendsig().
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled by _thr_sigoff(), 
 *	and tid's thread lock and _thr_tidveclock are  acquired.
 *
 * Return Values/Exit State:
 *	On exit, all signals are enabled by _thr_sigon(), and no locks are held.
 */

/* ARGSUSED */
int
thr_kill(thread_t tid, int sig)
{
	thread_desc_t *tp;
	thread_desc_t *ctp = curthread;
	int rval = 0;
	int rc;

	ASSERT(THR_ISSIGON(ctp));

	PRINTF3(" *** thr_kill: thread = %d sending signal = %d to thread = %d\n",ctp->t_tid, sig, tid);

	if (sig < 0 || sig > MAXSIG || sig == SIGLWP || sig == SIGWAITING) {
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_KILL, TR_CALL_ONLY,
		   tid, sig, EINVAL);
		return (EINVAL);
	}
	_thr_sigoff(ctp);
	tp = _thr_get_thread(tid);

	if (tp == NULL) {
		rval = ESRCH;
		goto thr_kill_out2;
	}
	/* thread lock is held */
	if (tp->t_state == TS_ZOMBIE) {
		rval = ESRCH;
		goto thr_kill_out1;
	}
	if (sig) {
		if (sigismember(&tp->t_psig, sig)) {
			/*
			 * are we assuming we don't need to queue the signals??
			 */
			PRINTF2(" *** thr_kill: tp = %d, signal already pending sig = %d\n", tp->t_tid, sig);

			goto thr_kill_out1;
		}
		sigaddset(&tp->t_psig, sig);
		if (sigismember(&tp->t_hold, sig)) {
                        if (tp->t_state == TS_SLEEP && tp->t_sig) {
                                if (!sigismember(&tp->t_oldmask, sig)) {
                                        rc = _thr_setrun(tp);
                                        UNLOCK_THREAD(tp);
                                        if (rc != INVALID_PRIO)
                                                _thr_activate_lwp(rc);
                                        goto thr_kill_out2;
                                }
                        }
			goto thr_kill_out1;
		}
		if ((tp->t_state == TS_ONPROC) && !PREEMPTED(tp)) {
			PRINTF3(" *** thr_kill: calling _lwp_kill() -- signal = %d  thread = %d lwp = %d ....\n", sig, tp->t_tid, LWPID(tp));
			sigdelset(&tp->t_psig, sig);
			rval = _lwp_kill(LWPID(tp), sig);
			goto thr_kill_out1;
		} else if (tp->t_state == TS_SLEEP) {
			PRINTF(" *** thr_kill calling _thr_setrun.............\n");
			rc = _thr_setrun(tp);
			UNLOCK_THREAD(tp);
			if (rc != INVALID_PRIO)
                        	_thr_activate_lwp(rc);
			goto thr_kill_out2;
		}
	}
thr_kill_out1:
	UNLOCK_THREAD(tp);
thr_kill_out2:
	TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_KILL, TR_CALL_ONLY,
	   tid, sig, rval);
	_thr_sigon(ctp);

	ASSERT(THR_ISSIGON(ctp));

	return(rval);
}

/*
 * __thr_sigwaitinghndlr()
 *      this is the handler for SIGWAITING when it is enabled.
 *      It causes a multiplexing LWP to be made available if there
 *      are runnable threads that can use an LWP.
 *
 *      The handler is enabled when a thread is placed on the
 *      runnable queue and no LWP is available to run it.  It
 *      is disabled by the handler itself if SIGWAITING is
 *      received and no runnable thread exists.
 *
 * Parameter/Calling State:
 *      takes no arguments; executed when process receives SIGWAITING
 *
 *      No locks are held but all signals are blocked on entry.
 *
 *      During processing, _thr_counterlock is acquired.
 *
 * Return Values/Exit State:
 *      If a runnable thread exists, a multiplexing LWP is activated
 *      via the _thr_aging condition variable or a new multiplexing
 *      LWP is created if none are sleeping on _thr_aging.
 *      If no runnable thread exists, the handler is disabled via a
 *      call to the sigaction system call.
 */

void
__thr_sigwaitinghndlr()
{
	int rval;
	struct sigaction sigact;
	thread_desc_t *curtp = curthread;
	int save_errno = errno;

	ASSERT(curtp->t_tid != -1);

	PRINTF("in __thr_sigwaitinghndlr()\n");
	/*
	 * There is no need to check if the current thread is
	 * in a critical section because if it were, the handler
	 * wouldn't be running.
	 */
	_thr_sigoff(curtp);
	/*
	 * _thr_sigoff() and _thr_sigon() are used only to satisfy
	 * library asserts that signal handlers are disabled during
	 * various function calls; they are actually unnecessary
	 * because all signals are blocked by the LWP when the handler
	 * is executing.
	 */
	/*
	 * The following line is to be able implement sleep within threads
	 * library. It is used by bound threads calling sleep.
	 * The T_SIGWAITUP is set here to indicate that sleep was interrupted 
	 * by SIGWAITING. The sleep() will check for this flag to detgermine if
	 * sleep() needs to be restarted.
	 */
	curtp->t_flags |= T_SIGWAITUP;
	LOCK_RUNQ;
	if ((_thr_nrunnable) && (!_thr_lwpswanted)) {
	/*
	 * Runnable threads are present and the housekeeper hasn't been 
	 * activated yet; if housekeeper is already activated, we disable
	 * the SIGWAITING handler since creation of more multiplexing LWPs
	 * has already been arranged.  The housekeeper will re-enable the
	 * handler after it has created the LWPs if there are still runnable
	 * threads.
	 */
		if (ISBOUND(curtp)) {
			/*
			 * We don't want a bound thread to create a multiplexing
			 * LWP since this may compromise the homogeneity of
			 * the LWP pool.  Therefore, we activate the
			 * housekeeping thread for this job.
			 */
			_thr_lwpswanted++;
			UNLOCK_RUNQ;
			PRINTF("__thr_sigwaitinghndlr: making new LWP\n");
			_thr_wakehousekeeper();
			_thr_sigon(curtp);
			errno = save_errno;
			return;
		} else {
			/*
			 * Since the current thread is multiplexed, it's ok
			 * for it to create a multiplexing LWP.  Therefore,
			 * we do it here instead of in the housekeeper to
			 * save the overhead of activating the housekeeper.
			 */
			UNLOCK_RUNQ;
			if (_thr_new_lwp(NULL, _thr_age, NULL) == 0) {
				/* LWP creation succeeded */
				PRINTF("__thr_sigwaitinghndlr: made new LWP\n");
				_thr_sigon(curtp);
				errno = save_errno;
				return;
			}
			/*
			 * If the LWP creation failed, we fall through and
			 * inactivate the signal handler.  We do this because
			 * we've reached some state where the signal handler
			 * can no longer create LWPs and therefore does not
			 * do us any good.
			 */
		}
	}
	/*
	 * If we get here, no runnable threads exist, the handler is unable
	 * to create LWPs (probably because we've exceeded the limit of LWPs
	 * for our user ID), or the housekeeper has already been activated to
	 * create more LWPs; in any case, we disable SIGWAITING handler.
	 */
	PRINTF("__thr_sigwaitinghndlr: disabling handler\n");
	sigact.sa_handler = SIG_IGN;
	rval = (*_sys__sigaction)(SIGWAITING, &sigact,NULL, _thr_sigacthandler);        _thr_sigwaitingset = B_FALSE;
	UNLOCK_RUNQ;
	if (rval < 0)
		_thr_panic("__thr_sigwaitinghndlr sigaction failed");
	_thr_sigon(curtp);
	errno = save_errno;
}

/*
 * int
 * _thr_resendsig(boolean_t lwpmask, thread_desc_t  *oldtp, thread_desc_t *tp)
 *
 *	_thr_resendsig is internal library function to ensure that:
 *	(1) for outgoing thread:
 *	(a) if the thread received a signal while being switched off and the thread 
 *	does not block the signal and the thread state is TS_SLEEP the signal 
 * 	will be delivered immediately before switch completes.
 *	(b) if thread's state is other than TS_SLEEP and the thread received 
 *	a signal while being switched off signal will not be processed.
 *	(2) for switching in thread if a pending signal (in tp->t_psig) is
 *	unblocked as a result, then the signal is send to the thread's lwp.
 *	
 *	If necessary, lwp signal mask is adjusted to match thread's signal mask.
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and both outgoing and switching in thread's
 *	thread lock is held.
 *	During processing, outgoing thread lock is released.
 *
 * Return Values/Exit State:
 *	On exit, signal handlers are disabled and the switching in thread 
 *	lock is held.
 */

int
_thr_resendsig(boolean_t lwpmask, thread_desc_t *oldtp, thread_desc_t *tp)
{
	sigset_t *t_set, psig;
	int signo, rc, unlocked = 0;
        ASSERT(THR_ISSIGOFF(tp));

	if (oldtp) {
		/*
		 * Clear preemption flag if it's set.
		 */
		if(oldtp->t_flags & T_PREEMPT) {
			oldtp->t_flags &= ~T_PREEMPT;
		}

		/*
		 * first, if the outgoing thread had received a signal while it was
		 * in critical section, we have to _thr_setrun() it and activate
		 * the lwp so the signal won't be potentially delayed indefinitely.
		 */

		if (oldtp->t_sig) {
		PRINTF1(" ------ in _thr_resendsig: oldtp tid %d\n", oldtp->t_tid);
		PRINTF1(" ------ in _thr_resendsig: oldtp t_sig %d\n", oldtp->t_sig);
		PRINTF2(" ------ in _thr_resendsig: oldtp oldmask: 0x%x 0x%x\n", 
			oldtp->t_oldmask.sa_sigbits[0], oldtp->t_oldmask.sa_sigbits[1]);
			ASSERT(oldtp->t_tid != -1);
			ASSERT(oldtp->t_state == TS_SLEEP
			   || oldtp->t_state == TS_ZOMBIE
 			   || oldtp->t_state == TS_RUNNABLE
			   || oldtp->t_state == TS_SUSPENDED);

			switch(oldtp->t_state) {
			case TS_SLEEP:
				if (!sigismember(&oldtp->t_oldmask, oldtp->t_sig)) {
					rc = _thr_setrun(oldtp);
					UNLOCK_THREAD(oldtp);
					unlocked = 1;
					/*
					 * We can't call _thr_activate_lwp since
					 * we hold our thread lock, so we set 
					 * t_exitval and when we return from 
					 * _thr_resume, call _thr_activate_lwp
					 * with the proper argument and clear
					 * t_exitval.  If rc is 0, we set 
					 * t_exitval to 1 to ensure that an
					 * attempt is made to get an LWP.
					 */
					if (rc != INVALID_PRIO) {
						if (rc == 0) {
						     tp->t_exitval = (void *)1;
						} else {
						     tp->t_exitval = (void *)rc;
						}
					}
				}
				break;

			case TS_RUNNABLE:
				/*
				 * oldtp is preempting
				 */
				PRINTF2(" ------ _thr_resendsig: tid %d RUNNABLE with sig %d\n", oldtp->t_tid, oldtp->t_sig);
				break;

			case TS_ZOMBIE:
				/*
				 * oldtp is exiting
				 */
				PRINTF2(" ------ _thr_resendsig: tid %d ZOMBIE exiting with sig %d\n", oldtp->t_tid, oldtp->t_sig);
				break;

			case TS_SUSPENDED:
				/*
				 * A thread being suspended does not accept signals
				 */
				PRINTF2(" ------ _thr_resendsig: tid %d SUSPENDED with sig %d\n", oldtp->t_tid, oldtp->t_sig);
				break;
                	default:
				 _thr_panic("_thr_resendsig: Illegal thread state during switching off");
                        	break;
                	}
		}
		if (unlocked == 0) {
			UNLOCK_THREAD(oldtp);
		}
	}

	/*
	 * for the new thread, if a pending signal (in tp->t_psig) is 
	 * unblocked (ie not in t_set) as a result, then we send it to the tp.
	 */
	t_set = &tp->t_hold;
	PRINTF1(" ------ _thr_resendsig: before while, switched in tid %d\n", tp->t_tid);
	PRINTF2(" ------ _thr_resendsig: before while switched tid's t_hold: 0x%x 0x%x\n", 
	t_set->sa_sigbits[0], t_set->sa_sigbits[1]);
	PRINTF2(" ------ _thr_resendsig: before while switched tid's t_psig: 0x%x 0x%x\n", 
	tp->t_psig.sa_sigbits[0], tp->t_psig.sa_sigbits[1]);

	/* handle any requests from a debugger to cancel
	 * a pending thr_kill signal
	 * we do this in the thread library to make sure our
	 * data structures stay sane
	 */
	if (_thr_debug.thr_debug_on) {
		tp->t_dbg_busy = 1;
		if (tp->t_dbg_cancel) {
			_thr_sigdiffset(&tp->t_psig, &tp->t_dbg_set);
			sigemptyset(&tp->t_dbg_set);
			tp->t_dbg_cancel = 0;
			tp->t_dbg_busy = 0;
			_thr_debug_notify(tp, tc_cancel_complete);
		}
		else
			tp->t_dbg_busy = 0;
	}
        if (_thr_sigisset(&tp->t_psig, t_set) != 0) {

	psig = tp->t_psig;
	while (signo = _thr_signext(&psig)) {
		sigdelset(&psig, signo);
		if (!sigismember(t_set, signo)) {
	PRINTF1(" ------ _thr_resendsig: calling _lwp_kill signo %d\n",signo);
			sigdelset(&tp->t_psig, signo);
			if (_lwp_kill(LWPID(tp), signo) < 0) {
				/*
				 * Is this a legit panic?  Maybe,
				 * because we've lost atomicity by
				 * now.  It would be better to try to
				 * recover.
				 */
                               	_thr_panic("_thr_sigsetmask: _lwp_kill");
			}
		}
	}
	}
	if(lwpmask == B_TRUE) {
		(*_sys_sigprocmask)(SIG_SETMASK, t_set, NULL);
	}
	return(0);
}

