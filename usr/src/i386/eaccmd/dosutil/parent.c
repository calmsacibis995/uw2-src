/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/parent.c	1.1.1.2"
#ident  "$Header: parent.c 1.1 91/07/03 $"

/*
 *	@(#) parent.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*	parent(fullpath)  --  given the full pathname of a DOS file, returns
 *		NULL if there is no  parent, or a pointer to the pathname of
 *		the immediate parent.
 */

#include	<stdio.h>
#include	"dosutil.h"

char *parent(fullpath)
char *fullpath;
{
	char *c;
	unsigned n;

	while (*fullpath == DIRSEP)		/* ignore leading DIRSEPs */
		fullpath++;

	if ((c = strrchr(fullpath,DIRSEP)) == NULL)
		return(NULL);
	n = c - fullpath;
	c = malloc(n + 1);
	*(c + n) = (char) NULL;
	return( strncpy(c,fullpath,n) );
}
