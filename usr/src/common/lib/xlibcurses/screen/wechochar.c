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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wechochar.c	1.6.2.3"
#ident  "$Header: wechochar.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 *  These routines short-circuit much of the innards of curses in order to get
 *  a single character output to the screen quickly! It is used by getch()
 *  and getstr().
 *
 *  wechochar(WINDOW *win, chtype ch) is functionally equivalent to
 *  waddch(WINDOW *win, chtype ch), wrefresh(WINDOW *win)
 */

wechochar (win, ch)
register WINDOW *win;
chtype ch;
{
    int	saveimm = win->_immed, rv;

    immedok(win,TRUE);
    rv = waddch(win,ch);
    win->_immed = saveimm;
    return (rv);
}
