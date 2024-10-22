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

#ident	"@(#)curses:common/lib/xlibcurses/screen/waddch.c	1.9.2.3"
#ident  "$Header: waddch.c 1.2 91/06/27 $"

#include	"curses_inc.h"

/*
 * This routine prints the character in the current position.
 * Think of it as putc.
 */

waddch(win, c)
register	WINDOW	*win;
register	chtype	c;
{
    register	int	x = win->_curx, y = win->_cury;
    register	chtype	rawc = _CHAR(c);
    chtype		rawattrs = _ATTR(c);
    int			rv = OK;
    int			savimmed = win->_immed;
    int			savsync = win->_sync;

    win->_immed = win->_sync = FALSE ;

#ifdef	DEBUG
    if (outf)
	if (c == rawc)
	    fprintf(outf, "'%c'", rawc);
	else
	    fprintf(outf, "'%c' %o, raw %o", c, c, rawc);
#endif	/* DEBUG */

    win->_insmode = FALSE ;
    if(_scrmax > 1 && _mbvalid(win) == ERR)
	goto next;
    if(_mbtrue && ISMBIT(rawc))
	{
	rv = _mbaddch(win,rawattrs,RBYTE(rawc));
	win->_immed = savimmed;
	win->_sync = savsync;
	goto nextw ;
	}

    switch (rawc)
    {
	case '\n':
	    (void) wclrtoeol(win);
	    goto new_line;
	case '\r':
	    goto move_to_begin_line;
        case '\b':
	    if (--x < 0)
move_to_begin_line:
		x = 0;
	    win->_curx = x;
	    win->_flags |= _WINMOVED;
	    goto out_move_only;
	default:
	    if (rawc < ' ' || rawc == _CTRL('?'))
	    {
		if (rawc == '\t')
		{
		    register	int	newx;
		    register	chtype	space = ' ' | rawattrs;

		    if ((newx = x + (TABSIZE - (x % TABSIZE))) > win->_maxx)
			newx = win->_maxx;
		    for ( ; x < newx; x++)
			if (waddch(win, space) == ERR)
			    goto next;
		}
		else
		{
		    if ((waddch(win, (chtype) '^'|rawattrs) == ERR) ||
			(waddch(win, (chtype)_UNCTRL(rawc)|rawattrs) == ERR))
		    {
next :
			rv = ERR;
		    }
		}
		x = win->_curx;
		y = win->_cury;
		win->_immed = savimmed;
		win->_sync = savsync;
		break;
	    }
#ifdef	DEBUG
	    if ((win->_attrs) && outf)
		fprintf(outf, "(attrs %o, %o=>%o)", win->_attrs, c, c | win->_attrs);
#endif	/* DEBUG */

	    /* clear any partial multi-column character */
	    if(_scrmax > 1 && ISMBIT(win->_y[y][x]) &&
		(rv = _mbclrch(win,y,x)) == ERR)
		{
		x = win->_curx;
		y = win->_cury;
		win->_immed = savimmed;
		win->_sync = savsync;
		break;
	    }

	    if ((c = _WCHAR(win, c)|rawattrs) != win->_y[y][x])
	    {
		if (x < win->_firstch[y])
		    win->_firstch[y] = x;
		if (x > win->_lastch[y])
		    win->_lastch[y] = x;
		win->_y[y][x] = c;
#ifdef	_VR3_COMPAT_CODE
		if (_y16update)
		    win->_y16[y][x] = _TO_OCHTYPE (c);
#endif	/* _VR3_COMPAT_CODE */
	    }
	    if (++x == win->_maxx)
	    {
new_line:
		if (y == win->_bmarg)
		{
		    if (wscrl(win, 1) == ERR)
		    {
			rv = ERR;
			if (x == win->_maxx)
			    --x;
#ifdef	DEBUG
			if (outf)
			{
			    int	i;

			    fprintf(outf, "ERR because (%d, %d) > (%d, %d)\n",
				    x, y, win->_maxx, win->_maxy);
			    fprintf(outf, "line: '");
			    for (i=0; i<win->_maxy; i++)
				fprintf(outf, "%c", win->_y[y-1][i]);
			    fprintf(outf, "'\n");
			}
#endif	/* DEBUG */
			break;
		    }
		    else
			savimmed = 1;
		}
		else
		    y++;
		x = 0;
	    }
	    else
		savimmed += 2;
#ifdef	FULLDEBUG
	    if (outf)
		fprintf(outf, "ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
#endif	/* FULLDEBUG */
	    break;
    }
    win->_cury = y;
    win->_curx = x;

nextw:
    /* sync with ancestor structures */
    if (win->_sync)
	wsyncup(win);

    if (savimmed == 3)
	return ((*_quick_ptr)(win, c));

    win->_flags |= _WINCHANGED;

out_move_only:

    return ((savimmed == 1) ? wrefresh(win) : rv);
}
