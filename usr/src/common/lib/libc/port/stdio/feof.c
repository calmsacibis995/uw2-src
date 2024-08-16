/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/feof.c	1.2.2.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>

#undef feof

int
#ifdef __STDC__
feof(FILE *fp)	/* does stream have end-of-file set? */
#else
feof(fp)FILE *fp;
#endif
{
	return fp->_flag & _IOEOF;
}
