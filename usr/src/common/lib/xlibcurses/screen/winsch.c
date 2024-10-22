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

#ident	"@(#)curses:common/lib/xlibcurses/screen/winsch.c	1.5.2.3"
#ident  "$Header: winsch.c 1.2 91/06/27 $"
#include	"curses_inc.h"
#include	<ctype.h>

/* Insert a character at (curx, cury). */

winsch(win, c)
register	WINDOW	*win;
chtype	c;
{
    register	int	x, endx, n, curx = win->_curx, cury = win->_cury;
    register	chtype	*wcp, a;
    int			rv;

    a = _ATTR(c);
    c &= A_CHARTEXT;

    rv = OK;
    win->_insmode = TRUE;
    if(_scrmax > 1 && (rv = _mbvalid(win)) == ERR)
	goto done;
    /* take care of multi-byte characters */
    if(_mbtrue && ISMBIT(c))
	{
	rv = _mbaddch(win,A_NORMAL,RBYTE(c));
	goto done;
	}
    win->_nbyte = -1;
    curx = win->_curx;

    /* let waddch() worry about these */
    if (c == '\r' || c == '\b')
	return (waddch(win, c));

    /* with \n, in contrast to waddch, we don't clear-to-eol */
    if (c == '\n')
    {
	if (cury >= (win->_maxy-1) || cury == win->_bmarg)
	    return (wscrl(win, 1));
	else
	{
	    win->_cury++;
	    win->_curx = 0;
	    return (OK);
	}
    }

    /* with tabs or control characters, we have to do more */
    if (c == '\t')
    {
	n = (TABSIZE - (curx % TABSIZE));
	if ((curx + n) >= win->_maxx)
	    n = win->_maxx - curx;
	c = ' ';
    }
    else
    {
	if (iscntrl((int) c))
	{
	    if (curx >= win->_maxx-1)
		return (ERR);
	    n = 2;
	}
	else
	    n = 1;
    }

    /* shift right */
    endx = curx + n;
    x = win->_maxx - 1;
    wcp = win->_y[cury] + curx;
    if((rv = _mbinsshift(win,n)) == ERR)
	goto done;

    /* insert new control character */
    if (c < ' ' || c == _CTRL('?'))
    {
	*wcp++ = '^' | win->_attrs | a;
	*wcp = _UNCTRL(c) | win->_attrs | a;
    }
    else
    {
	/* normal characters */
	c = _WCHAR(win, c) | a;
	for ( ; n > 0; --n)
	    *wcp++ = c;
    }

done:
    if (curx < win->_firstch[cury])
	win->_firstch[cury] = curx;
    win->_lastch[cury] = win->_maxx-1;

    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);

    return((rv == OK && win->_immed) ? wrefresh(win) : rv);
}
