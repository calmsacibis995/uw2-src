/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:mailst.c	2.7.7.3"
#ident "$Header: mailst.c 1.1 91/02/28 $"

#include "uucp.h"
#include <sys/stat.h>

extern int xfappend();

/*
 * fork and execute a mail command sending 
 * string (str) to user (user).
 * If file is non-null, the file is also sent.
 * (this is used for mail returned to sender.)
 *	user	 -> user to send mail to
 *	str	 -> string mailed to user
 *	infile	 -> optional stdin mailed to user
 *	errfile	 -> optional stderr mailed to user
 */
void
mailst(user, str, infile, errfile)
char *user, *str, *infile, *errfile;
{
	register FILE *fp, *fi;
	char cmd[BUFSIZ];
	char *c;

	/* get rid of some stuff that could be dangerous */
	if ( (c = strpbrk(user, ";&|<>^`\\('\"{}\n")) != NULL) {
		*c = NULLCHAR;
	}

	(void) sprintf(cmd, "%s mail '%s'", PATH, user);
	if ((fp = popen(cmd, "w")) == NULL)
		return;
	(void) fprintf(fp, "\n%s\n", str);

	/* copy back stderr */
	if (*errfile != '\0' && NOTEMPTY(errfile) && (fi = fopen(errfile, "r")) != NULL) {
		fputs("\n\t===== stderr was =====\n", fp);
		if (xfappend(fi, fp) != SUCCESS)
			fputs("\n\t===== well, i tried =====\n", fp);
		(void) fclose(fi);
		fputc('\n', fp);
	}

	/* copy back stdin */
 	if (*infile) {
 		fputs("\n\t===== stdin was ", fp);
 		if (!NOTEMPTY(infile))
 			fputs("empty =====\n", fp);
 		else if (chkpth(infile, CK_READ) || !READANY(infile)) {
 			fputs("denied read permission =====\n", fp);
			sprintf(cmd, "user %s, stdin %s", user, infile);
			logent(cmd, "DENIED");
		}
 		else if ((fi = fopen(infile, "r")) == NULL) {
 			fputs("unreadable =====\n", fp);
			sprintf(cmd, "user %s, stdin %s", user, infile);
			logent(cmd, "UNREADABLE");
		}
 		else {
 			fputs("=====\n", fp);
 			if (xfappend(fi, fp) != SUCCESS)
 				fputs("\n\t===== well, i tried =====\n", fp);
 			(void) fclose(fi);
 		}
		fputc('\n', fp);
	}

	(void) pclose(fp);
	return;
}
static char un[2*NAMESIZE];
void
setuucp(p)
char *p;
{
   char **envp;

    envp = Env;
    for ( ; *envp; envp++) {
	if(PREFIX("LOGNAME", *envp)) {
	    (void) sprintf(un, "LOGNAME=%s",p);
	    envp[0] = &un[0];
	}
    }
   return;
}
