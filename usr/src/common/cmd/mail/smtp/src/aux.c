/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/aux.c	1.3.7.2"
#ident "@(#)aux.c	1.5 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "v9regexp.h"
#include "xmail.h"
#include "aux.h"

/* return delivery status required by the mbox */
extern int
delivery_status(line)
	string *line;
{
	string *s1 = s_new(), *s2 = s_new();
	char *cline;
	int rv;

	s_restart(line);
	cline = s_to_c(line);
	if (*cline=='\0')
		rv = MF_NORMAL;
	else if (IS_HEADER(cline))
		rv = MF_NORMAL;
	else if (s_parse(line, s1) == 0)
		rv = MF_NOTMBOX;
	else if (s_parse(line, s2) == 0)
		return MF_NOTMBOX;
	else if (strcmp(s_to_c(s2), "to") != 0)
		rv = MF_NOTMBOX;
	else if (strcmp(s_to_c(s1), "Forward") == 0)
		rv = MF_FORWARD;
	else if (strcmp(s_to_c(s1), "Pipe") == 0)
		rv = MF_PIPE;
	else
		rv = MF_NOTMBOX;
	s_free(s1); s_free(s2);
	return rv;
}

/* append a sub-expression match onto a string */
extern void
append_match(subexp, sp, se)
	regsubexp *subexp;	/* regular subexpression matches */
	register string *sp;	/* string to append to */
	int se;			/* index of subexpression to append */
{
	register char *cp = subexp[se].sp;
	register char *ep = subexp[se].ep;

	for (; cp < ep; cp++)
		s_putc(sp, *cp);
	s_terminate(sp);
}

/*
 *  check for shell characters in a string
 */
#define CHARS "[]()<>{};\\'\"`^&|\r\n \t"
extern int
shellchars(cp)
	char *cp;
{
	char *sp;

	for(sp=CHARS; *sp; sp++)
		if(strchr(cp, *sp))
			return 1;
	for(; *cp; cp++)
		if(!isprint(*cp))
			return 1;
	return 0;
}

extern void tzset();
extern char *tzname[];

/*
 *  return an ascii daytime string
 */
extern char *
thedate()
{
	static char date[32];
	char *tp, *dp;
	struct tm *bp;
	long thetime;

	thetime = time((long *)0);
	tzset();
	bp = localtime(&thetime);
	dp = asctime(bp);
	tp = bp->tm_isdst ? tzname[1] : tzname[0];
	sprintf(date, "%.16s %.3s %.4s", dp, tp, dp+20);
	return date;
}
