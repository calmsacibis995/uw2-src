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

#ident	"@(#)curses:common/lib/xlibcurses/screen/winstr.c	1.4.2.3"
#ident  "$Header: winstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

winstr(win, str)
register	WINDOW	*win;
register	char	*str;
{
    register	int	counter = 0;
    int			cy = win->_cury;
    register	chtype	*ptr = &(win->_y[cy][win->_curx]),
			*pmax = &(win->_y[cy][win->_maxx]);
    chtype		*p1st = &(win->_y[cy][0]);
    chtype		wc;
    int			ew, sw, s;

    while (ISCBIT(*ptr) && (p1st < ptr))
	ptr--;

    while (ptr < pmax)
    {
	wc = RBYTE(*ptr);
	sw = mbscrw(wc);
	ew = mbeucw(wc);
	for (s = 0; s < sw; s++, ptr++)
	{
	    if ((wc = RBYTE(*ptr)) == MBIT)
		continue;
	    str[counter++] = wc;
	    if ((wc = LBYTE(*ptr) | MBIT) == MBIT)
		continue;
	    str[counter++] = wc;
	}
    }
    str[counter] = '\0';

    return (counter);
}
