/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/bsd/escape.c	1.1.5.4"
#ident	"$Header: $"

#include "string.h"

/*
 * Return 1 if character in string is escaped by \, 0 otherwise
 */
#if defined (__STDC__)
escaped(char *cp)
#else
escaped(cp)
char	*cp;
#endif
{
	register char	*cp2 = cp;

	while (*--cp2 == '\\')
		;
	return((cp - ++cp2) & 1);
}

/*
 * Remove excape characters from string
 */
void
#if defined (__STDC__)
rmesc(register char *cp)
#else
rmesc(cp)
register char	*cp;
#endif
{
	register char	*cp2 = cp;

	for (; *cp2; cp++, cp2++) {
		if (*cp2 == '\\' || *cp2 == '\'')
			++cp2;
		*cp = *cp2;
	}
	*cp = NULL;
}

/*
 * Copy string, escaping special characters
 */
void
#if defined (__STDC__)
canonize(register char *to, register char *from, char *echars)
#else
canonize(to, from, echars)
register char	*to;
register char	*from;
char		*echars;
#endif
{
	register char	*p;

	for (; *from; from++, to++) {
		for (p = echars; *p; p++)
			if (*p == *from) {
				*to++ = '\\';
				break;
			}
		*to = *from;
	}
	*to = NULL;
}
