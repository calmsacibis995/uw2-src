/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <stdio.h>

b_argprint(argc, argv)
int argc;
char *argv[];
{
	register int i;

	for (i = 1; i < argc; i++) {
		altprintf("ARG[%d] = %s\n", i, argv[i]);
	}
	return(0);
}
