/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/ptext.c	1.2.6.3"
#ident  "$Header: $"
#include <stdio.h>
#include <varargs.h>

extern int	puttext();
extern int	quietmode;
extern char	*prog;

/*VARARGS*/
void
ptext(va_alist)
va_dcl
{
	va_list ap;
	FILE	*fp;
	char	*fmt;
	char	buffer[2048];

	/*
	 * If quietmode is set, just return.
	 */
	if(quietmode)
		return;

	va_start(ap);
	fp = va_arg(ap, FILE *);
	fmt = va_arg(ap, char *);
	va_end(ap);
		
	(void) vsprintf(buffer, fmt, ap);

	(void) puttext(fp, buffer, 0, 70);
	(void) putc('\n', fp);
}
