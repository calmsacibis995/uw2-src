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

#ident	"@(#)curses:common/lib/xlibcurses/screen/iexpand.c	1.3.2.4"
#ident  "$Header: iexpand.c 1.2 91/06/26 $"

#include <stdio.h>
#include "print.h"

/*
    Convert a character, making appropriate changes to make it printable
    for a terminfo source entry. Change escape to \E, tab to \t, backspace
    to \b, formfeed to \f, newline to \n, and return to \r. Change other
    control characters into ^X notation. Change meta characters into octal
    (\nnn) notation. Also place a backslash in front of commas (,),
    carets (^), and backslashes (\). Return the number of characters printed.
*/

#define BACKSLASH	'\\'
#define BACKBACK	"\\\\"

static char retbuffer[1024];

/*
    Expand a string taking terminfo sequences into consideration.
*/
char *iexpand(string)
char *string;
{
    register int c;
    register char *retptr = retbuffer;

    retbuffer[0] = '\0';
    while (c = *string++)
        {
	/* should check here to make sure that there is enough room */
	/* in retbuffer and realloc it if necessary. */
	c &= 0377;
	/* we ignore the return value from sprintf because BSD/V7 */
	/* systems return a (char *) rather than an int. */
	if (c >= 0177)
	    { (void) sprintf (retptr, "\\%.3o", c); retptr += 4; }
	else if (c == '\033')
	    { (void) sprintf (retptr, "\\E"); retptr += 2; }
	else if (c == '\t')
	    { (void) sprintf (retptr, "\\t"); retptr += 2; }
	else if (c == '\b')
	    { (void) sprintf (retptr, "\\b"); retptr += 2; }
	else if (c == '\f')
	    { (void) sprintf (retptr, "\\f"); retptr += 2; }
	else if (c == '\n')
	    { (void) sprintf (retptr, "\\n"); retptr += 2; }
	else if (c == '\r')
	    { (void) sprintf (retptr, "\\r"); retptr += 2; }
	else if (c == ',')
	    { (void) sprintf (retptr, "\\,"); retptr += 2; }
	else if (c == '^')
	    { (void) sprintf (retptr, "\\^"); retptr += 2; }
	else if (c == BACKSLASH)
	    { (void) sprintf (retptr, BACKBACK); retptr += 2; }
	else if (c == ' ')
	    { (void) sprintf (retptr, "\\s"); retptr += 2; }
	else if (c < ' ')
	    { (void) sprintf (retptr, "^%c", c ^ 0100); retptr += 2; }
	else
	    { (void) sprintf (retptr, "%c", c); retptr++; }
	}
    *retptr = '\0';
    return retbuffer;
}

/*
    Print out a string onto a stream, changing unprintables into 
    terminfo printables.
*/
void tpr (stream, string)
register FILE *stream;
register char *string;
{
    if (string != NULL)
	(void) fprintf (stream, "%s", iexpand (string));
}
