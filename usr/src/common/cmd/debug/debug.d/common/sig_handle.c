/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:debug.d/common/sig_handle.c	1.13"

/* signal handling function
 * written in C to get around cfront problems with
 * signal structure definitions
 */

#include "Machine.h"
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "handlers.h"
#include "Proctypes.h"

sigset_t	debug_sset;
sigset_t	sset_PI;
sigset_t	orig_mask;

#ifndef FOLLOWER_PROC
#include <thread.h>
/* thread specific data key - keep track of whether follower
 * threads have received SIGUSR2.
 */
thread_key_t	thrkey;
#endif


void
signal_setup()
{
	struct sigaction	act;
	/*
	 * SIG_INFORM and SIGPOLL are used internally in the read routine,
	 * but to protect ourselves from unnecessary grief we put
	 * SIG_INFORM and SIGPOLL on hold except in those regions of
	 * the code where we are prepared to field them. SIGINT is
	 * a great deal trickier.
	 *
	 * SIG_INFORM is a macro that allows the informing signal to be
	 * implemented differently on different systems.
	 * Care must be taken if SIGPOLL is used as SIG_INFORM, since
	 * POLL is already used to watch I/O
	 */

	/* signal set is static - should be 0 on startup */
	praddset(&debug_sset, SIG_INFORM);
	praddset(&debug_sset, SIGPOLL);
	praddset(&debug_sset, SIGINT);
	praddset(&sset_PI, SIGPOLL);
	praddset(&sset_PI, SIGINT);

	/* set signal mask and save original mask so we can
	 * restore it for our child processes.
	 */
	sigprocmask(SIG_SETMASK, &debug_sset, &orig_mask);
	/* child processes should block interrupt, however */
	praddset(&orig_mask, SIGINT);

	/* set up handlers with SA_SIGINFO to assure
	 * reliable queuing of signals, even though
	 * we ignore siginfo structure
	 */
	premptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	act.sa_handler = SIG_IGN;
	sigaction(SIGALRM, &act, 0);
	sigaction(SIGQUIT, &act, 0);
	sigaction(SIGCLD, &act, 0);

	act.sa_handler = (void (*)())fault_handler();
	sigaction(SIGINT, &act, 0);
	sigaction(SIGPIPE, &act, 0);

#ifndef FOLLOWER_PROC
	act.sa_handler = (void (*)())usr2_handler();
	sigaction(SIGUSR2, &act, 0);
	thr_keycreate(&thrkey, 0);
#endif
	/* Make sure debug does not core dump on
	 * top of a user's core file
	 */

	act.sa_handler = (void (*)()) internal_error_handler();
	sigaction(SIGHUP, &act, 0);
	sigaction(SIGILL, &act, 0);
	sigaction(SIGTRAP, &act, 0);
	sigaction(SIGEMT, &act, 0);
	sigaction(SIGFPE, &act, 0);
	sigaction(SIGBUS, &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGSYS, &act, 0);
	sigaction(SIGTERM, &act, 0);

	/* SIGINT, SIGPOLL and SIG_INFORM are masked in 
	 * the inform_handler and poll handlers
	 */
	praddset(&act.sa_mask, SIGINT);
	praddset(&act.sa_mask, SIGPOLL);
	act.sa_handler = (void (*)())inform_handler();
	sigaction(SIG_INFORM, &act, 0);

	praddset(&act.sa_mask, SIG_INFORM);
	act.sa_handler = (void (*)())poll_handler();
	sigaction(SIGPOLL, &act, 0);

	/* workaround for bug in libedit with sh/csh */
	if (signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, suspend_handler());
}

void
signal_unset()
{
	struct sigaction	act;

	premptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = SIG_DFL;
	sigaction(SIGHUP, &act, 0);
	sigaction(SIGILL, &act, 0);
	sigaction(SIGTRAP, &act, 0);
	sigaction(SIGEMT, &act, 0);
	sigaction(SIGFPE, &act, 0);
	sigaction(SIGBUS, &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGSYS, &act, 0);
	sigaction(SIGTERM, &act, 0);
}

/* handle fatal errors from the floating-point emulation
 * package; we cannot call internal_error() directly, since
 * the fpemu package is in C and internal_error() is in C++;
 * this routine will get us there through the signal mechanism
 */
void
fpemu_error()
{
	/* send ourselves floating point signal */
	kill(getpid(), SIGFPE);
}
