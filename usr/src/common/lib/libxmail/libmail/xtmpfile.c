/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/xtmpfile.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)xtmpfile.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	xtmpfile - return pointer to temporary file

    SYNOPSIS
	FILE *xtmpfile()

    DESCRIPTION
	xtmpfile() invokes tmpfile() for an open file. If that fails,
	say because /var/tmp is unwritable, it attempts to use tempnam(0)
	and mktemp() to create the file. If that fails, it tries using
	tempnam("/tmp") and mktemp().
*/
FILE *xtmpfile()
{
    char *name;
    FILE *ret = tmpfile();
    if (ret) return ret;
    name = tempnam((char*)0, ".MM");
    if (!name) name = tempnam("/tmp", ".MM");
    if (!name) return 0;
    ret = fopen(name, "w+");
    if (ret) { (void) unlink(name); free(name); return ret; }
    free(name);
    name = tempnam("/tmp", ".MM");
    if (!name) return 0;
    ret = fopen(name, "w+");
    if (ret) { (void) unlink(name); free(name); return ret; }
    free(name);
    return 0;
}
