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

#ident	"@(#)xcplxcurses:touchwin.c	1.1.2.3"
#ident  "$Header: touchwin.c 1.1 91/07/09 $"

/*
 *	@(#) touchwin.c 1.1 90/03/30 lxcurses:touchwin.c
 */
# include	"ext.h"

static do_touch();

/*
 * make it look like the whole window has been changed.
 *
 * 5/9/83 (Berkeley) @(#)touchwin.c	1.2
 */
touchwin(win)
reg WINDOW	*win;
{
	reg WINDOW	*wp;

	do_touch(win);
	for (wp = win->_nextp; wp != win; wp = wp->_nextp)
		do_touch(wp);
}

/*
 * do_touch:
 *	Touch the window
 */
static
do_touch(win)
reg WINDOW	*win; {

	reg int		y, maxy, maxx;

	maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = 0; y < maxy; y++) {
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
}
