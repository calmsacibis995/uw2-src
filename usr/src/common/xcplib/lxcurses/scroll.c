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

#ident	"@(#)xcplxcurses:scroll.c	1.1.2.3"
#ident  "$Header: scroll.c 1.1 91/07/09 $"

/*
 *	@(#) scroll.c 1.1 90/03/30 lxcurses:scroll.c
 */
# include	"ext.h"

/*
 *	This routine scrolls the window up a line.
 *
 * 6/1/83 (Berkeley) @(#)scroll.c	1.3
 */
scroll(win)
reg WINDOW	*win; {

	reg char	*sp;
	reg int		i;
	reg char	*temp;

	if (!win->_scroll)
		return ERR;
	temp = win->_y[0];
	for (i = 1; i < win->_maxy; i++)
		win->_y[i - 1] = win->_y[i];
	for (sp = temp; sp < &temp[win->_maxx]; )
		*sp++ = ' ';
	win->_y[win->_maxy - 1] = temp;
	win->_cury--;
	if (win == curscr) {
		putchar('\n');
		if (!NONL)
			win->_curx = 0;
# ifdef DEBUG
		fprintf(outf, "SCROLL: win == curscr\n");
# endif
	}
# ifdef DEBUG
	else
		fprintf(outf, "SCROLL: win [0%o] != curscr [0%o]\n",win,curscr);
# endif
	touchwin(win);
	return OK;
}
