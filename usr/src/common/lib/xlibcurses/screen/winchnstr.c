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

#ident	"@(#)curses:common/lib/xlibcurses/screen/winchnstr.c	1.3.2.3"
#ident  "$Header: winchnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Read in ncols worth of data from window win and assign the
 * chars to string. NULL terminate string upon completion.
 * Return the number of chtypes copied.
 */

winchnstr(win,string,ncols)
register	WINDOW	*win;
chtype		*string;
int		ncols;
{
    register	chtype	*ptr = &(win->_y[win->_cury][win->_curx]);
    register	int	counter = 0;
    register	int	maxcols = win->_maxx - win->_curx;
    int			eucw, scrw, s, wc;
    chtype		rawc, attr;

    if (ncols < 0)
	ncols = MAXINT;

    while (ISCBIT(*ptr))
    {
	ptr--;
	maxcols++;
    }

    while ((counter < ncols) && maxcols > 0)
    {
	eucw = mbeucw(RBYTE(*ptr));
	scrw = mbscrw(RBYTE(*ptr));

	if (counter + eucw > ncols)
	    break;
	for (s = 0; s < scrw; s++, maxcols--, ptr++)
	{
	    attr = _ATTR(*ptr);
	    rawc = _CHAR(*ptr);
	    if ((wc = RBYTE(rawc)) == MBIT)
		continue;
	    *string++ = wc | attr;
	    counter++;
	    if ((wc = LBYTE(rawc) | MBIT) == MBIT)
		continue;
	    *string++ = wc | attr;
	    counter++;
	}
    }
    if (counter < ncols)
	*string = (chtype) 0;
    return (counter);
}
