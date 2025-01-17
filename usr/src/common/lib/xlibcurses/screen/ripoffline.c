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

#ident	"@(#)curses:common/lib/xlibcurses/screen/ripoffline.c	1.8.2.3"
#ident  "$Header: ripoffline.c 1.2 91/06/26 $"
/*
 * This routine is used by initialization routines. It sets it up
 * such that a line is removed from the user's screen by initscr. This
 * function must be called BEFORE initscr. It works by leaving a cookie 
 * which tells initscr to reduce the size of stdscr by one for each line
 * ripped off. This routine has been generalized so that other applications
 * can make use of it in a straightforward manner.
 */

#include "curses_inc.h"

static	struct _ripdef
{
    int line;
    int (*initfunction)();
} _ripstruct[5];

static char _ripcounter;

static	void
_init_rip_func ()

{
    int     i,flag;

    for (i = 0; i < _ripcounter; i++)
    {
	LINES = --SP->lsize;
/*
 * We don't need to check for newwin returning NULL because even if
 * we did and broke from the for loop, the application's initfunction
 * would not be called and they would have a NULL window pointer.  Their
 * code would then blow up if they don't check it anyway.  Therefore,
 * we can send in the newwin and their code has to check for NULL in either
 * case.
 *
 * NOTE:  The application has the responsibilty to do a delwin !!!
 */
	(*_ripstruct[i].initfunction) (newwin(1, COLS, ((flag = _ripstruct[i].line) > 0) ? 0 : LINES, 0), COLS);
	if (flag > 0)
	    SP->Yabove++;
    }
    _ripcounter = 0;
}

ripoffline (line, initfunction)
int line;
int (*initfunction)();
{
    if (_ripcounter < 5)
    {
	_ripstruct[_ripcounter].line = line;
	_ripstruct[_ripcounter++].initfunction = initfunction;
    }
    _rip_init = _init_rip_func;
    return (OK);
}
