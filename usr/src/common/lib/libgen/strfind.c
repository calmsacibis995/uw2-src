/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:strfind.c	1.2"

#ifdef __STDC__
	#pragma weak strfind = _strfind
#endif
#include "synonyms.h"

/*
	If `s2' is a substring of `s1' return the offset of the first
	occurrence of `s2' in `s1',
	else return -1.
*/

strfind(as1,as2)
const char *as1,*as2;
{
	register const char *s1,*s2;
	register char c;
	int offset;

	s1 = as1;
	s2 = as2;
	c = *s2;

	while (*s1)
		if (*s1++ == c) {
			offset = s1 - as1 - 1;
			s2++;
			while ((c = *s2++) == *s1++ && c) ;
			if (c == 0)
				return(offset);
			s1 = offset + as1 + 1;
			s2 = as2;
			c = *s2;
		}
	 return(-1);
}
