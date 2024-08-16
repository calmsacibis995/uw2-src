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

#ident	"@(#)curses:common/lib/xlibcurses/screen/mvwin.c	1.9.2.3"
#ident  "$Header: mvwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/* relocate the starting position of a _window */

mvwin(win, by, bx)
register	WINDOW	*win;
register	int	by, bx;
{
    if ((by + win->_maxy) > LINES || (bx + win->_maxx) > COLS ||
	 by < 0 || bx < 0)
         return ERR;
    win->_begy = by;
    win->_begx = bx;
    (void) wtouchln(win, 0, win->_maxy, -1);
    return (win->_immed ? wrefresh(win) : OK);
}
