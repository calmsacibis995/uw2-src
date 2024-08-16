/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fgetwc.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fgetwc = _fgetwc
	#pragma weak getwc = _fgetwc
#else
	#pragma weak getwc = fgetwc
#endif

wint_t
#ifdef __STDC__
fgetwc(FILE *fp)	/* return next wide character from stream */
#else
fgetwc(fp)FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	wint_t wi;

	STDLOCK(&bp->lock);
	if ((fp->_flag & _IOREAD) == 0)
	{
		if ((fp->_flag & _IORW) == 0)
		{
			errno = EBADF;
			wi = EOF;
			goto ret;
		}
		fp->_flag |= _IOREAD;
	}
	if (--fp->_cnt < 0)
	{
		if (bp->begptr == 0)
			bp = _findbuf(fp);
		wi = _ifilbuf(fp);
	}
	else
		wi = *fp->_ptr++;
	if (!ISONEBYTE(wi))
		_inwc(&wi, fp, &_ifilbuf);
ret:;
	STDUNLOCK(&bp->lock);
	return wi;
}
