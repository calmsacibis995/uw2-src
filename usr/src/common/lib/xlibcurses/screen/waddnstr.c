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

#ident	"@(#)curses:common/lib/xlibcurses/screen/waddnstr.c	1.4.2.6"
#ident  "$Header: waddnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Copyright (c) 1993, 1994 Intel Corporation
 * All Rights Reserved
 *
 * INTEL CORPORATION PROPRIETARY INFORMATION
 *
 * This software is supplied to Novell under the terms of a license
 * agreement with Intel Corporation and may not be copied nor
 * disclosed except in accordance with the terms of that agreement.
 */

/* This routine adds a string starting at (_cury, _curx) */

waddnstr(win, str, i)
register	WINDOW	*win; 
register	char	*str;
int		i;
{
    register	chtype	ch;
    register	int	maxx_1 = win->_maxx - 1, cury = win->_cury,
			curx = win->_curx;
    register	chtype	**_y = win->_y;
    int			savimmed = win->_immed, savsync = win->_sync;
    int			rv = OK;
    int			pflag;

#ifdef	DEBUG
    if (outf)
    {
	if (win == stdscr)
	    fprintf(outf, "waddnstr(stdscr, ");
	else
	    fprintf(outf, "waddnstr(%o, ", win);
	fprintf(outf, "\"%s\")\n", str);
    }
#endif	/* DEBUG */

    /*
     * Fast path to handle common case of:
     *	no integer string limit
     *	_scrmax is zero, so _mbvalid does not need to be evaluated
     *	_mbtrue is zero, so ISMBIT does not need to be evaluated
     *	BNKCHAR and _bkgd are the same, so the WCHAR macro and its
     *		attendant check for blank does not need to be used
     *
     * If any control characters are encountered, or the string takes
     * us to the end of the line, then go to the slow-path case at
     * the label "toohard".
     */

    if ((i < 0) && (_scrmax <= 1) && !_mbtrue &&
#ifdef	_VR3_COMPAT_CODE
	    !(_y16update) &&
#endif	/* _VR3_COMPAT_CODE */
	    (_BLNKCHAR == win->_bkgd)) {
	chtype l_attr = win->_attrs;
	unsigned char *scn = (unsigned char *) str;
	chtype *_yy = _y[cury]+curx;

	if (curx < win->_firstch[cury])
	    win->_firstch[cury] = curx;
	while (ch = *scn++) {
	    if ((ch < ' ') || (ch == _CTRL('?')) || (++curx > maxx_1))
		goto toohard;
	    *_yy++ = ch | l_attr;
	}
	if (curx > win->_lastch[cury])
	    win->_lastch[cury] = curx;
	goto done;
    }

toohard:

    curx = win->_curx;

    /* throw away any current partial character */
    win->_nbyte = -1;
    win->_insmode = FALSE;
    pflag = 1;

    win->_immed = win->_sync = FALSE;

    if (i < 0)
	i = MAXINT;

    while ((ch = (unsigned char) *str) && (i-- > 0))
    {
	if(pflag == 1)
		{
		if(_scrmax > 1 && (rv = _mbvalid(win)) == ERR)
			break;
		curx = win->_curx;
		cury = win->_cury;
		}
	if(_mbtrue && ISMBIT(ch))
		{
		register int m,k,ty;
		chtype	     c;
		/* make sure we have the whole character */
		c = RBYTE(ch);
		ty = TYPE(c);
		m = cswidth[ty] - (ty == 0 ? 1 : 0);
		for(k = 1; str[k] != '\0' && k <= m; ++k)
			if (!ISMBIT((unsigned char) str[k]))
				break;
		if(k <= m)
			break;
		if (m > i)
			break;
		for(k = 0; k <= m; ++k, ++str)
		{
			if((rv = _mbaddch(win,A_NORMAL,RBYTE((unsigned char) *str))) == ERR)
				goto done;
			if (k > 0)
				i--;
		}
		pflag = 1;
		cury = win->_cury;
		curx = win->_curx;
		continue;
		}

	/* do normal characters while not next to edge */
	if ((ch >= ' ') && (ch != _CTRL('?')) && (curx < maxx_1))
	{
	    if(_scrmax >1 && ISMBIT(_y[cury][curx])
			&& (rv = _mbclrch(win,cury,curx)) == ERR)
		break;
	    if (curx < win->_firstch[cury])
		win->_firstch[cury] = curx;
	    if (curx > win->_lastch[cury])
		win->_lastch[cury] = curx;
	    ch = _WCHAR(win, ch);
	    _y[cury][curx] = ch;
#ifdef	_VR3_COMPAT_CODE
	    if (_y16update)
		win->_y16[cury][curx] = _TO_OCHTYPE (ch);
#endif	/* _VR3_COMPAT_CODE */
	    curx++;
	    pflag = 0;
	}
	else
	{
	    win->_curx = curx;
	    /* found a char that is too tough to handle above */
	    if (waddch(win, ch) == ERR)
	    {
		rv = ERR;
		break;
	    }
	    cury = win->_cury;
	    curx = win->_curx;
	    pflag = 1;
	}
	str++;
	win->_curx = curx;
    }

done :
    win->_curx = curx;
    win->_flags |= _WINCHANGED;
    win->_sync = savsync;
    if (win->_sync)
	wsyncup(win);

    return ((win->_immed = savimmed) ? wrefresh(win) : rv);
}
