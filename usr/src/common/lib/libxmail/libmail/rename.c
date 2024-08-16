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

#ident	"@(#)libmail:libmail/rename.c	1.1"
#ident	"@(#)libmail:libmail/rename.c	1.1"
#include "libmail.h"
/*
    NAME
	rename - implementation of rename() for older systems

    SYNOPSIS
	int rename(char *old, char *new)

    DESCRIPTION
	Rename the given old filename to the given new filename.
*/
#ifdef SVR3
int rename(old, new)
char *old;
char *new;
{
    /* eliminate the new name */
    (void) unlink(new);

    /* link it in */
    if (link(old,new) != 0)
	return -1;

    /* unlink the old name */
    if (unlink(old) != 0)
	return -1;
    return 0;
}
#endif
