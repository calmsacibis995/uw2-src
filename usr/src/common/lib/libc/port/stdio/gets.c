/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/gets.c	3.13.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "stdiom.h"

char *
#ifdef __STDC__
gets(char *buf)	/* dangerous: fill buffer from stdin until newline */
#else
gets(buf)char *buf;
#endif
{
	register FILE *fp = stdin;
	register BFILE *bp = (BFILE *)fp->_base;
	register Uchar *ptr = (Uchar *)buf;
	register int cnt;

	STDLOCK(&bp->lock);
	READCHK(fp, bp, {buf = 0; goto ret;});
	/*
	* Take any currently buffered data first.
	* Only loop for layered buffers.
	*/
	for (;;)
	{
		register Uchar *p;

		if ((cnt = fp->_cnt) > 0)
		{
			if ((p = (Uchar *)memccpy((void *)ptr,
				(void *)fp->_ptr, '\n', (size_t)cnt)) != 0)
			{
				cnt = p - ptr;
				fp->_cnt -= cnt;
				fp->_ptr += cnt;
				BUFSYNC(fp, bp);
				p[-1] = '\0';	/* overwrite \n */
				goto ret;
			}
			ptr += cnt;
			fp->_cnt = 0;
		}
		if ((bp->eflags & IO_PUSHED) == 0)
			break;
		_popbuf(bp);
	}
	/*
	* Repeatedly read into buffer until newline copied.
	*/
	cnt = bp->endptr - bp->begptr;
	if (fp->_flag & (_IONBF | _IOLBF))
	{
		_flushlbf();
		if (fp->_flag & _IONBF)
			cnt = 1;
	}
	for (;;)
	{
		register int n;
		register Uchar *p;

		if ((n = read(bp->fd, (void *)bp->begptr, (size_t)cnt)) <= 0)
		{
			if (n == 0)
				fp->_flag |= _IOEOF;
			else
				fp->_flag |= _IOERR;
			if (ptr == (Uchar *)buf)
				buf = 0;
			else
				*ptr = '\0';
			fp->_ptr = bp->begptr;	/* in case "for update" */
			break;
		}
		if ((p = (Uchar *)memccpy((void *)ptr, (void *)bp->begptr,
			'\n', (size_t)n)) != 0)
		{
			cnt = p - ptr;
			fp->_ptr = bp->begptr + cnt;
			fp->_cnt = n - cnt;
			p[-1] = '\0';		/* overwrite \n */
			break;
		}
		ptr += n;
	}
ret:;
	STDUNLOCK(&bp->lock);
	return buf;
}
