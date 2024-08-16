/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strtok_r.c	1.2"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak strtok_r = _strtok_r
#endif

#include "synonyms.h"
#include <string.h>
#include <stddef.h>

/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */

char *
strtok_r(string, sepset, lasts)
register char *string;
const char *sepset;
char **lasts;
{
	register char	*q, *r;

	q = string;

	/*first or subsequent call*/
	if (q == NULL)
		q = *lasts;

	if(q == 0)		/* return if no tokens remaining */
		return(NULL);

	q = q + strspn(q, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		*lasts = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		*lasts = r+1;
	}
	return(q);
}
