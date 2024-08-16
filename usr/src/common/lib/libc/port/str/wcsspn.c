/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsspn.c	1.1"

/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters from charset.
 */

#include <wchar.h>

size_t
#ifdef __STDC__
wcsspn(const wchar_t *string, register const wchar_t *charset)
#else
wcsspn(string, charset)
const wchar_t *string;
register const wchar_t *charset;
#endif
{
	register const wchar_t *p, *q;

	for (q = string; *q; ++q) {
		for (p = charset; *p && *p != *q; ++p)
			;
		if (*p == 0)
			break;
	}
	return(q - string);
}
