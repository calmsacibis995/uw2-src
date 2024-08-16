/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcstok.c	1.1"

/*
 * uses wcspbrk and wcsspn to break string into tokens on
 * sequentially subsequent calls. returns NULL when no
 * non-separator characters remain.
 * 'subsequent' calls are calls with first argument NULL.
 *
 * Note: The version below is the ISO MSE version with three
 * arguments. The XPG4 version with two arguments is handled
 * in the header file and the #pragma below.
 */

#include <wchar.h>

#ifdef __STDC__
	#pragma weak _wcstok = wcstok
#endif

wchar_t *
#ifdef __STDC__
wcstok(wchar_t *string, const wchar_t *sepset, wchar_t **savept)
#else
wcstok(string, sepset, savept)
wchar_t *string;
const wchar_t *sepset;
wchar_t **savept;
#endif
{
	register wchar_t *p, *q, *r;

	/* first or subsequent call */
	p = ((string == 0) ? *savept: string);

	/* return if no tokens remaining */
	if (p == 0)
		return(0);

	/* skip leading separators */
	q = p + wcsspn(p, sepset);

	/* return if no tokens remaining */
	if (*q == 0)
		return(0);

	/* move past token */
	if ((r = wcspbrk(q, sepset)) == 0)
		*savept = 0; /* indicate this is last token */
	else {
		*r = 0;
		*savept = ++r;
	}

	return(q);
}
