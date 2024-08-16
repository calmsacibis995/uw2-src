/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/funlockfile.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak funlockfile = _funlockfile
#endif

void
#ifdef __STDC__
funlockfile(FILE *fp)	/* release user-requested locking for stream */
#else
funlockfile(fp)FILE *fp;
#endif
{
#ifdef _REENTRANT
	register BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	if (bp->userown == (*_libc_self)() && --bp->usercnt <= 0)
	{
		bp->userown = 0;
		STDUNLOCK(&bp->user);
	}
	STDUNLOCK(&bp->lock);
#endif
}
