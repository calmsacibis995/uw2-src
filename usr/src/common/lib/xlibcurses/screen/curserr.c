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

#ident	"@(#)curses:common/lib/xlibcurses/screen/curserr.c	1.5.2.3"
#ident  "$Header: curserr.c 1.2 91/06/26 $"

#include 	"curses_inc.h"

char	*curs_err_strings[] =
{
    "I don't know how to deal with your \"%s\" terminal",
    "I need to know a more specific terminal type than \"%s\"",	/* unknown */
#ifdef	DEBUG
    "malloc returned NULL in function \"%s\"",
#else	/* DEBUG */
    "malloc returned NULL",
#endif	/* DEBUG */
};

void
curserr()
{
    fprintf(stderr, "Sorry, ");
    fprintf(stderr, curs_err_strings[curs_errno], curs_parm_err);
    fprintf(stderr, ".\r\n");
}
