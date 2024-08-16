/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/jobidtopid.c	1.3.5.2"
#ident  "$Header: jobidtopid.c 1.2 91/06/21 $"

#include	<sys/types.h>

extern int strncmp();
extern int atoi();

/* given a jobid, return the corresponding pid */
pid_t
jobidtopid( jobid )
char *jobid;
{
	register pid;
	if( strncmp( jobid, "back-", 5 ) ) return( 0 );
	return( ((pid = (pid_t) atol( jobid + 5 )) < 0)? 0: pid );
}
