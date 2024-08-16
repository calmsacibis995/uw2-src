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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wclrtobot.c	1.2.2.3"
#ident  "$Header: wclrtobot.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* This routine erases everything on the window. */
wclrtobot(win)
register	WINDOW	*win;
{
    register	int	savimmed, savsync;
    register	int	cury = win->_cury;
    register	int	curx = win->_curx;

    if (win != curscr)
    {
	savimmed = win->_immed;
	savsync = win->_sync;
	win->_immed = win->_sync = FALSE;
    }

    /* set region to be clear */
    if (cury >= win->_tmarg && cury <= win->_bmarg)
	win->_cury = win->_bmarg;
    else
	win->_cury = win->_maxy - 1;

    win->_curx = 0;
    for ( ; win->_cury > cury; win->_cury--)
	(void) wclrtoeol(win);
    win->_curx = curx;
    (void) wclrtoeol(win);

    if (win == curscr)
	return (OK);

    /* not curscr */
    win->_sync = savsync;

    if (win->_sync)
	wsyncup(win);
    
    win->_flags |= _WINCHANGED;
    return ((win->_immed = savimmed) ? wrefresh(win) : OK);
}
