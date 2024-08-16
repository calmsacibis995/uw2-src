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

#ident	"@(#)libmail:libmail/abspath.c	1.1"
#ident	"@(#)libmail:libmail/abspath.c	1.1"
#include "libmail.h"
/*
    NAME
	abspath - expand a path relative to some `.'

    SYNOPSIS
	string *abspath(const char *path, const char *dot, string *to)

    DESCRIPTION
	If "path" is relative (does not start with `.'), the
	the value of "dot" will be prepended and the result
	returned in "to". Otherwise, the value of "path" is
	returned in "to".
*/

extern string *
abspath(path, dot, to)
	const char *path;
	const char *dot;
	string *to;
{
	return (*path == '/') ? s_append(to, path) : s_xappend(to, dot, path, (char*)0);
}
