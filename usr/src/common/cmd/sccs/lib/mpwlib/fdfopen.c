/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/fdfopen.c	6.5"
/*
	Interfaces with /lib/libS.a
	First arg is file descriptor, second is read/write mode (0/1).
	Returns file pointer on success,
	NULL on failure (no file structures available).
*/

# include	"stdio.h"
# include	"sys/types.h"
# include	"macros.h"

FILE *
fdfopen(fd, mode)
register int fd, mode;
{
	int fstat();
	if (fstat(fd, &Statbuf) < 0)
		return(NULL);
	if (mode)
		return(fdopen(fd,"w"));
	else
		return(fdopen(fd,"r"));
}
