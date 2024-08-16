/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/abort.c	1.21"
/*LINTLIBRARY*/
/*
 *	abort() - terminate current process with dump via SIGABRT
 */

#include "synonyms.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdlock.h"

#ifdef __STDC__
extern void _exithandle(void);
#else
extern void _exithandle();
#endif

void
#ifdef __STDC__
abort(void)
#else
abort()
#endif
{
	static int pass;
#ifdef _REENTRANT
	static StdLock lock;
#endif
	struct sigaction sa;

	sigrelse(SIGABRT);
	sigaction(SIGABRT, (struct sigaction *)0, &sa);
	if (sa.sa_handler == SIG_IGN)	/* POSIX.1 says to override */
	{
	reset:;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGABRT, &sa, (struct sigaction *)0);
	}
	if (sa.sa_handler == SIG_DFL)
	{
		STDLOCK(&lock);
		if (++pass == 1)
			_exithandle();
		STDUNLOCK(&lock);
	}
	kill(getpid(), SIGABRT);
	goto reset;		/* handler returned */
}
