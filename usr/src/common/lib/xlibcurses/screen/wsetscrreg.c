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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wsetscrreg.c	1.3.2.3"
#ident  "$Header: wsetscrreg.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 *	Change scrolling region. Since we depend on the values
 *	of tmarg and bmarg in various ways, this can no longer
 *	be a macro.
 */

wsetscrreg(win,topy,boty)
WINDOW	*win;
int	topy, boty;
{
    if (topy < 0 || topy >= win->_maxy || boty < 0 || boty >= win->_maxy)
	return (ERR);

    win->_tmarg = topy;
    win->_bmarg = boty;
    return (OK);
}
