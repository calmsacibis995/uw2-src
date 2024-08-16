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

#ident	"@(#)libmail:libmail/trimnl.c	1.1"
#ident	"@(#)libmail:libmail/trimnl.c	1.1"
#include "libmail.h"
/*
    NAME
	trimnl - trim trailing newlines from string

    SYNOPSIS
	void trimnl(char *s)

    DESCRIPTION
	trimnl() goes to the end of the string and
	removes an trailing newlines.
*/

void trimnl(s)
register char 	*s;
{
    register char	*p;

    p = s + strlen(s) - 1;
    while ((*p == '\n') && (p >= s))
	*p-- = '\0';
}
