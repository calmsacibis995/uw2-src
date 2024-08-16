/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strrchr.c	1.7"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#include "synonyms.h"
#include <string.h>
#include <stddef.h>

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
*/

char *
strrchr(sp, c)
register const char *sp;
int c;
{
	register const char *r = NULL;
	register char ch = (char)c;

	do {
		if(*sp == ch)
			r = sp;
	} while(*sp++);
	return((char *)r);
}
