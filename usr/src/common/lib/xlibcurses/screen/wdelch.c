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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wdelch.c	1.3.2.3"
#ident  "$Header: wdelch.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * This routine performs a delete-char on the line,
 * leaving (_cury, _curx) unchanged.
 */

wdelch(win)
register	WINDOW	*win;
{
    register	chtype	*temp1, *temp2;
    register	chtype	*end;
    register	int	cury = win->_cury;
    register	int	curx = win->_curx;
    register	chtype	*cp;
    register	int	s;

    end = &win->_y[cury][win->_maxx - 1];
    temp2 = &win->_y[cury][curx + 1];
    temp1 = temp2 - 1;

    s = 1;
    win->_nbyte = -1;
    if(_scrmax > 1)
	{
	if(ISMBIT(*temp1))
		{
		win->_insmode = TRUE;
		if(_mbvalid(win) == ERR)
			return ERR;
		curx = win->_curx;
		temp1 = &win->_y[cury][curx];
		}
	if(ISMBIT(*end))
		{
		for(cp = end; cp >= temp1; --cp)
			if(!ISCBIT(*cp))
				break;
		if(cp + _scrwidth[TYPE(*cp)] > end+1)
			end = cp - 1;
		}
	if(ISMBIT(*temp1))
		s = _scrwidth[TYPE(RBYTE(*temp1))];
	end -= s - 1;
	temp2 = &win->_y[cury][curx+s];
	}

    while (temp1 < end)
	*temp1++ = *temp2++;

    while (s--)
        *temp1++ = win->_bkgd;

#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
	(*_y16update)(win, 1, win->_maxx - curx, cury, curx);
#endif	/* _VR3_COMPAT_CODE */

    win->_lastch[cury] = win->_maxx - 1;
    if (win->_firstch[cury] > curx)
	win->_firstch[cury] = curx;

    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
