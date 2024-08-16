/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkstrncpy.c	1.3.5.2"
#ident  "$Header: bkstrncpy.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <string.h>

void
bkstrncpy( to, tosz, from, fromsz )
char *to, *from;
int tosz, fromsz;
{
	if( fromsz < tosz )
		(void) strcpy( to, from );
	else {
		(void) strncpy( to, from, tosz - 1 );
		to[ tosz - 1 ] = '\0';
	}
}

