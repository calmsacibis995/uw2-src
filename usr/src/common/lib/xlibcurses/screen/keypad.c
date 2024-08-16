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

#ident	"@(#)curses:common/lib/xlibcurses/screen/keypad.c	1.8.2.4"
#ident  "$Header: keypad.c 1.2 91/06/26 $"

#include	"curses_inc.h"

/* TRUE => special keys should be passed as a single value by getch. */

keypad(win, bf)
WINDOW	*win;
bool	bf;
{
#ifdef __STDC__
    extern  int     _outch(int);
#else
    extern  int     _outch();
#endif
    /*
     * Since _use_keypad is a bit and not a bool, we must check
     * bf, in case someone uses an odd number instead of 1 for TRUE
     */

    win->_use_keypad = (bf) ? TRUE : FALSE;
    if (bf && (!SP->kp_state))
    {
	tputs(keypad_xmit, 1, _outch);
	(void) fflush(SP->term_file);
	SP->kp_state = TRUE;
	return (setkeymap());
    }
    return (OK);
}
