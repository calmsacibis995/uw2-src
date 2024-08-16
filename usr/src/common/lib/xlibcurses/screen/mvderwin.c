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

#ident	"@(#)curses:common/lib/xlibcurses/screen/mvderwin.c	1.2.2.3"
#ident  "$Header: mvderwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * Move a derived window inside its parent window.
 * This routine does not change the screen relative
 * parameters begx and begy. Thus, it can be used to
 * display different parts of the parent window at
 * the same screen coordinate.
 */

mvderwin(win, pary, parx)
WINDOW	*win;
int	pary, parx;
{
    register	int	y, maxy, maxx;
    register	WINDOW	*par;
    chtype		obkgd, **wc, **pc;
    short		*begch, *endch;

    if ((par = win->_parent) == NULL)
	goto bad;
    if (pary == win->_pary && parx == win->_parx)
	return (OK);

    maxy = win->_maxy-1;
    maxx = win->_maxx-1;
    if ((parx + maxx) >= par->_maxx || (pary + maxy) >= par->_maxy)
bad:
	return (ERR);

    /* save all old changes */
    wsyncup(win);

    /* rearrange pointers */
    win->_parx = parx;
    win->_pary = pary;
    wc = win->_y;
    pc = par->_y + pary;
    begch = win->_firstch;
    endch = win->_lastch;
    for (y = 0; y <= maxy; ++y, ++wc, ++pc, ++begch, ++endch)
    {
	*wc = *pc + parx;
	*begch = 0;
	*endch = maxx;
    }

    /* change background to our own */
    obkgd = win->_bkgd;
    win->_bkgd = par->_bkgd;
    return (wbkgd(win, obkgd));
}
