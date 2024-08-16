/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/quit.c	1.11.8.20"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <limits.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include "install.h"

#include <pfmt.h>

extern struct admin
		adm;
struct pkgdev	pkgdev;
extern int	dparts,
		started,
		update,
		iflag,
		failflag,
		warnflag,
		reboot, ireboot;

extern char	tmpdir[],
		pkgloc[],
		uncompdir[PATH_MAX],
		*prog,
		*msgtext,
		*pkginst,
		*pkgname,
		*log,
		*quiet;

extern void	*calloc(),
		nqtext(),
		ptext(),
		logerr(),
		ds_skiptoend(),
		exit();
extern int	chdir(),
		rrmdir();

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

siginfo_t	infop;

void
quit(retcode)
int	retcode;
{
	struct	stat status;

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

	if(tmpdir[0])
		(void) rrmdir(tmpdir);

	/*
	 * If we just installed a compressed package,
	 * instvol created a directory into which it
	 * uncompresses files.  Let's remove it if
	 * it exists.
	 */
	if(stat(uncompdir, &status) == 0) 
		(void) rrmdir(uncompdir);
	
	/* remove the temp contents file if it exists */
	(void) sprintf(path, "%s/t.contents", PKGADM);
	if(access(path, 0) == 0) 
		(void) unlink(path);

	/* remove the .lockfile file if it exists */
	(void) sprintf(path, "%s/.lockfile", PKGADM);
	if(access(path, 0) == 0)
		(void) unlink(path);

	/* send mail to appropriate user list */
	mailmsg(retcode);

	/* display message about this installation */
	quitmsg(retcode);

	/* no need to umount device since calling process 
	 * was responsible for original mount
	 */

	if(!update && !started && pkgloc[0]) {
		(void) chdir("/");
		(void) rrmdir(pkgloc);
	}
	if(dparts > 0) 
		ds_skiptoend(pkgdev.cdevice);
	/* 
	 * If quit() was called because a Set Installation Package's request
	 * request script returned NOSET (no packages from set were selected
	 * for installation), quit gracefully.
	 */
	if(retcode == NOSET)
		retcode = 0;
	(void)ds_close(1);
	exit(retcode);
}

static void
quitmsg(retcode)
int	retcode;
{
	char	*status;
	extern	char *category;

	status = reason(retcode);

	if(!quiet || strcmp(quiet, "true")) {
		(void) putc('\n', stderr);
		if(iflag)
			ptext(stderr, gettxt(":424", "Processing of request script %s."), status);
		/*
		 * Do not display completion of set installation packages.
		 */
		else if(pkginst && strcmp(category, "set"))
			ptext(stderr, gettxt(":425", "Installation of %s (%s) %s."), 
				pkgname, pkginst, status);
	
		if(retcode == NOSET)
			ptext(stderr, gettxt(":426", "No packages were selected for installation from set <%s>\n"), pkgname);
		if(retcode && !started)
			ptext(stderr, gettxt(":427", "No changes were made to the system."));
	}
}

static void
mailmsg(retcode)
int retcode;
{
	struct utsname utsbuf;
	char	*status;
	extern	FILE	*mail_pp;
	extern	char	*package;
	extern	char	*pkgabrv;

	if(iflag || (adm.mail == NULL))
		return;

	if(mail_pp == NULL) {
		logerr(":428:WARNING:unable to send e-mail notification");
		return;
	}

	if(msgtext)
		nqtext(mail_pp, msgtext, pkgabrv);

	(void) strcpy(utsbuf.nodename, gettxt(":113", "(unknown)"));
	(void) uname(&utsbuf);
	status = reason(retcode);
	nqtext(mail_pp, gettxt(":429", "\nInstallation of %s on %s as package instance <%s> %s.\n"),
			pkgname, utsbuf.nodename, (pkginst ? pkginst : package), status);
	switch(retcode) {
	   case  1:
	   case  2:
	   case  5:
	   case 11:
	   case 12:
	   case 15:
	   case 21:
	   case 22:
	   case 25:
	   case 99:
		nqtext(mail_pp, gettxt(":206", "\nError messages are logged in /var/sadm/install/logs/%s.log.\n"),
			(pkginst ? pkginst : package));
		break;
	}
	if(pclose(mail_pp)) 
		logerr(":207:WARNING:e-mail notification may have failed");
	mail_pp = NULL;
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
	  case 33:
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

	  case NOSET:
		status = gettxt(":430", "was terminated - no set packages selected for installation");
		break;

	  case 99:
		if(started)
			status = gettxt(":431", "failed (internal error) - package partially installed");
		else
			status = gettxt(":120", "failed (internal error)");
		break;

	  default:
		status = gettxt(":121", "failed with an unrecognized error code.");
		break;
	}

	return(status);
}
