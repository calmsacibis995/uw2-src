/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/tmpfile.c	1.13.1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "stdiom.h"

FILE *
#ifdef __STDC__
tmpfile(void)	/* opened for update scratch file without file system access */
#else
tmpfile()
#endif
{
#ifdef _REENTRANT
	static StdLock seed_lock;
#endif
	static const char backsp[] = "\b"; /* prevents tmpnam() clashes */
	static const char update[] = "w+";
	static char seed[] = "aaa";
	char tfname[L_tmpnam];
	register char *q;
	FILE *fp;

	STDLOCK(&seed_lock);
	strlist(tfname, _str_tmpdir, seed, backsp, _str_xxxxxx, (char *)0);
	for (q = &seed[0]; *q == 'z'; *q++ = 'a')
		;
	if (*q != '\0')
		++*q;
	STDUNLOCK(&seed_lock);
	mktemp(tfname);
	if (tfname[0] == '\0')
		return 0;
	if ((fp = fopen(tfname, update)) != 0)
		unlink(tfname);
	return fp;
}
