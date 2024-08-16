/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_flsbuf.c	1.7.3.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak _flsbuf = __flsbuf
#endif

int
#ifdef __STDC__
_flsbuf(int ch, register FILE *fp)
#else
_flsbuf(ch, fp)int ch; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register int res;
	Uchar byte;

	STDLOCK(&bp->lock);
	if (((res = fp->_flag) & (_IOWRT | _IOEOF)) != _IOWRT)
	{
		if ((res & (_IOWRT | _IORW)) == 0)
		{
			errno = EBADF;
			res = EOF;
			goto ret;
		}
		fp->_flag &= ~_IOEOF;
		fp->_flag |= _IOWRT;
	}
	if (res & _IOLBF)	/* line buffered */
	{
		res = (Uchar)ch;
		if (fp->_ptr >= bp->endptr && _xflsbuf(bp) != 0)
			res = EOF;
	linebuf:;
		if ((*fp->_ptr++ = ch) == '\n' && _xflsbuf(bp) != 0)
			res = EOF;
		fp->_cnt = 0;
	}
	else if (res & _IONBF)	/* unbuffered */
	{
		res = (Uchar)ch;
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
		res = (Uchar)ch;
		if (fp->_ptr > bp->begptr)
		{
			if (_xflsbuf(bp) != 0)
				res = EOF;
		}
		else if (bp->begptr == 0)
		{
			bp = _findbuf(fp);
			if (fp->_flag & _IOLBF)
				goto linebuf;
		}
		fp->_cnt = bp->endptr - bp->begptr - 1;
		fp->_ptr = bp->begptr + 1;
		bp->begptr[0] = ch;
	}
ret:;
	STDUNLOCK(&bp->lock);
	return res;
}
