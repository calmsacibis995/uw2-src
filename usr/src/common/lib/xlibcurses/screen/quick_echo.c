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

#ident	"@(#)curses:common/lib/xlibcurses/screen/quick_echo.c	1.13.2.4"
#ident  "$Header: quick_echo.c 1.2 91/06/26 $"
#include	"curses_inc.h"

extern	int	outchcount;

/*
 *  These routines short-circuit much of the innards of curses in order to get
 *  a single character output to the screen quickly! It is used by waddch().
 */

_quick_echo(win, ch)
register	WINDOW	*win;
chtype			ch;
{
#ifdef __STDC__
    extern  int     _outch(int);
#else
    extern  int     _outch();
#endif
    int		y = win->_cury;
    register	int	SPy = y + win->_begy + win->_yoffset;
    register	int	SPx = (win->_curx - 1) + win->_begx;
    register	chtype	rawc = _CHAR(ch), rawattrs = _ATTR(ch);

    if ((curscr->_flags & _CANT_BE_IMMED) || (win->_flags & _WINCHANGED) ||
	(win->_clear) || (curscr->_clear) || (_virtscr->_flags & _WINCHANGED) ||
	(SPy > ((LINES + SP->Yabove) - 1)) || (SPx > (COLS - 1)) ||
	(SP->slk && (SP->slk->_changed == TRUE)))
    {
	win->_flags |= _WINCHANGED;
	return (wrefresh (win));
    }

    outchcount = 0;
    win->_firstch[y] = _INFINITY;
    win->_lastch[y] = -1;
    /* If the cursor is not in the right place, put it there! */
    if ((SPy != curscr->_cury) || (SPx != curscr->_curx))
    {
	(void) mvcur (curscr->_cury, curscr->_curx, SPy, SPx);
	curscr->_cury = SPy;
    }
    curscr->_curx = SPx + 1;
    _CURHASH[SPy] = _NOHASH;
    if (ch != ' ')
    {
	if (SPx > _ENDNS[SPy])
	    _ENDNS[SPy] = SPx;
	if (SPx < _BEGNS[SPy])
	    _BEGNS[SPy] = SPx;
    }
    _virtscr->_y[SPy][SPx] = curscr->_y[SPy][SPx] = ch;

    if (rawattrs != curscr->_attrs)
	_VIDS(rawattrs, curscr->_attrs);

    if (SP->phys_irm)
	_OFFINSERT();

    /* Write it out! */
    _outch((chtype) rawc);
    (void) fflush(SP->term_file);

    return (outchcount);
}
