/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mbmove.c	1.2.2.2"
#ident  "$Header: mbmove.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
**	Move (cury,curx) of win to (y,x).
**	It is guaranteed that the cursor is left at the start
**	of a whole character nearest to (y,x).
*/
wmbmove(win,y,x)
WINDOW	*win;
int	y, x;
{
	reg chtype	*wcp, *wp, *ep;

	if(y < 0 || x < 0 || y >= win->_maxy || x >= win->_maxx)
		return ERR;

	if(_scrmax > 1)
	{
		wcp = win->_y[y];
		wp = wcp + x;
		ep = wcp+win->_maxx;

		/* make wp points to the start of a character */
		if(ISCBIT(*wp))
		{
			for(; wp >= wcp; --wp)
				if(!ISCBIT(*wp))
					break;
			if(wp < wcp)
			{
				wp = wcp+x+1;
				for(; wp < ep; ++wp)
					if(!ISCBIT(*wp))
						break;
			}
		}

		/* make sure that the character is whole */
		if(wp + _scrwidth[TYPE(*wp)] > ep)
			return ERR;

		/* the new x position */
		x = wp-wcp;
	}

	if(y != win->_cury || x != win->_curx)
	{
		win->_nbyte = -1;
		win->_cury = y;
		win->_curx = x;
	}

	return win->_immed ? wrefresh(win) : OK;
}
