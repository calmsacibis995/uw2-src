/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsncpy.c	1.1"

/*
 * Copy s2 to s1, truncating or null-padding to always copy n characters.
 * Return s1.
 */

#include <wchar.h>

wchar_t *
#ifdef __STDC__
wcsncpy(register wchar_t *s1, register const wchar_t *s2, size_t n)
#else
wcsncpy(s1, s2, n)
register wchar_t *s1;
register const wchar_t *s2;
register size_t n;
#endif
{
	wchar_t *os1 = s1;

	n++;
	while ((--n != 0) && ((*s1++ = *s2++) != 0))
		;
	if (n != 0)
		while (--n != 0)
			*s1++ = 0;
	return(os1);
}
