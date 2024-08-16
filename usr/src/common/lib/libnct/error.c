/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnct:error.c	1.3"
/*
**  Netware Unix Client
**
**  MODULE:
**      error.c
**
**  ABSTRACT:
**		Handles errors for NWAttach, NWLogin, NWChangePassword.
*/

#include <stdio.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>

char *AttachError( uint32 rc );
char *LoginError( uint32 rc );
char *ChangePasswordErrors( uint32 rc );

char strbuf[200];

char *
AttachError( uint32 rc)
{
	char tmpbuf[50];

	switch( rc ){
	case NO_RESPONSE_FROM_SERVER:
		return((char *)gettxt( "uvlnuc:299",
			"The specified server is unknown.\n" ));
	case NO_MORE_SERVER_SLOTS:
	case CONNECTION_TABLE_FULL:
		return((char *)gettxt( "uvlnuc:297",
			"The maximum number of server attachments has been reached.\n" ));
	default:
		sprintf(strbuf, (char *)gettxt("uvlnuc:150",
			"An unexpected error occurred. "));
		sprintf(tmpbuf,  "(%i:%#8.8x)\n", 1, rc );
		strcat(strbuf, tmpbuf);
		return( strbuf );
	}
}

/*
 * DESCRIPTION
 *   Prints the error message associated with the number
 *   passed into the function.
 *
 * END_MANUAL_ENTRY
 */

char *
LoginError( uint32 rc)
{
	switch( rc ){

	case ACCOUNT_DISABLED:
		return((char *)gettxt( "uvlnuc:288",
			"This account has expired or been diabled by the supervisor.\n" ));

	case INTRUDER_DETECTION_LOCK:
		return((char *)gettxt( "uvlnuc:290",
			"Intruder detection lockout has disabled this account.\n" ));

	case LOGIN_DENIED_NO_ACCOUNT_BALANCE:
		return((char *)gettxt( "uvlnuc:291",
			"You do not have an account balance.\n" ));

	case LOGIN_DENIED_NO_CONNECTION:
		sprintf(strbuf, (char *)gettxt( "uvlnuc:292",
			"You are trying to log in to too many stations simultaneously.\n"));
		strcat(strbuf, (char *)gettxt( "uvlnuc:293",
			"The supervisor has limited the number of user connections you may have.\n" ));
		return(strbuf);

	case LOGIN_DENIED_NO_CREDIT:
		return((char *)gettxt( "uvlnuc:294",
			"Your credit limit has been exceeded.\n" ));

	case NO_SERVER_SLOTS:
	case E_NO_MORE_USERS:
	case NO_FREE_CONNECTION_SLOTS:
		sprintf(strbuf, (char *)gettxt( "uvlnuc:295",
			"The maximum number of connections allowed on this server has been reached.\n" ));
		strcat(strbuf, (char *)gettxt( "uvlnuc:296",
			"Wait until another user logs out.\n" ));
		return(strbuf);

	case INVALID_CONNECTION:
		return((char *)gettxt( "uvlnuc:298",
			"Access has been denied and you have been logged out.\n" ));

	case NO_SUCH_OBJECT:
	case NO_SUCH_OBJECT_OR_BAD_PASSWORD:
	case SERVER_CONNECTION_LOST:
		return((char *)gettxt( "uvlnuc:300", "Access has been denied.\n" ));

	case PASSWORD_HAS_EXPIRED_NO_GRACE:
		return((char *)gettxt( "uvlnuc:301",
			"Your password has expired and all grace logins have been used.\n"));

	case SUPERVISOR_HAS_DISABLED_LOGIN: /* SERVER_BINDERY_LOCKED */
		return((char *)gettxt( "uvlnuc:302",
			"The supervisor has disabled the login function for this server.\n"));

	case UNAUTHORIZED_LOGIN_STATION:
		sprintf(strbuf, (char *)gettxt( "uvlnuc:303",
			"You are trying to log in from an unauthorized station.\n" ));
		strcat(strbuf, (char *)gettxt( "uvlnuc:304",
			"The supervisor has restricted the stations you may log in from.\n" ));
		return(strbuf);

	case UNAUTHORIZED_LOGIN_TIME:
		sprintf(strbuf, (char *)gettxt( "uvlnuc:305",
			"You are trying to log in during an unauthorized time period.\n" ));
		strcat(strbuf, (char *)gettxt( "uvlnuc:306",
			"The supervisor has limited the times that you may log in.\n" ));
		return(strbuf);

	case PREFERRED_NOT_FOUND:
		return((char *)gettxt( "uvlnuc:307",
			"This utility could not find the preferred server.\n" ));

	default:
		sprintf(strbuf, "An unexpected error occurred. (%i:%#8.8x)\n", 1, rc );
		return((char *)gettxt("uvlnuc:150", strbuf));
	}
}


char *
ChangePasswordErrors( uint32 rc)
{
	switch( rc ){
	case SERVER_BINDERY_LOCKED:
		return((char *)gettxt( "uvlnuc:309",
			"Either the supervisor has locked the bindery\nor volume SYS: is not mounted.\n" ));

	case PASSWORD_TOO_SHORT:
		return((char *)gettxt( "uvlnuc:310",
			"The new password is too short.\n" ));

	case PASSWORD_NOT_UNIQUE:
		return((char *)gettxt( "uvlnuc:311",
			"The new password has been used previously.\n" ));

	case INTRUDER_DETECTION_LOCK:
		return((char *)gettxt( "uvlnuc:312",
			"Intruder detection lockout has disabled this account.\n" ));

	case NO_SUCH_OBJECT_OR_BAD_PASSWORD:
		return( (char *)gettxt( "uvlnuc:313", "Access has been denied.\n" ));

	case NWERR_PASSWORD_IS_USERNAME:
		sprintf(strbuf, (char *)gettxt( "uvlnuc:314",
			"The use of a user name as a password is a potential security problem.\n" ));
		strcat(strbuf, (char *)gettxt( "uvlnuc:315",
			"Please select another password.\n" ));
		return(strbuf);

	default:
		sprintf(strbuf, "An unexpected error occurred. (%i:%#8.8x)\n", 1, rc );
		return((char *)gettxt("uvlnuc:150", strbuf));
	}
}
