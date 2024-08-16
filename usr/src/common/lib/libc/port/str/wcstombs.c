/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcstombs.c	1.6"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <errno.h>
#include "wcharm.h"

size_t
#ifdef __STDC__
wcstombs(char *d, const wchar_t *s, size_t n)
#else
wcstombs(d, s, n)char *d; const wchar_t *s; size_t n;
#endif
{
	size_t sz, total = 0;

	while ((sz = _xwcstombs(d, &s, n)) != 0)
	{
		if (sz == (size_t)-1)
			return (size_t)-1;
		total += sz;
		if (s == 0 || errno == 0)
			break;
		d += sz;
		n -= sz;
	}
	if (s == 0 && d != 0 && sz != n)	/* reached end-of-string */
		d[sz] = '\0';
	return total;
}
