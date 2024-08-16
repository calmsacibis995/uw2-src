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

#ident	"@(#)xcplxcurses:box.c	1.1.2.3"
#ident  "$Header: box.c 1.1 91/07/09 $"

/*
 *	@(#) box.c 1.1 90/03/30 lxcurses:box.c
 */
# include	"ext.h"

/*
 *	This routine draws a box around the given window with "vert"
 * as the vertical delimiting char, and "hor", as the horizontal one.
 *
 * 2/16/82 (Berkeley) @(#)box.c	1.2
 */
box(win, vert, hor)
reg WINDOW	*win;
char		vert, hor; {

	reg int		i;
	reg int		endy, endx;
	reg char	*fp, *lp;

	endx = win->_maxx;
	endy = win->_maxy - 1;
	fp = win->_y[0];
	lp = win->_y[endy];
	for (i = 0; i < endx; i++)
		fp[i] = lp[i] = hor;
	endx--;
	for (i = 0; i <= endy; i++)
		win->_y[i][0] = (win->_y[i][endx] = vert);
	if (!win->_scroll && (win->_flags&_SCROLLWIN))
		fp[0] = fp[endx] = lp[0] = lp[endx] = ' ';
	touchwin(win);
}
