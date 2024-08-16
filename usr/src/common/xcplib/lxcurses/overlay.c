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

#ident	"@(#)xcplxcurses:overlay.c	1.1.2.3"
#ident  "$Header: overlay.c 1.1 91/07/09 $"

/*
 *	@(#) overlay.c 1.1 90/03/30 lxcurses:overlay.c
 */
# include	"ext.h"
# include	<ctype.h>

# define	min(a,b)	(a < b ? a : b)
# define	max(a,b)	(a > b ? a : b)

/*
 *	This routine writes win1 on win2 non-destructively.
 *
 * 11/5/82 (Berkeley) @(#)overlay.c	1.4
 */
overlay(win1, win2)
reg WINDOW	*win1, *win2; {

	reg char	*sp, *end;
	reg int		x, y, endy, endx, starty, startx;

# ifdef DEBUG
	fprintf(outf, "OVERLAY(%0.2o, %0.2o);\n", win1, win2);
# endif
	starty = max(win1->_begy, win2->_begy) - win1->_begy;
	startx = max(win1->_begx, win2->_begx) - win1->_begx;
	endy = min(win1->_maxy, win2->_maxy) - win1->_begy - 1;
	endx = min(win1->_maxx, win2->_maxx) - win1->_begx - 1;
	for (y = starty; y <= endy; y++) {
		end = &win1->_y[y][endx];
		x = startx + win1->_begx;
		for (sp = &win1->_y[y][startx]; sp <= end; sp++) {
			if (!isspace(*sp))
				mvwaddch(win2, y + win1->_begy, x, *sp);
			x++;
		}
	}
}
