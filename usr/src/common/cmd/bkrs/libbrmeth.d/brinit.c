/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brinit.c	1.8.5.2"
#ident  "$Header: brinit.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <backup.h>
#include <bkrs.h>

extern char *brcmdname;
extern int brtype;
extern pid_t bkdaemonpid;
extern int bklevels;

extern int bkm_init();
extern void brlog();
extern void exit();
extern int sprintf();
extern pid_t getppid();
extern void brloginit();
extern char *strdup();

char *prog;

/* Initialization routine for methods. */
int
brinit( methodname, type ) 
char *methodname;
int type;
{
	void	bkmessage();
	char	name[ 30 ];

	if( !methodname || !*methodname ) return( BRBADARGS );
	/* XXX - basename of methodname?? */
	prog = brcmdname = strdup( methodname );

	switch( brtype = type ) {

	case BACKUP_T:
		/* Set up message handler */
		(void) sigset( SIGUSR1, bkmessage );
		if( !(bkdaemonpid = bkm_init( BKNAME, TRUE ) ) ) {
			brlog( "brinit(): unable to talk to bkdaemon; errno %ld",
				errno );
			exit( 1 );
		}
		break;

	case RESTORE_T:
		(void) sprintf( name, "RSQ%d", getppid() );
		if( !(bkdaemonpid = bkm_init( name, TRUE ) ) ) {
			brlog( "brinit(): unable to initialize IPC; errno %ld", errno );
			exit( 1 );
		}
		break;

	default:
		return( BRBADARGS );
	}
	brloginit( methodname, type );

#ifdef	TRACE
	brlog( "brinit(): initialize method %s type %s", methodname, 
		(type == BACKUP_T? "BACKUP":
			(type == RESTORE_T? "RESTORE": "UNKNOWN" ) ) );
#endif

	return( 0 );
}
