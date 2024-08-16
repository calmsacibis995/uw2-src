/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/rew.c	1.11.2.2"

/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#include "stdiom.h"

void
#ifdef __STDC__
rewind(register FILE *fp)	/* reset stream to beginning */
#else
rewind(fp)register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	while (bp->eflags & IO_PUSHED)
		_popbuf(bp);
	if (fp->_flag & _IOWRT && fp->_ptr > bp->begptr)
		_xflsbuf(bp);
	lseek(bp->fd, 0L, SEEK_SET);
	fp->_cnt = 0;
	fp->_ptr = bp->begptr;
	fp->_flag &= ~(_IOEOF | _IOERR);
	if (fp->_flag & _IORW)
		fp->_flag &= ~(_IOREAD | _IOWRT);
	STDUNLOCK(&bp->lock);
}
