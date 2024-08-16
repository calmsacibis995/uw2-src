/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/snprintf.c	1.3"
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

#ifdef WIDE
#   ifdef __STDC__
	#pragma weak swprintf = _swprintf
#   endif
#   define FCNNAME	swprintf
#   define IDOPRNT	_iwsdoprnt
#   define CHAR		wchar_t
#else
#   ifdef __STDC__
	#pragma weak snprintf = _snprintf
#   endif
#   define FCNNAME	snprintf
#   define IDOPRNT	_idoprnt
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(CHAR *dst, size_t len, const CHAR *fmt, ...)
#else
/*VARARGS3*/
FCNNAME(dst, len, fmt, va_alist)CHAR *dst; size_t len; const CHAR *fmt; va_dcl
#endif
{
	BFILE stream;
	va_list ap;
	int res;

	if (len == 0)		/* no room even for \0 */
		return EOF;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	stream.file._base = (Uchar *)&stream;
	stream.file._ptr = (Uchar *)dst;
	if (--len > INT_MAX)
		stream.file._cnt = INT_MAX;
	else
		stream.file._cnt = len;
	stream.file._flag = _IOWRT;
	stream.begptr = (Uchar *)dst;
	stream.endptr = (Uchar *)(dst + len);
	stream.fd = -2;			/* distinguishes bounded string-based */
	res = IDOPRNT(&stream.file, fmt, ap);
#ifdef WIDE
	*(wchar_t *)stream.file._ptr = 0;	/* terminate wide string */
#else
	*stream.file._ptr = '\0';		/* terminate byte string */
#endif
	va_end(ap);
	return res;
}
