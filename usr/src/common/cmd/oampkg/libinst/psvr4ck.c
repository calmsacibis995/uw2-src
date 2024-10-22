/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/psvr4ck.c	1.7.10.7"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <langinfo.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <pfmt.h>
#include "install.h"

extern void	progerr(),
		quit(),
		ptext(),
		echo();
extern int	access(),
		ckyorn(),
		ckyorn_match();
extern char	*qstrdup();

#define INPBUF		128
#define ERR_MAIL	"uxpkgtools:99:unable to send electronic mail notification"
#define ERR_OVERWRITE	"uxpkgtools:100:unable to determine overwrite list"
#define ERR_PIPE	"uxpkgtools:101:unable to open pipe to process <%s>"
#define ASK_CONT	gettxt("uxpkgtools:102", \
			"Do you want to continue processing this package")
#define MSG_ICONFLICT \
	gettxt("uxpkgtools:103", "The following files are currently being used by other 	packages on the system, and may be overwritten by the 	installation of this pre-SVR4 package:")
#define MSG_RCONFLICT \
	gettxt("uxpkgtools:104", "The following files are currently being used by other 	packages on the system, and may be removed by the 	removal of this pre-SVR4 package:")
#define HLP_ICONFLICT \
	gettxt("uxpkgtools:105", "If you choose to continue installation, it is possible that you 	will overwrite files which are part of another package that is 	already installed on the system.  If you want to assure that the 	files are not overwritten, answer 'n' to stop the installation 	process.")
#define HLP_RCONFLICT \
	gettxt("uxpkgtools:106", "If you choose to continue removal, it is possible that you 	will remove files which are part of another package that is 	already installed on the system.  If you want to assure that the 	files are not removed, answer 'n' to stop the installation 	process.")
#define MSG_NOTVER \
	gettxt("uxpkgtools:107", "The media being processed is in an old (pre-SVR4) 	format and it is not possible to verify that 	the inserted media belongs to the <%s> package.")
#define HLP_NOTVER \
	gettxt("uxpkgtools:108", "If you choose to continue installation, it is possible that you 	will install the wrong package.  If you are sure the media being 	installed contains the package you wish to install, answer 'y' 	to continue the installation process.")
#define MSG_CONFIRM \
	gettxt("uxpkgtools:109", "The media being processed is in an old (pre-SVR4) 	format and appears to be part of the <%s> package.")
#define HLP_CONFIRM \
	gettxt("uxpkgtools:110", "The installation of older-style (pre-SVR4) packages is, in general, 	not as robust as installing standard packages.  Older packages may 	attempt things during installation which overwrite existing files 	or otherwise modify the system without your approval.  If you wish 	to allow installation of identified pre-SVR4 package, answer 'y' to 	continue the installation process.")

static char	*Rlist[] = {
	"/install/install/Rlist",
	"/install/install/RLIST",
	"/install/install/rlist",
	NULL
};
static char	ckcmd[] = "/usr/sbin/pkgchk -L -i %s";

void
psvr4pkg(ppkg)
char	**ppkg;
{
	struct dirent *drp;
	DIR	*dirfp;
	char	*pt;
	int	n;
	char	ans[INPBUF], path[PATH_MAX];

	if(*ppkg) {
		(void) sprintf(path, "/install/new/usr/options/%s.name", *ppkg);
		if(access(path, 0)) {
			ptext(stderr, MSG_NOTVER, *ppkg);
			if(n = ckyorn(ans, NULL, NULL, HLP_NOTVER, ASK_CONT))
				quit(n);
			if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
				quit(3);
		}
		return;
	}

	if(dirfp = opendir("/install/new/usr/options")) {
		while(drp = readdir(dirfp)) {
			if(drp->d_name[0] == '.')
				continue;
			if(pt = strchr(drp->d_name, '.')) {
				if(!strcmp(pt, ".name")) {
					*pt = '\0';
					*ppkg = qstrdup(drp->d_name);
					break;
				}
			}
		}
		(void) closedir(dirfp);
	}

	if(*ppkg) {
		ptext(stderr, MSG_CONFIRM, *ppkg);
		if(n = ckyorn(ans, NULL, NULL, HLP_CONFIRM, ASK_CONT))
			quit(n);
	} else {
		ptext(stderr, MSG_NOTVER, *ppkg);
		if(n = ckyorn(ans, NULL, NULL, HLP_NOTVER, ASK_CONT))
			quit(n);
	}
	if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
		quit(3);
}

void
psvr4cnflct(rflag)
ushort	rflag;
{
	FILE	*pp;
	int	n, found;
	char	*pt,
		ans[INPBUF],
		cmd[PATH_MAX+sizeof(ckcmd)],
		path[PATH_MAX];

	for(n=0; Rlist[n] != NULL; n++) {
		if(access(Rlist[n], 0) == 0)
			break;
	}
	if(Rlist[n] == NULL)
		return; /* Rlist file not found on device */

	(void) sprintf(cmd, ckcmd, Rlist[n]);
	echo("uxpkgtools:111:## Checking for conflicts with installed packages");
	echo("uxpkgtools:112:   (using %s provided by pre-SVR4 package)", Rlist[n]);
	if((pp = popen(cmd, "r")) == NULL) {
		progerr(ERR_PIPE, cmd);
		progerr(ERR_OVERWRITE);
		quit(99);
	}

	found = 0;
	while(fgets(path, PATH_MAX, pp)) {
		if(!found++) {
			if(rflag)
				ptext(stderr, MSG_RCONFLICT);
			else
				ptext(stderr, MSG_ICONFLICT);
		}
		if(pt = strpbrk(path, " \t\n"))
			*pt = '\0';
		echo("\t%s", path);
	}
	if(pclose(pp)) {
		progerr(ERR_OVERWRITE);
		quit(99);
	}

	if(found) {
		if(rflag) {
			if(n = ckyorn(ans, NULL, NULL, HLP_RCONFLICT, ASK_CONT))
				quit(n);
		} else {
			if(n = ckyorn(ans, NULL, NULL, HLP_ICONFLICT, ASK_CONT))
				quit(n);
		}
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
	}
}

void
psvr4mail(list, msg, retcode, pkg)
char	*list, *msg;
int	retcode;
char	*pkg;
{
	struct utsname utsbuf;
	FILE	*pp;
	char	cmd[BUFSIZ];

	if(list == NULL)
		return;
		
	while(isspace(*list))
		list++;
	if(*list == '\0')
		return;

	/* send e-mail notifications */
	(void) sprintf(cmd, "%s %s", MAILCMD, list);
	if((pp = popen(cmd, "w")) == NULL) {
		progerr(ERR_PIPE, MAILCMD);
		progerr(ERR_MAIL);
		quit(99);
	}

	(void) strcpy(utsbuf.nodename, gettxt("uxpkgtools:113", "(unknown)"));
	(void) uname(&utsbuf);
	ptext(pp, msg, pkg, utsbuf.nodename, retcode);

	if(pclose(pp)) {
		progerr(ERR_MAIL);
		quit(99);
	}
}
