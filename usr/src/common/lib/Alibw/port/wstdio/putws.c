/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstdio/putws.c	1.1.2.1"
#ident  "$Header: putws.c 1.2 91/06/26 $"

/*
 * Putws transforms process codes in wchar_t array pointed to by
 * "ptr" into a byte string in EUC, and writes the string followed
 * by a new-line character to stdout.
 */
#include <stdio.h>
#include <widec.h>

int
putws(ptr)
register const wchar_t *ptr;
{
	register const wchar_t *ptr0 = ptr;

	for ( ; *ptr; ptr++) { /* putwc till NULL */
		if (putwc(*ptr, stdout) == EOF)
			return(EOF);
	}
	putwc('\n', stdout); /* append a new line */

	if (fflush(stdout))  /* flush line */
		return(EOF);
	return(ptr - ptr0 + 1);
}
