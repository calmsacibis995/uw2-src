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

#ident	"@(#)curses:common/lib/xlibcurses/screen/wclrtoeol.c	1.5.2.3"
#ident  "$Header: wclrtoeol.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* This routine clears up to the end of line. */

wclrtoeol(win)
register	WINDOW	*win;
{
    register	int	y = win->_cury;
    register	int	x = win->_curx;
    register	int	maxx = win->_maxx;

    int			cx;
    chtype		wc;
    if(win != curscr)
	{
	win->_nbyte = -1;
	if(_scrmax > 1)
		{
		if(ISMBIT(win->_y[y][x]))
			{
			win->_insmode = TRUE;
			if(_mbvalid(win) == ERR)
				return ERR;
			x = win->_curx;
			}
		if(ISMBIT(win->_y[y][maxx-1]))
			{
			for(cx = maxx-1; cx >= x; --cx)
				if(!ISCBIT(win->_y[y][cx]))
					break;
			wc = RBYTE(win->_y[y][cx]);
			if(cx+_scrwidth[TYPE(wc)] > maxx)
				maxx = cx -1;
			}
		}
	}

    memSset(&win->_y[y][x], win->_bkgd, maxx - x);
    maxx = win->_maxx;

#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
	(*_y16update)(win, 1, maxx - x, y, x);
#endif	/* _VR3_COMPAT_CODE */

    /* if curscr, reset blank structure */
    if (win == curscr)
    {
	if (_BEGNS[y] >= x)
	    _BEGNS[y] = maxx;
	if (_ENDNS[y] >= x)
	    _ENDNS[y] = _BEGNS[y] > x ? -1 : x-1;

	_CURHASH[y] = x == 0 ? 0 : _NOHASH;

	if (_MARKS != NULL)
	{
	    register	char	*mkp = _MARKS[y];
	    register	int	endx = COLS / BITSPERBYTE + (COLS  %BITSPERBYTE ? 1 : 0);
	    register	int	m = x / BITSPERBYTE + 1;

	    for ( ; m < endx; ++m)
		mkp[m] = 0;
	    mkp += x / BITSPERBYTE;
	    if ((m = x % BITSPERBYTE) == 0)
		*mkp = 0;
	    else
		for (; m < BITSPERBYTE; ++m)
		    *mkp &= ~(1 << m);

	    /* if color terminal, do the same for color marks	*/

	    if (_COLOR_MARKS != NULL)
	    {
	        mkp = _COLOR_MARKS[y];

		m = x / BITSPERBYTE + 1;
		for (; m < endx; ++m)
		     mkp[m] = 0;
		mkp += x / BITSPERBYTE;
		if ((m = x % BITSPERBYTE) == 0)
		     *mkp = 0;
		else
		     for (; m < BITSPERBYTE; ++m)
			 *mkp &= ~(1 << m);
	    }
	}
	return (OK);
    }
    else
    {
	/* update firstch and lastch for the line. */
#ifdef	DEBUG
	if (outf)
	    fprintf(outf, "CLRTOEOL: line %d minx = %d, maxx = %d, firstch = %d, lastch = %d, next firstch %d\n", y, minx, maxx - win->_y[y], win->_firstch[y], win->_lastch[y], win->_firstch[y+1]);
#endif	/* DEBUG */

	if (win->_firstch[y] > x)
	    win->_firstch[y] = x;
	win->_lastch[y] = maxx - 1;
	win->_flags |= _WINCHANGED;
	/* sync with ancestors structures */
	if (win->_sync)
	    wsyncup(win);

	return (win->_immed ? wrefresh(win) : OK);
    }
}
