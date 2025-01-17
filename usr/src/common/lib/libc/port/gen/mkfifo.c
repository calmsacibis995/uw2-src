/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/mkfifo.c	1.4"
/*
 * mkfifo(3c) - create a named pipe (FIFO). This code provides
 * a POSIX mkfifo function.
 *
 */

#ifdef __STDC__
	#pragma weak mkfifo = _mkfifo
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/stat.h>
mkfifo(path,mode)
const char *path;
mode_t mode;
{
	mode &= 0777;		/* only allow file access permissions */
	mode |= S_IFIFO;	/* creating a FIFO 		      */
	return(mknod(path,mode,0));
}
