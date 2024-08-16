/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fputws.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <limits.h>
#endif
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fputws = _fputws
#endif

int
#ifdef __STDC__
fputws(register const wchar_t *wp, FILE *fp)	/* put wide chars on stream */
#else
fputws(wp, fp)register const wchar_t *wp; FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	register size_t total;
	register int n;

	STDLOCK(&bp->lock);
	WRTCHK(fp, bp, {n = EOF; goto ret;});
	total = 0;
	while (*wp != 0)
	{
		if ((n = _outwc((wint_t)*wp++, fp, &_iflsbuf)) <= 0)
		{
			n = EOF;
			goto ret;
		}
		total += n;
	}
	if (total > INT_MAX)
		total = INT_MAX;
	n = total;
ret:;
	STDUNLOCK(&bp->lock);
	return n;
}
