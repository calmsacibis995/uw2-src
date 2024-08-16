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

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)xcplxcurses:getch.c	1.1.2.3"
#ident  "$Header: getch.c 1.1 91/07/09 $"

/*
 *	@(#) getch.c 1.1 90/03/30 lxcurses:getch.c
 */
# include	"ext.h"

/*
 *	This routine reads in a character from the window.
 *
 * 5/11/81 (Berkeley) @(#)getch.c	1.2
 */
wgetch(win)
reg WINDOW	*win; {

	reg bool	weset = FALSE;
	reg char	inp;

	if (!win->_scroll && (win->_flags&_FULLWIN)
	    && win->_curx == win->_maxx - 1 && win->_cury == win->_maxy - 1)
		return ERR;
# ifdef DEBUG
	fprintf(outf, "WGETCH: _echoit = %c, _rawmode = %c\n", _echoit ? 'T' : 'F', _rawmode ? 'T' : 'F');
# endif
	if (_echoit && !_rawmode) {
		raw();
		weset++;
	}
	inp = getchar();
# ifdef DEBUG
	fprintf(outf,"WGETCH got '%s'\n",unctrl(inp));
# endif
	if (_echoit) {
		mvwaddch(curscr, win->_cury, win->_curx, inp);
		waddch(win, inp);
	}
	if (weset)
		noraw();
	return inp;
}
