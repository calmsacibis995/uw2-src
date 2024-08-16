/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/vscanf.c	1.2"
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
	#pragma weak vwscanf = _vwscanf
#   endif
#   define FCNNAME	vwscanf
#   define IDOSCAN	_iwdoscan
#   define CHAR		wchar_t
#else
#   ifdef __STDC__
	#pragma weak vscanf = _vscanf
#   endif
#   define FCNNAME	vscanf
#   define IDOSCAN	_idoscan
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(const CHAR *fmt, va_list ap)
#else
FCNNAME(fmt, ap)const CHAR *fmt; va_list ap;
#endif
{
	FILE *fp = stdin;
	BFILE *bp = (BFILE *)fp->_base;
	int res;

	STDLOCK(&bp->lock);
	READCHK(fp, bp, {res = EOF; goto ret;});
	res = IDOSCAN(fp, fmt, ap);
ret:;
	STDUNLOCK(&bp->lock);
	return res;
}
