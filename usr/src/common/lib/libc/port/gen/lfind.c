/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/lfind.c	1.9"

/*LINTLIBRARY*/
/*
 * Linear search algorithm, generalized from Knuth (6.1) Algorithm Q.
 *
 * This version no longer has anything to do with Knuth's Algorithm Q,
 * which first copies the new element into the table, then looks for it.
 * The assumption there was that the cost of checking for the end of the
 * table before each comparison outweighed the cost of the comparison, which
 * isn't true when an arbitrary comparison function must be called and when the
 * copy itself takes a significant number of cycles.
 * Actually, it has now reverted to Algorithm S, which is "simpler."
 */

#ifdef __STDC__
	#pragma weak lfind = _lfind
#endif
#include "synonyms.h"
#include <stdio.h>
#include <stdlib.h>

VOID *
lfind(ky, bs, nelp, width, compar)
const VOID *ky;		/* Key to be located */
const VOID *bs;		/* Beginning of table */
size_t *nelp;		/* Pointer to current table size */
register size_t width;	/* Width of an element (bytes) */
int (*compar)();	/* Comparison function */
{
	typedef char *POINTER;
	register POINTER key = (char *)ky;
	register POINTER base = (char *)bs;
	register POINTER next = base + *nelp * width;	/* End of table */

	for ( ; base < next; base += width)
		if ((*compar)(key, base) == 0)
			return (base);	/* Key found */
	return (NULL);
}
