/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/truncate.c	1.4"
/*LINTLIBRARY*/
/*********************************************************
 * ftruncate() and truncate() set a file to a specified
 * length using fcntl(F_FREESP) system call. If the file
 * was previously longer than length, the bytes past the
 * length will no longer be accessible. If it was shorter,
 * bytes not written will be zero filled.
 */

#if !defined(ABI) && !defined(DSHLIB)
#ifdef __STDC__
	#pragma weak ftruncate = _ftruncate
	#pragma weak truncate = _truncate
#endif
#endif
#include "synonyms.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

ftruncate(fildes, len)
int fildes;
off_t len;
{
	struct flock lck;

	lck.l_whence = 0;	/* offset l_start from beginning of file */
	lck.l_start = len;
	lck.l_type = F_WRLCK;	/* setting a write lock */
	lck.l_len = 0L;		/* until the end of the file address space */

	if(fcntl(fildes, F_FREESP, (int)&lck) == -1){
		return(-1);
	}
	else
		return(0);
}

truncate(path, len)
const char *path;
off_t len;
{

	register fd;


	if((fd = open(path, O_WRONLY)) == -1){
		return(-1);
	}

	if(ftruncate(fd, len) == -1){
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}
