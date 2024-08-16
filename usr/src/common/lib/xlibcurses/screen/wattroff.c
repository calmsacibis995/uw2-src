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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wattroff.c	1.12.2.3"
#ident  "$Header: wattroff.c 1.2 91/06/27 $"
#include	"curses_inc.h"

wattroff(win,a)
WINDOW	*win;
chtype	a;
{
    /* if attribute contains color information, but this is not a color    */
    /* terminal, or that color information doesn't match the one stored	   */
    /* inside _attrs,  ignore that information.				   */

    if (((a & A_COLOR) && (cur_term->_pairs_tbl == NULL)) ||
        ((a & A_COLOR) != (win->_attrs & A_COLOR)))
	 a &= ~A_COLOR;

    if ((a & A_ATTRIBUTES) == A_NORMAL)
	 return (1);

    /* turn off the attributes		*/

    win->_attrs &= ~a & A_ATTRIBUTES;

    /* if background contains color information different from the one */
    /* we have just turned off, turn that color on.  (Reason: the      */
    /* color we have just turned off was set by wattron(), so the back-*/
    /* ground color was blocked.  However, now the background color can*/
    /* be seen.							       */

    if ((a & A_COLOR) && ((a & A_COLOR) != (win->_bkgd & A_COLOR)))
	 win->_attrs |= (win->_bkgd & A_COLOR);

    return (1);
}
