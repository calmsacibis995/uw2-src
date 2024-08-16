/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsoper.d/mail.c	1.3.5.2"
#ident  "$Header: mail.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<errors.h>

extern pid_t getpid();
extern void bkerror();
extern char *brerrno();

void
m_send_msg( jobid, logname, msg )
char *jobid, *logname, *msg;
{
	FILE	*fptr;
	char tmpfname[ 25 ], command[ 80 ];

	if( !logname ) return;

	(void) sprintf( tmpfname, "/tmp/rsm-%ld", getpid() );
	if( !(fptr = fopen( tmpfname, "w+" )) ) {
		bkerror( stderr, ERROR6, brerrno( errno ), logname );
		return;
	}

	(void) fprintf( fptr, "Restore request %s %s.\n", jobid, msg );

	(void) fclose( fptr );
	(void) sprintf( command, "mail %s <%s; rm -f %s >/dev/null 2>&1", logname,
		tmpfname, tmpfname );
	(void) system( command );
}
