/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/funflush.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak funflush = _funflush
#endif

void
#ifdef __STDC__
funflush(register FILE *fp)	/* toss any pending buffered data */
#else
funflush(fp)register FILE *fp;
#endif
{
	BFILE *bp;

	if (fp == 0)	/* funflush all streams */
	{
		bp = STDIN;
		do
		{
			funflush((FILE *)bp->file._base);
		} while ((bp = bp->next) != 0);
		return;
	}
	bp = (BFILE *)fp->_base;
	STDLOCK(&bp->lock);
	fp->_cnt = 0;	/* seek back if _IOREAD?? */
	fp->_ptr = bp->begptr;
	if (fp->_flag & _IORW)
		fp->_flag &= ~(_IOREAD | _IOWRT);
	STDUNLOCK(&bp->lock);
}
