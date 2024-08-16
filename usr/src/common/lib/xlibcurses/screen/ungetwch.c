/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/ungetwch.c	1.2.2.4"
#ident  "$Header: ungetwch.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
**	Push a process code back into the input stream
*/
ungetwch(code)
wchar_t	code;
	{
	int	i, n;
	char	buf[CSMAX];

	n = wctomb(buf, code & TRIM);
	for(i = n-1; i >= 0; --i)
		if(ungetch((unsigned char) buf[i]) == ERR)
		{
		/* remove inserted characters */
		for(i = i+1; i < n; ++i)
			tgetch(0);
		return ERR;
		}

	return OK;
	}
