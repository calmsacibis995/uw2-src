/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fpos.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

int
#ifdef __STDC__
fgetpos(FILE *fp, fpos_t *p)	/* store current position */
#else
fgetpos(fp, p)FILE *fp; fpos_t *p;
#endif
{
	if ((*p = ftell(fp)) == -1)
		return -1;
	return 0;
}

int
#ifdef __STDC__
fsetpos(FILE *fp, const fpos_t *p)	/* reset stream to position */
#else
fsetpos(fp, p)FILE *fp; const fpos_t *p;
#endif
{
	if (fseek(fp, *p, SEEK_SET) != 0)
		return -1;
	return 0;
}
