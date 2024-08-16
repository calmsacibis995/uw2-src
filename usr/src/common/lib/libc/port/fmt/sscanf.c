/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/sscanf.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <limits.h>
#   include <stdarg.h>
#else
#   include <varargs.h>
#endif
#include <string.h>
#include "stdiom.h"
#include "format.h"

#ifdef WIDE
#   ifdef __STDC__
	#pragma weak swscanf = _swscanf
#   endif
#   define FCNNAME	swscanf
#   define IDOSCAN	_iwsdoscan
#   define STRLEN	wcslen
#   define CHAR		wchar_t
#else
#   define FCNNAME	sscanf
#   define IDOSCAN	_idoscan
#   define STRLEN	strlen
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(const CHAR *src, const CHAR *fmt, ...)
#else
/*VARARGS2*/
FCNNAME(src, fmt, va_alist)const CHAR *src, *fmt; va_dcl
#endif
{
	BFILE stream;
	va_list ap;
	int res;
	size_t len = STRLEN(src);

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	stream.file._base = (Uchar *)&stream;
	stream.file._ptr = (Uchar *)src;
	stream.file._cnt = len > INT_MAX ? INT_MAX : len;
	stream.file._flag = _IOREAD;
	stream.begptr = (Uchar *)src;
	stream.endptr = (Uchar *)(src + len);
	stream.fd = -1;			/* distinguishes string-based */
	res = IDOSCAN(&stream.file, fmt, ap);
	va_end(ap);
	return res;
}
