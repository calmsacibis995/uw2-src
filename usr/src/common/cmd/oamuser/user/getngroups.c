/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/getngroups.c	1.2.10.2"
#ident  "$Header: getngroups.c 2.0 91/07/13 $"

#include	<stdio.h>

extern void exit();
extern int get_ngm();

/* Print out the value of NGROUPS_MAX in the kernel */

main()
{
	(void) fprintf( stdout, "%d\n", get_ngm() );
	exit( 0 );
	/*NOTREACHED*/
}
