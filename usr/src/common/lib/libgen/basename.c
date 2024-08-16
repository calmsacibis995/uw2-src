/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:basename.c	1.3"

/*
	Return pointer to the last element of a pathname.
*/

#ifdef __STDC__
	#pragma weak basename = _basename
#endif
#include "synonyms.h"

#include	<string.h>


char *
basename( s )
char	*s;
{
	register char	*p;

	if( !s  ||  !*s )			/* zero or empty argument */
		return  ".";

	p = s + strlen( s );
	while( p != s  &&  *--p == '/' )	/* skip trailing /s */
		*p = '\0';
	
	if ( p == s && *p == '\0' )		/* all slashes */
		return "/";

	while( p != s )
		if( *--p == '/' )
			return  ++p;

	return  p;
}
