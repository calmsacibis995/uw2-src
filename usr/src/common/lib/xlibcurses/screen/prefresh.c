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

#ident	"@(#)curses:common/lib/xlibcurses/screen/prefresh.c	1.12.2.3"
#ident  "$Header: prefresh.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * Pad refresh. These routines are provided for upward compatibility
 * with the 'pad' structure of Sys V.2. Since windows now can be of
 * arbitrary size and derived windows can be moved within their
 * parent windows effortlessly, a separate notion of 'pad' as
 * a larger-than-screen window is no longer necessary.
 *
 * pminy, pminx: the area (pminy, pminx, maxy, maxx) of pad is refreshed
 * sminy, sminx, smaxy, smaxx: the screen area to be affected.
 */

prefresh(pad, pminy, pminx, sminy, sminx, smaxy, smaxx)
WINDOW	*pad;
int	pminy, pminx, sminy, sminx, smaxy, smaxx;
{
    extern	int	wrefresh();

    return (_prefresh(wrefresh, pad, pminy, pminx, sminy, sminx, smaxy, smaxx));
}

_prefresh(func, pad, pminy, pminx, sminy, sminx, smaxy, smaxx)
int	(*func)();
WINDOW	*pad;
int	pminy, pminx, sminy, sminx, smaxy, smaxx;
{

    /*
     * If pad->_padwin doesn't exist (meaning that this is
     * the first call to p*refresh) create it as a derived window
     */

    if (!pad->_padwin)
    {
	if ((pad->_padwin = derwin(pad, pad->_maxy, pad->_maxx, 0, 0)) == NULL)
	{
	    goto bad;
	}
	/* else
	    pad->_padwin->_use_idl = TRUE; */

	/*
	 * The following makes parent window to run in fast mode
	 * by creating an illusion that it has no derived windows
	 */

	pad->_ndescs--;
    }

    if (_padjust(pad, pminy, pminx, sminy, sminx, smaxy, smaxx) == ERR)
bad:
	return (ERR);

    (*func)(pad->_padwin);
    return (OK);
}


_padjust(pad, pminy, pminx, sminy, sminx, smaxy, smaxx)
WINDOW	*pad;
int	pminy, pminx, sminy, sminx, smaxy, smaxx;
{
    register int	prows, pcols, y;
    register WINDOW	*padwin = pad->_padwin;
    register chtype	**p_y, **o_y;

    /* make sure the area requested to be updated is on the pad */

    if ((pminy >= pad->_maxy) || (pminx >= pad->_maxx))
	 return (ERR);

    /* determine the area of the pad to be updated 	*/

    if (pminy < 0)
	pminy = 0;
    if (pminx < 0)
	pminx = 0;

    /* determine the screen area affected */

    if (sminy < 0)
	sminy = 0;
    if (sminx < 0)
	sminx = 0;
    if (smaxy < sminy)
	smaxy = LINES - 1;
    if (smaxx < sminx)
	smaxx = COLS - 1;

    /*
     * Modify the area of the pad to be updated taking into
     * consideration screen parameters.
     */

    if ((prows = (smaxy - sminy) + 1) > (y = pad->_maxy - pminy))
	prows = y;
    if ((pcols = (smaxx - sminx) + 1) > (y = pad->_maxx - pminx))
	pcols = y;

    if (((padwin->_cury = pad->_cury - pminy) < 0) || (padwin->_cury >= prows))
	 padwin->_cury = 0;
    if (((padwin->_curx = pad->_curx - pminx) < 0) || (padwin->_curx >= pcols))
	 padwin->_curx = 0;

    padwin->_leave = pad->_leave;
    padwin->_use_idl = pad->_use_idl;


    /*
     * If padwin refers to the same derwin, return.  Otherwise
     * update the coordinates and malloc'ed areas of the padwin
     */

    if ((padwin->_begy == sminy) && (padwin->_begx == sminx) &&
	(padwin->_maxy == prows) && (padwin->_maxx == pcols) &&
	(padwin->_y[0] == (pad->_y[pminy] + pminx)) &&
	(!(pad->_flags & _WINSDEL)))
    {
	 goto good;
    }

    /* update the coordinates of the pad */

    padwin->_maxy = prows;
    padwin->_maxx = pcols;
    padwin->_begy = sminy;
    padwin->_begx = sminx;
    padwin->_pary = pminy;
    padwin->_parx = pminx;

    /* update the malloc'ed areas */

    p_y = padwin->_y;
    o_y = pad->_y;

    for (y = 0; y < prows; y++, pminy++) 
	 p_y[y] = o_y[pminy] + pminx;

    (void) wtouchln(padwin, 0, prows, TRUE);
good:
    return (OK);
}
