/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fdopen.c	1.16.3.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak fdopen = _fdopen
#endif

FILE *
#ifdef __STDC__
fdopen(int fd, const char *type)	/* associate stream with fd */
#else
fdopen(fd, type)int fd; const char *type;
#endif
{
	register BFILE *bp;
	FILE *fp;
	int plus, mode, flag;

	if ((mode = fcntl(fd, F_GETFL)) == -1)	/* actual mode for fd */
		return 0;
	flag = mode & O_ACCMODE;
	switch (type[0])
	{
	default:
	badfd:;
		errno = EINVAL;
		return 0;
	case 'r':
		if (flag == O_WRONLY)	/* O_APPEND okay, unless "r+" */
			goto badfd;
		flag = _IOREAD;
		break;
	case 'w':
		if ((mode & O_APPEND) != 0 || flag == O_RDONLY)
			goto badfd;
		flag = _IOWRT;
		break;
	case 'a':
		if (flag == O_RDONLY)
			goto badfd;
		/*
		* O_APPEND not needed if connected to unseekable "device".
		*/
		if (mode & O_APPEND)
			(void)lseek(fd, 0L, SEEK_END);
		else if (lseek(fd, 0L, SEEK_CUR) != -1)
			goto badfd;
		flag = _IOWRT;
		break;
	}
	if ((plus = type[1]) == 'b')	/* ignore and skip "b" */
		plus = type[2];
	if (plus == '+')
	{
		if ((mode & O_ACCMODE) != O_RDWR
			|| flag == _IOREAD && (mode & O_APPEND) != 0)
		{
			goto badfd;
		}
		flag = _IORW;
	}
	if ((bp = _findiop()) == 0)
		return 0;
	fp = (FILE *)bp->file._base;
	fp->_file = fd;	/* for binary compatibility with fileno macro */
	bp->fd = fd;
	fp->_flag = flag;
	STDUNLOCK(&bp->lock);
	return fp;
}
