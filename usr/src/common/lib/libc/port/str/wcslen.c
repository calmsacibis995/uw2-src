/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcslen.c	1.1"

/*
 * Returns the number of non-NULL characters in s.
 */

#include <wchar.h>

size_t
#ifdef __STDC__
wcslen(const wchar_t *s)
#else
wcslen(s)
register const wchar_t *s;
#endif
{
	register const wchar_t *s0 = s + 1;

	while (*s++)
		;
	return(s - s0);
}
