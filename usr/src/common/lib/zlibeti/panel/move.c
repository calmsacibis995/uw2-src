/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* A panels subsystem built on curses--Move a panel */

#ident	"@(#)libeti:panel/move.c	1.4"

#include <curses.h>
#include "private.h"

	/****************/
	/*  move_panel  */
	/****************/

int move_panel (panel, starty, startx)
register PANEL	*panel;
int	starty;
int	startx;
{
	if (!panel)
		return ERR;

	/* Check for hidden panels and move the window */

	if (panel == panel -> below)
	{
		if (mvwin (panel -> win, starty, startx) == ERR)
			return ERR;
	}
	else
	{

		/* allocate nodes for overlap of new panel and move
		 * the curses window, removing it from the old location.
		 */
	
		if (!_alloc_overlap (_Panel_cnt - 1) ||
		    mvwin (panel -> win, starty, startx) == ERR)
			return ERR;

		_remove_overlap (panel);
	}

	/* Make sure we know where the window is */

	getbegyx (panel -> win, panel -> wstarty, panel -> wstartx);
	getmaxyx (panel -> win, panel -> wendy, panel -> wendx);
	panel -> wendy += panel -> wstarty - 1;
	panel -> wendx += panel -> wstartx - 1;

	/* Determine which panels the new panel obscures (if not hidden) */

	if (panel != panel -> below)
		_intersect_panel (panel);
	return OK;
}
