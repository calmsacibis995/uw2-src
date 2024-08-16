/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/basename.c	1.1"
#ident	"@(#)libmail:libmail/basename.c	1.1"
#include "libmail.h"
/*
    NAME
	basename - return base from pathname

    SYNOPSIS
	char *basename(const char *path)

    DESCRIPTION
	basename() returns a pointer to the base
	component of a pathname.

	Like strchr() and family, this function
	returns a "char*" instead of a "const char*".
	This is somewhat questionable, but works for us.
*/

char *
basename(path)
	const char *path;
{
	char *cp;

	cp = strrchr(path, '/');
	return cp==NULL ? (char*)path : cp+1;
}
