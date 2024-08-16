/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/putw.c	1.11.4.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak putw = _putw
#endif

int
#ifdef __STDC__
putw(int w, FILE *fp)	/* write a "word" to stream */
#else
putw(w, fp)int w; FILE *fp;
#endif
{
	if (fwrite((void *)&w, (size_t)1, sizeof(int), fp) != sizeof(int))
		return EOF;
	return 0;
}
