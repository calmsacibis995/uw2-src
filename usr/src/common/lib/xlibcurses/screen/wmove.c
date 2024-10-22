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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wmove.c	1.3.2.3"
#ident  "$Header: wmove.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* This routine moves the cursor to the given point */

wmove(win, y, x)
register	WINDOW	*win;
register	int	y, x;
{
#ifdef	DEBUG
    if (outf)
    {
	fprintf(outf, "MOVE to win ");
	if (win == stdscr)
	    fprintf(outf, "stdscr ");
	else
	    fprintf(outf, "%o ", win);
	fprintf(outf, "(%d, %d)\n", y, x);
    }
#endif	/* DEBUG */
    if (x < 0 || y < 0 || x >= win->_maxx || y >= win->_maxy)
	return (ERR);

    if (y != win->_cury || x != win->_curx)
	win->_nbyte = -1;

    win->_curx = x;
    win->_cury = y;
    win->_flags |= _WINMOVED;
    return (win->_immed ? wrefresh(win) : OK);
}
