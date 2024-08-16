/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Globbing for MS-DOS.  Relies on the expansion done by the library
 * startup code. (dds)
 */

#include <stdio.h>
#include <string.h>

main(int argc, char *argv[])
{
	register i;

	for (i = 1; i < argc; i++) {
		fputs(strlwr(argv[i]), stdout);
		putchar(0);
	}
}
