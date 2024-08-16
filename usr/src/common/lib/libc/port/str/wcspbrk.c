/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcspbrk.c	1.1"

/*
 * Return ptr to first occurance of any character from 'brkset'
 * in the wchar_t array 'string'; NULL if none exists.
 */

#include <wchar.h>

wchar_t *
#ifdef __STDC__
wcspbrk(register const wchar_t *string, register const wchar_t *brkset)
#else
wcspbrk(string, brkset)
register const wchar_t *string, *brkset;
#endif
{
	register const wchar_t *p;

	do {
		for (p = brkset; *p && *p != *string; ++p)
			;
		if (*p)
			return((wchar_t *)string);
	} while (*string++);
	return(0);
}
