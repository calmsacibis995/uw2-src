/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fputwc.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fputwc = _fputwc
	#pragma weak putwc = _fputwc
#else
	#pragma weak putwc = fputwc
#endif

wint_t
#ifdef __STDC__
fputwc(wint_t wi, FILE *fp)	/* add wide character to stream */
#else
fputwc(wi, fp)wint_t wi; FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	WRTCHK(fp, bp, {wi = EOF; goto ret;});
	if (_outwc(wi, fp, &_iflsbuf) <= 0)
		wi = EOF;
ret:;
	STDUNLOCK(&bp->lock);
	return wi;
}
