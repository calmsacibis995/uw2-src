/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)curses:common/lib/xlibcurses/screen/wcstombs.c	1.2.2.2"
#ident  "$Header: wcstombs.c 1.2 91/06/27 $"
/*LINTLIBRARY*/

#include <widec.h>
#include "synonyms.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

size_t
_curs_wcstombs(s, pwcs, n)
char *s;
const wchar_t *pwcs;
size_t n;
{
	int	val = 0;
	int	total = 0;
	char	temp[MB_LEN_MAX];
	int	i;

	for (;;) {
		if (*pwcs == 0) {
			*s = '\0';
			break;
		}
		if ((val = _curs_wctomb(temp, *pwcs++)) == -1)
			return(val);
		if ((total += val) > n) {
			total -= val;
			break;
		}
		for (i=0; i<val; i++)
			*s++ = temp[i];
	}
	return(total);
}
