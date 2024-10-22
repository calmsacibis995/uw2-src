/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/pechowchar.c	1.2.2.2"
#ident  "$Header: pechowchar.c 1.2 91/06/26 $"
/*
 *  These routines short-circuit much of the innards of curses in order to get
 *  a single character output to the screen quickly!
 *
 *  pechochar(WINDOW *pad, chtype ch) is functionally equivalent to
 *  waddch(WINDOW *pad, chtype ch), prefresh(WINDOW *pad, `the same arguments
 *  as in the last prefresh or pnoutrefresh')
 */

#include	"curses_inc.h"

pechowchar(pad, ch)
register	WINDOW	*pad;
chtype			ch;
{
    register WINDOW *padwin;
    int	     rv;

    /*
     * If pad->_padwin exists (meaning that p*refresh have been
     * previously called), call wechochar on it.  Otherwise, call
     * wechochar on the pad itself
     */

    if ((padwin = pad->_padwin) != NULL)
    {
	padwin->_cury = pad->_cury - padwin->_pary;
	padwin->_curx = pad->_curx - padwin->_parx;
	rv = wechowchar (padwin, ch);
	pad->_cury = padwin->_cury + padwin->_pary;
	pad->_curx = padwin->_curx + padwin->_parx;
	return (rv);
    }
    else
        return (wechowchar (pad, ch));
}
