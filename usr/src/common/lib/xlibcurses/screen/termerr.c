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

#ident	"@(#)curses:common/lib/xlibcurses/screen/termerr.c	1.6.2.3"
#ident  "$Header: termerr.c 1.2 91/06/27 $"

#include 	"curses_inc.h"
#include <signal.h>   /* use this file to determine if this is SVR4.0 system */

char	*term_err_strings[] =
{
    "",
#ifdef SIGSTOP	/* SVR4.0 and beyond */
    "/usr/share/lib/terminfo is unaccessible",
#else
    "/usr/lib/terminfo is unaccessible",
#endif
    "I don't know anything about your \"%s\" terminal",
    "corrupted terminfo entry",
    "terminfo entry too long",
    "TERMINFO pathname for device exceeds 512 characters",
#ifdef	DEBUG
    "malloc returned NULL in function \"%s\"",
#else	/* DEBUG */
    "malloc returned NULL",
#endif	/* DEBUG */
    "terminfo file for \"%s\" terminal is not readable",
};

void
termerr()
{
    (void) fprintf(stderr, "Sorry, ");
    (void) fprintf(stderr, term_err_strings[term_errno], term_parm_err);
    (void) fprintf(stderr, ".\r\n");
}
