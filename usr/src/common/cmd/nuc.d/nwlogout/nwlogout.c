/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwlogout:nwlogout.c	1.12"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwlogout/nwlogout.c,v 1.8.2.5 1995/02/13 19:29:44 hashem Exp $"

/*
**  NetWare Unix Client nwlogout Utility
**
**	MODULE:
**		nwlogout.c	-	The NetWare UNIX Client nwlogout Utility
**
**	ABSTRACT:
**		The nwlogout.c contains the UnixWare utility to allow a user
**		to log out (deauthenticate and detach) from a NetWare file
**		server(s).  This terminates a users access to the file server(s).
**
*/ 

#include <stdio.h>
#include <string.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>



/*
 * BEGIN_MANUAL_ENTRY(nwlogin(1), ./man/nwlogout )
 *
 * NAME
 *		nwlogout - Log out of a NetWare file server
 *
 * SYNOPSIS
 *		nwlogout [file server(s)]
 *
 * INPUT
 *		file server		The file server to be logged out from
 *
 * OUTPUT
 *		None.
 *
 * RETURN VALUE
 *		0				Successful completion.
 *		1				Error during execution.
 *
 * DESCRIPTION
 *		
 *	
 *
 * SEE ALSO
 *		nwlogin(1)
 *
 * END_MANUAL_ENTRY
 */
int
main( int argc, char* argv[] )
{
	int					c;
	int					i;
	char*				ServerName;
	int					LogoutAll = FALSE;
	int					use_force = FALSE;
	uint32				Flags;
	uint32				rc;
	uint32				ScanIndex=0;
	extern int			optind;
	NWCONN_HANDLE		ConnID;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nwlogout" );
	setcat( "uvlnuc" );

	if( argc == 1 ){
		usage();
		return( FAILED );
	}

	while( (c = getopt(argc, argv, "a?f")) != EOF ){
		switch( c ){
			case 'a' :
				LogoutAll=TRUE;
				break;
			case 'f':
				use_force = TRUE;
				break;
			case '?' : 
				usage();
				return( SUCCESS );
			default :
				usage();
				return( FAILED );
		}
	}

	i = optind;

	while( TRUE ){
		if( LogoutAll ){
			rc = NWScanConnID( &ScanIndex, &ConnID );
			if( rc == SCAN_COMPLETE ){
				break;
			}else if( rc ){
				Error( ":426:Unable to scan user connections. (0x%x)\n", rc );
				break;
			}
		}else{
			if( i >= argc ){
				break;
			}

			/*	Parse command line args for serverName
			 */
			ServerName = strtok( argv[i],  " \n\t" );
			if( ServerName == NULL) {
				Error(":427:Invalid server name argument.\n");
				return( FAILED );
			}

			/*	Look up ConnID (connection ID) using the server name
			 */
			i++;
			rc = NWGetConnIDByName( ServerName, &ConnID );
			if(rc) {
				nprintf(":435:No connection exists to server %s.\n", ServerName);
				continue;
			}
		}

		/*	Log out and Detach from the file server
		 */
		rc = NWLogout( ConnID, Flags );
		switch( rc ){
			case SUCCESS:
				break;
			case ERR_CONN_NOT_LOGGED_IN:
				break;
			default:
				Error( ":428:Logout from file server failed. (0x%x)\n", rc );
				break;
		}

		if (use_force == TRUE) {
			rc = sys_close_conn_with_force( ConnID );
			if(rc)
				Error(":429:Unable to destroy connection #%d. (0x%x)\n",
				  				ConnID, rc );
		} else {
			rc = NWDetach( ConnID );
			if(rc)
				Error(":430:Unable to detach from connection #%d. (0x%x)\n",
				  				ConnID, rc );
		}
	}

	return( SUCCESS );
}

int
usage()
{
	pfmt( stderr, MM_ACTION, ":326:Usage: nwlogout [-af] Server\n" );
}
