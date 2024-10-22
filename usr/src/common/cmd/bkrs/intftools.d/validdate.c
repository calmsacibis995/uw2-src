/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/validdate.c	1.2.5.2"
#ident  "$Header: validdate.c 1.2 91/06/21 $"

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

void exit();

/* Program validates that input string represents a legitimate date. */
/* It does this by invoking getdate(3C) via brgetdate(). */

main( argc, argv )
int argc;
char *argv[];
{
	time_t brgetdate();

	if ( argc != 2 ) {
		fprintf( stderr, "%s: expects exactly one argument.\n", argv[0] );
		exit( 1 );
	}

	if ( brgetdate( argv[1] ) == 0 )
		exit( 1 );
	else exit( 0 );
}

