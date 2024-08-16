/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/doprnt.c	3.34.1.5"
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
_doprnt(const char *fmt, va_list ap, FILE *fp)	/* only for compatibility */
#else
_doprnt(fmt, ap, fp)const char *fmt; va_list ap; FILE *fp;
#endif
{
	BFILE tmp_bfile;
	Uchar tmp_buffer[64];
	BFILE *bp;
	int res;

	if ((bp = (BFILE *)fp->_base) != (BFILE *)fp
		&& bp != STDIN && bp != STDOUT && bp != STDERR)
	{
		/*
		* Old FILE structure without an associated BFILE.
		*/
		tmp_bfile.file._base = (Uchar *)&tmp_bfile;
		tmp_bfile.file._flag = (fp->_flag & (_IONBF | _IOLBF)) | _IOWRT;
		if (fp->_flag & _IONBF)	/* use tmp_buffer to reduce write()s */
		{
			tmp_bfile.fd = fp->_file;
		localbuf:;
			tmp_bfile.file._ptr = &tmp_buffer[0];
			tmp_bfile.file._cnt = sizeof(tmp_buffer);
			tmp_bfile.begptr = &tmp_buffer[0];
			tmp_bfile.endptr = &tmp_buffer[sizeof(tmp_buffer)];
		}
		else if (fp->_flag & _IOREAD)	/* presumably string-based */
		{
			tmp_bfile.fd = -1;
			tmp_bfile.file._ptr = fp->_ptr;
			tmp_bfile.file._cnt = fp->_cnt;
			tmp_bfile.begptr = fp->_ptr;
			tmp_bfile.endptr = fp->_ptr;
		}
		else	/* assume regular stream; may guess at endptr */
		{
			tmp_bfile.fd = fp->_file;
			if ((tmp_bfile.begptr = fp->_base) == 0)
				goto localbuf;
			if (fp->_cnt <= 0)
				tmp_bfile.endptr = fp->_base + BUFSIZ - 8;
			else
				tmp_bfile.endptr = fp->_ptr + fp->_cnt;
			tmp_bfile.file._ptr = fp->_ptr;
			tmp_bfile.file._cnt
				= tmp_bfile.endptr - tmp_bfile.file._ptr;
		}
		tmp_bfile.next = (BFILE *)fp;
		bp = &tmp_bfile;
		fp = &tmp_bfile.file;
	}
	else	/* have a valid associated BFILE */
	{
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
			tmp_bfile.next = (BFILE *)fp;
			fp = &tmp_bfile.file;
		}
	}
	res = _idoprnt(fp, fmt, ap);
	if (bp == &tmp_bfile)
	{
		fp = (FILE *)tmp_bfile.next;
		if (fp->_base == 0 || fp->_flag & _IONBF)
		{
			if (tmp_bfile.file._ptr > tmp_bfile.begptr)
				_xflsbuf(&tmp_bfile);
		}
		else	/* copy updated _cnt and _ptr */
		{
			fp->_ptr = tmp_bfile.file._ptr;
			fp->_cnt = tmp_bfile.file._cnt;
		}
		fp->_flag |= tmp_bfile.file._flag & _IOERR;
	}
	else
	{
	ret:;
		STDUNLOCK(&bp->lock);
	}
	return res;
}
