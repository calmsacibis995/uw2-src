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

#ident	"@(#)xcplxcurses:mvwin.c	1.1.2.3"
#ident  "$Header: mvwin.c 1.1 91/07/09 $"

/*
 *	@(#) mvwin.c 1.1 90/03/30 lxcurses:mvwin.c
 */
# include	"ext.h"

/*
 * relocate the starting position of a window
 *
 * 1/26/81 (Berkeley) @(#)mvwin.c	1.1
 */

mvwin(win, by, bx)
reg WINDOW	*win;
reg int		by, bx; {

	if (by + win->_maxy > LINES || bx + win->_maxx > COLS)
		return ERR;
	win->_begy = by;
	win->_begx = bx;
	touchwin(win);
	return OK;
}
