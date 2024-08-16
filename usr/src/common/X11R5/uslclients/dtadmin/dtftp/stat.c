/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:stat.c	1.1.2.1"
#endif

#define STAT_C

#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


off_t
StatFile (char *filename, mode_t *m)
{
	struct stat status;
	
	if (filename == NULL || filename[0] == '\0') {
		return (off_t)-1;
	}
	if (stat (filename, &status) == -1) {
		return (off_t)-1;
	}
	if (m != (mode_t *)0) {
		*m = status.st_mode;
	}
	return status.st_size;
}
