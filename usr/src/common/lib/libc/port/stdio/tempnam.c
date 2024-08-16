/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/tempnam.c	1.7.3.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak tempnam = _tempnam
#endif

static char *
#ifdef __STDC__
trydir(const char *dir, const char *pfx)
#else
trydir(dir, pfx)const char *dir, *pfx;
#endif
{
#ifdef _REENTRANT
	static StdLock seed_lock;
#endif
	static char seed[] = "AAA";
	register const char *p;
	register char *q;
	char *str;
	size_t len;

	if (dir[0] == '\0' || access(dir, W_OK | X_OK | EFF_ONLY_OK) != 0)
		return 0;
	/*
	* Trim off trailing slashes and copy into allocated array.
	*/
	p = &dir[strlen(dir)];
	while (p > dir && *--p == '/')
		;
	len = p - dir + 1;
	if ((str = (char *)malloc(len + 1 + 5 + 3 + 6 + 1)) == 0)
		return 0;
	strncpy(str, dir, len);
	/*
	* Add /, at most 5 bytes of the optional prefix, seed, and Xs.
	*/
	str[len++] = '/';
	if (pfx == 0)
		str[len] = '\0';
	else
	{
		strncpy(&str[len], pfx, (size_t)5);
		str[len + 5] = '\0';
	}
	STDLOCK(&seed_lock);
	strcat(str, seed);
	for (q = &seed[0]; *q == 'Z'; *q++ = 'A')
		;
	if (*q != '\0')
		++*q;
	STDUNLOCK(&seed_lock);
	strcat(str, _str_xxxxxx);
	mktemp(str);
	if (str[0] == '\0')
	{
		free((void *)str);
		return 0;
	}
	return str;
}

char *
#ifdef __STDC__
tempnam(const char *dir, const char *pfx)
#else
tempnam(dir, pfx)const char *dir, *pfx;
#endif
{
	static const char TMPDIR[] = "TMPDIR";
	static const char tmp[] = "/tmp";
	char *p;

	if ((p = getenv(TMPDIR)) == 0 || (p = trydir(p, pfx)) == 0)
		if (dir == 0 || (p = trydir(dir, pfx)) == 0)
			if ((p = trydir(_str_tmpdir, pfx)) == 0)
				p = trydir(tmp, pfx);
	return p;
}
