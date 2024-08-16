/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/siglist.c	1.3"

#include "synonyms.h"
#include <signal.h>
#include "siginfom.h"

const char *const _sys_siglist[NSIG] =
{
	"UNKNOWN SIGNAL",
	"Hangup",			/* SIGHUP */
	"Interrupt",			/* SIGINT */
	"Quit",				/* SIGQUIT */
	"Illegal Instruction",		/* SIGILL */
	"Trace/Breakpoint Trap",	/* SIGTRAP */
	"Abort",			/* SIGABRT */
	"Emulation Trap",		/* SIGEMT */
	"Arithmetic Exception",		/* SIGFPE */
	"Killed",			/* SIGKILL */
	"Bus Error",			/* SIGBUS */
	"Segmentation Fault",		/* SIGSEGV */
	"Bad System Call",		/* SIGSYS */
	"Broken Pipe",			/* SIGPIPE */
	"Alarm Clock",			/* SIGALRM */
	"Terminated",			/* SIGTERM */
	"User Signal 1",		/* SIGUSR1 */
	"User Signal 2",		/* SIGUSR2 */
	"Child Status Changed",		/* SIGCLD */
	"Power-Fail/Restart",		/* SIGPWR */
	"Window Size Change",		/* SIGWINCH */
	"Urgent Socket Condition",	/* SIGURG */
	"Pollable Event",		/* SIGPOLL */
	"Stopped (signal)",		/* SIGSTOP */
	"Stopped (user)",		/* SIGTSTP */
	"Continued",			/* SIGCONT */
	"Stopped (tty input)",		/* SIGTTIN */
	"Stopped (tty output)",		/* SIGTTOU */
	"Virtual Timer Expired",	/* SIGVTALRM */
	"Profiling Timer Expired",	/* SIGPROF */
	"Cpu Limit Exceeded",		/* SIGXCPU */
	"File Size Limit Exceeded"	/* SIGXFSZ */
};

const int _sys_nsig = sizeof(_sys_siglist) / sizeof(_sys_siglist[0]);
