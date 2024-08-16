/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/check.c	1.2"
#ident  "$Header: $"
#include <stdio.h>
#include <limits.h>
#include <langinfo.h>
#include <string.h>
#include <utmp.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include "install.h"

#include <pfmt.h>

extern struct admin adm;
extern int	nointeract;
extern char	pkgloc[],
		*pkginst,
		*msgtext;

extern struct utmp *getutid();
extern void	free(),
		progerr(), ptext(),
		echo(), quit();
extern int	dockdeps(), ckyorn(), ckyorn_match();
extern char	*qstrdup(), *getenv();

#define ERR_RUNSTATE	":300:unable to determine current run-state"

#define MSG_DEPEND	gettxt(":306", "Dependency checking failed.")
#define HLP_DEPEND \
	gettxt(":530", "Other packages currently installed on the system have indicated 	a dependency on the package being removed.  If removal of this 	package occurs, it may render other packages inoperative.  If you 	wish to disregard this dependency, answer 'y' to continue the 	package removal process.")

#define MSG_RUNLEVEL \
	gettxt(":531", "\\nThe current run-level of this machine is <%s>, 	which is not a run-level suggested for removal of 	this package.  Suggested run-levels (in order of preference) include:")
#define HLP_RUNLEVEL \
	gettxt(":532", "If this package is not removed in a run-level which has been 	suggested, it is possible that the package may not remove 	properly.  If you wish to follow the run-level suggestions, 	answer 'n' to stop the package removal process.")
#define MSG_STATECHG \
	gettxt(":533", "\\nTo change states, execute\\n\\tshutdown -y -i%s -g0\\n	after exiting the package removal process. 	Please note that after changing states you 	may have to mount appropriate filesystem(s) 	in order to remove this package.")

#define MSG_PRIV \
	gettxt(":534", "\\nThis package contains scripts which may have a security 	impact and which will be executed during the process of removing 	this package.")
#define HLP_PRIV \
	gettxt(":535", "During the removal of this package, certain scripts provided 	with the package will be executed. These scripts may modify or 	otherwise change your system without your knowledge.  If you are 	certain of the origin and trustworthiness of the package being 	removed, answer 'y' to continue the removal process.")

#define ASK_CONTINUE \
	gettxt(":536", "Do you want to continue with the removal of this package")

void
rckrunlevel()
{
	struct utmp utmp;
	struct utmp *putmp;
	char	ans[INPBUF], *pt, *rstates, *pstate;
	int	n;
	char	*uxstate;

	if(ADM(runlevel, "nocheck"))
		return;

	pt = getenv("RSTATES");
	if(pt == NULL)
		return;

	utmp.ut_type = RUN_LVL;
	putmp = getutid(&utmp);
	if(putmp == NULL) {
		progerr(ERR_RUNSTATE);
		quit(99);
	}
	uxstate = strtok(&putmp->ut_line[10], " \t\n");
	
	rstates = qstrdup(pt);
	if((pt = strtok(pt, " \t\n,")) == NULL)
		return; /* no list is no list */
	pstate = pt;
	do {
		if(!strcmp(pt, uxstate)) {
			free(rstates);
			return;
		}
	} while(pt = strtok(NULL, " \t\n,"));

	msgtext = MSG_RUNLEVEL;
	ptext(stderr, msgtext, uxstate);
	pt = strtok(rstates, " \t\n,");
	do {
		ptext(stderr, "\\t%s", pt);
	} while(pt = strtok(NULL, " \t\n,"));
	free(rstates);
	if(ADM(runlevel, "quit"))
		quit(4);
	if(nointeract)
		quit(5);
	msgtext = NULL;

	if(n = ckyorn(ans, NULL, NULL, HLP_RUNLEVEL, ASK_CONTINUE))
		quit(n);

	if (ckyorn_match(ans, nl_langinfo(NOEXPR))) {
		ptext(stderr, MSG_STATECHG, pstate);
		quit(3);
	}
}

void
rckdepend()
{
	int	n;
	char	ans[INPBUF];
	extern char *setinst;

	if(ADM(rdepend, "nocheck"))
		return;

	echo(":792:## Verifying <%s> set package dependencies.", pkginst);
	if(dockdeps(pkginst, 1)) {
		msgtext = MSG_DEPEND;
		echo(msgtext);
		if(ADM(rdepend, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		if(n = ckyorn(ans, NULL, NULL, HLP_DEPEND, ASK_CONTINUE))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
	}
}

void
rckpriv()
{
	struct dirent *dp;
	DIR	*dirfp;
	int	n, found;
	char	ans[INPBUF], path[PATH_MAX];

	if(ADM(action, "nocheck"))
		return;
	
	(void) sprintf(path, "%s/install", pkgloc);
	if((dirfp = opendir(path)) == NULL) 
		return;

	found = 0;
	while((dp = readdir(dirfp)) != NULL) {
		if(!strcmp(dp->d_name, "preremove") ||
		   !strcmp(dp->d_name, "postremove") ||
		   !strncmp(dp->d_name, "r.", 2)) {
			found++;
			break;
		}
	}
	(void) closedir(dirfp);

	if(found) {
		ptext(stderr, MSG_PRIV);
		msgtext = gettxt(":319", "Package scripts were found.");
		if(ADM(action, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		if(n = ckyorn(ans, NULL, NULL, HLP_PRIV, ASK_CONTINUE))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
	}
}