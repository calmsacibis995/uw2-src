/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/cs_strncmp.c	1.5.5.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "ctype.h"

/*
 * Compare strings (at most n bytes) ignoring case
 *	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

int
#if	defined(__STDC__)
cs_strncmp(
	char *			s1,
	char *			s2,
	int			n
)
#else
cs_strncmp(s1, s2, n)
register char *s1, *s2;
register n;
#endif
{
	if(s1 == s2)
		return(0);
	while(--n >= 0 && toupper(*s1) == toupper(*s2++))
		if(*s1++ == '\0')
			return(0);
	return((n < 0)? 0: (toupper(*s1) - toupper(*--s2)));
}
