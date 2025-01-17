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

#ident	"@(#)curses:common/lib/xlibcurses/screen/tstp.c	1.11.2.3"
#ident  "$Header: tstp.c 1.2 91/06/27 $"
#include	<signal.h>
#include	"curses_inc.h"


/* handle stop and start signals */

#ifdef	SIGTSTP
void
_tstp(dummy)
int dummy;
{
#ifdef	DEBUG
    if (outf)
	(void) fflush(outf);
#endif	/* DEBUG */
    curscr->_attrs = A_ATTRIBUTES;
    (void) endwin();
    (void) fflush(stdout);
    kill(0, SIGTSTP);
    (void) signal(SIGTSTP, _tstp);
    fixterm();
    /* changed ehr3 SP->doclear = 1; */
    curscr->_clear = TRUE;
    (void) wrefresh(curscr);
}
#endif	/* SIGTSTP */

void
_ccleanup(signo)
int	signo;
{
    (void) signal(signo, SIG_IGN);

    /*
     * Fake curses into thinking that all attributes are on so that
     * endwin will turn them off since the <BREAK> key may have interrupted
     * the sequence to turn them off.
     */

    curscr->_attrs = A_ATTRIBUTES;
    (void) endwin();
#ifdef	DEBUG
    fprintf(stderr, "signal %d caught. quitting.\n", signo);
#endif	/* DEBUG */
    if (signo == SIGQUIT)
	(void) abort();
    else
	exit(1);
}
