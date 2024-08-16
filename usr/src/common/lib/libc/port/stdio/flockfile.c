/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/flockfile.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak flockfile = _flockfile
	#pragma weak ftrylockfile = _ftrylockfile
#endif

int
#ifdef __STDC__
ftrylockfile(FILE *fp)	/* try to acquire user-requested locking for stream */
#else
ftrylockfile(fp)FILE *fp;
#endif
{
#ifdef _REENTRANT
	register BFILE *bp = (BFILE *)fp->_base;
	int retval = 0;
	id_t id = (*_libc_self)();

	if (__multithreaded != 0)
	{
		STDLOCK(&bp->lock);
		if (STDTRYLOCK(&bp->user) == 0)	/* was unowned */
		{
			bp->userown = id;
			bp->usercnt = 1;
		}
		else if (bp->userown == id)	/* we own */
		{
			bp->usercnt++;
		}
		else	/* other owns */
		{
			retval = -1;
		}
		STDUNLOCK(&bp->lock);
		return retval;
	}
	else
#endif
		return 0;	/* or is -1 a better choice? */
}

void
#ifdef __STDC__
flockfile(FILE *fp)	/* acquire user-requested locking for stream */
#else
flockfile(fp)FILE *fp;
#endif
{
#ifdef _REENTRANT
	BFILE *bp = (BFILE *)fp->_base;

	if (__multithreaded != 0)
	{
		while (ftrylockfile(fp) != 0)
		{
			STDLOCK(&bp->user);
			STDUNLOCK(&bp->user);
		}
	}
#endif
}
