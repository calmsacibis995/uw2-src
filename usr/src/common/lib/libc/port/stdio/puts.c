/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/puts.c	3.13.2.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __STDC__
#   include <limits.h>
#endif
#include <unistd.h>
#include "stdiom.h"

int
#ifdef __STDC__
puts(const char *ptr)	/* output string + newline to stdout */
#else
puts(ptr)const char *ptr;
#endif
{
	register FILE *fp = stdout;
	register BFILE *bp = (BFILE *)fp->_base;
	register size_t size = strlen(ptr);
	int res;

	STDLOCK(&bp->lock);
	WRTCHK(fp, bp, {res = EOF; goto ret;});
	/*
	* Copy into buffer if it fits.
	*/
	if (bp->endptr - fp->_ptr > size)
	{
	copy:;
		memcpy((void *)fp->_ptr, (void *)ptr, size);
		fp->_ptr[size++] = '\n';	/* add newline */
		fp->_cnt -= size;
		fp->_ptr += size;
		BUFSYNC(fp, bp);
		res = size;	/* must fit */
		if (fp->_flag & _IOLBF && _xflsbuf(bp) != 0)
			res = EOF;
		goto ret;
	}
	/*
	* Empty the buffer if anything there.
	* If can now hold string, copy it.
	*/
	if (fp->_ptr > bp->begptr)
	{
		if (_xflsbuf(bp) != 0)
		{
			res = EOF;
			goto ret;
		}
		res = bp->endptr - bp->begptr;
		if ((fp->_flag & (_IONBF | _IOLBF)) == 0)
			fp->_cnt = res;
		if (res > size)
			goto copy;	/* now there's enough room */
	}
	/*
	* Buffer is empty and too short for the string.
	* Directly write and then add final newline byte.
	*/
	if (size > INT_MAX)
		res = INT_MAX;
	else
		res = size;
	for (;;)
	{
		register Ulong n;
		register int cnt;

		if ((n = size) > MAXBUFSIZ)
			n = MAXBUFSIZ;
		if ((cnt = write(bp->fd, (void *)ptr, (size_t)n)) <= 0)
		{
		err:;
			fp->_flag |= _IOERR;
			res = EOF;
			goto ret;
		}
		if ((size -= cnt) == 0)	/* done: add newline */
		{
			if (fp->_flag & (_IONBF | _IOLBF))	/* ugh */
			{
				static const char newline = '\n';

				if (write(bp->fd, (const void *)&newline,
					(size_t)1) <= 0)
				{
					goto err;
				}
				goto ret;
			}
			*fp->_ptr++ = '\n';
			fp->_cnt--;
			BUFSYNC(fp, bp);
			goto ret;
		}
		ptr += cnt;
	}
ret:;
	STDUNLOCK(&bp->lock);
	return res;
}
