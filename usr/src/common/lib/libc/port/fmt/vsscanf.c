/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/vsscanf.c	1.3"
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
	#pragma weak vswscanf = _vswscanf
#   endif
#   define FCNNAME	vswscanf
#   define IDOSCAN	_iwsdoscan
#   define STRLEN	wcslen
#   define CHAR		wchar_t
#else
#   ifdef __STDC__
	#pragma weak vsscanf = _vsscanf
#   endif
#   define FCNNAME	vsscanf
#   define IDOSCAN	_idoscan
#   define STRLEN	strlen
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(const CHAR *src, const CHAR *fmt, va_list ap)
#else
FCNNAME(src, fmt, ap)const CHAR *src, *fmt; va_list ap;
#endif
{
	BFILE stream;
	int res;
	size_t len = STRLEN(src);

	stream.file._base = (Uchar *)&stream;
	stream.file._ptr = (Uchar *)src;
	stream.file._cnt = len > INT_MAX ? INT_MAX : len;
	stream.file._flag = _IOREAD;
	stream.begptr = (Uchar *)src;
	stream.endptr = (Uchar *)(src + len);
	stream.fd = -1;			/* distinguishes string-based */
	res = IDOSCAN(&stream.file, fmt, ap);
	return res;
}
