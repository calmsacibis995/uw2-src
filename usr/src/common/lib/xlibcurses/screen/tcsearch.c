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

#ident	"@(#)curses:common/lib/xlibcurses/screen/tcsearch.c	1.3.2.4"
#ident  "$Header: tcsearch.c 1.2 91/06/27 $"
_tcsearch(cap, offsets, names, size, n)
char	*cap;
short	offsets[];
char	*names[];
int	size, n;
{
    register	int	l = 0, u = size - 1;
    int		m, cmp;

    while (l <= u)
    {
	m = (l + u) / 2;

	if (!names[offsets[m]]) return(-1);

	cmp = ((n == 0) ? strcmp(cap, names[offsets[m]]) :
			  strncmp(cap, names[offsets[m]], n));

	if (cmp < 0)
	    u = m - 1;
	else
	    if (cmp > 0)
		l = m + 1;
	    else
		return (offsets[m]);
    }
    return (-1);
}
