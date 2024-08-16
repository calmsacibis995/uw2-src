/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/setvbuf.c	1.18.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#ifdef __STDC__
#   include <limits.h>
#   include <stdlib.h>
#else
    extern char *malloc();
    extern void free();
#endif
#include "stdiom.h"
#include <sys/stat.h>

int
#ifdef __STDC__
setvbuf(register FILE *fp, char *ptr, int type, size_t size) /* set buffering */
#else
setvbuf(fp, ptr, type, size)register FILE *fp; char *ptr; int type; size_t size;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	int res = EOF;

	STDLOCK(&bp->lock);
	if (bp == 0) /* compatibility with old fopen()s: reset _base */
		bp = _fixbase(fp);
	switch (type)
	{
	default:
		ptr = 0;
		break;
	case _IONBF:
		ptr = (char *)fp->_buf;
		size = 0;
		break;
	case _IOLBF:
	case _IOFBF:
		if (size > MAXBUFSIZ)
			size = MAXBUFSIZ;
		if (ptr == 0)	/* allocate our own buffer */
		{
			type |= _IOMYBUF;
			if (size < BUFSIZ)
			{
				struct stat statbuf;

				if (isatty(bp->fd)
					|| fstat(bp->fd, &statbuf) != 0)
				{
					size = BUFSIZ;
				}
				else if ((size = statbuf.st_blksize) > MAXBUFSIZ)
					size = MAXBUFSIZ;
			}
			ptr = (char *)malloc(size);
		}
		break;
	}
	if (ptr != 0)	/* success */
	{
		res = 0;
		while (bp->eflags & IO_PUSHED)
			_popbuf(bp);
		if (fp->_flag & _IOMYBUF)
			free((void *)bp->begptr);
		fp->_flag &= ~(_IONBF | _IOLBF | _IOMYBUF);
		fp->_flag |= type;
		bp->begptr = (Uchar *)ptr;
		bp->endptr = (Uchar *)ptr + size;
		fp->_cnt = 0;
		fp->_ptr = (Uchar *)ptr;
	}
	STDUNLOCK(&bp->lock);
	return res;
}
