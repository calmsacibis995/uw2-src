/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mkdate.c	1.1.3.1"
#ident "@(#)mkdate.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mkdate - generate a date string of the proper From header format

    SYNOPSIS
	void mkdate(char datestring[60])

    DESCRIPTION
	mkdate() calls time() to get the current time, then
	formats it properly for the From header of a message.

    NAME
	fromdate - generate a date string of the proper From header format

    SYNOPSIS
	void fromdate(char datestring[60], long time)

    DESCRIPTION
	fromdate() formats the given time for the From header of a message.

    NAME
	rfc822date - convert a From date string into an rfc 822 date string

    SYNOPSIS
	void rfc822date(const char indatestring[60], char outdatestring[60])

    DESCRIPTION
	rfc822date() converts a From date string as returned from fromdate()
	into the format required by RFC 822.
*/

/* not everyone declares this properly */
extern char *tzname[2];

void mkdate(datestring)
char *datestring;
{
    long	ltmp;		/* temp variable for time() */
    time(&ltmp);
    fromdate(datestring, ltmp);
}

void fromdate(datestring, ltmp)
char *datestring;
long ltmp;
{
    char	*tp, *zp;
    struct tm	*bp;
    /* asctime: Fri Sep 30 00:00:00 1986\n */
    /*          0123456789012345678901234  */
    /* date:    Fri Sep 30 00:00 EDT 1986  */

    bp = localtime(&ltmp);
    tp = asctime(bp);
    zp = tzname[bp->tm_isdst];
    sprintf(datestring, "%.16s %.3s %.4s", tp, zp, tp+20);
}

/* asctime: Fri Sep 30 00:00:00 1986\n */
/*          0123456789012345678901234  */
/* date:    Fri Sep 30 00:00 EDT 1986  */
/*          0123456789012345678901234  */
/* RFCtime: Fri, 28 Jul 89 10:30 EDT   */
void rfc822date(indatestring, outdatestring)
const char *indatestring;
char *outdatestring;
{
    sprintf(outdatestring, "%.3s, %.2s %.3s %.4s %.5s %.3s",
	/* Fri */	indatestring,
	/* 28  */	indatestring+8,
	/* Jul */	indatestring+4,
	/* 89  */	indatestring+23,
	/* 10:30 */	indatestring+11,
	/* EDT */	indatestring+17);
}
