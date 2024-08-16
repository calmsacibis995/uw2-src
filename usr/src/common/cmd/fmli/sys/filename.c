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
#ident	"@(#)fmli:sys/filename.c	1.1.4.3"

#include <stdio.h>

char *
filename(pt)
register char *pt;
{
	register char *name;
	char *strrchr();

	if (pt == NULL)
		return "(null)";
	if ((name = strrchr(pt, '/')) == NULL)
		return pt;

	return name + 1;
}
