/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/ungetc.c	2.11.2.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

int
#ifdef __STDC__
ungetc(int ch, register FILE *fp)	/* pushback byte on read stream */
#else
ungetc(ch, fp)int ch; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	if ((fp->_flag & (_IOREAD | _IORW)) == 0)
	{
		errno = EBADF;
		return EOF;
	}
	STDLOCK(&bp->lock);
	if (fp->_ptr == bp->begptr)	/* at start of [nonexistant?] buffer */
	{
		if (bp->begptr == 0)
			bp = _findbuf(fp);
		if (ch == EOF)
			goto ret;
	}
	else if (*--fp->_ptr == ch)	/* unget original byte */
	{
		fp->_cnt++;
		fp->_flag &= ~_IOEOF;
		goto ret;
	}
	else if (ch == EOF)
	{
		fp->_ptr++;	/* restore value */
		goto ret;
	}
	else if (bp->eflags & IO_PUSHED) /* pushback buffer with room */
	{
		*fp->_ptr = ch;
		fp->_cnt++;
		goto ret;
	}
	else
		fp->_ptr++;	/* restore value */
	if ((ch = _pushbuf(bp, ch)) != EOF)
		fp->_flag &= ~_IOEOF;
ret:;
	STDUNLOCK(&bp->lock);
	return ch;
}
