/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fileno.c	1.6.3.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fileno = _fileno
#endif

int
#ifdef __STDC__
fileno(FILE *fp)	/* file descriptor for stream */
#else
fileno(fp)FILE *fp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;

	return bp->fd;
}
