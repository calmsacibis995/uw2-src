/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgexecv.c	1.8.8.7"
#ident "$Header: $"

#include <errno.h>
#include <pfmt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define	MAXTRY 5

extern char	**environ;

extern int	ds_close();
extern void	progerr();

extern int ds_curpartcnt;
/*VARARGS*/
int
pkgexecv(filein, fileout, arg)
char	*filein, *fileout;
char	*arg[];
{
	int	n, status, upper, lower;
	pid_t	pid;
	void	(*func)();
	int	try = 0;

	while((pid = vfork()) == -1) {
		if(errno == EAGAIN || errno == ENOMEM) {
			if(++try < MAXTRY) {
				(void) pfmt(stderr, MM_WARNING,
					"uxpkgtools:642:pkgexev: fork failed, pid=%d retrying fork()\n", pid);
				(void) sleep(4);
				continue;
			}
			progerr("uxpkgtools:643:bad fork(), errno=%d", errno);
			return(-1);
		}
	}
	if(pid) {
		/* parent */
		func = signal(SIGINT, SIG_IGN);
		if(ds_curpartcnt >= 0) {
			if(n = ds_close(0))
				return -1;
		}
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
	/* child */
	if(filein)
		(void) freopen(filein, "r", stdin);
	if(fileout) {
		(void) freopen(fileout, "a", stdout);
		(void) freopen(fileout, "a", stderr);
	}

	(void) execve(arg[0], arg, environ);
	progerr("uxpkgtools:645:exec of %s failed, errno=%d", arg[0], errno);
	exit(99);
	/*NOTREACHED*/
}
