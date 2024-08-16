/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsncmp.c	1.2"

/*
 * Compare strings (at most n characters)
 * 	returns:  s1>s2: >0  s1==s2: 0  s1<s2: <0
*/

#include <wcharm.h>

int
#ifdef __STDC__
wcsncmp(const wchar_t *ws1, const wchar_t *ws2, register size_t n)
#else
wcsncmp(ws1, ws2, n)
const wchar_t *ws1, *ws2;
register size_t n;
#endif
{
	register const wuchar_t *s1 = (wuchar_t *) ws1, 
			        *s2 = (wuchar_t *) ws2;

	if (s1 == s2)
		return(0);

	n++;
	while (--n != 0 && *s1 == *s2++)
		if (*s1++ == 0)
			return(0);
	return((n == 0) ? 0 : (*s1 - *--s2));
}
