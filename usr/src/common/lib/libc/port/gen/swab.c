/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/swab.c	1.11"

/*LINTLIBRARY*/
/*
 * Swab bytes
 */

#ifdef __STDC__
	#pragma weak swab = _swab
#endif

#include "synonyms.h"
#include <stdlib.h>

#define	STEP	temp = *s1++,*s2++ = *s1++,*s2++ = temp

void
swab(from, to, n)
	const void *from;
	void *to;
	register ssize_t n;
{
	register char temp;
	register const char *s1=from;
	register char *s2=to;
	
	if (n <= 1)
		return;
	n >>= 1; n++;
	/* round to multiple of 8 */
	while ((--n) & 07)
		STEP;
	n >>= 3;
	while (--n >= 0) {
		STEP; STEP; STEP; STEP;
		STEP; STEP; STEP; STEP;
	}
}
