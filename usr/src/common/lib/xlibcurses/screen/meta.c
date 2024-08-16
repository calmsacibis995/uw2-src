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

#ident	"@(#)curses:common/lib/xlibcurses/screen/meta.c	1.5.2.4"
#ident  "$Header: meta.c 1.2 91/06/26 $"

#include	"curses_inc.h"

/* TRUE => all 8 bits of input character should be passed through. */

_meta(bf)
bool	bf;
{
#ifdef __STDC__
    extern  int     _outch(int);
#else
    extern  int     _outch();
#endif
    /*
     * Do the appropriate fiddling with the tty driver to make it send
     * all 8 bits through.  On SYSV this means clearing ISTRIP, on
     * V7 you have to resort to RAW mode.
     */
#ifdef	SYSV
    if (bf)
	PROGTTY.c_iflag &= ~ISTRIP;
    else
	PROGTTY.c_iflag |= ISTRIP;
    reset_prog_mode();
#else	/* SYSV */
    if (bf)
	raw();
    else
	noraw();
#endif	/* SYSV */

    /* Do whatever is needed to put the terminal into meta-mode. */

    if (SP->fl_meta = bf)
	tputs(meta_on, 1, _outch);
    else
	tputs(meta_off, 1, _outch);
    (void) fflush(SP->term_file);
    return (OK);
}
