/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgexecl.c	1.8.10.10"
#ident "$Header: $"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pfmt.h>
#include <pkglocs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <varargs.h>

extern char	**environ;

extern	char	*pkginst;
extern	char	*prog;

extern void	progerr();

#define MAXARGS	16
#define	MAXTRY	5

/*VARARGS*/
int
pkgexecl(va_alist)
va_dcl
{
	va_list ap;
	char	*pt, *arg[MAXARGS+1];
	char	*filein, *fileout, *filerr;
	int	n, status, upper, lower;
	pid_t	pid;
	void	(*func)();
	int	try = 0;

	va_start(ap);
	filein = va_arg(ap, char *);
	fileout = va_arg(ap, char *);
	filerr = va_arg(ap, char *);
	if( filerr && !strcmp(filerr, "NULL"))
		filerr = NULL;

	n = 0;
	while(pt = va_arg(ap, char *)) {
		arg[n++] = pt;
	}
	arg[n] = NULL;
	va_end(ap);

	while((pid  = vfork()) == -1) {
		if( errno == EAGAIN || errno == ENOMEM) {
			if(++try < MAXTRY) {
				(void) pfmt(stderr, MM_WARNING,
					"uxpkgtools:642:pkgexev: fork failed, pid=%d retrying fork()\n");
				(void) sleep(4);
				continue;
			}
			progerr("uxpkgtools:643:bad fork(), errno=%d", errno);
			return(-1);
		}
	}
	if(pid) {
		func = signal(SIGINT, SIG_IGN);
		n = waitpid(pid, &status, 0);
		if(n != pid) {
			progerr("uxpkgtools:644:wait for %d failed, pid=%d errno=%d", 
				pid, n, errno);
			return(-1);
		}
		upper = (status>>8) & 0177;
		lower = status & 0177;
		(void) signal(SIGINT, func);
		return(lower ? (-1) : upper);
	}
	/* CHILD */
	for (status = 3; status < 101; status++)
		close (status);

	/*
	 * If any of the passed in file args are not null, reopen stream
	 * to that file.  If filerr is set, then logmode is in effect.
	 * In this case filerr is the name of the error logfile.
	 */
	if(filein)
		(void) freopen(filein, "r", stdin);
	if(fileout)
		(void) freopen(fileout, "w", stdout);
	if(filerr) {
		(void) freopen(filerr, "a", stderr);
	}
	(void) execve(arg[0], arg, environ);
	progerr("uxpkgtools:645:exec of %s failed, errno=%d", arg[0], errno);
	exit(99);
	/*NOTREACHED*/
}
