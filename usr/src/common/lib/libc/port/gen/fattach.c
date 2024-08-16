/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/fattach.c	1.5"

/*
 * Attach a STREAMS-based file descriptor to an object in the file 
 * system name space.
 */
#ifdef __STDC__
	#pragma weak fattach = _fattach
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/vnode.h>
#include <sys/fs/namenode.h>
#include <sys/mount.h>

int
fattach(fildes, path)
	int fildes;
	char *path;
{
	extern int isastream();
	struct namefd  namefdp;

	switch ( isastream( fildes ) ) {
	case 0:
		errno = EINVAL;
		return( -1 );
	case 1:
		namefdp.fd = fildes;
		return (mount((char *)NULL, path, MS_DATA,(const char *)"namefs", 
			(char *)&namefdp, sizeof(struct namefd)));
	default:
		return( -1 );
	}
}

