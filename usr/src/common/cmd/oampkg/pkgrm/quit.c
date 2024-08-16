/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/quit.c	1.7.9.6"
#ident  "$Header: quit.c 1.2 91/06/27 $"

#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include "install.h"

#include <pfmt.h>

extern struct pkgdev
		pkgdev;
extern int	intfchg,
		npkgs,
		failflag,
		admnflag,
		nullflag,
		intrflag,
		warnflag,
		reboot, ireboot;

extern int	setpreremove;
extern pid_t	pid;
extern char	pre_path[PATH_MAX];

extern void	ckreturn(),
		intf_reloc(),
		ptext(),
		echo(),
		exit();
extern int	chdir(),
		pkgumount();

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
	char t_contents[PATH_MAX];
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
			retcode += 20;
		if(reboot)
			retcode += 10;
	}

	/* remove the temp contents file if it exists */
	(void) sprintf(t_contents, "%s/t.contents", PKGADM);
	if(access(t_contents, 0) == 0) 
		(void) unlink(t_contents);
	/*
	 * Just in case something went wrong, if we executed any set
	 * preremove script, we rename the script to its original name.
	 */
	if (setpreremove) {
		if (pre_path[0]) {
			(void)sprintf(t_contents, "%s.%d", pre_path, pid);
			if (access(t_contents, 0) == 0) {
				(void)rename(t_contents, pre_path);
			}
		}
		/*
		 * Remove the .lockfile if it exists.
		 */
		(void)sprintf(pre_path, "%s/.lockfile", PKGADM);
		(void)unlink(pre_path);
	}

	if(reboot || ireboot)
		ptext(stderr, MSG_REBOOT_R);

	if(pkgdev.mount) {
		(void) chdir("/");
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
