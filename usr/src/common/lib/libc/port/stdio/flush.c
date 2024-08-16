/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/flush.c	1.23.1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include "stdiom.h"

	/*
	* This file must be included in the program if it uses
	* any writable, buffered streams.  All the stdio functions
	* that can leave unwritten buffered data have dependencies
	* on at least one of the functions (_xflsbuf) in this file.
	*/

void
#ifdef __STDC__
_cleanup(void)	/* called at process end to flush output streams */
#else
_cleanup()
#endif
{
	fflush((FILE *)0);
}

int
#ifdef __STDC__
_xflsbuf(register BFILE *bp)	/* write stream's buffered contents */
#else
_xflsbuf(bp)register BFILE *bp;
#endif
{
	register FILE *fp = (FILE *)bp->file._base;
	register Uchar *ptr;
	register int cnt, n;

	/*
	* Assumptions:
	*	1. All validity checks have been passed.
	*	2. The stream has a buffer.
	*	3. fp->_ptr > bp->begptr.	(some characters to write)
	*/
	fp->_cnt = 0;
	ptr = bp->begptr;
	cnt = fp->_ptr - ptr;
	fp->_ptr = ptr;
	while ((n = write(bp->fd, (void *)ptr, (size_t)cnt)) != cnt)
	{
		if (n <= 0)
		{
			fp->_flag |= _IOERR;
			return EOF;
		}
		ptr += n;
		cnt -= n;
	}
	return 0;
}

void
#ifdef __STDC__
_flushlbf(void)	/* flush all line-buffered output streams with data */
#else
_flushlbf()
#endif
{
	register BFILE *bp = STDIN;

	do
	{
		register FILE *fp = (FILE *)bp->file._base;

		if ((fp->_flag & (_IOLBF | _IOWRT)) == (_IOLBF | _IOWRT))
		{
			STDLOCK(&bp->lock);
			if (fp->_ptr > bp->begptr)
				_xflsbuf(bp);
			if (fp->_flag & _IORW)
				fp->_flag &= ~_IOWRT;
			else if ((fp->_flag & (_IONBF | _IOLBF)) == 0)
				fp->_cnt = bp->endptr - bp->begptr;
			STDUNLOCK(&bp->lock);
		}
	} while ((bp = bp->next) != 0);
}

int
#ifdef __STDC__
fflush(register FILE *fp)	/* write pending buffered data */
#else
fflush(fp)register FILE *fp;
#endif
{
	register BFILE *bp;
	int res = 0;

	if (fp == 0)	/* fflush all writable streams */
	{
		bp = STDIN;
		do
		{
			fp = (FILE *)bp->file._base;
			if (fp->_flag & _IOWRT)
			{
				STDLOCK(&bp->lock);
				if (fp->_ptr > bp->begptr)
					res |= _xflsbuf(bp);
				if (fp->_flag & _IORW)
					fp->_flag &= ~_IOWRT;
				else if ((fp->_flag & (_IONBF | _IOLBF)) == 0)
					fp->_cnt = bp->endptr - bp->begptr;
				STDUNLOCK(&bp->lock);
			}
		} while ((bp = bp->next) != 0);
		return res;
	}
	bp = (BFILE *)fp->_base;
	STDLOCK(&bp->lock);
	if ((fp->_flag & _IOWRT) == 0)	/* toss buffer contents */
	{
		if (fp->_cnt > 0)	/* reset location */
		{
			if (lseek(bp->fd, (long)-fp->_cnt, SEEK_CUR) < 0)
				res = EOF;
		}
		fp->_cnt = 0;
		fp->_ptr = bp->begptr;
		if (fp->_flag & _IORW)
			fp->_flag &= ~_IOREAD;
	}
	else if (fp->_flag & _IOWRT)	/* write any pending data */
	{
		if (fp->_ptr > bp->begptr)
			res = _xflsbuf(bp);
		if (fp->_flag & _IORW)
			fp->_flag &= ~_IOWRT;
		else if ((fp->_flag & (_IONBF | _IOLBF)) == 0)
			fp->_cnt = bp->endptr - bp->begptr;
	}
	STDUNLOCK(&bp->lock);
	return res;
}
