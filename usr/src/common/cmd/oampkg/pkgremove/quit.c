/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgremove/quit.c	1.5.6.8"
#ident  "$Header: quit.c 1.2 91/06/27 $"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <limits.h>
#include <pkglocs.h>
#include "install.h"

#include <pfmt.h>

extern struct admin
		adm;
extern int	started,
		failflag,
		warnflag,
		reboot, ireboot;
extern char	*prog,
		*msgtext,
		*pkginst;

extern void	*calloc(),
		exit(),
		logerr(),
		ptext();

static char	*reason();

void		trap(), quit();
static void	mailmsg(), quitmsg();

void
trap(signo)
int signo;
{
	if((signo == SIGINT) || (signo == SIGHUP))
		quit(3);
	else
		quit(1);
}

void
quit(retcode)
int	retcode;
{
	char path[PATH_MAX];

	(void) signal(SIGINT, SIG_IGN);

	if(retcode != 99) {
		if((retcode % 10) == 0) {
			if(failflag)
				retcode += 1;
			else if(warnflag)
				retcode += 2;
		}

		if(ireboot)
			retcode = (retcode % 10) + 20;
		if(reboot)
			retcode = (retcode % 10) + 10;
	}

	/* remove the .lockfile file if it exists */
	(void) sprintf(path, "%s/.lockfile", PKGADM);
	if(access(path, 0) == 0)
		(void) unlink(path);

	/* send mail to appropriate user list */
	mailmsg(retcode);

	/* display message about this installation */
	quitmsg(retcode);
	exit(retcode);
	/*NOTREACHED*/
}

static void
quitmsg(retcode)
int	retcode;
{
	char	*status;

	status = reason(retcode);

	(void) putc('\n', stderr);
	ptext(stderr, gettxt(":558", "Removal of <%s> %s."), pkginst, status);

	if(retcode && !started)
		ptext(stderr, gettxt(":427", "No changes were made to the system."));
}

static void
mailmsg(retcode)
int retcode;
{
	struct utsname utsbuf;
	FILE	*pp;
	char	*status, *cmd;
	extern	char *getenv();

	if(!started || (adm.mail == NULL))
		return;

	cmd = calloc(strlen(adm.mail) + sizeof(MAILCMD) + 2, sizeof(char));
	if(cmd == NULL) {
		logerr(":204:WARNING:unable to send e-mail notification");
		return;
	}

	(void) sprintf(cmd, "%s %s", MAILCMD, adm.mail);
	if((pp = popen(cmd, "w")) == NULL) {
		logerr(":204:WARNING:unable to send e-mail notification");
		return;
	}

	if(msgtext)
		ptext(pp, msgtext);

	(void) strcpy(utsbuf.nodename, gettxt(":113", "(unknown)"));
	(void) uname(&utsbuf);
	status = reason(retcode);
	ptext(pp, gettxt(":791", "\nRemoval of %s as package instance <%s> on %s %s."),
		getenv("NAME"), pkginst, utsbuf.nodename, status);

	if(pclose(pp)) 
		logerr(":207:WARNING:e-mail notification may have failed");
}

static char *
reason(retcode)
{
	char	*status;

	switch(retcode) {
	  case  0:
	  case 10:
	  case 20:
		status = gettxt(":114", "was successful");
		break;

	  case  1:
	  case 11:
	  case 21:
		status = gettxt(":115", "failed");
		break;

	  case  2:
	  case 12:
	  case 22:
		status = gettxt(":116", "partially failed");
		break;

	  case  3:
	  case 13:
	  case 23:
		status = gettxt(":117", "was terminated due to user request");
		break;

	  case  4:
	  case 14:
	  case 24:
		status = gettxt(":118", "was suspended (administration)");
		break;

	  case  5:
	  case 15:
	  case 25:
		status = gettxt(":119", "was suspended (interaction required)");
		break;

	  case 99:
		status = gettxt(":120", "failed (internal error)");
		break;

	  default:
		status = gettxt(":121", "failed with an unrecognized error code.");
		break;
	}

	return(status);
}
