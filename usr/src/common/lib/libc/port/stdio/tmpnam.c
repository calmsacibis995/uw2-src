/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/tmpnam.c	1.11.2.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stdiom.h"

char *
#ifdef __STDC__
tmpnam(char *s)
#else
tmpnam(s)char *s;
#endif
{
#ifdef _REENTRANT
	static StdLock seed_lock;
#endif
	static char seed[] = "aaa";
	static char str[L_tmpnam];
	register char *p, *q;

	if ((p = s) == 0)
		p = str;
	STDLOCK(&seed_lock);
	strlist(p, _str_tmpdir, seed, _str_xxxxxx, (char *)0);
	for (q = &seed[0]; *q == 'z'; *q++ = 'a')
		;
	if (*q != '\0')
		++*q;
	STDUNLOCK(&seed_lock);
	return mktemp(p);
}
