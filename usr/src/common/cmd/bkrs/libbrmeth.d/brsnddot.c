/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brsnddot.c	1.5.5.2"
#ident  "$Header: brsnddot.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<time.h>
#include	<bkrs.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<errno.h>

extern int brtype;
extern pid_t bkdaemonpid;

extern int bkm_send();
extern void brlog();

int
brsnddot()
{
	switch( brtype ) {
	case BACKUP_T:
		if( bkm_send( bkdaemonpid, DOT, (char *)0 ) == -1 ) {
			brlog( "Unable to send DOT message to bkdaemon; errno %ld",
				errno );
			return( BRFAILED );
		}
		break;
	
	case RESTORE_T:
		(void) fprintf( stdout, "." );
		break;

	default:
		return( BRNOTINITIALIZED );
	}

	return( BRSUCCESS );
}
