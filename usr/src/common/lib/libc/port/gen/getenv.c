/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getenv.c	1.10"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>

extern char **environ;

char *
#ifdef __STDC__
getenv(const char *name)
#else
getenv(name)const char *name;
#endif
{
	register const char *q;
	register int ch;
	register char *s;
	register char **p;

	if ((p = environ) != 0 && (s = *p) != 0)
	{
		q = name;
		ch = *q;
		do
		{
			if (*s != ch)
				continue;
			while (*++q == *++s)
			{
				/*
				* For compatibility, allow "foo=bar" names,
				* but only search for "foo".
				*/
				if (*q == '=')
					return s + 1;
			}
			if (*q == '\0' && *s == '=')
				return s + 1;
			q = name;
		} while ((s = *++p) != 0);
	}
	return 0;
}
