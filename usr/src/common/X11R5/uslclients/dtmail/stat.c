/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:stat.c	1.7"
#endif

#define STAT_C

#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


time_t
StatFile(filename, inode, size)
char *		filename;
o_ino_t	*	inode;
off_t *		size;
{
	struct stat status;
	
	if (size != (off_t *)0) {
		*size = (off_t)0;
	}
	if (inode != (o_ino_t *)0) {
		*inode = -1;
	}
	if (stat (filename, &status) == -1) {
		return (time_t)0;
	}
	if (inode != (o_ino_t *)0) {
		*inode = status.st_ino;
		if (size != (off_t *)0) {
			*size = status.st_size;
		}
	}
	return status.st_mtime;
}

mode_t
GetUmask ()
{
	mode_t	mask;

	mask = umask (0);
	(void)umask (mask);
	mask = ~mask & (S_IRWXU | S_IRWXG | S_IRWXO);

	return mask;
}
/*
 * Return codes:
 *	-1: file doesn't exist.
 *       0: file is a directory.
 *       1: file is a regular file.
 *	 2: file is something else.
 */

int
CheckFileType (char *name)
{
	struct stat status;
	
	if (stat (name, &status) == -1) {
		return -1;
	}
	if (S_ISREG(status.st_mode)) {
		return 1;
	}
	if (S_ISDIR(status.st_mode)) {
		return 0;
	}
	return 2;
}
