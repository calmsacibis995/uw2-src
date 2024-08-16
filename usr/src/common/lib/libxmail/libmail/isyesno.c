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

#ident	"@(#)libmail:libmail/isyesno.c	1.1"
#ident	"@(#)libmail:libmail/isyesno.c	1.1"
#include "libmail.h"
/*
    NAME
	isyesno - check a string for a yes/no answer

    SYNOPSIS
	int isyesno(const char *str, Default_Answer default)

    DESCRIPTION
	Check the string for either "yes" or "no". If the string is yes,
	return 1. If the string is no, return 0. If the string is empty
	or anything else, return the value of the default.
*/

int isyesno(str, def)
const char *str;
Default_Answer def;
{
    if (str)
	{
	int len = strlen(str);
	if (len == 0)
	    return def;
	if (casncmp(str, "yes", len) == 0)
	    return 1;
	if (casncmp(str, "no", len) == 0)
	    return 0;
	return def;
	}
    else
	return def;
}
