/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/sprintf.c	1.13.3.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <limits.h>
#   include <stdarg.h>
#else
#   include <varargs.h>
#endif
#include "stdiom.h"
#include "format.h"

int
#ifdef __STDC__
sprintf(char *dst, const char *fmt, ...)
#else
/*VARARGS2*/
sprintf(dst, fmt, va_alist)char *dst; const char *fmt; va_dcl
#endif
{
	BFILE stream;
	va_list ap;
	int res;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	stream.file._base = (Uchar *)&stream;
	stream.file._ptr = (Uchar *)dst;
	stream.file._cnt = INT_MAX;
	stream.file._flag = _IOWRT;
	stream.begptr = (Uchar *)dst;
	stream.endptr = (Uchar *)dst;
	stream.fd = -1;			/* distinguishes unlimited string-based */
	res = _idoprnt(&stream.file, fmt, ap);
	*stream.file._ptr = '\0';	/* terminate string */
	va_end(ap);
	return res;
}
