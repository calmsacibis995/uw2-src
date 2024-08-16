/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:local/lib/fnewer.c	3.1" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

// sets exit status true (0) if file argv[1] is newer than file argv[2]
// snarfed this code from Path/ksh/test.c

// returns the modification time of f1 - modification time of f2
static time_t 
ftime_compare(const char* file1, const char* file2) {
	struct stat statb1,statb2;
	if(stat(file1,&statb1)<0)
		statb1.st_mtime = 0;
	if(stat(file2,&statb2)<0)
		statb2.st_mtime = 0;
	return(statb1.st_mtime-statb2.st_mtime);
}

main(int, char *argv[]) {
	if (ftime_compare(argv[1], argv[2]) > 0)
		return 0;
	else
		return 1;
}
