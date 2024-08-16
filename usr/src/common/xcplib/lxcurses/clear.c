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

#ident	"@(#)xcplxcurses:clear.c	1.1.2.3"
#ident  "$Header: clear.c 1.1 91/07/09 $"

/*
 *	@(#) clear.c 1.1 90/03/30 lxcurses:clear.c
 */
# include	"ext.h"

/*
 *	This routine clears the window.
 *
 * 1/26/81 (Berkeley) @(#)clear.c	1.1
 */
wclear(win)
reg WINDOW	*win; {

	if (win == curscr) {
# ifdef DEBUG
		fprintf(outf,"WCLEAR: win == curscr\n");
		fprintf(outf,"WCLEAR: curscr = %d\n",curscr);
		fprintf(outf,"WCLEAR: stdscr = %d\n",stdscr);
# endif
		clear();
		return refresh();
	}
	werase(win);
	win->_clear = TRUE;
	return OK;
}
