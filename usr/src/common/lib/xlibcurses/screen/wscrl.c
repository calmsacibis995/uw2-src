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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wscrl.c	1.2.2.3"
#ident  "$Header: wscrl.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Scroll the given window up/down n lines. */

wscrl(win, n)
register	WINDOW	*win;
{
    register	int	curx, cury, savimmed, savsync;

#ifdef	DEBUG
    if (outf)
	if (win == stdscr)
	    fprintf(outf, "scroll(stdscr, %d)\n", n);
	else
	    if (win == curscr)
		fprintf(outf, "scroll(curscr, %d)\n", n);
	    else
		fprintf(outf, "scroll(%x, %d)\n", win, n);
#endif	/* DEBUG */
    if (!win->_scroll || (win->_flags & _ISPAD))
	return (ERR);

    savimmed = win->_immed;
    savsync = win->_sync;
    win->_immed = win->_sync = FALSE;

    curx = win->_curx; cury = win->_cury;

    if (cury >= win->_tmarg && cury <= win->_bmarg)
	win->_cury = win->_tmarg;
    else
	win->_cury = 0;

    (void) winsdelln(win, -n);
    win->_curx = curx;
    win->_cury = cury;

    win->_sync = savsync;

    if (win->_sync)
	wsyncup(win);

    return ((win->_immed = savimmed) ? wrefresh(win) : OK);
}
