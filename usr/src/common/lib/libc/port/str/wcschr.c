/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcschr.c	1.1"

/*
 * Return the ptr in sp at which the character c appears;
 * Null if not found.
 */

#include <wchar.h>

wchar_t *
#ifdef __STDC__
wcschr(register const wchar_t *sp, register wint_t c)
#else
wcschr(sp, c)
register const wchar_t *sp;
register wint_t c;
#endif
{
	do {
		if (*sp == c)
			return((wchar_t *)sp); /* found c in sp */
	} while (*sp++);
	return( 0 ); /* c not found */
}
