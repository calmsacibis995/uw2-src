/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/vfscanf.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <stdarg.h>
#else
#   include <varargs.h>
#endif
#include <errno.h>
#include "stdiom.h"
#include "format.h"

#ifdef WIDE
#   ifdef __STDC__
	#pragma weak vfwscanf = _vfwscanf
#   endif
#   define FCNNAME	vfwscanf
#   define IDOSCAN	_iwdoscan
#   define CHAR		wchar_t
#else
#   ifdef __STDC__
	#pragma weak vfscanf = _vfscanf
#   endif
#   define FCNNAME	vfscanf
#   define IDOSCAN	_idoscan
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(FILE *fp, const CHAR *fmt, va_list ap)
#else
FCNNAME(fp, fmt, ap)FILE *fp; const CHAR *fmt; va_list ap;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	int res;

	STDLOCK(&bp->lock);
	READCHK(fp, bp, {res = EOF; goto ret;});
	res = IDOSCAN(fp, fmt, ap);
ret:;
	STDUNLOCK(&bp->lock);
	return res;
}
