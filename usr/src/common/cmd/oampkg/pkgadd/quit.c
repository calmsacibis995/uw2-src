/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgadd/quit.c	1.8.12.20"
#ident  "$Header: $"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <wait.h>
#include <errno.h>
#include <pkgdev.h>
#include <limits.h>
#include "install.h"

extern struct pkgdev
		pkgdev;
extern int	intfchg,
		npkgs,
		failflag,
		warnflag,
		intrflag,
		admnflag,
		nullflag,
		reboot, ireboot;

extern void	exit(),
		echo(),
		nqtext(),
		ptext(),
		intf_reloc(),
		ckreturn();
extern int	pkgumount(),
		ds_close(),
		rrmdir(),
		chdir();

extern char	*ids_name;	

void		trap(), quit();

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
int retcode;
{
	extern	char	setrespdir[PATH_MAX];	/* temporary directory name for set response files */ 
	extern	char	setlistf[PATH_MAX];	/* file name in which packages selected for installation are listed */
	siginfo_t	infop;
	static void mailmsg();

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if(retcode != 99) {
		ckreturn(retcode);
		if(failflag)
			retcode = 1;
		else if(warnflag)
			retcode = 2;
		else if(intrflag)
			retcode = 3;
		else if(admnflag)
			retcode = 4;
		else if(nullflag)
			retcode = 5;
		else
			retcode = 0;
		if(ireboot)
			retcode = (retcode % 10) + 20;
		if(reboot)
			retcode = (retcode % 10) + 10;
	}
	if(retcode)
		mailmsg(retcode);

	if(reboot || ireboot)
		nqtext(stderr, MSG_REBOOT_I);

	(void) chdir("/");

	/*
	 * If a set response directory was set up, remove it.
	 * If set list file exists, remove it.
	 */
	if (strcmp(setrespdir, ""))
		(void) rrmdir(setrespdir);
	if (strcmp(setlistf, ""))
		(void) unlink(setlistf);

	if(ids_name) { /* datastream */
		if(pkgdev.dirname)
			(void) rrmdir(pkgdev.dirname); /* from tempnam */
		(void)ds_close(1);
	} else if (pkgdev.mount) {
		(void) fpkginst(NULL);
		(void) pkgumount(&pkgdev);
	}

	if(npkgs == 1)
		echo(":202:\n1 package was not processed!\n");
	else if(npkgs)
		echo(":203:\n%d packages were not processed!\n", npkgs);

	if(intfchg)
		intf_reloc();

	exit(retcode); 
} 


static void
mailmsg(retcode)
int retcode;
{
	FILE	*pp;
	char	*cmd, *calloc();
	extern	char	*pkginst;
	extern	struct	admin adm;

	/* If no user yet specified don't send mail */
	if(adm.mail == NULL)
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

	if(pkginst) {
		nqtext(pp, gettxt(":205", "\nInstallation of package instance <%s> failed.\n"),
			pkginst);
		nqtext(pp, gettxt(":206", "\nError messages are logged in /var/sadm/install/logs/%s.log.\n"), 
			pkginst);
	}

	if(pclose(pp)) 
		logerr(":207:WARNING:e-mail notification may have failed");
}
