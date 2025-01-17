/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstdio/getws.c	1.1.1.2"
#ident  "$Header: getws.c 1.2 91/06/26 $"

/*
 * Getws reads EUC characters from stdin, converts them to process
 * codes, and places them in the array pointed to by "s". Getws
 * reads until a new-line character is read or an EOF.
 */

#include <stdio.h>
#include <widec.h>

wchar_t *
getws(ptr)
wchar_t *ptr;
{
	wchar_t *ptr0 = ptr;
	register c;

	for ( ; ; ) {
		if ((c = getwc(stdin)) == EOF) {
			if (ptr == ptr0) /* no data */
				return(NULL);
			break; /* no more data */
		}
		if (c == '\n') /* new line character */
			break;
		*ptr++ = c;
	}
	*ptr = 0;
	return(ptr0);
}
