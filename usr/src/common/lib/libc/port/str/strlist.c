/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strlist.c	1.3"

#ifdef __STDC__
        #pragma weak strlist = _strlist
#endif

#include "synonyms.h"
#include <stdarg.h>
#include <string.h>

char *
strlist(char *s1, const char *s2, ...)
{
	va_list ap;

	va_start(ap, s2);
	*s1 = '\0';
	while (s2 != 0)
	{
		while ((*s1 = *s2++) != '\0')
			++s1;
		s2 = va_arg(ap, const char *);
	}
	va_end(ap);
	return s1;	/* points to \0 */
}
