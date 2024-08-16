/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/memset.c	1.3.1.5"
/*LINTLIBRARY*/
/*
 * Set an array of n chars starting at sp to the character c.
 * Return sp.
 */
#include "synonyms.h"
#include <string.h>

VOID *
memset(sp1, c, n)
VOID *sp1;
register int c;
register size_t n;
{
    if (n != 0) {
	register char *sp = sp1;
	do {
	    *sp++ = (char)c;
	} while (--n != 0);
    }
    return( sp1 );
}
