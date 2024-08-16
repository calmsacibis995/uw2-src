/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strcmp.c	1.7"
/*LINTLIBRARY*/
/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

#include "synonyms.h"
#include <string.h>

int
strcmp(s1, s2)
register const char *s1;
register const char *s2;
{

	if((unsigned char *)s1 == (unsigned char *)s2)
		return(0);
	while((unsigned char)*s1 == (unsigned char)*s2++)
		if((unsigned char)*s1++ == '\0')
			return(0);
	return((unsigned char)*s1 - (unsigned char)s2[-1]);
}
