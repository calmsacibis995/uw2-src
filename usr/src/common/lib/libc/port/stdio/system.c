/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/system.c	1.22.1.8"
/*LINTLIBRARY*/

#ifdef DSHLIB
#   define WAITPID_ONLY
#endif

#include "synonyms.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#ifndef WAITPID_ONLY
#   include <sys/utsname.h>
#   undef uname
#   ifdef __STDC__
	int uname(struct utsname *);
#   else
	int uname();
#   endif
#endif
#include "stdiom.h"

#ifdef _REENTRANT
#define FORK fork1
#else
#define FORK fork
#endif

int
#ifdef __STDC__
system(const char *cmd)
#else
system(cmd)const char *cmd;
#endif
{
#ifndef WAITPID_ONLY
	static int vers;
	struct utsname buf;
#endif
	static const struct sigaction ign = {0, SIG_IGN};
	static const struct sigaction dfl = {0, SIG_DFL};
	struct sigaction intr, quit, chld;
	pid_t pid;
	int status;

	if (cmd == 0)
		return access(_str_shpath, X_OK | EFF_ONLY_OK) + 1;
	/*
	* Set interrupt and quit to be ignored, and SIGCHLD to be blocked
	* but with non-ignore handling (so that the wait[pid]s can work).
	* Note that use of the above constant struct sigactions (ign, dfl)
	* presumes that sa_handler is the second member, and that an empty
	* signal set is denoted by all zeroes.
	*/
	sigaction(SIGINT, &ign, &intr);
	sigaction(SIGQUIT, &ign, &quit);
	sighold(SIGCHLD);
	sigaction(SIGCHLD, (struct sigaction *)0, &chld);
	if (chld.sa_handler == SIG_IGN)
		sigaction(SIGCHLD, &dfl, (struct sigaction *)0);
	if ((pid = FORK()) == -1)
	{
		status = -1;
	}
	else if (pid != 0)	/* parent */
	{
#ifndef WAITPID_ONLY
	again:;
		if (vers > 1) /* use waitpid() as available */
		{
#endif
			while (waitpid(pid, &status, 0) < 0)
			{
				if (errno != EINTR)
				{
					status = -1;
					break;
				}
			}
#ifndef WAITPID_ONLY
		}
		else if (vers == 1) /* older system: can't use waitpid() */
		{
			int w;

			while ((w = wait(&status)) != pid)
			{
				if (w == -1 && errno != EINTR)
				{
					status = -1;
					break;
				}
			}
		}
		else /* first time called */
		{
			vers = 1;
			if (uname(&buf) > 0) /* at least SVR4 system */
				vers = 2;
			goto again;
		}
#endif
	}
	/*
	* Restore handling for all signals touched.  Note that we're
	* either the child or the parent AFTER the child has completed.
	*/
	sigaction(SIGINT, &intr, (struct sigaction *)0);
	sigaction(SIGQUIT, &quit, (struct sigaction *)0);
	if (chld.sa_handler == SIG_IGN)
		sigaction(SIGCHLD, &chld, (struct sigaction *)0);
	sigrelse(SIGCHLD);
	if (pid == 0)
	{
		execl(_str_shpath, _str_shname, _str_sh_arg, cmd, (char *)0);
		_exit(127);
	}
	return status;
}
