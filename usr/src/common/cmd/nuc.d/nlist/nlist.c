/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlist:nlist.c	1.8"
/*
**  NetWare Unix Client nwlogout Utility
**
**	MODULE:
**		nlist.c	-	The NetWare UNIX Client nlist Utility
**
**	ABSTRACT:
**		The nlist.c contains the UnixWare utility to allow a user
**		to log out (deauthenticate and detach) from a NetWare file
**		server(s).  This terminates a users access to the file server(s).
**
*/ 

#include <pfmt.h>
#include <locale.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include "nlist.h"

#define DEFAULT_PAGER					"/usr/bin/pg"
#define PREFERRED_DEFAULT_PAGER			"/usr/bin/more"

void	usage( void );
int		InitPager( pid_t* pid );

int
main( int argc, char* argv[] )
{
	char			ObjTypeString[NWMAX_OBJECT_NAME_LENGTH];
	char			ServiceName[NWMAX_OBJECT_NAME_LENGTH];
	char			ObjectName[NWMAX_OBJECT_NAME_LENGTH];
	extern char*	optarg;
	extern int		optind;
	int				c;
	pid_t			pid=-1;
	uint32			Flags=USE_DEFAULT_FS|BINDERY;
	uint32			rc;
	uint32			rc2;
	NWCONN_HANDLE	ConnID;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nlist" );
	setcat( "uvlnuc" );
	strcpy( ObjTypeString, "" );
	strcpy( ServiceName, "" );
	while( (c = getopt(argc, argv, "abcdo:ns:")) != EOF ){
		switch( c ){
			case 'a':
				Flags |= ACTIVE;
				break;
			case 'b':
				Flags |= BINDERY;
				break;
			case 'c':
				Flags |= CONTINUOUS_LISTING;
				break;
			case 'o':
				strcpy( ObjTypeString, optarg );
				break;
			case 'n':
				Flags |= NAMES_ONLY;
				break;
			case 's':
				Flags &= ~USE_DEFAULT_FS;
				strcpy( ServiceName, optarg );
				break;
			case '?':
				usage();
				return;
		}
	}

	if( optind == (argc - 1) ){
		strcpy( ObjectName, argv[optind] );
	}else if( optind == argc ){
		strcpy( ObjectName, "*" );
	}else{
		usage();
		return(-1);
	}

	if( (strlen(ObjTypeString)) == 0 ){
		strcpy( ObjTypeString, gettxt(":263", "SERVER") );
	}

	if( !(Flags & CONTINUOUS_LISTING) ){
		rc = InitPager( &pid );
		if( rc ){
			Error( ":140:Error initializing pager\n" );
		}
	}

	if( Flags & BINDERY ){
		rc = BinderyListObjects( ServiceName, ObjectName, ObjTypeString, Flags);
		if( rc == NOT_VALID_OBJECT_TYPE ){
			Error( ":141:%s is not a valid object class type.\n",
			  ObjTypeString );
			usage();
			close( STDOUT_FILENO ); /* close stdout so PAGER will exit */
			rc2 = waitpid( pid, NULL, 0 );
			if( rc2 < 0 ){
				Error( ":142:waitpid error, rc2=%i\n", rc2 );
			}
			return( FAILED );
		}
	}

	if( pid > 0 ){
		fflush( NULL );
		close( STDOUT_FILENO ); /* close stdout so PAGER will exit */
		rc2 = waitpid( pid, NULL, 0 );
		if( rc2 < 0 ){
			Error( ":142:waitpid error, rc2=%i\n", rc2 );
		}
	}
	return( rc );
}

void
usage( void )
{
	pfmt( stderr, MM_ACTION,
	  ":363:nlist [-a] [-b] [-c] [-o type] [-n] [-s server] object\n" );
}

int
InitPager( pid_t* pid )
{
	char*			argv0;
	char*			Pager;
	int				fd[2];
	int				rc;

	Pager = getenv( "PAGER" );
	rc = access( Pager, (F_OK | X_OK | EFF_ONLY_OK) );
	if( Pager == NULL || rc == FAILED ){
		Pager = PREFERRED_DEFAULT_PAGER;
		rc = access( Pager, (F_OK | X_OK | EFF_ONLY_OK) );
		if( rc ){
			Pager = DEFAULT_PAGER;
			rc = access( Pager, (F_OK | X_OK | EFF_ONLY_OK) );
			if( rc ){
				return( SUCCESS );
			}
		}
	}

	rc = pipe( fd );
	if( rc < 0 ){
		Error( ":143:pipe error\n" );
		return( FAILED );
	}

	*pid = fork();
	if( *pid < 0 ){
		Error( ":144:fork error\n" );
		return( FAILED );
	}else if( *pid > 0 ){				/* Parent */
		close( fd[0] );					/* Close read end of the pipe */
		if( fd[1] != STDOUT_FILENO ){
			if( dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO ){
				Error( ":145:dup2 error to stdin\n" );
				return( FAILED );
			}
			close( fd[1] );				/* We don't need this after dup2 */
		}
	}else{								/* Child */
		close( fd[1] );					/* Close write end of the pipe */
		if( fd[0] != STDIN_FILENO ){
			if( dup2(fd[0], STDIN_FILENO) != STDIN_FILENO ){
				Error( ":145:dup2 error to stdin\n" );
				return( FAILED );
			}
			close( fd[0] );				/* We don't need this after dup2 */
		}

		/* Set argv0 to point to the program name with out the path */
		argv0 = strrchr( Pager, '/' );
		if( argv0 != NULL ){
			argv0++;					/* Step past the rightmost slash */
		}else{
			argv0 = Pager;				/* No slashes in Pager */
		}

		rc = execl( Pager, argv0, (char*)0 );
		if( rc < 0 ){
			Error( ":146:execl error for %s\n", Pager );
			return( FAILED );
		}
	}

	return( SUCCESS );
}
