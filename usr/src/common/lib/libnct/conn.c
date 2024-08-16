/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnct:conn.c	1.12"
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/fcntl.h>
#include <nw/nwcalls.h>
#include <nw/nwerror.h>
#include <sys/sap_app.h>

#include "nct.h"
#include "nwbinderyprops.h"


#define	LIST_INCREMENT	400

void_t			NWFreeNetWareServerList (char **, uint32);
uint32			NWGetConnIDByName (char *, NWCONN_HANDLE *);
boolean_t		NWIsPrimaryServerSet (int32, char *);

extern	char	*getenv();

/*
 * BEGIN_MANUAL_ENTRY
 *
 * NAME
 *	NWLogin
 *
 * SYNOPSIS
 *	uint32
 *	NWLogin( char* ServiceName, char* UserName, char* Password, int Flags )
 *
 * INPUT
 *	Flags
 *	Password
 *	ServiceName
 *	UserName
 *
 * RETURN VALUES
 *	0				Successfull Completion.
 *
 * DESCRIPTION
 *	main
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
uint32
NWLogin( NWCONN_HANDLE ConnID, char* UserName, char* Password, uint32 Flags )
{
	char	TmpPassword[NWMAX_PASSWORD_LENGTH];
	uint32	rc;

	/*	DOS bindery utilities convert the password to upper
	 *	case.  We'll do the same to provide consistancy from
	 *	the user's perspective.
	 */
	strtoupper( strcpy(TmpPassword, Password) );

	rc = NWLoginToFileServer( ConnID, (char *)UserName, OT_USER,
			(char*)TmpPassword );

	ClearPassword( TmpPassword );

#ifdef NEWSTUFF
	if( !rc ){
		/*	We need to update the last logged in time.
		 */
		rc = NWGetFileServerDateAndTime( connID, date );
		if( rc ){
			return( rc );
		}else{
			rc = NWWritePropertyValue(connID, name, OT_USER, "MISC_LOGIN_INFO",
			  1, date, (BYTE)0);
			if( rc && (rc != NO_SUCH_PROPERTY)
			  && (rc != NO_SUCH_OBJECT_OR_BAD_PASSWORD) ){
				return( rc );
			}
		}
	}
#endif NEWSTUFF

	return( rc );
}

uint32
NWLogout( NWCONN_HANDLE ConnID, uint32 Flags )
{
	uint32	rc;

	rc = NWLogoutFromFileServer( ConnID );

	return( rc );
}

uint32
NWDetach( NWCONN_HANDLE ConnID )
{
	uint32	rc;

	if( isAuthenticated(ConnID) ){
		return( CONNECTION_LOGGED_IN );
	}

	rc = NWCloseConn( ConnID );

	return( rc );
}

uint32
CheckGraceLogins( NWCONN_HANDLE ConnID, char* UserName, int* GraceLogins )
{
	uint32				rc;
	LOGIN_CONTROL_T*	LoginControl;
	NWSEGMENT_DATA		SegData[NWMAX_SEGMENT_DATA_LENGTH];
	NWFLAGS				MoreSegFlag;

	rc = NWReadPropertyValue( ConnID, (char *)UserName, OT_USER,
			(char *)"LOGIN_CONTROL", 1, (uint8*)SegData,
			&MoreSegFlag, NULL );
	if( rc ){
		return( rc );
	}

	LoginControl = (LOGIN_CONTROL_T*)SegData;
	*GraceLogins = LoginControl->Pass_Grace;

	if( strcmpi(UserName, "SUPERVISOR") != 0 ){
		/* Should we set *GraceLogins = UNLIMITED_GRACE_LOGINS ? */
		return( SUCCESS );
	}

	return( SUCCESS );
}

uint32
NWChangePassword (NWCONN_HANDLE	ConnID, char *UserName, char *OldPassword,
		char *NewPassword )
{
	uint32	rc;
	char	TmpNewPassword[NWMAX_PASSWORD_LENGTH];
	char	TmpOldPassword[NWMAX_PASSWORD_LENGTH];

	if( strcmpi(NewPassword, UserName) == 0 ){
		return( NWERR_PASSWORD_IS_USERNAME );
	}

	strtoupper( strcpy(TmpNewPassword, NewPassword) );
	strtoupper( strcpy(TmpOldPassword, OldPassword) );

	rc = NWChangeObjectPassword( ConnID, (char *)UserName, OT_USER,
	  (char *)TmpOldPassword, (char *)TmpNewPassword);

	ClearPassword( TmpNewPassword );
	ClearPassword( TmpOldPassword );
	return( rc );
}

uint32
isAuthenticated( NWCONN_HANDLE ConnID )
{
	uint32			rc;
	uint32			AuthFlags;
	CONNECT_INFO	ConnInfo;

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

int
isServerInNNSDomain( NWCONN_HANDLE ConnID )
{
	char	ServerName[NWMAX_OBJECT_NAME_LENGTH];
	uint8	ServerDomain[NWMAX_SEGMENT_DATA_LENGTH];
	uint32	rc;

	rc = NWGetServerNameByConnID( ConnID, ServerName );
	if( rc ){
		return( FALSE );
	}

	rc = NWReadPropertyValue( ConnID, (char *)ServerName, OT_FILE_SERVER,
			(char *)"DOMAIN_NAME", 1, ServerDomain, NULL, NULL );
	if( rc == 0 ){
		return( TRUE );
	}else{
		return( FALSE );
	}
}

uint32
NWGetUserNameByConnID( NWCONN_HANDLE ConnID, char* UserName )
{
	int			rc;
	NWOBJ_ID	ObjID;
	NWOBJ_ID	ObjID2;

	rc = NWGetConnInformation( ConnID, NWC_CONN_INFO_USER_ID,
			sizeof(NWOBJ_ID), &ObjID );
	
#define NWuint32Swap( w ) ( (((w)&0xff000000) >> 24)+(((w)&0x00ff0000) >> 8)+ \
			  (((w)&0x0000ff00) << 8) + (((w)&0x000000ff) << 24) )

	ObjID2 = NWuint32Swap( ObjID );

	rc = NWGetObjectName( ConnID, ObjID2, (char *)UserName, NULL );

	return( rc );
}

uint32
NWGetConnIDByName( char* ServerName, NWCONN_HANDLE* ConnID )
{
	char		UCServerName[NWMAX_OBJECT_NAME_LENGTH];
	uint32		rc;
	uint32		ConnRef;
	uint32		tmp2=0;
	NWCONN_NUM	tmp;

	strtoupper( strcpy(UCServerName, ServerName) );
	rc = NWScanConnInformation( &tmp2, NWC_CONN_INFO_SERVER_NAME,
			strlen(UCServerName), UCServerName, NWC_MATCH_EQUALS,
			NWC_CONN_INFO_CONN_NUMBER, sizeof(NWCONN_NUM),
			&ConnRef, &tmp );
	
	if( rc == SCAN_COMPLETE ){
		return( NO_RESPONSE_FROM_SERVER ); 
	}

	rc = NWOpenConnByReference( ConnRef,
			NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED, ConnID );

	return( rc );
}

uint32
NWGetConnNumberByConnID( NWCONN_HANDLE ConnID, NWCONN_NUM* ConnNum )
{
	uint32	rc;

	rc = NWGetConnInformation( ConnID, NWC_CONN_INFO_CONN_NUMBER,
			sizeof(NWCONN_NUM), ConnNum );

	return( rc );
}

uint32
NWGetServerNameByConnID( NWCONN_HANDLE ConnID, char* ServerName )
{
	uint32	rc;

	rc = NWGetConnInformation( ConnID, NWC_CONN_INFO_SERVER_NAME,
			NWMAX_OBJECT_NAME_LENGTH, ServerName );

	rc = get_conn_info(ConnID, NWC_CONN_INFO_SERVER_NAME,
			NWMAX_OBJECT_NAME_LENGTH, ServerName );
	return( rc );
}

uint32
NWAttach( char* ServerName, NWCONN_HANDLE* ConnID, uint32 Flags )
{
	uint32			rc;
	NWCConnString	String;

	rc = NWGetConnIDByName( ServerName, ConnID );
	if( rc && rc != NWERR_ALREADY_ATTACHED){
		String.pString = ServerName;
		String.uStringType = NWC_STRING_TYPE_ASCII;
		String.uNameFormatType = NWC_NAME_FORMAT_BIND;

		rc = NWOpenConnByName( NULL, &String, (char *)"NCP_SERVER",
				NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED,
				NWC_TRAN_TYPE_WILD, ConnID );
	}else{
		rc = NWERR_ALREADY_ATTACHED;
	}

	return( rc );
}

int
VerifyChangePasswordRights( NWCONN_HANDLE ConnID, char* UserName, uint32 Flags )
{
	char				MyUserName[NWMAX_OBJECT_NAME_LENGTH];
	uint8 				SegData[NWMAX_SEGMENT_DATA_LENGTH];
	uint8				MoreSegs;
	uint32				rc;
	LOGIN_CONTROL_T*	LoginControl;

	/*	The LOGIN_CONTOL property can only be read
	 *	by the Object, SUPERVISOR (including
	 *	OBJ_SUPERVISORS members), and File Server.
	 *	So if the user can read this property he/she
	 *	must be the object or it's manager.
	 */
	rc = NWReadPropertyValue( ConnID, (char *)UserName, OT_USER,
			(char *)"LOGIN_CONTROL", 1, SegData, &MoreSegs, NULL );
	if( rc ){
		return( FALSE );
	}

	/*	If the user is changing his/her own password
	 *	check to make sure he/she has rights to do this.
	 */
	rc = NWGetUserNameByConnID( ConnID, MyUserName );
	if( rc ){
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

uint32
NWGetPrimaryConnID( NWCONN_HANDLE* ConnID )
{
	uint32	ConnRef;
	uint32	rc;

	rc = NWGetPrimaryConnRef( &ConnRef );
	if( rc == SUCCESS ){
		rc = NWOpenConnByReference( ConnRef,
				NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED, ConnID );
	}
	return( rc );
}

uint32
NWScanConnID( uint32* ScanIndex, NWCONN_HANDLE* ConnID )
{
	uint32		ConnRef;
	uint32		rc;
	NWCONN_NUM	tmp;
	char		buf[100];

	rc = NWScanConnInformation( ScanIndex, NWC_CONN_INFO_RETURN_ALL,
			0, NULL, NWC_MATCH_EQUALS, NWC_CONN_INFO_CONN_NUMBER,
			sizeof(NWCONN_NUM), &ConnRef, &tmp );

	if( rc ){
		return( rc );
	}

	rc = NWOpenConnByReference( ConnRef,
			NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED, ConnID );

	return( rc );
}

/*
 * Before calling this function, the requester must be initialize for the
 * user calling this function.
 */
char **
NWGetNetWareServerList (int32	userID, uint32	*serverCount)
{ 
	int				serverEntry = NULL;
	SAPI			serverBuf;
	char			**serverList;
	uint32			index = 0, listCount = 0;
	uint32			primaryListStart = 0, primaryListStop = 0;
	int				i;
	char			primaryServer[NWC_MAX_SERVER_NAME_LEN];
	char			serverName[NWC_MAX_SERVER_NAME_LEN];
	NWCCODE			ccode;
	NWCONN_HANDLE	connID;
	NWOBJ_ID		objectID;
	char			objectName[NWMAX_OBJECT_NAME_LENGTH];
	int				serverIndex = 0;
	uint32			scanIndex = 0;
	boolean_t		attached = TRUE;

	/*
	 * Loop through and get SAP to generate a list of all the servers -- 
	 * build the various server lists
	 */
	while ( 1 ) {
		/*
		 * Get the next entry.
		 */
		if ((i = SAPGetAllServers (FILE_SERVER_TYPE, &serverEntry,
				&serverBuf, 1)) <= 0 ) {
			/*
		 	 * No more entries.
			 */
			break;
		}

		/*
		 * Add the server to the serverList. Do we need to allocate the
		 * serverList?
		 */
		if (index == 0) {
			/*
			 * Make a new serverList.
		 	 */
			listCount = 1;
			if ((serverList = (char **)malloc (LIST_INCREMENT *
					sizeof (char *))) == NULL) {
				/*
				 * Unable to malloc server list.
				 */
				*serverCount = 0;
				return ((char **) NULL);
			}
		} else {
			if (index == (listCount * LIST_INCREMENT)) {
				/*
				 * Need to make the serverList longer.
				 */
				listCount++;
				if ((serverList = (char **)realloc (
						(char **)serverList,
						(size_t)((listCount *
						LIST_INCREMENT) *
						sizeof (char *)))) == NULL) {
					NWFreeNetWareServerList (serverList,
						index);
					*serverCount = 0;
					return ((char **) NULL);
				}
			}
		}

		/*
		 * Copy serverName into the serverList.
		 */
		serverList[index] = strdup ((char *)serverBuf.serverName); 
		index++;
	}

	/*
	 * Do we need to get the primary server list?
	 */
	if (NWIsPrimaryServerSet (userID, primaryServer)) {
		/*
		 * Attach to the primary server.
		 */
		ccode = NWAttach (primaryServer, &connID, 0);
		if (ccode && ccode != NWERR_ALREADY_ATTACHED) {
			NWFreeNetWareServerList (serverList, index);
			*serverCount = 0;
			return ((char **) NULL);
		}

		if (ccode == NWERR_ALREADY_ATTACHED) {
		/*	attached = FALSE; */
		}

		/*
		 * loop through and get server names from the primary server and
		 * build the various server lists
		 */
		primaryListStart = index;
		objectID = -1;
		while ( 1 ) {
			/*
			 * Get the next entry
			 */
			ccode = NWScanObject (connID, (char *)"*",
					OT_FILE_SERVER, &objectID,
					(char *)objectName, NULL, NULL, NULL,
					NULL);
			if (ccode) {
				if (ccode == NO_SUCH_OBJECT) {
					/*
				 	 * No more entries.
					 */
					if (attached) {
						NWDetach (connID); 
					}
					break;
				}

				/*
				 * Something bad happened.
				 */
				NWFreeNetWareServerList (serverList, index);
				*serverCount = 0;
				if (attached)
					NWDetach (connID);
				return ((char **) NULL);
			}

			/*
			 * Check to see if the server exits in the SAP NetWare
			 * server list.
			 */
			serverIndex = 0;
			if (SAPGetServerByName ((uint8 *)objectName,
					(uint16)FILE_SERVER_TYPE, &serverIndex,
					&serverBuf, 1) != 1) {
				/*
				 * The specified server is not in the SAP
				 * NetWare server list. Add the server to the
				 * serverList. Do we need to allocate the
				 * serverList?
				 */
				if (index == 0) {
					/*
					 * Make a new serverList.
				 	 */
					listCount = 1;
					if ((serverList = (char **)malloc (
							LIST_INCREMENT *
							sizeof (char *))) ==
							NULL) {
						/*
						 * Unable to malloc server list.
						 */
						*serverCount = 0;
						if (attached)
							NWDetach (connID);
						return ((char **) NULL);
					}
				} else {
					if (index == (listCount *
							LIST_INCREMENT)) {
						/*
						 * Need to make the serverList
						 * longer.
						 */
						listCount++;
						if ((serverList =
							(char **)realloc (
							   (char **)serverList,
							   (size_t)((listCount *
							   LIST_INCREMENT) *
							   sizeof (char *)))) ==
							   NULL) {
							NWFreeNetWareServerList
							    (serverList, index);
							*serverCount = 0;
							if (attached)
								NWDetach (connID);
							return ((char **) NULL);
						}
					}
				}

				/*
				 * Copy serverName into the serverList.
				 */
				serverList[index] = strdup (objectName); 
				index++;
			}
		}
		primaryListStop = index;
	}

	/*
	 * Also get the list of servers the user is attached to.
	 */
	while ( 1 ) {
		ccode = NWScanConnID (&scanIndex, &connID);
		if (ccode == SCAN_COMPLETE) {
			NWCloseConn(connID);
			break;
		} else {
			if (ccode) {
				/*
				NWFreeNetWareServerList (serverList, index);
				*serverCount = 0;
				return ((char **) NULL);
				*/
				break;
			}
		}

		ccode = NWGetServerNameByConnID (connID, serverName);
		NWCloseConn(connID);
		if (ccode) {
			NWFreeNetWareServerList (serverList, index);
			*serverCount = 0;
			NWCloseConn(connID);
			return ((char **) NULL);
		}

		/*
		 * Check to see if the server exits in the SAP NetWare server
		 * list.
		 */
		serverIndex = 0;
		if (SAPGetServerByName ((uint8 *)serverName,
				(uint16)FILE_SERVER_TYPE, &serverIndex,
				&serverBuf, 1) !=1) {
			/*
			 * The specified server is not in the SAP NetWare
			 * server list. Also need to make sure it is not part
			 * of the primary server list.
			 */
			for (i = primaryListStart; i < primaryListStop; i++) {
				if ((strcmp (serverName, serverList[i])) != 0) {
					/*
					 * Also not in the primary server list.
					 * Add it to the serverList.
					 */
					/*
					 * Add the server to the serverList. Do
					 * we need to allocate the serverList?
					 */
					if (index == 0) {
						/*
						 * Make a new serverList.
					 	 */
						listCount = 1;
						if ((serverList = (char **)
							malloc (LIST_INCREMENT *
							sizeof (char *))) ==
							NULL) {
							/*
							 * Unable to malloc
							 * server list.
							 */
							*serverCount = 0;
							if (attached)
								NWDetach (
									connID);
							return ((char **) NULL);
						}
					} else {
						if (index == (listCount *
							      LIST_INCREMENT)) {
							/*
							 * Need to make the
							 * serverList longer.
							 */
							listCount++;
							if ((serverList =
							     (char **)realloc (
							    (char **)serverList,
							    (size_t)((listCount
							    * LIST_INCREMENT) *
							    sizeof (char *))))
							    == NULL) {
								NWFreeNetWareServerList (serverList, index);
								*serverCount = 0;
								if (attached)
									NWDetach (connID);
								return ((char **) NULL);
							}
						}
					}

					/*
					 * Copy serverName into the serverList.
					 */
					serverList[index] = strdup (serverName);
					index++;
				}
			}
		}
	}

	/*
	 * Set the last entry to NULL to indicate the end of the list.
	 */
	serverList[index] = (char *)NULL;
	*serverCount = index;
	if (attached)
		NWDetach (connID);
	return (serverList);
}

boolean_t
NWIsPrimaryServerSet (int32 userID, char *primaryServer)
{
	int				fd;
	struct	passwd	*pw;
	char			NWprimary[1024];
	int				i;

	/*
	 * Does .NWprimary file exit in user's home directory.
	 */
	if ((pw = (struct passwd *)getpwuid ((uid_t)userID)) ==
			(struct passwd *)NULL) {
		return (FALSE);
	}
	(void) sprintf (NWprimary, "%s/%s", pw->pw_dir, ".NWprimary");

	if ((fd = open (NWprimary, O_RDONLY)) == -1) {
		/*
		 * Could not open user's primary server file.
		 */
		return (FALSE);
	}

	if ((i = read (fd, primaryServer, NWC_MAX_SERVER_NAME_LEN)) == 0) {
		/*
		 * Counld not read the primary server file.
		 */
		close (fd);
		return (FALSE);
	}

	/*
	 * NULL terminate the primaryServer name.
	 */
	primaryServer[i] = '\0';

	close(fd);
	return (TRUE);
}

void_t
NWFreeNetWareServerList (char **serverList, uint32 numEntries)
{
	int i;

	if (serverList[0] == NULL)
		return;
	
	for (i=0; i < numEntries; i++)
		free (serverList[i]);
}



/*********************************************************************
 This fills an array of NWCONN_HANDLEs with the current conection IDs
 BEWARE DANGER:  This array of connection IDs will need to be cleaned up.
 If you fill this and then want to refill it is necessary to free the first
 set of connids.  IF you your process exits these connids wil automatically
 get freed.
 BY: Patrick Felsted
 DATE: 9/27/94
*********************************************************************/

uint32
NWGetConnectedConnIDs(NWCONN_HANDLE *connListBuffer, short * numConns)
{
	int             connListSize = 255;
	nuint32         scanIndex = 0;
	NWCCODE         ccode;
	int             index = 0;
	NWCONN_HANDLE   connID;

	*numConns = 0;
	while (connListSize > 0) {
		ccode = NWScanConnID( &scanIndex, &connID );
		if( ccode == SCAN_COMPLETE ){
			break;
		}else if( ccode ){
			return( ccode );
		}
		connListBuffer[index++] = connID;
		connListSize--;
	}
	*numConns = index;
	return(SUCCESS);
}

/*******************************************************************
 This will free up an array of connection IDs
 BY: Patrick Felsted
 DATE: 9/27/94
*******************************************************************/

uint32
NWFreeConnectedConnIDs(NWCONN_HANDLE *connListBuffer, short * numConns)
{
        int     index = *numConns;

        while(index >= 0) {
                NWCloseConn(connListBuffer[index--]);
        }
        connListBuffer[0] = 0;
        *numConns = 0;
        return (SUCCESS);
}



/*******************************************************************
 This will give you a REAL list of connected connIDs
*******************************************************************/

uint32
GetServerConnID( uint8 *serverName, NWCONN_HANDLE *connID )
{
	NWCCODE				ccode;
	nuint				repLen;
	int					x, y;
	nuint				numConns;
	char				buffer[NWC_MAX_SERVER_NAME_LEN+1];
	NWCONN_HANDLE N_FAR *connList;
	NWCDeclareAccess(access);


	/*
	 *	Get the number of connections that we will be sending this request to.
	 *	(public + private connections)
	 */
	ccode = (NWCCODE)NWCGetNumConns(&access, NULL, (pnuint)&x,
								(pnuint)&y, NULL);
	numConns = x + y;

	connList = (NWCONN_HANDLE N_FAR *)NWCMalloc(sizeof(NWCONN_HANDLE)*numConns);
	if (!connList)
	{
		ccode = INSUFFICIENT_RESOURCES;
		goto FreeMemory;
	}

	/*
	 *	Get conn handle list for this process
	 */
	numConns = getConnHandleList(connList, numConns);

	ccode = 0;
	for (x=0; x<numConns; x++)
	{
		NWCMemSet(buffer, 0, sizeof(buffer));
		NWCSetConn(access, connList[x]);
		ccode = NWCGetConnInfo(&access, NWC_CONN_INFO_SERVER_NAME,
								sizeof(buffer), (nptr)buffer);
		if(ccode)
			continue;

		if(!strcmp((char *)serverName, buffer)) {
			*connID = connList[x];
			break;
		}
	}
	if(x == numConns)
		ccode = NO_CONNECTION_TO_SERVER;

FreeMemory:
	if (connList )
		NWCFree(connList );

	return(ccode);
}
