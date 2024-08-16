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
#ident	"@(#)fmli:sys/chgepenv.c	1.2.4.4"

#include	<stdio.h>
#include	"sizes.h"


char *
chgepenv(name, value)
char	*name, *value;
{
	char dirpath[PATHSIZ];
	extern char	*Home;
	char	*strcat();
	char	*strcpy();
	char	*chgenv();

	return chgenv(strcat(strcpy(dirpath, Home ? Home : ""), "/pref/.environ"), name, value);
}
