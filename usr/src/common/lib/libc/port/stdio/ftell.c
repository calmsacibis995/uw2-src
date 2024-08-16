/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/ftell.c	1.13.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "stdiom.h"

long
#ifdef __STDC__
ftell(register FILE *fp)	/* return current location */
#else
ftell(fp)register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	register int adj = 0;
	register long loc;

	STDLOCK(&bp->lock);
	if (fp->_flag & _IOREAD)
	{
		if (fp->_cnt > 0)
			adj = -fp->_cnt;
		if (bp->eflags & IO_PUSHED)
			adj -= _hidecnt(bp);
	}
	else if (fp->_flag & (_IOWRT | _IORW))
	{
		if ((fp->_flag & (_IOWRT | _IONBF)) == _IOWRT && bp->begptr != 0)
			adj = fp->_ptr - bp->begptr;
	}
	else
	{
		errno = EBADF;
		loc = -1;
		goto ret;
	}
	if ((loc = lseek(bp->fd, 0L, SEEK_CUR)) >= 0)
		loc += adj;
ret:;
	STDUNLOCK(&bp->lock);
	return loc;
}
