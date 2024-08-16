/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/log.c	1.2"
#ident "@(#)log.c	1.3 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "xmail.h"

#ifndef va_arg
# if defined(__STDC__) || defined(__cplusplus)
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
#endif

struct loglevellookup {
	char *name;
	int level;
} loglevellookup[] = {
	{ "emerg",	LOG_EMERG },
	{ "alert",	LOG_ALERT },
	{ "crit",	LOG_CRIT },
	{ "err",	LOG_ERR },
	{ "warning",	LOG_WARNING },
	{ "notice",	LOG_NOTICE },
	{ "info",	LOG_INFO },
	{ "debug",	LOG_DEBUG },
	{ NULL,		-1 },
};


extern int debug;

/*
    interface to syslog(3)
*/

/* PRINTFLIKE2 */
#ifdef __STDC__
void Syslog(int level, const char *fmt, ...)
#else
# ifdef lint
void Syslog(int Xlevel, const char *Xfmt, va_alist)
	int Xlevel;
	const char *Xfmt;
	va_dcl
# else
void Syslog(va_alist)
	va_dcl
# endif
#endif
{
#ifndef __STDC__
	int level;
	const char *fmt;
#endif
	va_list args;

#ifndef __STDC__
# ifdef lint
	level = Xlevel;
	fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	level = va_arg(args, int);
	fmt = va_arg(args, const char*);
#endif

	vsyslog(level, fmt, args);
	if (debug)
		vfprintf(stderr, fmt, args);
}

void Openlog(label, flags, log)
const char *label;
int flags;
int log;
{
	openlog(label, flags, log);
}

void setloglevel(arg)
	char *arg;
{
	int newlevel = -1, i;
	char *cp;

	for (cp=arg; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);

	if (isdigit(*arg)) {
		newlevel = atoi(arg);
	} else {
		for (i=0; loglevellookup[i].name; i++)
			if (strcmp(loglevellookup[i].name, arg) == 0) {
				newlevel = loglevellookup[i].level;
				break;
			}
	}
	if (newlevel < 0)
		Syslog(LOG_WARNING, "Illegal log parameter - %s\n", arg);
	else
		setlogmask(LOG_UPTO(newlevel));
}

#ifdef SVR3
/* quick-and-dirty syslog() replacement for S5 */

#define	SYSLOG		"/usr/spool/smtpq/LOG"

void syslog(va_alist)
va_dcl
{
	va_list ap;
	register FILE *fp;
	register char *p;
	long now;
	int unused;

	va_start(ap);
	unused = va_arg(ap, int);
	if ((fp = fopen(SYSLOG, "a")) != NULL) {
		now = time((long*)0);
		p = ctime(&now);
		p[strlen(p)-1] = '\0';
		fprintf(fp, "%s: %d ", p, getpid());
		p = va_arg(ap, char *);
		vfprintf(fp, p, ap);
		fclose(fp);
	}
	va_end(ap);
}
#endif
