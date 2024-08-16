/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#   Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#   Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#     All Rights Reserved

#   THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#   The copyright notice above does not evidence any
#   actual or intended publication of such source code.
*/

#ident	"@(#)autoauthent:conn.c	1.1"
#ident	"$Header: $"


#include <stdio.h>
#include <string.h>
#include <nwapi.h>
#include <nwerrors.h>
#include "nct.h"
#include "nwbinderyprops.h"
/*#include "utl.c"*/

/*
 * BEGIN_MANUAL_ENTRY
 *
 * NAME
 *		NWLogin
 *
 * SYNOPSIS
 *		uint16
 *		NWLogin( char* ServiceName, char* UserName, char* Password, int Flags )
 *
 * INPUT
 *		Flags
 *		Password
 *		ServiceName
 *		UserName
 *
 * RETURN VALUES
 *		0				Successfull Completion.
 *
 * DESCRIPTION
 *		main
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

uint16
NWLogin( uint16 ConnID, char* UserName, char* Password, uint32 Flags )
{
char			TmpPassword[NWMAX_PASSWORD_LENGTH];
char			date[NWMAX_SEGMENT_DATA_LENGTH];

	/*	DOS bindery utilities convert the password to upper
	 *	case.  We'll do the same to provide consistancy from
	 *	the user's perspective.
	 */
	strtoupper( strcpy(TmpPassword, Password) );

	if(NWLoginToServerPlatform( ConnID, UserName, NWOT_USER, TmpPassword ) == NWERROR) {
		return(NWErrno);
	}	

	ClearPassword( TmpPassword );

	/*	We need to update the last logged in time.
	 */

/*
	if(NWGetServerPlatformDateAndTime( ConnID, (NWServerPlatformDateAndTime_t *) date ) == NWERROR) {
		return(NWErrno);
	}else{

		if(NWWritePropertyValue(ConnID, UserName, NWOT_USER, "MISC_LOGIN_INFO", 1, date, NULL) == NWERROR){
			return(NWErrno);
		}
	}
*/
	return(NWSUCCESS);
}


uint16
NWLogout( uint16 ConnID, uint32 Flags )
{

	if( isAuthenticated(ConnID) ){

		if(NWLogoutFromServerPlatform( ConnID ) == NWERROR) {
			return(NWErrno);
		}
	}else{
			return( ERR_CONN_NOT_LOGGED_IN );
	}

	return(NWSUCCESS);
}

uint16
NWDetach( uint16 ConnID )
{

	if( isAuthenticated(ConnID) ){
		return( CONNECTION_LOGGED_IN );
	}

	if(NWDetachFromServerPlatform( ConnID ) == NWERROR) {
		return(NWErrno);
	}

	return(NWSUCCESS);
}

uint16
CheckGraceLogins( uint16 ConnID, char* UserName, int* GraceLogins )
{
LOGIN_CONTROL_T*		LoginControl;
uint8			SegData[NWMAX_SEGMENT_DATA_LENGTH];
uint8					MoreSegFlag;
int32					seq;
NWObjectInfo_t			obj;
uint8					segnum = 1;
/*
	NWScanObject(ConnID, UserName, NWOT_USER, "LOGIN_CONTROL", &seq, &obj);
	printf("Object: %s, %d, \n%s, \n%d, %d, %s, %s\n", UserName, seq, obj.objectName, obj.objectID, obj.objectType, obj.objectState, obj.objectSecurity);
	printf("NWErrno: %d, %x\n", NWErrno, NWErrno);
*/
	if(NWScanPropertyValue( ConnID, UserName, NWOT_USER, "LOGIN_CONTROL", &segnum, SegData, &MoreSegFlag, NULL ) == FALSE) {
		return(NWErrno);
	}

	LoginControl = (LOGIN_CONTROL_T*)SegData;
	*GraceLogins = LoginControl->Pass_Grace;

	if( strcmpi(UserName, "SUPERVISOR") != 0 ){
		return(NWSUCCESS);
/* Should we set *GraceLogins = UNLIMITED_GRACE_LOGINS ? */
	}

	return(NWSUCCESS);
}

uint16
NWChangePassword(uint16 ConnID, char* UserName, char* OldPassword, char* NewPassword )
{
char			TmpNewPassword[NWMAX_PASSWORD_LENGTH];
char			TmpOldPassword[NWMAX_PASSWORD_LENGTH];

	if( strcmpi(NewPassword, UserName) == 0 ){
		return( NWERR_PASSWORD_IS_USERNAME );
	}

	strtoupper( strcpy(TmpNewPassword, NewPassword) );
	strtoupper( strcpy(TmpOldPassword, OldPassword) );

	if(NWChangeObjectPassword( ConnID, UserName, NWOT_USER, TmpOldPassword, TmpNewPassword) == NWERROR){
		return(NWErrno);
	}

	ClearPassword( TmpNewPassword );
	ClearPassword( TmpOldPassword );
	return(NWSUCCESS);
}

uint16
isAuthenticated( uint16 ConnID )
{
uint32	objectID;
uint8	binderyAccessLevel;


    if(NWGetBinderyAccessLevel( ConnID, &binderyAccessLevel, &objectID) == NWERROR) {
        return(FALSE);
    }
    if ( (int)(binderyAccessLevel & 0x0f) >= (int)NWBS_LOGGED_READ ) {
		return(TRUE);
	}else {
		return(FALSE);
	}
}

int
isServerInNNSDomain( uint16 ConnID )
{
char			ServerName[NWMAX_SERVER_NAME_LENGTH];
uint8			ServerDomain[NWMAX_SEGMENT_DATA_LENGTH];
uint32			rc;

	rc = GetServerName( ConnID, ServerName );
	if( rc ){
		return( FALSE );
	}

	if(NWScanPropertyValue( ConnID, ServerName, NWOT_FILE_SERVER, "DOMAIN_NAME", 1, ServerDomain, NULL, NULL ) == 0) {
		return (FALSE);
	} 

	return(TRUE);
}

uint16
GetUserName( uint16 ConnID, char* UserName )
{
	if(NWGetConnectionInformation( ConnID, GetConnNum(ConnID), UserName, NULL, NULL, NULL ) == NWERROR) {
		return(NWErrno);
	}

	return(NWSUCCESS);
}

uint16
GetConnID( char* ServerName )
{
uint16	ConnID;

	if(NWGetServerConnID( ServerName, &ConnID ) == NWERROR) {
		return(FALSE);
	}
	return(ConnID);
}

uint16
GetConnNum( uint16 ConnID )
{
uint16			ConnNum;


	if(NWGetClientConnID(ConnID, &ConnNum) == NWERROR) {
		return(NWErrno);
	}

	return(ConnNum);
}

uint16
GetServerName( uint16 ConnID, char* ServerName )
{

	if(NWGetServerPlatformName( ConnID, ServerName) == NWERROR) {
		return(NWErrno);
	}

	return( NWSUCCESS );
}

uint16
NWAttach( char* ServerName, uint16* ConnID, uint32 Flags )
{
uint16 rc;

	if(NWAttachToServerPlatform( ServerName, ConnID ) == NWERROR) {
		rc = NWErrno;
		if(rc == NWERR_NO_RESPONSE_FROM_SERVER) {
			return( NWErrno |= ERR_ATTACH );
		}
		else {
			return(NWErrno);
		}
	}

	return( NWSUCCESS );
}

int
VerifyChangePasswordRights( uint16 ConnID, char* UserName, uint32 Flags)
{
char                MyUserName[NWMAX_OBJECT_NAME_LENGTH];
uint8               SegData[NWMAX_SEGMENT_DATA_LENGTH];
uint8               MoreSegs;
uint32              rc;
LOGIN_CONTROL_T*    LoginControl;

    /*  The LOGIN_CONTOL property can only be read
     *  by the Object, SUPERVISOR (including
     *  OBJ_SUPERVISORS members), and File Server.
     *  So if the user can read this property he/she
     *  must be the object or it's manager.
     */
    if(NWScanPropertyValue( ConnID, UserName, NWOT_USER, "LOGIN_CONTROL", 1, SegData, &MoreSegs, NULL ) == 0) {
        return( FALSE );
    }

    /*  If the user is changing his/her own password
     *  check to make sure he/she has rights to do this.
     */
    if(GetUserName( ConnID, MyUserName )) {
        return( FALSE );
    }
    if( strcmpi(UserName, MyUserName) == SUCCESS ){
        LoginControl = (LOGIN_CONTROL_T*)SegData;
        if( LoginControl->RestrictionFlags & ONLY_SUPERVISOR_CHANGE ){
           return( FALSE );
        }
    }
    return( TRUE );
}

