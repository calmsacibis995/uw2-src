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

#ident	"@(#)curses:common/lib/xlibcurses/screen/flushinp.c	1.6.2.3"
#ident  "$Header: flushinp.c 1.2 91/06/26 $"

#include	"curses_inc.h"

flushinp()
{
#ifdef	DEBUG
    if (outf)
	fprintf(outf, "flushinp(), file %x, SP %x\n", cur_term->Filedes, SP);
#endif	/* DEBUG */

#ifdef	SYSV
    (void) ioctl(cur_term -> Filedes, TCFLSH, 0);
#else	/* SYSV */
    /* for insurance against someone using their own buffer: */
    (void) ioctl(cur_term -> Filedes, TIOCGETP, &(PROGTTY));

    /*
     * SETP waits on output and flushes input as side effect.
     * Really want an ioctl like TCFLSH but Berkeley doesn't have one.
     */
    (void) ioctl(cur_term -> Filedes, TIOCSETP, &(PROGTTY));
#endif	/* SYSV */

    /*
     * Get rid of any typeahead which was read().
     * Leave characters which were ungetch()'d.
     */
    cur_term->_chars_on_queue = cur_term->_ungotten;

    /*
     * Have to doupdate() because, if we have stopped output due to
     * typeahead, now that typeahead is gone, so we had better catch up.
     */
    if (_INPUTPENDING)
	(void) doupdate();
    return (OK);
}
