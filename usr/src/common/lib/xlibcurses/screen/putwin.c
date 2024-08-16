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

#ident	"@(#)curses:common/lib/xlibcurses/screen/putwin.c	1.4.2.3"
#ident  "$Header: putwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * Write a window to a file.
 *
 * win:	the window to write out.
 * filep:	the file to write to.
 */

putwin(win, filep)
WINDOW	*win;
FILE	*filep;
{
    int			maxx, nelt;
    register	chtype	**wcp, **ecp;

    /* write everything from _cury to _bkgd inclusive */
    nelt = sizeof(WINDOW) - sizeof(win->_y) - sizeof(win->_parent) -
	   sizeof(win->_parx) - sizeof(win->_pary) -
	   sizeof(win->_ndescs) - sizeof(win->_delay);

    if (fwrite((char *) &(win->_cury), 1, nelt, filep) != nelt)
	goto err;

    /* Write the character image */
    maxx = win->_maxx;
    ecp = (wcp = win->_y) + win->_maxy;
    while (wcp < ecp)
	if (fwrite((char *) *wcp++, sizeof(chtype), maxx, filep) != maxx)
err:
	    return (ERR);

    return (OK);
}
