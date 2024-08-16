/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/cosh.c	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)cosh.c	1.2 'attmail mail(1) command'"
/* from ferret SCCSid cosh.c 3.1 */
#include "../mail/libmail.h"
#include "mailproc.h"

static	int	System ARGS((char*));
static	int	dumb_system ARGS((char*));
static	char	*findsh ARGS((void));

/*  === Initialization script ===
	if alias @='_mailproc_ exit'
	  then
		exec %d>&%d %d>&%d %d>&-
		function _mailproc_
		{
			print -nu%d - "$@"
		}
		typeset -xf _mailproc_
	  else
		exit 1
	fi
*/
static char INIT[] = "if alias @='_mailproc_ exit'\n  then\n\texec %d>&%d %d>&%d %d>&-\n\tfunction _mailproc_\n\t{\n\t\tprint -nu%d - \"$@\"\n\t}\n\ttypeset -xf _mailproc_\n  else\n\texit 1\nfi\n%s";

static struct	shpath
{
	char	envar[16];			/* environment variable */
	char	rpath[512];			/* relative path */
} ksh_ar[] =
{
	{"", 		"/usr/bin/ksh"},
	{"",		"/bin/ksh"},
	{"",		"/sbin/ksh"},
	{"",		"/usr/lbin/ksh"},
	{"TOOLS",	"/bin/ksh"},
	{"",		""},
};

static int	shpid = 0;			/* shell pid */
static char	*shell = NULL;			/* shell name */
static int	(*sysfn)() = NULL;		/* system function */

static int System(cmd)
char *cmd;
{
	int		exitcode;		/* command exit code */
	int		len;			/* command length */
	int		ret;			/* return value */
	char		cbuf[1024];		/* command buffer */
	char		*p;			/* character pointer */
	static	int	cmdp[2];		/* command pipe */
	static	int	datap[2];		/* data pipe */

	if (!shpid)
	{
		if (pipe(datap) < 0)
			return -1;
		if (pipe(cmdp) < 0)
		{
			(void) close(datap[0]);
			(void) close(datap[1]);
			return -1;
		}

		switch(shpid = fork())
		{
			case -1:
				return -1;
			case 0:			/* child action */
				/* attach read end of cmdp to fd 0 */
				(void) close(cmdp[1]);
				(void) close(0);
				(void) dup(cmdp[0]);
				(void) close(cmdp[0]);
				(void) close(datap[0]);
				(void) execl(shell, "Fksh", 0);
				exit(1);
			default:		/* parent action */
				(void) close(cmdp[0]);
				(void) close(datap[1]);
				(void) sprintf(cbuf, INIT, datap[1]-1,
					datap[1], datap[1],
					datap[1]-1, datap[1]-1, datap[1],
					oflg ? "" : "exec >/dev/null 2>&1\n");
				if (write(cmdp[1], cbuf, strlen(cbuf)) < 0)
					return -1;
		}

	}

	(void) sprintf(cbuf, "%s ; _mailproc_ exit $?\n", cmd);
	len = strlen(cbuf);
	if (write(cmdp[1], cbuf, len) <= 0)
		return -1;

	if ((ret = read(datap[0], cbuf, sizeof(cbuf)-1)) <= 0)
		return -1;

	cbuf[ret] = '\0';
	if (dflg)
		(void)pfmt(stderr, MM_INFO, ":566:Coshell returned %s\n", cbuf);
	p = cbuf + strspn(cbuf, " \t");
	p = strpbrk(p, " \t\n");
	p += strspn(p, " \t");
	exitcode = atoi(p);

	return exitcode;
}

static char *findsh()
{
	static	char	shbuf[256];
	char		*shptr;
	struct	shpath	*shp;

	for(shp = ksh_ar; *shp->rpath; shp++)
	{
		if (*shp->envar)
		{
			(void) sprintf(shbuf, "%s%s", getenv(shp->envar),
						shp->rpath);
			shptr = shbuf;
		}
		else
			shptr = shp->rpath;

		if (access(shptr, 1) == 0)
			return shptr;
	}

	return NULL;
}


static int dumb_system(s)
char *s;
{
	char		cbuf[1024];
	char		*p;

	p = (oflg ? "exec 2>&1" : "exec 1>/dev/null 2>&1");
	(void) sprintf(cbuf, "%s ; %s", p, s);

	return system(cbuf);
}

/*
    front end to system(3) and coshell version
*/
int do_system(s)
char *s;
{
	if (!sysfn)
	{
		if ((shell = findsh()) == NULL)
			sysfn = dumb_system;
		else
			sysfn = System;
	}

	return (*sysfn)(s);
}
