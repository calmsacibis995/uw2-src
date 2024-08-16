/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwwhoami:nwwhoami.c	1.6"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwwhoami/nwwhoami.c,v 1.4.4.3 1995/02/13 19:38:08 hashem Exp $"

/*
**  NetWare Unix Client nwwhoami Utility
**
**	MODULE:
**		nwwhoami.c	-	The NetWare UNIX Client nwwhoami Utility
**
**	ABSTRACT:
**		The nwwhoami.c contains the UnixWare utility to allow a user
**		to display all of their current connections to NetWare file
**		servers.
**
*/ 

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>


uint32 OutputConnInfo( NWCONN_HANDLE LocalConnID );

#define LOGIN_TIME_SIZE						7
#define DESCRIPTION_LENGTH					80


int
main( int argc, char* argv[] )
{
	int				AnyConnection=FALSE;
	NWCONN_HANDLE	ConnID;
	uint32			rc;
	uint32			ScanIndex=0;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nwwhoami" );
	setcat( "uvlnuc" );

	while( TRUE ){
		rc = NWScanConnID( &ScanIndex, &ConnID );
		if( rc == SCAN_COMPLETE ){
			break;
		}else if( rc ){
			Error(":426:Unable to scan user connections. (0x%x)\n", rc );
			return( rc );
		}

		AnyConnection = TRUE;

		nprintf( ":130:\n" );
		rc = OutputConnInfo( ConnID );
	}

	if( AnyConnection == FALSE ){
		nprintf( ":330:You are not logged into any servers.\n" );
	}
	return( SUCCESS );
}

uint32
OutputConnInfo( NWCONN_HANDLE ConnID )
{
	char   			LoginTimeString[BUFSIZ];
	char			Revision[DESCRIPTION_LENGTH];
	char			ServerName[NWMAX_OBJECT_NAME_LENGTH];
	char			UserName[NWMAX_OBJECT_NAME_LENGTH]="";
	struct tm		timePtr;
	uint8			LoginTime[LOGIN_TIME_SIZE];
	NWCONN_NUM		ServerConnID;
	uint32			rc;

	if( isAuthenticated(ConnID) ){
		rc = NWGetUserNameByConnID( ConnID, UserName );
		if( rc ) {
			Error(":432:Unable to obtain User Name for connection %d. (0x%x)\n", 
									ConnID, rc);
			return(rc);
		}
	}else{
		strcpy( UserName, (char *)gettxt(":331", "[Not Authenticated]") );
	}
	nprintf( ":332:User ID:\t%s\n", UserName );

	rc = NWGetServerNameByConnID( ConnID, ServerName );
	if( rc ){
		Error( ":433:Unable to obtain Server Name for connection %d. (0x%x)\n",
									ConnID, rc);
		return( rc );
	}
	rc = NWGetFileServerDescription( ConnID, NULL, (pnstr8)Revision, NULL,
	  NULL );
	if( rc ){
		Error(":434:Unable to obtain Server Description for connection %d. (0x%x)\n",
									ConnID, rc);
		return( rc );
	}
	nprintf( ":333:Server:\t\t%s %s\n", ServerName, Revision );

	rc = NWGetConnNumberByConnID( ConnID, &ServerConnID );
	if( rc ){
		Error(":435:Unable to obtain Server Connection Number for connection %d. (0x%x)\n",
									ConnID, rc);
		return( rc );
	}
	nprintf( ":334:Connection:\t%i\n", ServerConnID );

	if( isAuthenticated(ConnID) ){
		rc = NWGetConnectionInformation( ConnID, ServerConnID, NULL, NULL, NULL,
		  LoginTime );
		if( rc ){
			Error( ":436:Unable to obtain Login Time for connection %d. (0x%x)\n",
									ConnID, rc);
			return( rc );
		}

		timePtr.tm_year	= LoginTime[0];
		timePtr.tm_mon	= LoginTime[1] - 1;
		timePtr.tm_mday	= LoginTime[2];
		timePtr.tm_hour	= LoginTime[3];
		timePtr.tm_min	= LoginTime[4];
		timePtr.tm_sec	= LoginTime[5];
		timePtr.tm_isdst = 0;
		mktime( &timePtr );
		strftime( LoginTimeString, BUFSIZ, "%c", &timePtr);
		nprintf( ":335:Login time:\t%1$s\n", LoginTimeString);
	}
	return( SUCCESS );
}
