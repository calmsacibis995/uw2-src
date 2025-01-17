/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/parent.c	1.2.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"sizes.h"

char *
parent(path)
register char *path;
{
	register char	*dst;
	register char	*place;
	register bool	slash;
	static char	dirname[PATHSIZ];

	/* first, put a "well-behaved" path into dirname */
	place = NULL;
	slash = FALSE;
	for (dst = dirname; *path; path++)
		if (*path == '/')
			slash = TRUE;
		else {
			if (slash) {
				place = dst;
				*dst++ = '/';
				slash = FALSE;
			}
			*dst++ = *path;
		}
	if (dst == dirname) {
		place = dst;
		*dst++ = '/';
	}
	if (place == NULL) {
		dirname[0] = '.';
		dirname[1] = '\0';
	}
	else if (place == dirname)
		dirname[1] = '\0';
	else
		*place = '\0';
	return dirname;
}
