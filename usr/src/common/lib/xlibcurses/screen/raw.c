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

#ident	"@(#)curses:common/lib/xlibcurses/screen/raw.c	1.12.2.3"
#ident  "$Header: raw.c 1.2 91/06/26 $"
/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

raw()
{
#ifdef SYSV
    /* Disable interrupt characters */
    PROGTTY.c_lflag &= ~(ISIG|ICANON);
    PROGTTY.c_cc[VMIN] = 1;
    PROGTTY.c_cc[VTIME] = 0;
    PROGTTY.c_iflag &= ~IXON;
#else
    PROGTTY.sg_flags &= ~CBREAK;
    PROGTTY.sg_flags |= RAW;
#endif

#ifdef DEBUG
# ifdef SYSV
    if (outf)
	fprintf(outf, "raw(), file %x, iflag %x, cflag %x\n",
	    cur_term->Filedes, PROGTTY.c_iflag, PROGTTY.c_cflag);
# else
    if (outf)
	fprintf(outf, "raw(), file %x, flags %x\n",
	    cur_term->Filedes, PROGTTY.sg_flags);
# endif /* SYSV */
#endif

    if (!needs_xon_xoff)
	xon_xoff = 0;	/* Cannot use xon/xoff in raw mode */
    cur_term->_fl_rawmode = 2;
    cur_term->_delay = -1;
    reset_prog_mode();
#ifdef FIONREAD
    cur_term->timeout = 0;
#endif /* FIONREAD */
    return (OK);
}
