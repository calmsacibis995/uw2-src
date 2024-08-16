/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcscpy.c	1.1"

/*
 * Copy string s2 to s1. S1 must be large enough.
 * Return s1.
 */

#include <wchar.h>

wchar_t *
#ifdef __STDC__
wcscpy(register wchar_t *s1, register const wchar_t *s2)
#else
wcscpy(s1, s2)
register wchar_t *s1;
register const wchar_t *s2;
#endif
{
	wchar_t *os1 = s1;

	while (*s1++ = *s2++)
		;
	return(os1);
}
