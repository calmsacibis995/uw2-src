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

#ident	"@(#)libmail:libmail/long2str.c	1.1"
#ident	"@(#)libmail:libmail/long2str.c	1.1"
#include "libmail.h"
/*
    NAME
	long_to_string - create a string from a long
	long_to_string_format - create a string from a long in the given printf format

    SYNOPSIS
	string *long_to_string_format(long l, const char *format)
	string *long_to_string(long l)

    DESCRIPTION
	Turn a long into its string representation, using the given format.
	The caller is responsible for destroying the returned string.
*/

string *long_to_string_format(l, format)
long l;
const char *format;
{
    char buf[20];
    (void) sprintf (buf, format, l);
    return s_copy(buf);
}

string *long_to_string(l)
long l;
{
    return long_to_string_format(l, "%ld");
}
