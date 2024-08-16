/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/l64a.c	1.14"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * convert long int to base 64 ascii
 * char set is [./0-9A-Za-z]
 * two's complement negatives are assumed,
 * but no assumptions are made about sign propagation on right shift
 *
 */

#ifdef __STDC__
	#pragma weak l64a = _l64a
	#pragma weak l64a_r = _l64a_r
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <values.h>

#define BITSPERCHAR	6 /* to hold entire character set */
#define BITSPERLONG	(BITSPERBYTE * sizeof(long))
#define NMAX	((BITSPERLONG + BITSPERCHAR - 1)/BITSPERCHAR)
#define SIGN	(-(1L << (BITSPERLONG - BITSPERCHAR - 1)))
#define CHARMASK	((1 << BITSPERCHAR) - 1)
#define WORDMASK	((1L << ((NMAX - 1) * BITSPERCHAR)) - 1)

static char buf[NMAX + 1];

char *
#ifdef __STDC__
l64a_r(register long lg, char *buffer, size_t buflen)
#else
l64a_r(lg, buffer, buflen)
register long lg;
char *buffer;
size_t buflen;
#endif
{
	register char *s = buffer;

	while (lg != 0) {

		register int c = ((int)lg & CHARMASK) + ('0' - 2);

		if (c > '9')
			c += 'A' - '9' - 1;
		if (c > 'Z')
			c += 'a' - 'Z' - 1;
		if (--buflen < 1)
			break;
		*s++ = (char)c;
		/* fill high-order CHAR if negative */
		/* but suppress sign propagation */
		lg = ((lg < 0) ? (lg >> BITSPERCHAR) | SIGN :
			lg >> BITSPERCHAR) & WORDMASK;
	}
	*s = '\0';
	return (buffer);
}

char *
#ifdef __STDC__
l64a(register long lg)
#else
l64a(lg)
register long lg;
#endif
{
	return(l64a_r(lg, buf, NMAX+1));
}
