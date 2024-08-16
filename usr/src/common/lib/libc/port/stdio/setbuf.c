/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/setbuf.c	2.11.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <stdlib.h>
#else
    extern void free();
#endif
#include <unistd.h>
#include "stdiom.h"

void
#ifdef __STDC__
setbuf(register FILE *fp, char *ptr)	/* specify buffering for stream */
#else
setbuf(fp, ptr)register FILE *fp; char *ptr;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	if (bp == 0) /* compatibility with old fopen()s: reset _base */
		bp = _fixbase(fp);
	while (bp->eflags & IO_PUSHED)
		_popbuf(bp);
	if (fp->_flag & _IOMYBUF)
		free((void *)bp->begptr);
	fp->_flag &= ~(_IONBF | _IOLBF | _IOMYBUF);
	if (ptr == 0)	/* unbuffered */
	{
		fp->_flag |= _IONBF;
		bp->begptr = &fp->_buf[0];
		bp->endptr = &fp->_buf[0];
	}
	else	/* buffer with provided BUFSIZ-long array */
	{
		if (isatty(bp->fd))
			fp->_flag |= _IOLBF;
		bp->begptr = (Uchar *)ptr;
		bp->endptr = (Uchar *)ptr + BUFSIZ;
	}
	fp->_cnt = 0;
	fp->_ptr = bp->begptr;
	STDUNLOCK(&bp->lock);
}
