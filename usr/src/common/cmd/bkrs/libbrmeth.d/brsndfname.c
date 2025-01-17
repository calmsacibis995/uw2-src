/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brsndfname.c	1.5.5.2"
#ident  "$Header: brsndfname.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<time.h>
#include	<stdio.h>
#include	<bkrs.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<errno.h>

extern int brtype;
extern pid_t bkdaemonpid;
    
extern unsigned int strlen();
extern char *strcpy();
extern char *strncpy();
extern int bkm_send();
extern void brlog();

int
brsndfname( fname )
char *fname;
{
	bkdata_t msg;

	switch( brtype ) {
	case BACKUP_T:
		if( strlen( fname ) < BKTEXT_SZ ) 
			(void) strcpy( msg.text.text, fname );
		else {
			(void) strncpy( msg.text.text, fname, BKTEXT_SZ - 1 );
			(msg.text.text)[ BKTEXT_SZ - 1 ] = '\0';
		}

		if( bkm_send( bkdaemonpid, TEXT, &msg ) == -1 ) {
			brlog(
				"brsndfname(): Unable to send TEXT message to bkdaemon; errno %ld",
				errno );
			return( BRFAILED );
		}
		break;
	
	case RESTORE_T:
		(void) fprintf( stdout, "%s\n", fname );
		break;

	default:
		return( BRNOTINITIALIZED );
	}

	return( BRSUCCESS );
}

