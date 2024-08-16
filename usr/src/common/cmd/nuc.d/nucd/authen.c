/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:authen.c	1.15"
/******************************************************************************
 ******************************************************************************
 *
 *	AUTHEN.C
 *
 *	All-In-One Authentication Support
 *
 ******************************************************************************
 *****************************************************************************/

#define	DISPLAY_NODE	":0.0"
#define	DISPLAY_VAR		"DISPLAY="

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <dirent.h>
#include <sys/nwctypes.h>
#include <netdir.h>
#include <sys/tiuser.h>
#include <sys/nucam_common.h>
#include <nw/ntypes.h>
#include <nw/nwalias.h>
#include <nw/nwerror.h>
#include <nw/nwclient.h>
#include <nw/nwconnec.h>
#include <nw/nwbindry.h>
#include <sys/utsname.h>
#include <siginfo.h>
#include <wait.h>

#include <pfmt.h>
#include <locale.h>
#include <nct.h>

#ifndef	FAILURE
#define	FAILURE	-1
#endif
#ifndef	SUCCESS
#define	SUCCESS	0
#endif

extern int		nuc_debug;
extern FILE		*log_fd;

/******************************************************************************
 *
 *	nwauthen (  
 *
 *	Authenticate to a server
 *
 *	Entry :
 *
 *	Exit :
 *		0			cool
 *		x			error
 *
 *****************************************************************************/

int
nwauthen (	char			*server,
			int32			userID,
			int				origuid,
			int				sendpid,
			NWCONN_HANDLE	*serverConnID,
			int16			xautoFlags )
{
	int					retval;
	int					newpid;
	CONNECT_INFO		connInfo;
	ccode_t				ccode;
	struct passwd 		*passwdEntry;
	int					status;
	char				env_variable[BUFSIZ];
	struct utsname		sys_name;
	int					saveuid;
	char				snamebuf [ 64 ];
	char				unamebuf [ 150 ];
	char 				uidbuf [ 16 ];
	char				pidbuf [ 16 ];
	NWCConnString		nwcserver;
	siginfo_t			sig_info;
	boolean_t			nwattached = B_FALSE;

	/*
	 *	If there is a connection ID for this user to the server use it.
	 */
	if (GetServerConnID (server, serverConnID) != 0) {
		/*
		 * Need to Attach.
		 */
		ccode = NWAttach( server, serverConnID, 0 );
		if ((ccode == SUCCESS) || (ccode == NWERR_ALREADY_ATTACHED)) {
			nwattached = B_TRUE;
		} else {
			/*
			 *	unable to attach to server
			 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_ERROR,
					":27:authen: Unable to attach to '%s'\n",server );
			(void) seteuid (origuid);
			return ( FAILURE );
		}
	}

	/*
	 * 	Got the serverConnID.  Is the connection authenticated?
	 */
	if (nwisauthen (*serverConnID) == FALSE) {
		/*
		 * Not authenticated.  Need to authenticate to continue. Get the 
		 * NetWare userid name.
		 */
		if ((passwdEntry = getpwuid(userID)) == NULL) {
			/*
			 *	can't get the password entry
			 */
			if (nuc_debug)
				(void)pfmt ( log_fd, MM_ERROR,
					":18:authen: %d not in passwd.\n", userID);
			(void) seteuid (origuid);
			if (nwattached)
				NWDetach (*serverConnID);
			return (FAILURE);
		}

		/*
		 *	fire off xauto
		 */
		if (nuc_debug)
			(void)pfmt ( log_fd, MM_INFO,
				":19:authen: Calling XAUTO - passing PID %d UID = %d\n",
				sendpid,geteuid());

		saveuid = geteuid ();

		newpid = fork1 ();
		if (newpid == 0) {
			uname(&sys_name);
			strcpy (env_variable, DISPLAY_VAR);
			strcat (env_variable, sys_name.nodename);
			strcat (env_variable, DISPLAY_NODE);
			putenv (env_variable);
			sprintf ( snamebuf, "%s", server );
			sprintf ( unamebuf, "%s", passwdEntry->pw_name );
			sprintf ( uidbuf, "%d", saveuid );
			sprintf ( pidbuf, "%d", sendpid );
			if (xautoFlags & NUC_SINGLE_LOGIN_ONLY) {
				/*
				 * Xauto will not prompt the user for authentication 
				 * information.  It only tries to the single login.
				 */
				execl ("/usr/X/bin/xauto", "xauto", "-s", snamebuf, "-u",
						unamebuf, "-i", uidbuf, "-p", pidbuf, "-q", "1", 0);
			} else {
				/*
				 * Xauto will prompt the user for authentication
				 * information.
				 */
				execl ("/usr/X/bin/xauto", "xauto", "-s", snamebuf, "-u",
						unamebuf, "-i", uidbuf, "-p", pidbuf, 0);
			}
		} else {
			if (newpid == -1) {
				if (nuc_debug)
					(void)pfmt ( log_fd, MM_ERROR,
						":20:authen: forking XAUTO failed\n");
				(void) seteuid ( origuid );
				if (nwattached)
					NWDetach (*serverConnID);
				return (FAILURE);
			} else {
				if ((waitid(P_PID, newpid, &sig_info, 
							WEXITED)) == -1) {
					if (nuc_debug)
					     (void)pfmt ( log_fd, MM_ERROR,
							":21:authen: XAUTO wait failed\n");
					(void) seteuid ( origuid );
					if (nwattached)
						NWDetach (*serverConnID);
					return (FAILURE);
				}
			}
		}

		/*
		 * 	To make sure the right serverConnID is passed to
		 * 	NWIsConnection Authenticated, get the server connection
		 * 	ID for user with the specified user ID.
		 */

		if (nuc_debug) 
			(void)pfmt ( log_fd, MM_ERROR,
				":337:authen: Returned from XAUTO -- UID = %d\n",geteuid () );

		seteuid ( saveuid );		/* restore for some unknown reason */

		if ( nuc_debug ) 
			(void)pfmt ( log_fd, MM_ERROR,
				":338:authen: Cleaned up -- now UID = %d\n",geteuid () );

		/*
		 * 	The auto authenticator has been called. But to find out
		 * 	if we are authenticated, nwisauthen should be called again.
		 */
		if ( nwisauthen ( *serverConnID ) == FALSE ) {
			/*
			 * Authentication did not happen. 
			 */
			if (nuc_debug)
				(void)pfmt ( log_fd, MM_ERROR,
					":24:authen: authentication for '%s' failed\n", server );
			(void) seteuid ( origuid );
			if (nwattached)
				NWCloseConn( *serverConnID );
			return (FAILURE);
		}
	}

	/*
	 * 	Go back to being root.
	 */

	if (seteuid ( origuid ) > 0) {
		/*
		 *	failed to switch back to original
		 */
		if (nuc_debug)
			(void)pfmt ( log_fd, MM_ERROR,
				":25:authen: setuid to original failed\n");
		if (nwattached)
			NWCloseConn( *serverConnID );
		return ( FAILURE );
	}

	/*
	 *	hey, it worked!
	 */
	return ( SUCCESS );
}

/******************************************************************************
 *
 *	nwisauthen ( NWCONN_HANDLE ConnID )
 *
 *	Check to see if authenticated to a server
 *
 *	Entry :
 *		ConnID			connection ID
 *
 *	Exit :
 *		TRUE			is authenticated
 *		FALSE			not authenticated
 *
 *****************************************************************************/

int
nwisauthen ( NWCONN_HANDLE ConnID )
{
	uint32				rc;
	uint32				AuthFlags;
	CONNECT_INFO		ConnInfo;

	rc = NWGetConnInformation( ConnID, NWC_CONN_INFO_AUTH_STATE,
			sizeof(uint32), &AuthFlags );
	if( rc ){
		return( rc );
	}

	if( AuthFlags == NWC_AUTH_STATE_NONE ){
		return( FALSE );
	}

	return( TRUE );
}
