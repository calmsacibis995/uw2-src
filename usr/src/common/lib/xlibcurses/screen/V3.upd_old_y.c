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

#ident	"@(#)curses:common/lib/xlibcurses/screen/V3.upd_old_y.c	1.2.2.3"
#ident  "$Header: V3.upd_old_y.c 1.2 91/06/26 $"

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
void
_update_old_y_area(win, nlines, ncols, start_line, start_col)
WINDOW	*win;
int	nlines, ncols, start_line, start_col;
{
    int	row, col, num_cols;

    for (row = start_line; nlines > 0; nlines--, row++)
	for (num_cols = ncols, col = start_col; num_cols > 0; num_cols--, col++)
	    win->_y16[row][col] = _TO_OCHTYPE(win->_y[row][col]);
}
#endif	/* _VR3_COMPAT_CODE */
