/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsstr.c	1.1"

#include <wchar.h>

/*
	wcsstr() locates the first occurrence in the string as1 of
	the sequence of characters (excluding the terminating null
	character) in the string as2. wcsstr() returns a pointer 
	to the located string, or a null pointer if the string is
	not found. If as2 is "", the function returns as1.

	XPG4 has the same function but with a different name based
	on an earlier draft of the ISO MSE. This is handled by the 
	header file wchar.h
*/

wchar_t *
#ifdef __STDC__
wcsstr(const wchar_t *as1, const wchar_t *as2)
#else
wcsstr(as1, as2)
const wchar_t *as1;
const wchar_t *as2;
#endif
{
	register const wchar_t *s1,*s2;
	register wchar_t c;
	register const wchar_t *tptr;

	s1 = as1;
	s2 = as2;

	if (s2 == 0 || *s2 == '\0')
		return((wchar_t *)s1);
	c = *s2;

	while (*s1)
		if (*s1++ == c) {
			tptr = s1;
			while ((c = *++s2) == *s1++ && c) ;
			if (c == 0)
				return((wchar_t *)tptr - 1);
			s1 = tptr;
			s2 = as2;
			c = *s2;
		}
	 return(0);
}
