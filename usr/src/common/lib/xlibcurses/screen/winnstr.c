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

#ident	"@(#)curses:common/lib/xlibcurses/screen/winnstr.c	1.4.2.4"
#ident  "$Header: winnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Copy n chars in window win from current cursor position to end
 * of window into char buffer str.  Return the number of chars copied.
 */

winnstr(win, str, ncols)
register	WINDOW	*win;
register	char	*str;
register	int	ncols;
{
    register	int	counter = 0;
    int			cy = win->_cury;
    register	chtype	*ptr = &(win->_y[cy][win->_curx]),
			*pmax = &(win->_y[cy][win->_maxx]);
    chtype		wc;
    int			eucw, scrw, s;


    while (ISCBIT(*ptr))
	ptr--;

    if (ncols < -1)
	ncols = MAXINT;

    while (counter < ncols)
    {
	scrw = mbscrw(RBYTE(*ptr));
	eucw = mbeucw(RBYTE(*ptr));
	if (counter + eucw > ncols)
	    break;

	for (s = 0; s < scrw; s++, ptr++)
	{
	    if ((wc = RBYTE(*ptr)) == MBIT)
		continue;
	    *str++ = wc;
	    counter++;
	    if ((wc = LBYTE(*ptr) | MBIT) == MBIT)
		continue;
	    *str++ = wc;
	    counter++;
	}

	if (ptr >= pmax)
	{
	    if (++cy == win->_maxy)
		break;

	    ptr = &(win->_y[cy][0]);
	    pmax = ptr + win->_maxx;
	}
    }

    *str = '\0';
    return (counter);
}
