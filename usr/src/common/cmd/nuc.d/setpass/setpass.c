/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)setpass:setpass.c	1.6"
#include <stdio.h>
#include <string.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>

#define	CHANGING_OWN_PASSWORD	0x00100010
#define	NO_PASSWORD				0x00100011

uint32 ChangePassword( char* ServerName, char* UserName );

uint32
main( int argc, char* argv[] )
{
	char				ServerName[NWMAX_OBJECT_NAME_LENGTH];
	char				UserName[NWMAX_OBJECT_NAME_LENGTH];
	char*				s;
	int					c;
	extern int			optind;
	uint32				rc;

	setlocale( LC_ALL, "" );
	setlabel( "UX:setpass" );
	setcat( "uvlnuc" );

	while( TRUE ){
		c = getopt(argc, argv, "");
		if( c == EOF ){
			break;
		}
		switch( c ){
			case '?':
			default:
				usage();
				return;
		}
	}

	if( (argc - optind) == 0 ){
		strcpy( ServerName, "" );
		strcpy( UserName, "" );
#ifdef NDS
		NWisDSAuthenticated();
#endif NDS
	}else if( (argc - optind) == 1 ){
		strncpy( ServerName, strtok(argv[optind], "/\0\n\t"),
		  NWMAX_OBJECT_NAME_LENGTH );
		s = strtok( NULL, "/\0\n\t");
		if( s == NULL ){
			strcpy( UserName, "" );
		}else{
			strncpy( UserName, s, NWMAX_OBJECT_NAME_LENGTH );
		}
	}else if( (argc - optind) > 1 ){
		usage();
	}

	rc = ChangePassword( ServerName, UserName );

	return( rc );
}

uint32
ChangePassword( char* ServerName, char* UserName )
{
	char				OldPassword[NWMAX_PASSWORD_LENGTH];
	char				NewPassword[NWMAX_PASSWORD_LENGTH];
	char				RetypedNewPassword[NWMAX_PASSWORD_LENGTH];
	char				TmpUserName[NWMAX_OBJECT_NAME_LENGTH];
	uint32				Flags;
	uint32				rc;
	NWCONN_HANDLE		ConnID;

	if( strlen( ServerName ) == 0 ){
		rc = NWGetPrimaryConnID( &ConnID );
		if( rc ){
			return( rc );
		}
		rc = NWGetServerNameByConnID( ConnID, ServerName );
		if( rc ){
			return( rc );
		}
	}else{
		rc = NWGetConnIDByName( ServerName, &ConnID );
		if( rc ){
			Error( ":360:You are not logged in to server %s.\n", ServerName );
			return( rc );
		}
	}

	if( !isAuthenticated(ConnID) ){
		Error( ":360:You are not logged in to server %s.\n", ServerName );
		return( ERR_CONN_NOT_LOGGED_IN );
	}

	/*	We need to get the name of the user who is authenticated
	 *	so we know if we are changing our own password or not.
	 */
	rc = NWGetUserNameByConnID( ConnID, TmpUserName );
	if( rc ){
		return( rc );
	}

	if( (strlen(UserName) == 0) || (strcmpi(UserName, TmpUserName) == 0) ){
		Flags |= CHANGING_OWN_PASSWORD;
		strcpy( UserName, TmpUserName );
	}

	rc = VerifyChangePasswordRights( ConnID, UserName, Flags );
	if( !rc ){
		Error( ":340:You do not have rights to change password for %s.\n",
		  UserName );
		Error( ":341:Contact your system administrator to have it changed.\n" );
		return( rc );
	}

	if( isServerInNNSDomain(ConnID) ){
		Error(
		  ":342:This utility can not change your password on this server.\n" );
		Error( ":343:This server is part of a NetWare Name Service domain.\n" );
		return( SERVER_IN_NNS_DOMAIN );
	}

	/*	If we are changing our own password check to see if
	 *	our old password was NULL (no password).
	 */
	if( Flags & CHANGING_OWN_PASSWORD ){
		rc = NWVerifyObjectPassword( ConnID, (uint8*)UserName, OT_USER,
		  (uint8*)"" );
		if( rc == SUCCESS ){
			Flags |= NO_PASSWORD;
		}
	}

	/*	If we are not changing our own password or if our account
	 *	had a password we need to prompt the user for a password.
	 */
	if( (Flags & CHANGING_OWN_PASSWORD) || !(Flags & NO_PASSWORD) ){
		if( !(Flags & NO_PASSWORD) ){
			nprintf( ":344:Enter your old password:" );
		}
		if( (Flags & CHANGING_OWN_PASSWORD) ){
			nprintf( ":345:Enter your password:" );
		}
		GetPasswd( OldPassword );
		nprintf( ":130:\n" );
		/*	Should we verify the password here ? */
	}

	while( TRUE ){
		nprintf( ":346:Enter the new password for %s:", UserName );
		GetPasswd( NewPassword );
		nprintf( ":130:\n" );
		nprintf( ":347:Retype the new password for %s:", UserName );
		GetPasswd( RetypedNewPassword );
		nprintf( ":130:\n" );

		rc = strcmp(NewPassword, RetypedNewPassword);
		ClearPassword( RetypedNewPassword );
		if( rc ){
			Error( ":348:The new password was retyped incorrectly.\n" );
			nprintf( ":349:Do you still want to change your password? (Y/N) " );
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
					nprintf( ":350:The password has been changed.\n" );
					break;
				case PASSWORD_TOO_SHORT:
				case PASSWORD_NOT_UNIQUE:
				case NWERR_PASSWORD_IS_USERNAME:
					fprintf(stderr, (char *)ChangePasswordErrors( rc ));
					nprintf(
					  ":349:Do you still want to change the password? (Y/N) " );
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

	if( rc == SUCCESS ){
		/* We should sync passwords right here. */
	}

	ClearPassword( NewPassword );
	return( rc );
}



int 
usage(void)
{
	pfmt( stderr, MM_ACTION, ":359:Usage: setpass [serverName[userName]]\n" );
}
