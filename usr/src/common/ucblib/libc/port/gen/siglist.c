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

#ident	"@(#)ucb:common/ucblib/libc/port/gen/siglist.c	1.3"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include <signal.h>

#ifndef NSIG
#define NSIG 32
#endif

char	*sys_siglist[NSIG] = {
	"Signal 0",
	"Hangup",			/* SIGHUP */
	"Interrupt",			/* SIGINT */
	"Quit",				/* SIGQUIT */
	"Illegal instruction",		/* SIGILL */
	"Trace/BPT trap",		/* SIGTRAP */
	"Abort",			/* SIGABRT */
	"Emulator trap",		/* SIGEMT */
	"Arithmetic exception",		/* SIGFPE */
	"Killed",			/* SIGKILL */
	"Bus error",			/* SIGBUS */
	"Segmentation fault",		/* SIGSEGV */
	"Bad system call",		/* SIGSYS */
	"Broken pipe",			/* SIGPIPE */
	"Alarm clock",			/* SIGALRM */
	"Terminated",			/* SIGTERM */
	"User defined signal 1",	/* SIGUSR1 */
	"User defined signal 2",	/* SIGUSR2 */
	"Child status change",		/* SIGCLD */
	"Power-fail restart",		/* SIGPWR */
	"Window changed",		/* SIGWINCH */
	"Handset, line status change",	/* SIGPHONE */
	"Pollable event occurred",	/* SIGPOLL */
	"Stopped (signal)",		/* SIGSTOP */
	"Stopped",			/* SIGTSTP */
	"Continued",			/* SIGCONT */
	"Stopped (tty input)",		/* SIGTTIN */
	"Stopped (tty output)",		/* SIGTTOU */
	"Virtual Timer Expried",	/* SIGVTALRM */
	"Profiling Timer Expired",	/* SIGPROF */
	"Cpu Limit Exceeded",		/* SIGXCPU */
	"File Size Limit Exceeded", 	/* SIGXFSZ */
};
