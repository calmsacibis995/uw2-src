/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/xperror.c	1.1.2.2"
#ident "@(#)xperror.c	1.2 'attmail mail(1) command'"
#include <stdio.h>
#include <string.h>

extern char *progname;

void xperror(s)
char *s;
{
	char buf[2048];

	(void) strcpy(buf, progname);
	(void) strcat(buf, ": ");
	(void) strcat(buf, s);
	perror(buf);
}
