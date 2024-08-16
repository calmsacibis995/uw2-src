/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/doscan.c	2.41.1.4"
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

int
#ifdef __STDC__
_doscan(FILE *fp, const char *fmt, va_list ap)	/* only for compatibility */
#else
_doscan(fp, fmt, ap)FILE *fp; const char *fmt; va_list ap;
#endif
{
	BFILE stream;
	BFILE *bp;
	int res;

	if ((bp = (BFILE *)fp->_base) != (BFILE *)fp
		&& bp != STDIN && bp != STDOUT && bp != STDERR)
	{
		/*
		* It must be one not returned by the current
		* _findiop().  Use a temporary local BFILE.
		*/
		stream.file._base = (Uchar *)&stream;
		stream.file._ptr = fp->_ptr;
		stream.file._cnt = fp->_cnt;
		stream.file._flag = (fp->_flag & (_IONBF | _IOLBF)) | _IOREAD;
		stream.endptr = fp->_ptr + fp->_cnt;	/* safe */
		if (fp->_flag & _IOWRT) /* presumably means string-based */
		{
			stream.begptr = fp->_ptr;
			stream.fd = -1;
		}
		else
		{
			stream.begptr = fp->_base;
			if (fp->_file != (Uchar)-1)
				stream.fd = fp->_file;
			else
				stream.fd = -1;
		}
		stream.next = (BFILE *)fp;
		bp = &stream;
		fp = &stream.file;
	}
	else
		STDLOCK(&bp->lock);
	READCHK(fp, bp, {res = EOF; goto ret;});
	res = _idoscan(fp, fmt, ap);
ret:;
	if (bp == &stream)	/* copy up all interesting members */
	{
		fp = (FILE *)stream.next;
		fp->_flag |= stream.file._flag;
		fp->_ptr = stream.file._ptr;
		fp->_cnt = stream.file._cnt;
		if (fp->_base == 0)
			fp->_base = stream.begptr;
	}
	else
		STDUNLOCK(&bp->lock);
	return res;
}
