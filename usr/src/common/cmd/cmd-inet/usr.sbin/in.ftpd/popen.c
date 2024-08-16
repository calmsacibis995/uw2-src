/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.ftpd/popen.c	1.6.9.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *	Copyrighted as an unpublished work.
 *      (c) Copyright 1990 INTERACTIVE Systems Corporation
 *      All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef lint
static char sccsid[] = "@(#)popen.c	5.8 (Berkeley) 6/1/90";
#endif /* not lint */

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include "../security.h"

#define bzero(s,n)	memset((s), 0, (n))

#include <sys/resource.h>

#define NOFILES 20      /* just in case */

static int
getdtablesize()
{
	struct rlimit   rl;
	
	if ( getrlimit(RLIMIT_NOFILE, &rl) == 0 )
		return(rl.rlim_max);
	else
		return(NOFILES);
}

/*
 * Special version of popen which avoids call to shell.  This insures noone
 * may create a pipe to a hidden program as a side effect of a list or dir
 * command.
 */
static int *pids;
static int fds;

FILE *
ftpd_popen(program, type)
	char *program, *type;
{
	register char *cp;
	FILE *iop;
	int argc, gargc, pdes[2], pid;
	char **pop, *argv[100], *gargv[1000], *vv[2];
	extern char **glob(), **copyblk(), *strtok();
	extern void *malloc();

	if (*type != 'r' && *type != 'w' || type[1])
		return(NULL);

	if (!pids) {
		if ((fds = getdtablesize()) <= 0)
			return(NULL);
		if ((pids = (int *)malloc((u_int)(fds * sizeof(int)))) == NULL)
			return(NULL);
		bzero((char *)pids, fds * sizeof(int));
	}
	if (pipe(pdes) < 0)
		return(NULL);

	/* break up string into pieces */
	for (argc = 0, cp = program;; cp = NULL)
		if (!(argv[argc++] = strtok(cp, " \t\n")))
			break;

	/* glob each piece */
	gargv[0] = argv[0];
	for (gargc = argc = 1; argv[argc]; argc++) {
		if (!(pop = glob(argv[argc]))) {	/* globbing failed */
			vv[0] = argv[argc];
			vv[1] = NULL;
			pop = copyblk(vv);
		}
		argv[argc] = (char *)pop;		/* save to free later */
		while (*pop && gargc < 1000)
			gargv[gargc++] = *pop++;
	}
	gargv[gargc] = NULL;

	iop = NULL;
	switch(pid = fork()) {
	case -1:			/* error */
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		goto pfree;
		/* NOTREACHED */
	case 0:				/* child */
		if (*type == 'r') {
			if (pdes[1] != 1) {
				dup2(pdes[1], 1);
				dup2(pdes[1], 2);	/* stderr, too! */
				(void)close(pdes[1]);
			}
			(void)close(pdes[0]);
		} else {
			if (pdes[0] != 0) {
				dup2(pdes[0], 0);
				(void)close(pdes[0]);
			}
			(void)close(pdes[1]);
		}
		CLR_MAXPRIVS_FOR_EXEC execv(gargv[0], gargv);
		_exit(1);
	}
	/* parent; assume fdopen can't fail...  */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
		(void)close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], type);
		(void)close(pdes[0]);
	}
	pids[fileno(iop)] = pid;

pfree:	for (argc = 1; argv[argc] != NULL; argc++) {
		/*blkfree((char **)argv[argc]);*/
		free((char *)argv[argc]);
	}
	return(iop);
}

ftpd_pclose(iop)
	FILE *iop;
{
	register int fdes;
	sigset_t set, oset;
	int stat_loc;
	pid_t pid;

	/*
	 * pclose returns -1 if stream is not associated with a
	 * `popened' command, or, if already `pclosed'.
	 */
	if (pids == 0 || pids[fdes = fileno(iop)] == 0)
		return(-1);
	(void)fclose(iop);
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGHUP);
	(void) sigprocmask(SIG_BLOCK, &set, &oset);
	while ((pid = wait(&stat_loc)) != pids[fdes] && pid != (pid_t) -1);
	(void) sigprocmask(SIG_SETMASK, &oset, (sigset_t *) 0);
	pids[fdes] = 0;
	return(pid == (pid_t) -1 ? -1 : WEXITSTATUS(stat_loc));
}
