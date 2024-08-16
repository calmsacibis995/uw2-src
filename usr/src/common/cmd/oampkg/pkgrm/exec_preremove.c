/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/exec_preremove.c	1.1"
#ident  "$Header: $"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <pkglocs.h>
#include "install.h"

#include <locale.h>
#include <pfmt.h>

#define	DEFPATH		"/sbin:/usr/sbin:/usr/bin"
#define	ERR_PKGINFO	":463:unable to process pkginfo in <%s>"

/*
 * The following variables are declared global because of their use
 * in rckdepend(), rckpriv(), and rckrunlevel() routines of check.c.
 */
char *msgtext;			/* Global messages buffer. */
char pkgloc[PATH_MAX];		/* Global location of package. */

static char pkgbin[PATH_MAX];
extern char **environ;

extern void putparam();
extern void rckdepend();
extern void rckpriv();
extern void rckrunlevel();
extern char *fpkgparam();

exec_preremove(pkginst)
char *pkginst;
{
	FILE *fp;
	char	*value;
	char	script[PATH_MAX],
		path[PATH_MAX],
		param[64];
	char	*lang,
		*lc_messages,
		*lc_collate,
		*lc_time,
		*lc_numeric,
		*lc_ctype,
		*lc_monetary;
	void	*msg_locale;
	void *xptr;
		
	(void) sprintf(pkgloc, "%s/%s", PKGLOC, pkginst);
	(void) sprintf(pkgbin, "%s/install", pkgloc);

	/*
	 * Copy the locale environment variables
	 */
	lang = DUPENV(xptr,"LANG");
	lc_messages = DUPENV(xptr,"LC_MESSAGES");
	lc_collate = DUPENV(xptr,"LC_COLLATE");
	lc_time = DUPENV(xptr,"LC_TIME");
	lc_numeric = DUPENV(xptr,"LC_NUMERIC");
	lc_ctype = DUPENV(xptr,"LC_CTYPE");
	lc_monetary = DUPENV(xptr,"LC_MONETARY");

	/*
	 * process all parameters from the pkginfo file
	 * and place them in the execution environment
	 */
	msg_locale = setlocale(LC_MESSAGES, NULL);
	if (strcmp(msg_locale, "C") != 0) {
		(void) sprintf(path, "%s/inst/locale/%s/pkginfo",
					pkgloc, msg_locale);
		if (access(path, R_OK) == -1) {
			(void)sprintf(path, "%s/pkginfo", pkgloc);
		}
	} else {
		(void) sprintf(path, "%s/pkginfo", pkgloc);
	}
	if((fp = fopen(path, "r")) == NULL) {
		progerr(ERR_PKGINFO, path);
		return(1);
	}

	environ = NULL;
	msgtext = NULL;
	/*
	 * Now rebuild the environment with the variables we want
	 * to pass through...
	 */
	PUTPARAM("LANG", lang);
	PUTPARAM("LC_MESSAGES", lc_messages);
	PUTPARAM("LC_COLLATE", lc_collate);
	PUTPARAM("LC_TIME", lc_time);
	PUTPARAM("LC_NUMERIC", lc_numeric);
	PUTPARAM("LC_CTYPE", lc_ctype);
	PUTPARAM("LC_MONETARY", lc_monetary);

	param[0] = '\0';
	while (value = fpkgparam(fp, param)) {
		if(strcmp(param, "PATH") && strcmp(param, "TERM"))
			putparam(param, value);
		free(value);
		param[0] = '\0';
	}
	(void) fclose(fp);
	(void) sprintf(path, "%s:%s", DEFPATH, PKGBIN);
	putparam("PATH", path);

	/*
	 *  make sure current runlevel is appropriate
	 */
	rckrunlevel();

	/*
	 * determine if any packaging scripts provided with
	 * this package will execute as a privileged user
	 */
	rckpriv();

	/*
	 *  verify package dependencies
	 */
	rckdepend();

	/*
	 *  execute preremove script, if any
	 */
	(void) sprintf(script, "%s/preremove", pkgbin);
	if(access(script, 0) == 0) {
		echo(":793:## Executing set preremove script.");
		return(pkgexecl(NULL, NULL, NULL, SHELL, script, NULL));
	}
}
