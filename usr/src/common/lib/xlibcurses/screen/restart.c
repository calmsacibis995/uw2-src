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

#ident	"@(#)curses:common/lib/xlibcurses/screen/restart.c	1.9.2.3"
#ident  "$Header: restart.c 1.2 91/06/26 $"

#include	"curses_inc.h"

/*
 * This is useful after saving/restoring memory from a file (e.g. as
 * in a rogue save game).  It assumes that the modes and windows are
 * as wanted by the user, but the terminal type and baud rate may
 * have changed.
 */

extern	char	_called_before;

restartterm(term, filenum, errret)
char	*term;
int	filenum;	/* This is a UNIX file descriptor, not a stdio ptr. */
int	*errret;
{
    int	saveecho = SP->fl_echoit;
    int	savecbreak = cur_term->_fl_rawmode;
    int	savenl;

#ifdef	SYSV
    savenl = PROGTTY.c_iflag & ONLCR;
#else	/* SYSV */
    savenl = PROGTTY.sg_flags & CRMOD;
#endif	/* SYSV */

    _called_before = 0;
    (void) setupterm(term, filenum, (int *) 0);

    /* Restore curses settable flags, leaving other stuff alone. */
    SP->fl_echoit = saveecho;

    nocbreak();
    noraw();
    if (savecbreak == 1)
	cbreak();
    else
	if (savecbreak == 2)
	    raw();

    if (savenl)
	nl();
    else
	nonl();

    reset_prog_mode();

    LINES = SP->lsize;
    COLS = columns;
    return (OK);
}
