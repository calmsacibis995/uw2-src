/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)curses:common/lib/xlibcurses/screen/vsprintf.c	1.8.2.3"
#ident  "$Header: vsprintf.c 1.2 91/06/27 $"
/*LINTLIBRARY*/
#include "curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

extern	int	_doprnt();

int
vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
	register int count;
	FILE siop;

	siop._cnt = MAXINT;
#ifdef _NFILE
	siop._file = _NFILE;
#endif
	siop._flag = _IOWRT;
#ifdef SYSV
	siop._base = siop._ptr = (unsigned char *)string;
#else
	siop._flag |= _IOSTRG;
	siop._ptr = string;
#endif
	count = _doprnt(format, ap, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return (count);
}

int
vfprintf(fp, format, ap)
FILE *fp;
char *format;
va_list ap;
{
	int n;
	char spbuf[512];

	n = vsprintf(spbuf, format, ap);
	(void) fputs(spbuf, fp);
	return (n);
}
