/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/expdate.c	1.2.11.3"
#ident  "$Header: expdate.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<users.h>
#include	<locale.h>
#include	<pfmt.h>

extern void exit();
extern int valid_expire();

/* Validate an expiration date */
main(argc, argv)
	char *argv[];
{
	if (argc != 2) {
		(void) pfmt(stderr, MM_ERROR, ":1336:synopsis: expdate date\n");
		exit(EX_SYNTAX);
	}
	exit(valid_expire(argv[1], 0 ) == INVALID ? EX_FAILURE : EX_SUCCESS);
	/*NOTREACHED*/
}
