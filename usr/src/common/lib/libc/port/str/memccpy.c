/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/memccpy.c	1.3.1.6"
/*LINTLIBRARY*/
/*
 * Copy s0 to s, stopping if character c is copied. Copy no more than n bytes.
 * Return a pointer to the byte after character c in the copy,
 * or NULL if c is not found in the first n bytes.
 */
#ifdef __STDC__
	#pragma weak memccpy = _memccpy
#endif
#include "synonyms.h"
#include <stddef.h>

VOID *
memccpy(s, s0, c, n)
VOID *s;
const VOID *s0;
register int c;
register size_t n;
{
	if (n != 0) {
	    register char *s1 = s;
	    register const char *s2 = s0;
	    do {
		if ((*s1++ = *s2++) == c)
			return (s1);
	    } while (--n != 0);
	}
	return (NULL);
}
