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

#ident	"@(#)curses:common/lib/xlibcurses/screen/draino.c	1.7.2.4"
#ident  "$Header: draino.c 1.2 91/06/26 $"

#include	"curses_inc.h"

/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */

#define	NAPINTERVAL	100
/*
 * Wait until the output has drained enough that it will only take
 * ms more milliseconds to drain completely.
 * Needs Berkeley TIOCOUTQ ioctl.  Returns ERR if impossible.
 */
draino(ms)
int	ms;
{
#ifdef	TIOCOUTQ
#define	_DRAINO
    /* number of chars = that many ms */
    long	ncneeded;

    /* 10 bits/char, 1000 ms/sec, baudrate in bits/sec */
    ncneeded = SP->baud * ms / (10 * 1000);
    while (TRUE)
    {
	int	rv;		/* ioctl return value */
	int	ncthere = 0;/* number of chars actually in output queue */

	rv = ioctl(cur_term->Filedes, TIOCOUTQ, &ncthere);
#ifdef	DEBUG
	if (outf)
	    fprintf(outf, "draino: rv %d, ncneeded %d, ncthere %d\n",
		rv, ncneeded, ncthere);
#endif	/* DEBUG */
	if (rv < 0)
	    return (ERR);	/* ioctl didn't work */
	if (ncthere <= ncneeded)
	    return (OK);
	napms(NAPINTERVAL);
    }
#else	/* TIOCOUTQ */

#ifdef	TCSETSW
#define	_DRAINO
	/*
	 * SYSV simulation - waits until the entire queue is empty,
	 * then sets the state to what it already is (e.g. no-op).
	 * Unfortunately this only works if ms is zero.
	 */
    if (ms <= 0)
    {
	(void) ioctl(cur_term->Filedes, TCSETSW, &PROGTTY);
	return (OK);
    }
    else
	return (ERR);
#endif	/* TCSETSW */
#endif	/* TIOCOUTQ */

#ifndef	_DRAINO
    /*
     * No way to fake it, so we return failure.
     * Used #else to avoid warning from compiler about unreached stmt
     */
    return (ERR);
#endif	/* _DRAINO */
}
