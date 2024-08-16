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

#ident	"@(#)libmail:libmail/substr.c	1.1"
#ident	"@(#)libmail:libmail/substr.c	1.1"
#include "libmail.h"
/*
    NAME
	substr - find substring

    SYNOPSIS
	int substr(char *string, char *substring)

    DESCRIPTION
	This routine looks for substring in string.
	If found, it returns the position substring is found at,
	otherwise it returns a -1.
*/

int substr(string1, string2)
const char *string1, *string2;
{
	register int i,j, len1, len2;

	len1 = strlen(string1);
	len2 = strlen(string2);
	for (i = 0; i < len1 - len2 + 1; i++) {
		for (j = 0; j < len2 && string1[i+j] == string2[j]; j++)
		    ;
		if (j == len2) return(i);
	}
	return(-1);
}
