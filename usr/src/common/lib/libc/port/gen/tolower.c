/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tolower.c	1.7"
/*
 * If arg is upper-case, return lower-case, otherwise return arg.
 * International version
 */

#include "synonyms.h"
#include <ctype.h>

int
tolower(c)
register int c;
{
	if (isupper(c))
		c = _tolower(c);
	return(c);
}

