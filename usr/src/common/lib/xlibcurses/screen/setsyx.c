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

#ident	"@(#)curses:common/lib/xlibcurses/screen/setsyx.c	1.8.2.3"
#ident  "$Header: setsyx.c 1.2 91/06/26 $"
/*
 * Set the current screen coordinates (y, x).
 *
 * This routine may be called before doupdate(). It tells doupdate()
 * where to leave the cursor instead of the location of (x, y) of the
 * last window that was wnoutrefreshed or pnoutrefreshed.
 * If x and y are negative, then the cursor will be left wherever
 * curses decides to leave it, as if leaveok() had been TRUE for the
 * last window refreshed.
 */
#include	"curses_inc.h"

setsyx(y, x)
int	y, x;
{
    if (y < 0 && x < 0)
    {
	SP->virt_scr->_leave = TRUE;
    }
    else
    {
	_virtscr->_cury = y + SP->Yabove;
	_virtscr->_curx = x;
	_virtscr->_leave = FALSE;
	_virtscr->_flags |= _WINMOVED;
    }
    return (OK);
}
