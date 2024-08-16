/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/findgid.c	1.2.10.2"
#ident  "$Header: findgid.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>

extern gid_t findnextgid();
extern void exit();

/* return the next available gid */
main()
{
	gid_t gid = findnextgid();
	if( gid == -1 )
		exit( EX_FAILURE );
	(void) fprintf( stdout, "%ld\n", gid );
	exit( EX_SUCCESS );
	/*NOTREACHED*/
}
