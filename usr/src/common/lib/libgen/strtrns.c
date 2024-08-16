/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:strtrns.c	1.2"

#ifdef __STDC__
	#pragma weak strtrns = _strtrns
#endif
#include "synonyms.h"

/*
	Copy `str' to `result' replacing any character found
	in both `str' and `old' with the corresponding character from `new'.
	Return `result'.
*/

char *
strtrns(str,old,new,result)
register const char *str;
const char *old, *new; 
char *result;
{
	register char *r;
	register const char *o;

	for (r = result; *r = *str++; r++)
		for (o = old; *o; )
			if (*r == *o++) {
				*r = new[o - old -1];
				break;
			}
	return(result);
}
