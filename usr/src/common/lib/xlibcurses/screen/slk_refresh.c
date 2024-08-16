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

#ident	"@(#)curses:common/lib/xlibcurses/screen/slk_refresh.c	1.6.2.4"
#ident  "$Header: slk_refresh.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Update the soft-label window. */

slk_refresh()
{
    if (_slk_update())
    {
	return (wrefresh (SP->slk->_win));
    }
    return (0);
}

/* Update soft labels. Return TRUE if a window was updated. */

_slk_update()
{
#ifdef __STDC__
    extern  int     _outch(int);
#else
    extern  int     _outch();
#endif
    register	WINDOW	*win;
    register	SLK_MAP	*slk;
    register	int	i;

    if ((slk = SP->slk) == NULL || (slk->_changed != TRUE))
	return (FALSE);

    win = slk->_win;
    for (i = 0; i < slk->_num; ++i)
	if (slk->_lch[i])
	{
	    if (win)	
		(void) mvwaddstr(win, 0, slk->_labx[i], slk->_ldis[i]);
	    else
		_PUTS(tparm(plab_norm, i + 1, slk->_ldis[i]), 1);

	    slk->_lch[i] = FALSE;
	}
    if (!win)
    {
	_PUTS(label_on, 1);
	/*
	 * Added an fflush because if application code calls a slk_refresh
	 * or a slk_noutrefresh and a doupdate nothing will get flushed
	 * since this information is not being kept in curscr or _virtscr.
	 */
	 (void) fflush (SP->term_file);
    }

    slk->_changed = FALSE;

    return (win ? TRUE : FALSE);
}
