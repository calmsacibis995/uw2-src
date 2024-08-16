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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wattrset.c	1.11.2.3"
#ident  "$Header: wattrset.c 1.2 91/06/27 $"
#include	"curses_inc.h"

wattrset(win,a)
WINDOW	*win;
chtype	a;
{
    chtype temp_bkgd;

    /* if 'a' contains color information, then if we are not on color	*/
    /* terminal erase color information from 'a'		 	*/

    if ((a & A_COLOR) && (cur_term->_pairs_tbl == NULL))
	 a &= ~A_COLOR;

    /* combine 'a' with the background.  if 'a' contains color 		*/
    /* information delete color information from the background		*/

    temp_bkgd = (a & A_COLOR) ? (win->_bkgd & ~A_COLOR) : win->_bkgd;
    win->_attrs = (a | temp_bkgd) & A_ATTRIBUTES;
    return (1);
}
