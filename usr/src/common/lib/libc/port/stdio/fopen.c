/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fopen.c	1.20.2.11"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <stdlib.h>
#else
    extern void free();
#endif
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fopen = _fopen
	#pragma weak freopen = _freopen
#endif
	#pragma weak _realfopen = fopen		/* linkage for libucb.a */
	#pragma weak _realfreopen = freopen	/* linkage for libucb.a */

static FILE *
#ifdef __STDC__
endopen(const char *name, const char *type, register BFILE *bp) /* open file */
#else
endopen(name, type, bp)const char *name, *type; register BFILE *bp;
#endif
{
	register FILE *fp = (FILE *)bp->file._base;
	int mode, plus, fd;

	switch (type[0])
	{
	default:
		errno = EINVAL;
		return 0;
	case 'r':
		mode = O_RDONLY;
		break;
	case 'w':
		mode = O_WRONLY | O_TRUNC | O_CREAT;
		break;
	case 'a':
		mode = O_WRONLY | O_APPEND | O_CREAT;
		break;
	}
	if ((plus = type[1]) == 'b')	/* ignore and skip "b" */
		plus = type[2];
	if (plus == '+')
	{
		mode &= ~(O_RDONLY | O_WRONLY);
		mode |= O_RDWR;
	}
	if ((fd = open(name, mode, 0666)) < 0)
		return 0;
	fp->_file = fd;	/* for binary compatibility with fileno macro */
	bp->fd = fd;
	if (plus == '+')
		fp->_flag = _IORW;
	else if (type[0] == 'r')
		fp->_flag = _IOREAD;
	else
		fp->_flag = _IOWRT;
	if (mode == (O_WRONLY | O_APPEND | O_CREAT))	/* type == "a" */
		lseek(fd, 0L, SEEK_END);
	return fp;
}

FILE *
#ifdef __STDC__
fopen(const char *name, const char *type) /* open name, return new stream */
#else
fopen(name, type)const char *name, *type;
#endif
{
	BFILE *bp;
	FILE *fp;

	if ((bp = _findiop()) == 0)
		return 0;
	if ((fp = endopen(name, type, bp)) == 0)
		bp->eflags = 0;		/* deallocate it */
	STDUNLOCK(&bp->lock);
	return fp;
}

FILE *
#ifdef __STDC__
freopen(const char *name, const char *type, register FILE *fp) /* reuse stream */
#else
freopen(name, type, fp)const char *name, *type; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;

	STDLOCK(&bp->lock);
	if (bp->eflags & IO_ACTIVE) /* protect against fclose()d streams */
	{
		while (bp->eflags & IO_PUSHED)
			_popbuf(bp);
		if (fp->_flag & _IOWRT && fp->_ptr > bp->begptr)
			_xflsbuf(bp);
		close(bp->fd);
		if (fp->_flag & _IOMYBUF)
			free((void *)bp->begptr);
	}
	bp->eflags = IO_ACTIVE;
	bp->begptr = 0;
	bp->endptr = 0;
	fp->_ptr = 0;
	fp->_cnt = 0;
	fp->_flag = 0;
	if ((fp = endopen(name, type, bp)) == 0)
		bp->eflags = 0;		/* deallocate it */
	STDUNLOCK(&bp->lock);
	return fp;
}
