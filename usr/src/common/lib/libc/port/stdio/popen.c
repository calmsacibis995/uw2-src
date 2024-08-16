/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/popen.c	1.30.1.5"
/*LINTLIBRARY*/

#ifdef DSHLIB
#   define WAITPID_ONLY
#endif

#include "synonyms.h"
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
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

#ifdef __STDC__
	#pragma weak pclose = _pclose
	#pragma weak popen = _popen
#endif

#ifdef _REENTRANT
#define FORK fork1
#else
#define FORK fork
#endif

FILE *
#ifdef __STDC__
popen(const char *cmd, const char *mode)
#else
popen(cmd, mode)const char *cmd, *mode;
#endif
{
	int fd[2], writing;
	pid_t pid;
	FILE *fp;
	BFILE *bp;

	if (pipe(fd) < 0)
		return 0;
	writing = (mode[0] != 'r');
	switch (pid = FORK())
	{
	case -1:	/* fork failed */
		return 0;
	case 0:		/* child */
		bp = STDIN;	/* Close all other popen'd pipelines */
		do
		{
			if (bp->eflags & IO_POPEN)
				close(bp->fd);
		} while ((bp = bp->next) != 0);
		close(fd[writing]);
		writing ^= 1;
		if (writing != fd[writing])
		{
			close(writing);
			fcntl(fd[writing], F_DUPFD, writing);
			close(fd[writing]);
		}
		execl(_str_shpath, _str_shname, _str_sh_arg, cmd, (char *)0);
		_exit(127);	/* POSIX.2 value */
	default:	/* parent */
		close(fd[writing ^ 1]);
		if ((fp = fdopen(fd[writing], mode)) == 0)
		{
			close(fd[writing]);
			kill(pid, SIGTERM);
		}
		else
		{
			bp = (BFILE *)fp->_base;
			STDLOCK(&bp->lock);
			bp->pid = pid;
			bp->eflags |= IO_POPEN;
			STDUNLOCK(&bp->lock);
		}
		return fp;
	}
}

int
#ifdef __STDC__
pclose(FILE *fp)
#else
pclose(fp)FILE *fp;
#endif
{
#ifndef WAITPID_ONLY
	static int vers;
	struct utsname buf;
#endif
	BFILE *bp = (BFILE *)fp->_base;
	pid_t pid;
	int status;

	pid = -1;
	if (bp->eflags & IO_POPEN)
		pid = bp->pid;
	fclose(fp);		/* close stream even if not popen'd */
	if (pid == -1)
		return -1;
#ifndef WAITPID_ONLY
again:;
	if (vers > 1)	/* use waitpid() as available */
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
	else if (vers == 1)	/* older system: can't use waitpid() */
	{
		int w;

		sighold(SIGINT);
		sighold(SIGQUIT);
		sighold(SIGHUP);
		while ((w = wait(&status)) != pid)
		{
			if (w == -1 && errno != EINTR)
			{
				status = -1;
				break;
			}
		}
		sigrelse(SIGHUP);
		sigrelse(SIGQUIT);
		sigrelse(SIGINT);
	}
	else	/* first time called */
	{
		vers = 1;
		if (uname(&buf) > 0)
			vers = 2;	/* at least SVR4 system */
		goto again;
	}
#endif
	return status;
}
