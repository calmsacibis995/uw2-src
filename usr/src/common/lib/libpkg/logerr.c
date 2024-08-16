/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/logerr.c	1.6.5.9"
#ident "$Header: $"

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <sys/types.h>
#include <limits.h>
#include <pfmt.h>

extern	char *prog;

extern	int	logmode;

/*VARARGS*/
void
logerr(va_alist)
va_dcl
{
	va_list ap;
	char	*fmt;
	char	*pt, buffer[2048];
	int	flag;
	char	lname[PATH_MAX];	/* log file name		*/
	FILE	*logfp = NULL;		/* log errors to this file	*/
	char 	*pkginst, *getenv();	/* PKGINST in environment	*/

	char	*defmsg;
	int	flags, colons;

	/*
	 * If either LOGMODE environment variable was set to "true"
	 * by user or via the -l option on the pkgadd command line
	 * and we know the package instance, do not display error
	 * message on screen.   Instead, we place errors in logfile
	 * /var/sadm/install/logs/<pkginst>.log.  Otherwise, the 
	 * error message goes to stderr and logfile.
	 */

	if(!strcmp(prog, "pkgadd")) {
		pkginst = getenv("PKGINST");
		if(pkginst) {
			(void) sprintf(lname, "/var/sadm/install/logs/%s.log", pkginst);
			if ((logfp = fopen(lname, "a")) == NULL)
				(void) pfmt(stderr, MM_WARNING,
				 "uxpkgtools:641:Could not open logfile <%s>\n", lname);

		}
	}

	va_start(ap);
	fmt = va_arg(ap, char *);

	defmsg = fmt; pt = buffer; colons = 0;
	while (*defmsg && colons < 2)
	{
		if (*defmsg == ':')
		{
			++colons;
			if (colons < 2)
				*pt++ = *defmsg++;
			else
				defmsg++;
		}
		else
			*pt++ = *defmsg++;
	}
	*pt = '\0';

	
	flag = 0;
        if (strncmp(defmsg, "ERROR:", 6) == 0) {
                flags = MM_NOGET|MM_ERROR;
                defmsg += 6;
        } else if (strncmp(defmsg, "WARNING:", 8) == 0) {
                flags = MM_NOGET|MM_WARNING;
                defmsg += 8;
        } else {
                flag = 1;
                if(!logmode)
                        (void) fprintf(stderr, "    ");
                if(logfp)
                        (void) fprintf(logfp, "    ");
        }

        if (colons == 2)
	{
		if (*defmsg)
                	fmt = gettxt(buffer, defmsg);
		else
			fmt = defmsg;
	}
	(void) vsprintf(buffer, fmt, ap);
	va_end(ap);

        if (!flag && colons == 2) {
		if(!logmode)
			pfmt(stderr, flags, "");
		if(logfp)
			pfmt(logfp, flags, "");
	}

        for(pt=buffer; *pt; pt++) {
                if(!logmode)
                        (void) putc(*pt, stderr);
                if(logfp)
                        (void) putc(*pt, logfp);
                if(flag && (*pt == '\n') && pt[1]) {
                        if(!logmode)
                                (void) fprintf(stderr, "    ");
                        if(logfp)
                                (void) fprintf(logfp, "    ");
                }
	}
	if(!logmode)
		(void) putc('\n', stderr);
	if(logfp) {
		(void) putc('\n', logfp);
		(void) fclose(logfp);
	}
}
