/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fread.c	3.29.2.5"
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
fread(void *dp, size_t size, size_t count, register FILE *fp)
#else
int
fread(dp, size, count, fp)char *dp; size_t size, count; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register Ulong n, nleft;
	register int cnt;
	Uchar *ptr = (Uchar *)dp;
	size_t res_count;

	STDLOCK(&bp->lock);
	READCHK(fp, bp, {res_count = 0; goto ret;});
	if ((res_count = count) == 0)
		goto ret;
	/*
	* Only loop if (size * count) overflows.
	*/
	do
	{
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
		* Take any buffered data first.
		* Only loop for layered buffers.
		*/
		for (;;)
		{
			if ((cnt = fp->_cnt) > 0)
			{
				if (cnt >= nleft)
				{
					memcpy((void *)ptr, (void *)fp->_ptr,
						(size_t)nleft);
					fp->_cnt -= nleft;
					fp->_ptr += nleft;
					BUFSYNC(fp, bp);
					goto bot;
				}
				memcpy((void *)ptr, (void *)fp->_ptr,
					(size_t)cnt);
				ptr += cnt;
				nleft -= cnt;
				fp->_cnt = 0;
			}
			if ((bp->eflags & IO_PUSHED) == 0)
				break;
			_popbuf(bp);
		}
		/*
		* Read into buffer if it more than covers the data.
		*/
		if (fp->_flag & (_IONBF | _IOLBF))
		{
			_flushlbf();
			if (fp->_flag & _IONBF)
				goto direct;
		}
		if ((n = bp->endptr - bp->begptr) > nleft)
		{
			for (;;) /* read until failure or enough buffered */
			{
				if ((cnt = read(bp->fd, (void *)bp->begptr,
					(size_t)n)) <= 0)
				{
				err:;
					if (cnt == 0)
						fp->_flag |= _IOEOF;
					else
						fp->_flag |= _IOERR;
					n = nleft / size;
					if ((nleft % size) != 0)
						n++;
					res_count -= n;
					fp->_ptr = bp->begptr;
					goto ret;
				}
				if (cnt < nleft)
				{
					memcpy((void *)ptr, (void *)bp->begptr,
						(size_t)cnt);
					ptr += cnt;
					nleft -= cnt;
					continue;
				}
				memcpy((void *)ptr, (void *)bp->begptr,
					(size_t)nleft);
				fp->_ptr = bp->begptr + nleft;
				fp->_cnt = cnt - nleft;
				goto bot; /* because count might not be 0 */
			}
		}
		/*
		* Otherwise, read the rest directly into the target array.
		*/
	direct:;
		do
		{
			if ((n = nleft) > MAXBUFSIZ)
				n = MAXBUFSIZ;
			if ((cnt = read(bp->fd, (void *)ptr, (size_t)n)) <= 0)
			{
				res_count -= count;
				goto err;
			}
			ptr += cnt;
		} while ((nleft -= cnt) != 0);
	bot:;
	} while (count != 0);
ret:;
	STDUNLOCK(&bp->lock);
	return res_count;
}
