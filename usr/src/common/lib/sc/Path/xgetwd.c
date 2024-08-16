/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Path/xgetwd.c	3.2" */
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

#include "Pathlib.h"

#ifdef hpux
char *getcwd(char*, int);
#endif

#ifndef MAXPATHLEN
// hope this is big enough
#define MAXPATHLEN 1026
#endif

Path Path::xgetwd()
{
	static char wd[MAXPATHLEN+1];  // +1 is just to be safe
#ifdef GETWD
	if (getwd(wd) == 0)
	{
		if (Path::no_wd.raise(wd) == 0)
			die(wd);
		*wd = '/';
		wd[1] = '\0';
	}
#else
	getcwd(wd, MAXPATHLEN);
#endif
	return Path(wd);
}
