/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/fprintf.c	1.15.1.2"
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
	#pragma weak fwprintf = _fwprintf
#   endif
#   define FCNNAME	fwprintf
#   define IDOPRNT	_iwdoprnt
#   define CHAR		wchar_t
#else
#   define FCNNAME	fprintf
#   define IDOPRNT	_idoprnt
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(FILE *fp, const CHAR *fmt, ...)
#else
/*VARARGS2*/
FCNNAME(fp, fmt, va_alist)FILE *fp; const CHAR *fmt; va_dcl
#endif
{
	BFILE tmp_bfile;
	Uchar tmp_buffer[64];
	BFILE *bp = (BFILE *)fp->_base;
	va_list ap;
	int res;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	STDLOCK(&bp->lock);
	WRTCHK(fp, bp, {res = EOF; goto ret;});
	if (fp->_flag & _IONBF)	/* temporary stream reduces write()s */
	{
		tmp_bfile.file._flag = fp->_flag;
		tmp_bfile.file._ptr = &tmp_buffer[0];
		tmp_bfile.file._cnt = sizeof(tmp_buffer);
		tmp_bfile.file._base = (Uchar *)&tmp_bfile;
		tmp_bfile.begptr = &tmp_buffer[0];
		tmp_bfile.endptr = &tmp_buffer[sizeof(tmp_buffer)];
		tmp_bfile.fd = bp->fd;
		tmp_bfile.next = (BFILE *)fp;	/* gives access to true FILE */
		fp = &tmp_bfile.file;
	}
	res = IDOPRNT(fp, fmt, ap);
ret:;
	STDUNLOCK(&bp->lock);
	va_end(ap);
	return res;
}
