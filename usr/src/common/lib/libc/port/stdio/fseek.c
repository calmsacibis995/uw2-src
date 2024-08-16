/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fseek.c	1.15.2.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include "stdiom.h"

int
#ifdef __STDC__
fseek(register FILE *fp, long offset, int whence) /* change current position */
#else
fseek(fp, offset, whence)register FILE *fp; long offset; int whence;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	if (fp->_flag & _IOREAD)
	{
		while (bp->eflags & IO_PUSHED)
			_popbuf(bp);
		if (whence == SEEK_CUR && bp->begptr != 0
			&& (fp->_flag & _IONBF) == 0)
		{
			if ((fp->_flag & _IORW) == 0
				&& fp->_cnt > 0 && offset <= fp->_cnt
				&& offset >= bp->begptr - fp->_ptr)
			{
				fp->_cnt -= offset;
				fp->_ptr += offset;
				BUFSYNC(fp, bp);
				fp->_flag &= ~_IOEOF;
				offset = 0;
				goto out;
			}
			if (fp->_cnt > 0)
				offset -= fp->_cnt;
		}
		if (fp->_flag & _IORW)
			fp->_flag &= ~_IOREAD;
	}
	else if (fp->_flag & _IOWRT)
	{
		if (fp->_ptr > bp->begptr && _xflsbuf(bp) != 0)
		{
			offset = -1;
			goto out;
		}
		if (fp->_flag & _IORW)
			fp->_flag &= ~_IOWRT;
	}
	if ((offset = lseek(bp->fd, offset, whence)) >= 0)
	{
		offset = 0;
	reset:;
		fp->_cnt = 0;
		fp->_ptr = bp->begptr;
		fp->_flag &= ~_IOEOF;
	}
	else if (fp->_flag & _IORW)
		goto reset;
out:;
	STDUNLOCK(&bp->lock);
	return offset;
}
