/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fwrite.c	3.24.4.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __STDC__
#   include <limits.h>
#else
#   define ULONG_MAX	(~(Ulong)0)
#endif
#include <unistd.h>
#include "stdiom.h"

#ifdef __STDC__
size_t
fwrite(const void *sp, size_t size, size_t count, register FILE *fp)
#else
int
fwrite(sp, size, count, fp)const void *sp; size_t size, count; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register Ulong nleft, n;
	register const Uchar *ptr = (const Uchar *)sp;
	size_t res_count;

	STDLOCK(&bp->lock);
	WRTCHK(fp, bp, {res_count = 0; goto ret;});
	if ((res_count = count) == 0)
		goto ret;
	/*
	* Only loop if (size * count) overflows.
	*/
	do
	{
	top:;
		/*
		* Determine how many bytes to transfer in this pass.
		*/
		if ((nleft = size * count) >= count && nleft >= size)
		{
			count = 0;
		}
		else if (size == 0)
		{
			res_count = 0;
			goto ret;
		}
		else /* overflow: handle as many "size" chunks as possible */
		{
			n = ULONG_MAX / size;
			nleft = n * size;
			count -= n;
		}
		/*
		* Use buffer if its remaining space covers nleft.
		*/
		if (bp->endptr - fp->_ptr >= nleft)
		{
		copy:;
			memcpy((void *)fp->_ptr, (void *)ptr, (size_t)nleft);
			fp->_cnt -= nleft;
			fp->_ptr += nleft;
			BUFSYNC(fp, bp);
			if (count != 0)	/* just might be possible */
			{
				ptr += nleft;
				goto top;
			}
			if ((fp->_flag & _IOLBF)
				&& memchr((void *)(fp->_ptr - nleft),
					'\n', (size_t)nleft) != 0
				&& _xflsbuf(bp) != 0)
			{
			err:;
				n = nleft / size;
				if ((nleft % size) != 0)
					n++;
				res_count -= n;
			}
			goto ret;
		}
		/*
		* If current buffer not empty, flush it
		* and recheck to see if nleft is now covered by it.
		*/
		if (fp->_ptr > bp->begptr)
		{
			if (_xflsbuf(bp) != 0)
				goto err;
			n = bp->endptr - bp->begptr;
			if ((fp->_flag & (_IONBF | _IOLBF)) == 0)
				fp->_cnt = n;
			if (n >= nleft)
				goto copy;	/* now there's enough room */
		}
		/*
		* Buffer must be empty and too small for nleft.
		* Directly call write.
		*/
		for (;;)
		{
			register int cnt;

			if ((n = nleft) > MAXBUFSIZ)
				n = MAXBUFSIZ;
			if ((cnt = write(bp->fd, (void *)ptr, (size_t)n)) <= 0)
			{
				fp->_flag |= _IOERR;
				res_count -= count;
				goto err;
			}
			ptr += cnt;
			if ((nleft -= cnt) == 0)
				break;
		}
	} while (count != 0);
ret:;
	STDUNLOCK(&bp->lock);
	return res_count;
}
