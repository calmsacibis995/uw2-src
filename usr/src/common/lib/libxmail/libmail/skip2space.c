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

#ident	"@(#)libmail:libmail/skip2space.c	1.1"
#ident	"@(#)libmail:libmail/skip2space.c	1.1"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	skiptospace - skip up to white space

    SYNOPSIS
	const char *skiptospace(const char *p)

    DESCRIPTION
	skiptospace() looks through the string for either
	end of the string or a space character.
*/

const char *skiptospace(p)
register const char	*p;
{
    while (*p && !Isspace(*p))
	p++;
    return (p);
}
