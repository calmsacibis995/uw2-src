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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wsyncup.c	1.6.2.3"
#ident  "$Header: wsyncup.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Sync the changes in a window with its ancestors. */

void
wsyncup(win)
register	WINDOW	*win;
{
    register	short	*wbch, *wech, *pbch, *pech;
    register	int	wy, px, py, endy, bch, ech;
    register	WINDOW	*par;

    for (par = win->_parent; par != NULL; win = par, par = par->_parent)
    {
	py = win->_pary;
	px = win->_parx;
	endy = win->_maxy;

	wbch = win->_firstch;
	wech = win->_lastch;
	pbch = par->_firstch+ py;
	pech = par->_lastch+ py;

	/*
	 * I don't think we need check WINCHANGED first.
	 * The reasoning is that all internal calls will have come
	 * from a function that did change the window.  And assumably
	 * all external calls will work the same way.
	 */
	par->_flags |= _WINCHANGED;
	/* check each line */
	for (wy = 0; wy < endy; ++wy, ++wbch, ++wech, ++pbch, ++pech)
	    if (*wbch != _INFINITY)
	    {
		bch = px + *wbch;
		ech = px + *wech;
		if (*pbch > bch)
		    *pbch = bch;
		if (*pech < ech)
		    *pech = ech;
	    }
    }
}

void
wcursyncup(win)
register	WINDOW	*win;
{
    register	WINDOW	*par = win->_parent;

    while (par != NULL)
    {
	par->_cury = win->_cury + win->_pary;
	par->_curx = win->_curx + win->_parx;
	par = (win = par)->_parent;
    }
}
