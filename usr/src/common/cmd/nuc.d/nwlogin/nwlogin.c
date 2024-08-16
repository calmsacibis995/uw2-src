/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwlogin:nwlogin.c	1.14"
/*
**  Netware Unix Client
**
**	MODULE:
**		nwlogin.c -	command used to authenticate a user to
**					a NetWare server via the NetWare UNIX
**					Client.
**
**	ABSTRACT:
**		The nwlogin.c contains the NetWare UNIX Client command nwlogin.
**		This command is used to attach and authenticate users to a
**		NetWare server.
**
*/ 
#include <stdio.h>
#include <string.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>

#define NO_PASSWORD				0x00000001
#define PASSWORD_NOT_CHANGED	0x00100002


int		usage(void);
int		ParseCmdLine( int argc, char* argv[], char* ServiceName, char* UserName,
		  uint32* Flags, int* forceFlag );
uint32	ChangePassword( NWCONN_HANDLE ConnID, char* UserName, char* OldPassword,
			uint32* Flags );


int
strcasecmp( char *s1, char *s2 )
{
	char	c1;
	char	c2;

	while((c1 = toupper(*s1)) == (c2 = toupper(*s2)) && *s1 != '\0') {
		s1++;
		s2++;
	}

	if(c1 > c2) {
		return(1);
	}

	if(c2 > c1) {
		return(-1);
	}

	return(0);
}

int
main( int argc, char* argv[] )
{
	char			Password[NWMAX_PASSWORD_LENGTH];
	char			ServiceName[NWMAX_OBJECT_NAME_LENGTH];
	char			UserName[NWMAX_OBJECT_NAME_LENGTH];
	char			CurrentUserName[NWMAX_OBJECT_NAME_LENGTH];
	uint32			Flags=NO_PASSWORD;
	int				forceFlag = 0;
	int				userMatch;
	uint32			rc;
	NWCONN_HANDLE	ConnID;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nwlogin" );
	setcat( "uvlnuc" );

	rc = ParseCmdLine( argc, argv, ServiceName, UserName, &Flags, &forceFlag );
	if( rc ){
		usage();
		return( rc );
	}

	rc = NWAttach( ServiceName, &ConnID, Flags );
	if( rc == SUCCESS || rc == NWERR_ALREADY_ATTACHED ){
		/*	First we'll check to see if the user is already
		 *	logged in.
		 */
		if( isAuthenticated(ConnID) ){
			rc = NWGetUserNameByConnID( ConnID, CurrentUserName );
			if (rc) {
				if (rc == SERVER_CONNECTION_LOST)
					rc = NWSysCloseConn (ConnID);
				else
					rc = NWCloseConn (ConnID);
				rc = NWAttach( ServiceName, &ConnID, Flags );
				if (rc != SUCCESS && rc != NWERR_ALREADY_ATTACHED) {
					fprintf(stderr, (char *)LoginError( rc ));
					return( rc );
				}
			} else {
				userMatch = !strcasecmp(CurrentUserName, UserName);
				if(!forceFlag ){
					nprintf( ":270:You are already logged in to %s as user %s.\n",
					  ServiceName, CurrentUserName );
					nprintf( ":271:Do you want to reattach? (Y/N) " );
					fflush( stdout );
					rc = YesNo();
					if( !rc ){
						return( SUCCESS );
					}
					rc = NWLogout( ConnID, Flags );
				} else {
					if(!userMatch) {
						fprintf(stderr,
							"no Match, CurrentUserName = %s, UserName = %s.\n",
							CurrentUserName, UserName);
						rc = NWLogout( ConnID, Flags );
					} else {
						return( SUCCESS );
					}
				}
			}
		}

		/*	We'll try logging in with no password if that fails
		 *	then we'll prompt the user for the password and try
		 *	logging in again.
		 */
		rc = NWLogin( ConnID, UserName, NULL, Flags );
		if( (rc == NO_SUCH_OBJECT_OR_BAD_PASSWORD) || (rc == NO_SUCH_OBJECT) ){
			/*	Well, the user must have a passwd so prompt
			 *	for it and pass that to NWLogin.
			 */
			Flags &= ~NO_PASSWORD;
			nprintf( ":272:Password for %s on server %s: ", UserName,
			  ServiceName );
			GetPasswd( Password );
			nprintf( ":130:\n" );
			rc = NWLogin( ConnID, UserName, Password, Flags );
		}

		/*	Lets handle the return code.
		*/
		switch( rc ){
			case SUCCESS:
				nprintf( ":273:You Logged in successfully.\n" );
				break;
			case PASSWORD_HAS_EXPIRED:
				nprintf( ":274:The password for %s on %s has expired.\n",\
				  UserName, ServiceName );
				if( Flags & NO_PASSWORD ){
					rc = ChangePassword( ConnID, UserName, NULL, &Flags );
				}else{
					rc = ChangePassword( ConnID, UserName, Password, &Flags );
				}
				break;
			default:
				fprintf(stderr, (char *)LoginError( rc ));
				NWDetach( ConnID );
				break;
		}

		ClearPassword( Password );
	}else{
		fprintf(stderr, (char *)AttachError( rc ));
	}

	return( rc );
}

uint32
ChangePassword(		NWCONN_HANDLE	ConnID,
					char*			UserName,
					char*			OldPassword,
					uint32*			Flags)
{
	char		NewPassword[NWMAX_PASSWORD_LENGTH];
	char		NewPasswordCheck[NWMAX_PASSWORD_LENGTH];
	int			GraceLogins;
	uint32		rc;

	rc = CheckGraceLogins( ConnID, UserName, &GraceLogins );
	if( rc ){
		return( rc );
	}

	if( GraceLogins > 0 ){
		if( GraceLogins != UNLIMITED_GRACE_LOGINS ){
			nprintf( ":275:You have %i grace login(s) left to change your "
			  "password.\n", GraceLogins );
		}
	}else{
		nprintf( ":276:This is your last chance to change your password.\n"
		  "You have no grace logins remaining.\n" );
	}

	rc = VerifyChangePasswordRights( ConnID, UserName, *Flags );
	if( rc ){
		nprintf(":277:You do not have rights to change your password.\n");
		nprintf(":278:Contact your system administrator to have it changed.\n");
		return( rc );
	}

	if( isServerInNNSDomain(ConnID) ){
		Error(
		  ":279:This utility can not change your password on this server.\n" );
		Error( ":280:This server is part of a NetWare Name Service domain.\n" );
		return( SERVER_IN_NNS_DOMAIN );
	}

	if( GraceLogins > 0 ){
		nprintf( ":281:Do you want to change your password? (Y/N) " );
		if( YesNo() == FALSE ){
			return( PASSWORD_NOT_CHANGED );
		}
	}

	ClearPassword( NewPassword );
	ClearPassword( NewPasswordCheck );

	while( TRUE ){
		nprintf( ":282:Enter your new password:" );
		GetPasswd( NewPassword );
		nprintf( ":130:\n" );

		nprintf( ":283:Retype your new password:" );
		GetPasswd( NewPasswordCheck );
		nprintf( ":130:\n" );

		/*	We need to check the following:
		 *		The password was retyped correctly.
		 *		The password is not the same as the user's name.
		 */
		rc = strcmp( NewPassword, NewPasswordCheck );
		ClearPassword( NewPasswordCheck );
		if ( rc ){
			nprintf( ":284:The new password was retyped incorrectly.\n" );
			nprintf( ":285:Do you still want to change your password? (Y/N) " );
			if( YesNo() ){
				continue;
			}
		}else{
			/*	all is ok, try to change the password
			 */
			if( strlen(NewPassword) == 0 ){
				rc = NWChangePassword( ConnID, UserName, OldPassword,
				  NULL );
			}else{
				rc = NWChangePassword( ConnID, UserName, OldPassword,
				  NewPassword );
			}
			switch( rc ){
				case SUCCESS:
					nprintf( ":286:Your password has been changed.\n" );
					break;
				case PASSWORD_TOO_SHORT:
				case PASSWORD_NOT_UNIQUE:
				case NWERR_PASSWORD_IS_USERNAME:
					fprintf(stderr, (char *)ChangePasswordErrors( rc ));
					nprintf(
					  ":287:Do you still want to change your password? (Y/N) ");
					if( YesNo() ){
						continue;
					}
					break;
				default:
					fprintf(stderr, (char *)ChangePasswordErrors( rc ));
					break;
			}
		}
		break;
	}

	if( !rc ){
		/* We should sync passwords right here. */
	}

	ClearPassword( NewPassword );
	return( rc );
}


/*	Possible command line formats:
 *		nwlogin					Prompt for UserName, use default server
 *		nwlogin user			Use default server
 *		nwlogin -s server		Prompt for UserName
 *		nwlogin server/user
 */
int
ParseCmdLine(	int		argc,
				char*	argv[],
				char*	ServiceName,
				char*	UserName,
				uint32*	Flags,
				int*	forceFlag )
{
	char*				s1;
	char*				s2;
	int					c;
	uint32				rc;
	NWCONN_HANDLE		ConnID;
	extern int			optind;

	if( argc > 3 ){
		return( FAILED );
	} 

	while( TRUE ){
		c = getopt(argc, argv, "s:f");
		if( c == EOF ){
			break;
		}
		switch( c ){
			case 's':
				strcpy( ServiceName, optarg );
				nprintf( ":317:Enter your login name for server %s: ",
						ServiceName );
				GetUserInput( UserName, NWMAX_OBJECT_NAME_LENGTH, NULL, ECHO );
				if( strlen(UserName) == 0 ){
					return( FAILED );
				}
				return( SUCCESS );
			case 'f':
				(*forceFlag)++;
				break;
			case '?':
				return( FAILED );
			default:
				break;
		}
	}

	s1 = strtok(argv[optind],  "/\\");
	s2 = strtok(NULL,  "/\\");

	UserName[NWMAX_OBJECT_NAME_LENGTH-1] = '\0';
	ServiceName[NWMAX_OBJECT_NAME_LENGTH-1] = '\0';

	if( s2 == NULL ){
		rc = NWGetPrimaryConnID( &ConnID );
		if( rc ){
			return( FAILED );
		}
		rc = NWGetServerNameByConnID( ConnID, ServiceName );
		if( rc ){
			return( FAILED );
		}
		if( s1 == NULL ){
			nprintf( ":317:Enter your login name: " );
			GetUserInput( UserName, NWMAX_OBJECT_NAME_LENGTH, NULL, ECHO );
		}else{
			strncpy( UserName, s1, NWMAX_OBJECT_NAME_LENGTH - 1 );
		}
	}else{
		strncpy( ServiceName, s1, NWMAX_OBJECT_NAME_LENGTH - 1 );
		strncpy( UserName, s2, NWMAX_OBJECT_NAME_LENGTH - 1 );
	}

	return( SUCCESS );
}

int 
usage(void)
{
	pfmt( stderr, MM_ACTION, ":320:Usage: nwlogin serverName/userName\n" );
}
