/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_findbuf.c	1.6.3.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <unistd.h>
#ifdef __STDC__
#   include <limits.h>
#   include <stdlib.h>
#else
    extern char *malloc();
#endif
#include "stdiom.h"
#include <sys/stat.h>

BFILE *
#ifdef __STDC__
_findbuf(register FILE *fp)	/* associate a buffer with the stream */
#else
_findbuf(fp)register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	if (bp == 0) /* compatibility with old fopen()s: reset _base */
		bp = _fixbase(fp);
	if (fp->_flag & _IONBF)	/* use "micro" buffer, but with 0 size */
	{
		bp->begptr = &fp->_buf[0];
		bp->endptr = &fp->_buf[0];
	}
	else	/* attempt to allocate a full-sized buffer */
	{
		struct stat statbuf;
		register Uchar *p;

		if (isatty(bp->fd))
		{
			fp->_flag |= _IOLBF;
			statbuf.st_blksize = BUFSIZ;
		}
		else if (fstat(bp->fd, &statbuf) != 0)
		{
			statbuf.st_blksize = BUFSIZ;
		}
		else if (statbuf.st_blksize > MAXBUFSIZ)
		{
			statbuf.st_blksize = MAXBUFSIZ;
		}
		if ((p = (Uchar *)malloc((size_t)statbuf.st_blksize)) == 0)
		{
			p = fp->_buf;
			statbuf.st_blksize = sizeof(fp->_buf);
		}
		else
		{
			fp->_flag |= _IOMYBUF;
		}
		bp->begptr = &p[0];
		bp->endptr = &p[statbuf.st_blksize];
	}
	fp->_ptr = bp->begptr;
	fp->_cnt = 0;
	if ((fp->_flag & (_IOWRT | _IONBF | _IOLBF)) == _IOWRT)
		fp->_cnt = bp->endptr - bp->begptr;
	return bp;
}

BFILE *
#ifdef __STDC__
_fixbase(register FILE *fp)	/* resets "broken" FILE/BFILEs */
#else
_fixbase(fp)register FILE *fp;
#endif
{
	register BFILE *bp;

	/*
	* Some older programs contain their own copy of an old fopen.c,
	* which clears fp->_base just before returning.  This used to
	* indicate that _findbuf() needed to be called to locate a
	* buffer for the stream.  _fixbase() is now called by _findbuf(),
	* setbuf() and setvbuf() for a null bp (due to a null fp->_base).
	*
	* Note that often this null bp will be dereferenced before
	* getting repaired here, and thus this compatibility code
	* depends on having a zero-valued readable page at virtual
	* address zero.
	*/
	if (fp == stdin)
		bp = STDIN;
	else if (fp == stdout)
		bp = STDOUT;
	else if (fp == stderr)
		bp = STDERR;
	else
		bp = (BFILE *)fp;
	if (bp->_base != (Uchar *)bp)
	{
		fp->_base = (Uchar *)bp;
		bp->fd = fp->_file; /* believe the FILE version */
	}
	return bp;
}
