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

#ident	"@(#)curses:common/lib/xlibcurses/screen/winsnstr.c	1.5.2.3"
#ident  "$Header: winsnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Insert to 'win' at most n chars of a string
 * starting at (cury, curx). However, if n <= 0,
 * insert the entire string.
 * \n, \t, \r, \b are treated in the same way
 * as other control chars.
 */

winsnstr(win, sp, n)
register	WINDOW	*win;
char	*sp;
int	n;
{
    register	chtype	*wcp;
    register	int	x, cury, endx, maxx, len;
    bool		savscrl,savsync,savimmed;
    int			savx, savy;

    /* only insert at the start of a character */
    win->_nbyte = -1;
    win->_insmode = TRUE;
    if(_scrmax > 1 && _mbvalid(win) == ERR)
	return (ERR);

    if (n < 0)
	n = MAXINT;

    /* compute total length of the insertion */
    endx = win->_curx;
    maxx = win->_maxx;
    for (x = 0; sp[x] != '\0' && x < n && endx < maxx; ++x)
    {
	len = (sp[x] < ' '|| sp[x] == _CTRL('?')) ? 2 : 1;

	if(ISMBIT(sp[x]))
		{
		register int	m,k,ty;
		register chtype	c;

		/* see if the entire character is defined */
		c = RBYTE(sp[x]);
		ty = TYPE(c);
		m = x + cswidth[ty] - (ty == 0 ? 1 : 0);
		for(k = x+1; sp[k] != '\0' && k <= m ; ++k)
			{
			c = RBYTE(sp[k]);
			if(TYPE(c) != 0)
				break;
			}
		if(k <= m)
			break;
		/* recompute # of columns used */
		len = _scrwidth[ty];
		/* skip an appropriate number of bytes */
		x = m ;
		}

	if ((endx += len) > maxx)
	{
		endx -= len;
	    break;
	}
    }

    /* length of chars to be shifted */
    if ((len = endx - win->_curx) <= 0)
	return (ERR);

    /* number of chars insertible */
    n = x;

    /* shift data */
    cury = win->_cury;

    if(_mbinsshift(win,len) == ERR)
	return(ERR);

    /* insert new data */
    wcp = win->_y[cury] + win->_curx;

    /* act as if we are adding characters */
    savx = win->_curx;
    savy = win->_cury;
    win->_insmode = FALSE;
    savscrl = win->_scroll;
    savimmed = win->_immed;
    savsync = win->_sync;
    win->_scroll = win->_sync;

    for ( ; n > 0; --n, ++wcp, ++sp)
    {
	/* multi-byte characters */
	if(_mbtrue && ISMBIT(*sp))
		{
		_mbaddch(win,A_NORMAL,RBYTE(*sp));
		wcp = win->_y[cury] + win->_curx;
		continue;
		}
	if(_scrmax > 1 && ISMBIT(*wcp))
		_mbclrch(win,cury,win->_curx);
	/* single byte character */
	win->_nbyte = -1;
	win->_curx += (*sp < ' ' || *sp == _CTRL('?')) ? 2 : 1;

	if (*sp < ' ' || *sp == _CTRL('?'))
	{
	    *wcp++ = _CHAR('^') | win->_attrs;
	    *wcp = _CHAR(_UNCTRL(*sp)) | win->_attrs;
	}
	else
	    *wcp = _CHAR(*sp) | win->_attrs;
    }
    win->_curx = savx;
    win->_cury = savy;

    /* update the change structure */
    if (win->_firstch[cury] > win->_curx)
	win->_firstch[cury] = win->_curx;
    win->_lastch[cury] = maxx - 1;

    win->_flags |= _WINCHANGED;

    win->_scroll = savscrl;
    win->_sync = savsync;
    win->_immed = savimmed;

    if (win->_sync)
	wsyncup(win);
    return (win->_immed ? wrefresh(win) : OK);
}
