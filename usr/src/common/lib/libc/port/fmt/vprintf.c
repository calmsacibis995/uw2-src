/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/vprintf.c	1.7.3.2"
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
	#pragma weak vwprintf = _vwprintf
#   endif
#   define FCNNAME	vwprintf
#   define IDOPRNT	_iwdoprnt
#   define CHAR		wchar_t
#else
#   define FCNNAME	vprintf
#   define IDOPRNT	_idoprnt
#   define CHAR		char
#endif

int
#ifdef __STDC__
FCNNAME(const CHAR *fmt, va_list ap)
#else
FCNNAME(fmt, ap)const CHAR *fmt; va_list ap;
#endif
{
	BFILE tmp_bfile;
	Uchar tmp_buffer[64];
	FILE *fp = stdout;
	BFILE *bp = (BFILE *)fp->_base;
	int res;

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
	return res;
}
