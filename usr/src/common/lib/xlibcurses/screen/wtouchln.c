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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wtouchln.c	1.4.2.3"
#ident  "$Header: wtouchln.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Make a number of lines look like they have/have not been changed.
 * y: the start line
 * n: the number of lines affected
 * changed:	1: changed
 * 		0: not changed
 * 		-1: changed. Called internally - In this mode
 * 		    even REDRAW lines are changed.
 */

wtouchln(win, y, n, changed)
WINDOW	*win;
int	y, n, changed;
{
    register	short	*firstch, *lastch;
    register	int	b, e, maxy = win->_maxy;

    if (y >= maxy)
	return (ERR);
    if (y < 0)
	y = 0;
    if ((y + n) > maxy)
	n = maxy - y;
    firstch = win->_firstch + y;
    lastch = win->_lastch + y;
    if (changed)
    {
	win->_flags |= _WINCHANGED;
	b = 0;
	e = win->_maxx - 1;
    }
    else
    {
	b = _INFINITY;
	e = -1;
	win->_flags &= ~_WINCHANGED;
    }

    for ( ; n-- > 0; firstch++, lastch++)
    {
	if (changed == -1 || *firstch != _REDRAW)
	    *firstch = b, *lastch = e;
    }

    if ((changed == 1) && win->_sync)
	wsyncup(win);

    return ((changed == 1) && win->_immed) ? wrefresh(win) : OK;
}
