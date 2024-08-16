/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/presvr4.c	1.8.8.4"
#ident  "$Header: $"

#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <valtools.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include "install.h"

extern struct admin
		adm;
extern struct pkgdev
		pkgdev;
extern char	*tmpdir;
extern int	nointeract,
		started;
extern void	psvr4cnflct(), psvr4pkg(), psvr4mail();

extern void	(*func)(),
		echo(),
		progerr(),
		logerr(),
		quit();
extern int	unlink(),
		chdir(),
		chmod(),
		devtype(),
		copyf(),
		pkgmount(),
		pkgumount(),
		pkgexecl(),
		ckstr();

int	intfchg = 0;

#define PATH_FLAGS	P_EXIST|P_ABSOLUTE|P_BLK

#define MSG_DEVICE \
":578:Removal of a pre-SVR4 package requires the original medium from which the package was installed." 

#define ASK_DEVICE \
gettxt(":579", "Enter the alias or pathname for the device to be used (e.g., diskette1 or /dev/diskette)")

#define ASK_INSERT \
gettxt(":580", "Insert the first volume for package <%s> into %s")

#define ERR_NOCOPY \
":188:unable to create copy of UNINSTALL script in <%s>"

#define ERR_NOINT \
":189:-n option cannot be used when removing pre-SVR4 packages" 

#define ERR_BADDEV \
":581:Unknown or bad device <%s> specified"

#define MSG_MAIL \
gettxt(":582", "An attempt to remove the <%s> pre-SVR4 package on <%s> completed with exit status <%d>.")

presvr4(pkg)
char	*pkg;
{
	char	alias[PATH_MAX];
	char	path[PATH_MAX];
	char	*tmpcmd;
	int	n, retcode;
	void	(*tmpfunc)();

	echo(":583:*** Removing Pre-SVR4 Package ***");
	if(nointeract) {
		progerr(ERR_NOINT);
		quit(1);
	}

	/* should accept device alias?? */

	echo(MSG_DEVICE);
	for(;;) {
		if(n = ckstr(alias, NULL, PATH_MAX, NULL, NULL, NULL, 
		ASK_DEVICE))
			return(n);
	
		if(devtype(alias, &pkgdev))
			continue;
		if(!pkgdev.mount || !pkgdev.bdevice) {
			logerr(ERR_BADDEV, alias);
			continue;
		}
		break;
	}
	pkgdev.mount = pkgdev.dirname = "/install";
	pkgdev.rdonly = 1;

	if(n = pkgmount(&pkgdev, pkg, 1, 0, 1))
		quit(n);

	psvr4pkg(&pkg);

	/*
	 * check to see if we can guess (via Rlist) what 
	 * pathnames this package is likely to remove;
	 * if we can, check these against the 'contents'
	 * file and warn the administrator that these
	 * pathnames might be modified in some manner
	 */
	psvr4cnflct(1);

	if(chdir(tmpdir)) {
		progerr(":178:unable to change directory to <%s>", tmpdir);
		quit(99);
	}

	(void) sprintf(path, "%s/install/UNINSTALL", "/install");
	tmpcmd = tempnam(tmpdir, "UNINSTALL");
	if(!tmpcmd || copyf(path, tmpcmd, 0) || chmod(tmpcmd, 0500)) {
		progerr(ERR_NOCOPY, tmpdir);
		quit(99);
	}

	started++;

	echo(":584:## Executing package UNINSTALL script");
	tmpfunc = signal(SIGINT, func);
	retcode = pkgexecl(NULL, NULL, NULL, SHELL, "-c", tmpcmd, NULL);
	(void) signal(SIGINT, SIG_IGN);
	(void) unlink(tmpcmd);
	if(retcode) 
		echo(":585:\nPre-SVR4 package reported failed removal.\n");
	else
		echo(":586:\nPre-SVR4 package reported successful removal.\n");

	psvr4mail(adm.mail, MSG_MAIL, retcode, pkg);
	(void) pkgumount(&pkgdev);
	(void) signal(SIGINT, tmpfunc);

	intfchg++;
	return(retcode);
}

void
intf_reloc()
{
	char	path[PATH_MAX];

	(void) sprintf(path, "%s/intf_reloc", PKGBIN);
	(void) pkgexecl(NULL, NULL, NULL, SHELL, "-c", path, NULL);
}
