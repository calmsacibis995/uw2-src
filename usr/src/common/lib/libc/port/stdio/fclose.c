/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fclose.c	1.5"
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

int
#ifdef __STDC__
fclose(register FILE *fp)	/* close and deallocate stream */
#else
fclose(fp)register FILE *fp;
#endif
{
	register BFILE *bp;
	int res = 0;

	if (fp == 0 || fp->_flag == 0)	/* survive bogus calls */
		return EOF;
	bp = (BFILE *)fp->_base;
	STDLOCK(&bp->lock);
	while (bp->eflags & IO_PUSHED)
		_popbuf(bp);
	if (fp->_flag & _IOWRT)
	{
		if (fp->_ptr > bp->begptr)
			_xflsbuf(bp);
		if (fp->_flag & _IOERR)
			res = EOF;
	}
	if (close(bp->fd) < 0)
		res = EOF;
	if (fp->_flag & _IOMYBUF)
		free((void *)bp->begptr);
	fp->_flag = 0;
	bp->eflags = 0;		/* deallocates the stream */
	STDUNLOCK(&bp->lock);
	return res;
}
