/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strcspn.c	1.6"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <string.h>
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters NOT from charset.
 */
size_t
strcspn(string, charset)
const char *string;
const char *charset;
{
	register const char *p, *q;

	for(q=string; *q != '\0'; ++q) {
		for(p=charset; *p != '\0' && *p != *q; ++p)
			;
		if(*p != '\0')
			break;
	}
	return(q-string);
}
