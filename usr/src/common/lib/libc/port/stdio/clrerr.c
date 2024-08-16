/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/clrerr.c	1.10.2.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#undef clearerr

void
#ifdef __STDC__
clearerr(FILE *fp)	/* unset end-of-file and error flags for stream */
#else
clearerr(fp)FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	fp->_flag &= ~(_IOEOF | _IOERR);
	STDUNLOCK(&bp->lock);
}
