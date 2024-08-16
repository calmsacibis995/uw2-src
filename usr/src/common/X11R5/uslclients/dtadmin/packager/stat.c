/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:packager/stat.c	1.1"
#endif

#define STAT_C

#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


time_t
StatFile (filename)
char *		filename;
{
	struct stat status;
	
	if (stat (filename, &status) == -1) {
		return (time_t)0;
	}
	return status.st_mtime;
}
