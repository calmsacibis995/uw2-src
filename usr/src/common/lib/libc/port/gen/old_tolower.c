/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/old_tolower.c	1.4"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * If arg is upper-case, return the lower-case, else return the arg.
 * The purpose of this file is to prevent a.outs compiled with the
 * shared C library from failing with the new tolower function
 * The new tolower function introduces an unresolved reference to _libc__ctype
 * that will make those a.outs not work right.
 */
#include "synonyms.h"

int
old_tolower(c)
register int c;
{
	if(c >= 'A' && c <= 'Z')
		c -= 'A' - 'a';
	return(c);
}
