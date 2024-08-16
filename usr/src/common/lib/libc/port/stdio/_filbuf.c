/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_filbuf.c	1.8.3.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak _filbuf = __filbuf
	#pragma weak fgetc = __filbuf
	#pragma weak getc = __filbuf
#   ifdef _REENTRANT
	#pragma weak getc_unlocked = __filbuf
	#pragma weak _getc_unlocked = __filbuf
#   endif
#else /*!__STDC__*/
	#pragma weak fgetc = _filbuf
	#pragma weak getc = _filbuf
#   ifdef _REENTRANT
	#pragma weak getc_unlocked = _filbuf
	#pragma weak _getc_unlocked = _filbuf
#   endif
#endif /*__STDC__*/

int
#ifdef __STDC__
_filbuf(FILE *fp)
#else
_filbuf(fp)FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	register int ch;

	STDLOCK(&bp->lock);
	if ((fp->_flag & _IOREAD) == 0)
	{
		if ((fp->_flag & _IORW) == 0)
		{
			errno = EBADF;
			ch = EOF;
			goto ret;
		}
		fp->_flag |= _IOREAD;
	}
	if (--fp->_cnt < 0)
	{
		if (bp->begptr == 0)
			bp = _findbuf(fp);
		ch = _ifilbuf(fp);
	}
	else
		ch = *fp->_ptr++;
ret:;
	STDUNLOCK(&bp->lock);
	return ch;
}

int
#ifdef __STDC__
_ifilbuf(register FILE *fp)	/* get some characters into the buffer */
#else
_ifilbuf(fp)register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register int n;

	/*
	* Assumptions:
	*	1. All validity checks have been passed.
	*	2. There is a buffer for the stream.
	*	3. fp->_cnt <= 0.	(no characters in current buffer)
	*/
	if (bp->eflags & IO_PUSHED)	/* remove covering layer */
	{
		_popbuf(bp);
		if (--fp->_cnt >= 0)	/* nonempty buffer uncovered */
		{
			n = *fp->_ptr++;
			BUFSYNC(fp, bp);
			return n;
		}
	}
	/*
	* Buffer is empty.  No layered buffers remain.
	*/
	if (fp->_flag & (_IONBF | _IOLBF))
		_flushlbf();
	if ((n = read(bp->fd, (void *)bp->begptr,
		(size_t)((fp->_flag & _IONBF) ? 1 : (bp->endptr - bp->begptr))))
		<= 0)
	{
		if (n == 0)
			fp->_flag |= _IOEOF;
		else
			fp->_flag |= _IOERR;
		fp->_ptr = bp->begptr;  /* in case "for update" */
		return EOF;
	}
	fp->_ptr = bp->begptr + 1;
	fp->_cnt = n - 1;
	return bp->begptr[0];
}
