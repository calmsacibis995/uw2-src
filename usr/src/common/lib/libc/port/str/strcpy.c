/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strcpy.c	1.6"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#include "synonyms.h"
#include <string.h>

/*
 * Copy string s2 to s1.  s1 must be large enough.
 * return s1
 */

char *
strcpy(s1, s2)
register char *s1;
register const char *s2;
{
	char *os1 = s1;

	while(*s1++ = *s2++)
		;
	return(os1);
}
