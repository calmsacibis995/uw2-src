/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_findiop.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <stdlib.h>
#else
    extern char *malloc();
#endif
#include "stdiom.h"

BFILE *
#ifdef __STDC__
_findiop(void)	/* allocate new stream; leave locked */
#else
_findiop()
#endif
{
	static const BFILE empty = {0};
#ifdef _REENTRANT
	static StdLock list_lock;
#endif
	register BFILE *bp = STDIN;

	STDLOCK(&list_lock);
	do
	{
		if (bp->eflags == 0)	/* reuse old stream */
		{
			register FILE *fp = (FILE *)bp->file._base;

			fp->_cnt = 0;
			fp->_ptr = 0;
			bp->begptr = 0;
			bp->endptr = 0;
			goto found;
		}
	} while ((bp = bp->next) != 0);
	/*
	* Allocate new one and insert just after STDERR.
	*/
	if ((bp = (BFILE *)malloc(sizeof(BFILE))) == 0)
	{
		STDUNLOCK(&list_lock);
		return 0;
	}
	*bp = empty;
	bp->file._base = (Uchar *)bp;
	/*
	* The following two assignments are order-dependent.
	* They must be "flushed" to memory if an on-chip data
	* cache does not guarantee the ordering.
	*/
	bp->next = STDERR->next;
	STDERR->next = bp;
found:;
	bp->eflags = IO_ACTIVE;
	STDUNLOCK(&list_lock);
	STDLOCK(&bp->lock);
	return bp;
}
