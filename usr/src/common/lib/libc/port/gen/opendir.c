/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/opendir.c	1.16"
/*
	opendir -- C library extension routine

*/

#ifdef __STDC__
	#pragma weak opendir = _opendir
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<dirent.h>
#include   	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<unistd.h>
#include	"dirm.h"	/* includes stdlock.h */

DIR *
#ifdef __STDC__
opendir(const char *filename)
#else
opendir(filename) const char *filename;	/* name of directory */
#endif
{
	register struct dirplus	*dirp;	/* -> malloc'ed storage */
	register int	fd;		/* file descriptor for read */
	struct stat	sbuf;		/* result of fstat() */

	if ( (fd = open( filename, O_RDONLY|O_NONBLOCK )) < 0 )
		return NULL;
	/* POSIX mandated behavior
	 * close on exec if using file descriptor 
 	 */
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) 
		return NULL;
	if ( (fstat( fd, &sbuf ) < 0)
	  || ((sbuf.st_mode & S_IFMT) != S_IFDIR)
	  || ((dirp = (struct dirplus *)calloc(sizeof(struct dirplus), 1)) == NULL)
	   )	{
		if ((sbuf.st_mode & S_IFMT) != S_IFDIR)
			errno = ENOTDIR;
		
		(void)close( fd );
		return NULL;		/* bad luck today */
		}

	dirp->dirdir.dd_fd = fd;
	dirp->dirdir.dd_loc = dirp->dirdir.dd_size = 0;	/* refill needed */
	dirp->dirdir.dd_buf = dirp->u.buf;

	return (DIR *)dirp;
}
