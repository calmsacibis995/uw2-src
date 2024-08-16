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

#ident	"@(#)xcplxcurses:overwrite.c	1.1.2.3"
#ident  "$Header: overwrite.c 1.1 91/07/09 $"

/*
 *	@(#) overwrite.c 1.1 90/03/30 lxcurses:overwrite.c
 */
# include	"ext.h"

# define	min(a,b)	(a < b ? a : b)

/*
 *	This routine writes win1 on win2 destructively.
 *
 * 3/27/83 (Berkeley) @(#)overwrite.c	1.3
 */
overwrite(win1, win2)
reg WINDOW	*win1, *win2; {

	reg int		x, y, minx, miny, startx, starty;

# ifdef DEBUG
	fprintf(outf, "OVERWRITE(0%o, 0%o);\n", win1, win2);
# endif
	miny = min(win1->_maxy, win2->_maxy);
	minx = min(win1->_maxx, win2->_maxx);
# ifdef DEBUG
	fprintf(outf, "OVERWRITE:\tminx = %d,  miny = %d\n", minx, miny);
# endif
	starty = win1->_begy - win2->_begy;
	startx = win1->_begx - win2->_begx;
	if (startx < 0)
		startx = 0;
	for (y = 0; y < miny; y++)
		if (wmove(win2, y + starty, startx) != ERR)
			for (x = 0; x < minx; x++)
				waddch(win2, win1->_y[y][x]);
}
