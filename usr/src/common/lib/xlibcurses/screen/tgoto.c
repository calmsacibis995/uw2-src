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

#ident	"@(#)curses:common/lib/xlibcurses/screen/tgoto.c	1.5.2.3"
#ident  "$Header: tgoto.c 1.2 91/06/27 $"
/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
extern	char	*tparm();

char	*
tgoto(cap, col, row)
char	*cap;
int	col, row;
{
    char	*cp;

    cp = tparm(cap, row, col);
    return (cp);
}
