/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fgetws.c	1.7"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fgetws = _fgetws
#endif

wchar_t *
#ifdef __STDC__
fgetws(register wchar_t *wp, int sz, FILE *fp) /* get wide char line */
#else
fgetws(wp, sz, fp)register wchar_t *wp; int sz; FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	wchar_t *wp0;
	int ch;

	STDLOCK(&bp->lock);
	READCHK(fp, bp, {wp0 = 0; goto ret;});
	wp0 = wp;
	while (--sz > 0)
	{
		if (--fp->_cnt < 0)
		{
			if ((ch = _ifilbuf(fp)) == EOF)
				break;
			*wp = ch; /* change buffer only if not EOF */
		}
		else
			*wp = *fp->_ptr++;
		if (!ISONEBYTE(*wp) && _inwc(wp, fp, &_ifilbuf) <= 0)
			break;
		if (*wp++ == '\n')
			break;
	}
	if (wp == wp0)
		wp0 = 0;
	else
		*wp = 0;
ret:;
	STDUNLOCK(&bp->lock);
	return wp0;
}
