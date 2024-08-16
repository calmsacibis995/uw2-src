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

#ident	"@(#)curses:common/lib/xlibcurses/screen/mvscanw.c	1.9.2.3"
#ident  "$Header: mvscanw.c 1.2 91/06/26 $"

# include	"curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 *
 */

/*VARARGS*/
#ifdef __STDC__
mvscanw(int y, int x, ...)
#else
mvscanw(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
	register int	y, x;
#endif
	register char	*fmt;
	va_list		ap;

#ifdef __STDC__
	va_start(ap, x);
#else
	va_start(ap);
	y = va_arg(ap, int);
	x = va_arg(ap, int);
#endif
	fmt = va_arg(ap, char *);
	return move(y, x) == OK ? vwscanw(stdscr, fmt, ap) : ERR;
}
