/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/brerrno.c	1.3.5.2"
#ident  "$Header: brerrno.c 1.2 91/06/21 $"

#include	<errno.h>

extern char *sys_errlist[];
extern int sys_nerr;
extern int sprintf();

char *
brerrno( L_errno )
int L_errno;
{
	static char buffer[ 30 ];
	if( L_errno < sys_nerr ) return( sys_errlist[ L_errno ] );
	(void) sprintf( buffer, "Unknown errno %d", L_errno );
	return( buffer );
}

