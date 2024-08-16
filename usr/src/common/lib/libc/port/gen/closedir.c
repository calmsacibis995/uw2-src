/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/closedir.c	1.13"
/*
	closedir -- C library extension routine

*/

#ifdef __STDC__
	#pragma weak closedir = _closedir
#endif
#include "synonyms.h"
#include	<sys/types.h>
#include	<dirent.h>
#include	<unistd.h>
#include	<malloc.h>

int
closedir( dirp )
register DIR	*dirp;		/* stream from opendir() */
{
	register int 	tmp_fd = dirp->dd_fd;

	free( (char *)dirp );
	return(close( tmp_fd ));
}
