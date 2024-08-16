/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsstatus.d/admin_auth.c	1.2.5.2"
#ident  "$Header: admin_auth.c 1.2 91/06/21 $"

#define TRUE	1
#define FALSE	0

/* Routine determines whether user is an authorized administrator.  */
/* It returns TRUE if user is an administrator, FALSE otherwise.    */
/* THIS ROUTINE MUST BE REPLACED WHEN THE AUTHORIZATION SOFTWARE IS */
/* AVAILABLE. */
admin_auth()
{
	unsigned short getuid();

	return ( getuid() == (unsigned short)0 );
}
