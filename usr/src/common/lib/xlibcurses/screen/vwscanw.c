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

#ident	"@(#)curses:common/lib/xlibcurses/screen/vwscanw.c	1.9.2.3"
#ident  "$Header: vwscanw.c 1.2 91/06/27 $"
/*
 * scanw and friends
 *
 */

# include	"curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 *	This routine actually executes the scanf from the window.
 *
 *	This code calls vsscanf, which is like sscanf except
 * 	that it takes a va_list as an argument pointer instead
 *	of the argument list itself.  We provide one until
 *	such a routine becomes available.
 */

/*VARARGS2*/
vwscanw(win, fmt, ap)
WINDOW	*win;
char *fmt;
va_list	ap;
{
	wchar_t code[256];
	char	*buf;
	extern char *_strcode2byte();
	register int n;

	if (wgetwstr(win, code) == ERR)
		n = ERR;
	else {
		buf = _strcode2byte(code, NULL, -1);
		n = vsscanf(buf, fmt, ap);
	}

	va_end(ap);
	return n;
}
