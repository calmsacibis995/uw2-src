/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/strupr.c	1.1.1.2"
#ident  "$Header: strupr.c 1.1 91/07/03 $"

/* #define	DEBUG		1	/* */

#include	<stdio.h>

/*
			strupr(string)

	Converts passed string to upper case.
*/
strupr(target_file)
char	*target_file;
{
	int	i;

#ifdef DEBUG
	(void) fprintf(stderr, "strupr(): DEBUG - Original value \"%s\"\n", target_file);
#endif

	/*
		Convert target_file to all uppercase
	*/
	for (i = 0; *(target_file + i) != '\0'; i++)
		if (*(target_file + i) > '\140' && *(target_file + i) < '\173')
			*(target_file + i) &= '\337';

#ifdef DEBUG
	(void) fprintf(stderr, "strupr(): DEBUG - Final value \"%s\"\n", target_file);
#endif
}
