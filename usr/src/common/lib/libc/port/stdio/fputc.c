/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fputc.c	1.8.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "stdiom.h"

	#pragma weak putc = fputc
#ifdef _REENTRANT
	#pragma weak putc_unlocked = fputc
	#pragma weak _putc_unlocked = fputc
#endif

int
#ifdef __STDC__
fputc(int ch, FILE *fp)
#else
fputc(ch, FILE *fp)int ch; FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	if ((fp->_flag & (_IOWRT | _IOEOF)) != _IOWRT)
	{
		if ((fp->_flag & (_IOWRT | _IORW)) == 0)
		{
			errno = EBADF;
			ch = EOF;
			goto ret;
		}
		fp->_flag &= ~_IOEOF;
		fp->_flag |= _IOWRT;
	}
	if (--fp->_cnt < 0)
	{
		if (bp->begptr == 0)
			bp = _findbuf(fp);
		ch = _iflsbuf(ch, fp);
	}
	else
	{
		*fp->_ptr++ = ch;
		ch = (Uchar)ch;
	}
ret:;
	STDUNLOCK(&bp->lock);
	return ch;
}

int
#ifdef __STDC__
_iflsbuf(int ch, register FILE *fp)	/* internal version of _flsbuf */
#else
_iflsbuf(ch, fp)int ch; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register int res = (Uchar)ch;
	Uchar byte;

	/*
	* Assumptions:
	*	1. All validity checks passed.
	*	2. There is a buffer for the stream.
	*	3. fp->_cnt <= 0.
	*/
	if (fp->_flag & _IOLBF)
	{
		if (fp->_ptr >= bp->endptr && _xflsbuf(bp) != 0)
			res = EOF;
		if ((*fp->_ptr++ = ch) == '\n' && _xflsbuf(bp) != 0)
			res = EOF;
		fp->_cnt = 0;
	}
	else if (fp->_flag & _IONBF)
	{
		byte = ch;
		if (write(bp->fd, (void *)&byte, (size_t)1) != 1)
		{
			fp->_flag |= _IOERR;
			res = EOF;
		}
		fp->_cnt = 0;
	}
	else	/* full buffered */
	{
		if (fp->_ptr > bp->begptr && _xflsbuf(bp) != 0)
			res = EOF;
		fp->_cnt = bp->endptr - bp->begptr - 1;
		fp->_ptr = bp->begptr + 1;
		bp->begptr[0] = ch;
	}
	return res;
}
