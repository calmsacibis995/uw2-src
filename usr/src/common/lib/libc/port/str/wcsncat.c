/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsncat.c	1.1"

/*
 * Concatenate s2 on the end of s1. S1's space must be large enough.
 * At most n characters are moved.
 * return s1.
 */

#include <wchar.h>

wchar_t *
#ifdef __STDC__
wcsncat(register wchar_t *s1, register const wchar_t *s2, register size_t n)
#else
wcsncat(s1, s2, n)
register wchar_t *s1;
register const wchar_t *s2;
register size_t n;
#endif
{
	wchar_t *os1 = s1;

	while (*s1++) /* find end of s1 */
		;
	++n;
	--s1;
	while (*s1++ = *s2++) /* copy s2 to s1 */
		if (--n == 0) {  /* at most n chars */
			*--s1 = 0;
			break;
		}
	return(os1);
}
