/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlist:bindery.c	1.7"
/*
**  NetWare Unix Client nlist Utility
**
**	MODULE:
**		nlist.c	-	The NetWare UNIX Client nlist Utility
**
**	ABSTRACT:
**		The nlist.c contains the UnixWare utility to allow a user
**		to display a list of objects ( server, users, groups, volumes, etc.)
**		and their properties.
**
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <nwbinderyprops.h>
#include "nlist.h"
#include "list.h"

#define LC_SUPERVISOR_CHANGE_PASSWORD	0x01
#define LC_FORCE_UNIQUE_PASSWORD		0x02
#define NWMAX_VOLUME_NAME_LENGTH		32

#define SUPERVISOR_OBJ_ID 		0x010000000
#define DEFAULT_STRING_LENGTH	80
#define OBJECT_LIST_LENGTH		32/*NWMAX_SEGMENT_DATA_LENGTH/sizeof(NWOBJ_ID)*/
#define NWNET_ADDR_LENGTH		12 /* Actually 10, 12 is for NET_ADDRESS_T */
#define NW_SECTOR_SIZE			512
#define LINE					":210:----------------------------------------"\
								"---------------------------------------\n"

#ifdef NWWordSwap
#undef NWWordSwap
#endif NWWordSwap
#define NWWordSwap( w ) ( (((w)&0x0000ff00) >> 8) + (((w)&0x000000ff) << 8) )

void BinderyDisplayHeader( char* ServerName, char* ObjTypeString,
		NWOBJ_TYPE ObjType, uint32 Flags );
uint32 BinderyDisplayObjects( NWCONN_HANDLE ConnID, char* SearchName,
		NWOBJ_TYPE SearchObjType, int* NumObjs, uint32 Flags );
uint32 PrintPropertyObjList( NWCONN_HANDLE ConnID, char* PropName,
		NWOBJ_TYPE ObjType, char* ObjName, char* PropTitle );
uint32 GetPropertyObjList( NWCONN_HANDLE ConnID, char* PropName,
		NWOBJ_TYPE ObjType, char* ObjName, SL_LIST_T* List );
uint32 BinderyDisplayUser( NWCONN_HANDLE ConnID, char* UserName, uint32 Flags );
uint32 BinderyDisplayServer( NWCONN_HANDLE ConnID, char* ServerName,
		uint32 Flags );
uint32 BinderyDisplayGroup( NWCONN_HANDLE ConnID, char* GroupName,
		uint32 Flags );
uint32 BinderyDisplayVolume( NWCONN_HANDLE ConnID, NWVOL_NUM VolNum,
		uint32 Flags );
uint32 BinderyDisplayActiveUser( NWCONN_HANDLE ConnID, NWCONN_NUM ConnNum,
		uint32 Flags );
uint32 BinderyDisplayQueue( NWCONN_HANDLE ConnID, char* QueueName,
		uint32 Flags );
NWOBJ_TYPE ParseObjType( char* ObjTypeString );

uint32
BinderyListObjects( char* ServerName,
					char* SearchName,
					char* ObjTypeString,
					int Flags )
{
	char			ObjName[NWMAX_OBJECT_NAME_LENGTH];
	int				c;
	int				i;
	int				n_Objs=0;
	uint32			rc;
	CONNECT_INFO	ConnInfo;
	NWCONN_HANDLE	ConnID;
	NWOBJ_TYPE		ObjType;

	ObjType = ParseObjType( ObjTypeString );
	if( ObjType == 0 ){
		return( NOT_VALID_OBJECT_TYPE );
	}

	if( Flags & USE_DEFAULT_FS ){
		rc = NWCallsInit( NULL, NULL );
		if( rc ){
			Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n", 72, rc );
			return( rc );
		}
		rc = NWGetPrimaryConnID( &ConnID );
		if( rc ){
			Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n", 73, rc );
			return( rc );
		}
		rc = NWGetServerNameByConnID( ConnID, ServerName );
	}else{
		rc = NWGetConnIDByName( ServerName, &ConnID );
		if( rc ){
			Error(":361:You are not logged-in to %s.\n", ServerName );
			return( rc );
		}
	}

	rc = CheckAuthentication( ConnID, ObjType );
	if( !rc ){
		Error(":361:You are not logged-in to %s.\n", ServerName );
		Error(":362:You do not have rights to search for %s objects.\n",
		  ObjTypeString );
		return( rc );
	}

	if( !(Flags & NAMES_ONLY) ){
		BinderyDisplayHeader( ServerName, ObjTypeString, ObjType, Flags );
	}

	rc = BinderyDisplayObjects( ConnID, SearchName, ObjType, &n_Objs, Flags );
	if( (rc == SUCCESS) && !(Flags & NAMES_ONLY) ){
		nprintf( ":130:\n" );
		if( n_Objs == 1 ){
			nprintf( ":151:One %s object was found on Server %s.\n",
			  ObjTypeString, ServerName );
		}else{
			nprintf( ":152:%i %s objects were found on Server %s.\n", n_Objs,
			  ObjTypeString, ServerName );
		}
	}

	return( rc );
}

uint32
BinderyDisplayObjects(	NWCONN_HANDLE	ConnID,
						char*			SearchName,
						NWOBJ_TYPE		SearchObjType,
						int*			n_Objs,
						uint32			Flags )
{
	char			ObjName[NWMAX_OBJECT_NAME_LENGTH];
	char*			TmpName;
	uint32			rc;
	NWCONN_NUM		ConnNum;
	NWNUMBER		MaxConns;
	NWNUMBER		MaxVols;
	NWOBJ_ID		ObjID = -1;
	NWVOL_NUM		VolNum;
	SL_LIST_T*		List;
	void*			Node;

	*n_Objs=0;

	rc = NWGetFileServerInformation( ConnID, NULL, NULL, NULL, NULL,
	  &MaxConns, NULL, NULL, &MaxVols, NULL, NULL );
	if( rc ){
		Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n", 9, rc );
		return( rc );
	}

	switch( SearchObjType ){
		case OT_VOLUME:
			for( VolNum=0; VolNum < MaxVols; VolNum++ ){
				rc = BinderyDisplayVolume( ConnID, VolNum, Flags );
				if( rc == SUCCESS ){
					(*n_Objs)++;
				}else if( rc != VOLUME_DOES_NOT_EXIST ){
					Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n",
					  7, rc );
					return( rc );
				}
			}
			break;
		case OT_USER:
			if( Flags & ACTIVE ){
				for( ConnNum=1; ConnNum <= MaxConns; ConnNum++ ){
					rc = BinderyDisplayActiveUser( ConnID, ConnNum, Flags );
					if( rc == SUCCESS ){
						(*n_Objs)++;
					}
				}
				break;
			}
			/* Fall through if the ACTIVE flag is not set */
		default:
			rc = InitSLList( &List, free, strcmp, NULL, 0 );
			while( TRUE ){
				rc = NWScanObject( ConnID, (uint8*)SearchName, SearchObjType,
				  &ObjID, (uint8*)ObjName, NULL, NULL, NULL, NULL );
				if( rc ){
					if( rc == NO_SUCH_OBJECT ){
						break;
					}
					Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n",
						3, rc );
					FreeSLList( &List, FREE_DATA );
					return( rc );
				}else{
/*
					rc = AppendToSLList( List, ObjName, strlen(ObjName)+1 );
*/
					rc = AppendToSLList( List, strdup(ObjName), 0 );
					if( rc ){
						Error(":150:An unexpected error occurred. (%i:%#8.8x)\n",
						  3, rc );
						FreeSLList( &List, FREE_DATA );
						return( rc );
					}
				}
			}
			if( isSLListEmpty( List ) == FALSE ){
				Node = GetSLListHead( List );
				while( TRUE ){
					(*n_Objs)++;
					if( Flags & NAMES_ONLY ){
						nprintf( ":153:%s\n", GetSLListData(Node) );
					}else{
						TmpName = GetSLListData( Node );
						switch( SearchObjType ){
							case OT_FILE_SERVER:
								rc = BinderyDisplayServer( ConnID, TmpName,
								  Flags ); 
								break;
							case OT_PRINT_QUEUE:
								rc = BinderyDisplayQueue( ConnID, TmpName,
								  Flags );
								break;
/*
							case OT_UNIXWARE:
								break;
							case OT_UNIXWARE_INSTALL_SERVER:
								break;
*/
							case OT_USER:
								rc = BinderyDisplayUser( ConnID, TmpName,
								  Flags );
								break;
							case OT_USER_GROUP:
								rc = BinderyDisplayGroup( ConnID, TmpName,
								  Flags);
								break;
							default:
								break;
						}
					}
					if( isSLListLastNode( Node ) ){
						break;
					}
					Node = GetSLListNextNode( Node );
				}
			}
			FreeSLList( &List, FREE_DATA );
			break;
	}

	return( SUCCESS );
}

void
BinderyDisplayHeader(	char*		ServerName,
						char*		ObjTypeString,
						NWOBJ_TYPE	ObjType,
						uint32		Flags )
{

	nprintf( ":154:Object Class: %s\n", ObjTypeString );
	nprintf( ":155:Known to Server: %s\n", ServerName );

	switch( ObjType ){
		case OT_FILE_SERVER:
			nprintf( ":156:Active NetWare Server = The NetWare Server that is "
			  "currently running\n" );
			nprintf( ":157:Address               = The network address\n" );
			nprintf( ":158:Node                  = The network node\n" );
			nprintf( ":159:Status                = The status of your "
			  "connection\n" );
			nprintf( ":130:\n" );
			nprintf( ":160:Active NetWare Server                        Address"
			  "   Node           Status\n" );
			break;
		case OT_PRINT_QUEUE:
			nprintf( ":161:Queue name = The name of the queue\n" );
			nprintf( ":162:Operator   = The name of the operator\n" );
			nprintf( ":130:\n" );
			nprintf(":163:Queue Name                              Operators\n");
			break;
/*
		case OT_UNIXWARE:
			break;
		case OT_UNIXWARE_INSTALL_SERVER:
			break;
*/
		case OT_USER:
			if( Flags & ACTIVE ){
				nprintf( ":164:Conn       = The server connection number\n" );
				nprintf( ":165:*          = The asterisk means this is your "
                                  "connection\n" );

				nprintf( ":166:User Name  = The login name of the user\n" );
				nprintf( ":167:Address    = The network address\n" );
				nprintf( ":168:Node       = The network node\n" );
				nprintf(":169:Login time = The time when the user logged in\n");
				nprintf( ":130:\n" );
				nprintf( ":170:Conn *User Name                    Address"
                                  "   Node           Login Time\n" );
			}else{
				nprintf( ":171:Login name = The user login name\n" );
				nprintf( ":172:Full name  = The full name of the user\n" );
				nprintf( ":173:Dis        = Yes if the account is disabled\n" );
				nprintf(":174:Expires    = The date the account will expire\n");
				nprintf( ":175:Pwd        = Yes if passwords are required\n" );
				nprintf( ":176:Expires    = The date the password expires\n" );
				nprintf( ":177:Uni        = Yes if unique passwords are "
                                  "required\n" );
				nprintf( ":178:Min        = The minimum password length\n" );
				nprintf( ":179:Conn       = The maximum concurrent connections"
                                  ", 0 if no limit\n" );
				nprintf( ":130:\n" );
				nprintf( ":180:Login Name     Full Name                 Dis  "
                                  "Expires Pwd  Expires Uni Min Conn \n" );

			}
			break;
		case OT_USER_GROUP:
			nprintf( ":181:Group name  = The name of the group\n" );
			nprintf( ":182:Description = The description of the group\n" );
			nprintf( ":130:\n" );
			nprintf(":183:Group Name                          ""Description\n");
			break;
		case OT_VOLUME:
			nprintf( ":184:Mounted Volumes\n" );
			break;
		default:
			break;
	}

	nprintf( LINE );
}

uint32
BinderyDisplayUser( NWCONN_HANDLE ConnID, char* LoginName, uint32 Flags )
{
	char					FullName[NWMAX_SEGMENT_DATA_LENGTH];
	char					AcctExpDate[DEFAULT_STRING_LENGTH];
	char					PassExpDate[DEFAULT_STRING_LENGTH];
	char					LastLoginTime[DEFAULT_STRING_LENGTH];
	int						AllowPasswordChange;
	int						ForcePasswordChange;
	int						GraceLogins;
	int						GraceLoginsRemaining;
	int						LoginControlRead;
	int						PasswordRequired;
	int						UniquePasswordRequired;
	int						i;
	uint32					rc;
	LOGIN_CONTROL_T*		LC;
	NWFLAGS					MoreSegs;
	NWOBJ_ID				ObjID;
	NWSEGMENT_DATA			SegData[NWMAX_SEGMENT_DATA_LENGTH];

	rc = NWGetObjectID( ConnID, (uint8*)LoginName, OT_USER, &ObjID );
	if( rc ){
		return( rc );
	}

	rc = NWReadPropertyValue( ConnID, (uint8*)LoginName, OT_USER,
	  (uint8*)"LOGIN_CONTROL", 1, (uint8*)SegData, &MoreSegs, NULL );
	if( rc ){
		LoginControlRead = FALSE;
	}else{
		LC = (LOGIN_CONTROL_T*)SegData;

		ForcePasswordChange = !( (LC->Pass_Date[1]==0) &&
		  (LC->Pass_Date[2]==0) && (LC->Pass_Date[0]==0) );
		PasswordRequired = (LC->Min_Pass_Length > 0) ? TRUE : FALSE;
		AllowPasswordChange = !( (ObjID != SUPERVISOR_OBJ_ID) &&
		  (LC->RestrictionFlags & LC_SUPERVISOR_CHANGE_PASSWORD) );
			
		if( ForcePasswordChange ){
			sprintf( PassExpDate, "%2.2i-%2.2i-%2.2i",
				  LC->Exp_Date[1], LC->Exp_Date[2], LC->Exp_Date[0] );
			GraceLogins = LC->Grace_Reset;
			GraceLoginsRemaining = LC->Pass_Grace;
		}

		if( ForcePasswordChange || PasswordRequired ){
			UniquePasswordRequired =
			  ( (LC->RestrictionFlags&LC_FORCE_UNIQUE_PASSWORD) ? TRUE : FALSE);
		}
		sprintf( AcctExpDate, "%2.2i-%2.2i-%2.2i",
		  LC->Exp_Date[1], LC->Exp_Date[2], LC->Exp_Date[0] );
		sprintf( LastLoginTime, "%2.2i-%2.2i-%2.2i %2.2i:%2.2i:%2.2i%s",
		  LC->Last_Log_Time[1],
		  LC->Last_Log_Time[2],
		  LC->Last_Log_Time[0],
		  ( (LC->Last_Log_Time[3] > 12) ?
		    (LC->Last_Log_Time[3] - 12) : LC->Last_Log_Time[3] ),
		  LC->Last_Log_Time[4],
		  LC->Last_Log_Time[5],
		  ( (LC->Last_Log_Time[3]<12) ? "am" : "pm" ) );
	}

	/* Get full name */
	rc = NWReadPropertyValue( ConnID, (uint8*)LoginName, OT_USER,
	  (uint8*)"IDENTIFICATION", 1, (uint8*)FullName, &MoreSegs, NULL );
	if( rc ){
		FullName[0] = '\0';
	}

	if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":185:%-15.15s%-26.26s", LoginName, FullName );
		if( LoginControlRead ){
			nprintf( ":186:%-5.5s%-8.8s%-5.5s%-8.8s%-4.4s%-4.4i%-4.4i",
			  (LC->Acct_Expired ? gettxt(":190", "Yes") : gettxt(":191", "No")),
			  AcctExpDate,
			  ((LC->Min_Pass_Length > 0) ?
				gettxt(":190", "Yes") : gettxt(":191", "No")),
			  PassExpDate,
			  (UniquePasswordRequired ?
				gettxt(":190", "Yes") : gettxt(":191", "No")),
			  LC->Min_Pass_Length,
			  NWWordSwap(LC->Max_Connections) );
		}
		nprintf( ":130:\n" );
	}else{
		nprintf( ":187:User: %-74.74s\n", LoginName );
		nprintf( ":188:\tFull Name: %s\n", FullName );
		nprintf( ":189:\tObject ID: %8.8x\n", ObjID );
		if( LoginControlRead ){
			nprintf( ":192:\tAccount Disabled: %s\n",
			  (LC->Acct_Expired ? gettxt(":190","Yes") : gettxt(":191","No")) );
			nprintf( ":193:\tAccount Expiration Date: %s\n", AcctExpDate );
			nprintf( ":194:\tPassword Allow Change: %s\n",
			  (AllowPasswordChange ?
				gettxt(":190", "Yes") : gettxt(":191", "No")) );
			nprintf( ":195:\tPassword Required: %s\n",
			  (PasswordRequired ?
				gettxt(":190", "Yes") : gettxt(":191", "No")) );
			nprintf( ":196:\tPassword Force Change: %s\n",
			  (ForcePasswordChange ?
				gettxt(":190", "Yes") : gettxt(":191", "No")) );
			nprintf( ":197:\tPassword Expiration Days: %i\n",
			  NWWordSwap(LC->Exp_Interval) );
			nprintf( ":198:\tPassword Expiration Date: %s\n", PassExpDate );
			nprintf( ":199:\tPassword Length: %i\n", LC->Min_Pass_Length );
			nprintf( ":200:\tUnique Passwords: %s\n",
			  (UniquePasswordRequired ?
				gettxt(":190", "Yes") : gettxt(":191", "No")) );
			if( GraceLoginsRemaining == UNLIMITED_GRACE_LOGINS ){
			  nprintf( ":201:\tGrace Logins: Unlimited\n" );
			  nprintf( ":202:\tGrace Logins Remaining: Unlimited\n" );
			}else{
			  nprintf( ":203:\tGrace Logins: %i\n", GraceLogins );
			  nprintf( ":204:\tGrace Logins Remaining: %i\n",
				GraceLoginsRemaining);
			}
			nprintf( ":205:\tMaximum Connections: %i\n",
			  NWWordSwap(LC->Max_Connections) );
			nprintf( ":206:\tLast Login: %s\n", LastLoginTime );
		}
		nprintf( ":207:\tGroups:\n" );
		PrintPropertyObjList( ConnID, "GROUPS_I'M_IN", OT_USER,
		  LoginName, NULL );

		PrintPropertyObjList( ConnID, "OBJ_SUPERVISORS", OT_USER,
		  LoginName, gettxt(":208", "Managers:") );

		PrintPropertyObjList( ConnID, "SECURITY_EQUALS", OT_USER,
		  LoginName, gettxt(":209", "Security Equals:") );

		nprintf( LINE );
		rc = SUCCESS;
	}

	return( rc );
}

uint32
BinderyDisplayServer( NWCONN_HANDLE ConnID, char* ServerName, uint32 Flags )
{
	char					ConnStatus[DEFAULT_STRING_LENGTH]="";
	char					Network[DEFAULT_STRING_LENGTH]="";
	char					Node[DEFAULT_STRING_LENGTH]="";
	char					Revision[DEFAULT_STRING_LENGTH]="";
	int						AcctInstalled=FALSE;
	int						AttachedFlag=FALSE;
	uint32					rc;
	CONNECT_INFO			ConnInfo;
	NET_ADDRESS_T*			NetAddr;
	NWCONN_HANDLE			ConnID_2;
	NWCONN_HANDLE			PrimConnID;
	NWFLAGS					MoreSegs;
	NWNUMBER				n_ConnsInUse;
	NWNUMBER				MaxConns;
	NWNUMBER				MaxVols;
	NWOBJ_ID				ObjID;
	NWSEGMENT_DATA			SegData[NWMAX_SEGMENT_DATA_LENGTH];

	rc = NWReadPropertyValue( ConnID, (uint8*)ServerName, OT_FILE_SERVER,
	  (uint8*)"NET_ADDRESS", 1, (uint8*)SegData, &MoreSegs, NULL );
	if( !rc ){
		NetAddr = (NET_ADDRESS_T*)SegData;
		sprintf( Network, "%2.2X%2.2X%2.2X%2.2X", NetAddr->network[0],
		  NetAddr->network[1], NetAddr->network[2], NetAddr->network[3] );
		sprintf( Node, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
		  NetAddr->node[0], NetAddr->node[1], NetAddr->node[2],
		  NetAddr->node[3], NetAddr->node[4], NetAddr->node[5] );
	}

	rc = NWGetConnIDByName( ServerName, &ConnID_2 );
	if( rc == SUCCESS ){
		AttachedFlag = TRUE;
		if( isAuthenticated(ConnID_2) ) {
			strcpy( ConnStatus, gettxt(":211", "Logged-in") );
		}else{
			strcpy( ConnStatus, gettxt(":212", "Attached") );
		}

		rc = NWGetPrimaryConnID( &PrimConnID );
		if( rc ){
			return( rc );
		}
		if( ConnID_2 == PrimConnID ){
			strcat( ConnStatus, gettxt(":213", "*") );
		}

		rc = NWGetFileServerInformation( ConnID_2, NULL, NULL, NULL, NULL,
		  &MaxConns, NULL, &n_ConnsInUse, &MaxVols, NULL, NULL );

		rc = NWGetFileServerDescription( ConnID_2, NULL, (uint8*)Revision, NULL,
		  NULL );

		rc = NWScanProperty( ConnID_2, (uint8*)ServerName, OT_FILE_SERVER,
		  (uint8*)"ACCOUNT_SERVERS", NULL, NULL, NULL, NULL, NULL, NULL );
		if( !rc ){
			AcctInstalled = TRUE;
		}
	}else if( rc != NO_RESPONSE_FROM_SERVER ){
		return( rc );
	}


	if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":214:%-45.45s[%8.8s][%12.12s]%-10.10s\n", ServerName,
		  Network, Node, ConnStatus );
	}else{
		nprintf( ":215:Name: %s\n", ServerName );
		nprintf( ":216:\tAttachment Status: %s\n", ConnStatus ); /* Default */
#ifdef NOT_NEEDED
		nprintf( ":217:\tObject ID: %8.8X\n", ObjID ); /* ffffffff */
#endif NOT_NEEDED
		nprintf( ":218:\tNetwork: %8.8s\n", Network );
		nprintf( ":219:\tNode: %12.12s\n", Node );
		if( AttachedFlag ){
			nprintf( ":220:\tVersion: %s\n", Revision );
			nprintf( ":221:\tAccounting Installed: %s\n",
			  (AcctInstalled ? gettxt(":190", "Yes") : gettxt(":191", "No")) );
			nprintf( ":222:\tMaximum Volumes: %i\n", MaxVols );
			nprintf( ":223:\tMaximum Connections: %i\n", MaxConns );
			nprintf( ":224:\tConnections In Use: %i\n", n_ConnsInUse );
		}
	}

	return( rc );
}

uint32
BinderyDisplayGroup( NWCONN_HANDLE ConnID, char* GroupName, uint32 Flags )
{
	char					Description[NWMAX_SEGMENT_DATA_LENGTH];
	int						i;
	uint32					rc;
	NWFLAGS					MoreSegs;
	NWOBJ_ID				ObjID;

	rc = NWGetObjectID( ConnID, (uint8*)GroupName, OT_USER_GROUP, &ObjID );

	rc = NWReadPropertyValue( ConnID, (uint8*)GroupName, OT_USER_GROUP,
	  (uint8*)"IDENTIFICATION", 1, (uint8*)Description, &MoreSegs, NULL );
	if( rc ){
		strcpy( Description, "" );
	}

	if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":225:%-36.36s%-43.43s\n", GroupName, Description );
	}else{
		nprintf( ":226:Name: %s\n", GroupName );
		nprintf( ":227:\tObject ID:  %8.8X\n", ObjID );
		nprintf( ":228:\tMembers:\n" );
		PrintPropertyObjList( ConnID, "GROUP_MEMBERS", OT_USER_GROUP,
		  GroupName, NULL );

		PrintPropertyObjList( ConnID, "OBJ_SUPERVISORS", OT_USER_GROUP,
		  GroupName, gettxt(":229", "Managers:") );
	}
	return( SUCCESS );
}

uint32
BinderyDisplayVolume( NWCONN_HANDLE ConnID, NWVOL_NUM VolNum, uint32 Flags )
{
	char				NS_String[DEFAULT_STRING_LENGTH];
	char				VolumeName[NWMAX_VOLUME_NAME_LENGTH];
	char				ServerName[NWMAX_OBJECT_NAME_LENGTH];
	int					i;
	int					VolSize;
	int					VolSpaceAvailable;
	uint32				rc;
	NWNS_LIST_SIZE		n_NSEntries;
	NWNS_NUM			NSList[DEFAULT_STRING_LENGTH];
	NWNUMBER			AvailableBlocks;
	NWNUMBER			AvailableDirEntries;
	NWNUMBER			SectorsPerBlock;
	NWNUMBER			TotalBlocks;
	NWNUMBER			TotalDirEntries;
	NWVOL_FLAGS			Removable;

	rc = NWGetVolumeInfoWithNumber( ConnID, VolNum, (uint8*)VolumeName,
	  &TotalBlocks, &SectorsPerBlock, &AvailableBlocks, &TotalDirEntries,
	  &AvailableDirEntries, &Removable );
	if( rc ){
		return( VOLUME_DOES_NOT_EXIST );
	}
	if( strlen(VolumeName) < NWMIN_VOLUME_NAME_LENGTH ){
		return( VOLUME_DOES_NOT_EXIST );
	}

	rc = NWGetNSLoadedList( ConnID, VolNum, sizeof(NSList), NSList,
		&n_NSEntries );
	if( rc ){
		Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n", 51, rc );
/*
		return( rc );
*/
	}else{
		strcpy( NS_String, gettxt(":230", "DOS") );
		for( i=0; i<(int)n_NSEntries; i++ ){
			switch( NSList[i] ){
				case NW_NS_FTAM:
					strcat( NS_String, gettxt(":231", ", FTAM") );
					break;
				case NW_NS_MAC:
					strcat( NS_String, gettxt(":232", ", MAC") );
					break;
				case NW_NS_NFS:
					strcat( NS_String, gettxt(":233", ", NFS") );
					break;
				case NW_NS_OS2:
					strcat( NS_String, gettxt(":234", ", OS/2") );
					break;
				default:
					break;
			}
		}
	}

	if( Flags & NAMES_ONLY ){
		nprintf( ":153:%s\n", VolumeName );
	}else if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":235:%-40.40s%-26.26s\n", VolumeName, NS_String );
	}else{
		rc = NWGetServerNameByConnID( ConnID, ServerName );
		if( rc ){
			strcpy( ServerName, "" );
		}

		VolSize = (int)(TotalBlocks * SectorsPerBlock) / 2;
		VolSpaceAvailable = (int)(AvailableBlocks * SectorsPerBlock) / 2;

		nprintf( ":236:Name: %s\n", VolumeName );
		nprintf( ":237:\tServer: %s\n", ServerName );
		nprintf( ":238:\tName Spaces: %s\n", NS_String );
		nprintf( ":239:\tVolume Size: %i\n", VolSize );
		nprintf( ":240:\tVolume Space Available: %i\n", VolSpaceAvailable );
		nprintf( ":241:\tTotal Directory Entries: %i\n", TotalDirEntries );
		nprintf( ":242:\tAvailable Directory Entries: %i\n",
		  AvailableDirEntries );
		nprintf( ":243:\tRemovable Volume: %s\n", (Removable ?
		  gettxt(":190", "Yes") : gettxt(":191", "No")) );
	}
	return( SUCCESS );
}

uint32
BinderyDisplayActiveUser(	NWCONN_HANDLE	ConnID,
							NWCONN_NUM		ConnNum,
							uint32			Flags )
{
	char				ConnNumString[DEFAULT_STRING_LENGTH];
	char				LoginTimeString[DEFAULT_STRING_LENGTH];
	char				Network[DEFAULT_STRING_LENGTH];
	char				Node[DEFAULT_STRING_LENGTH];
	char				UserName[NWMAX_OBJECT_NAME_LENGTH];
	uint8				LoginTime[DEFAULT_STRING_LENGTH];
	uint32				rc;
	NET_ADDRESS_T*		NetAddr;
	NWCONN_NUM			MyConnNum;
	NWOBJ_ID			ObjID;
	NWOBJ_TYPE			ObjType;
	NWNET_ADDR			NWNetAddress[NWNET_ADDR_LENGTH];

	rc = NWGetConnectionInformation( ConnID, ConnNum, (uint8*)UserName,
	  &ObjType, &ObjID, LoginTime );
	if( (rc != SUCCESS) || (ObjType != OT_USER) ){
		return( FAILED );
	}

	sprintf( LoginTimeString, "%2.2i-%2.2i-%2.2i %2.2i:%2.2i:%2.2i%s",
	  LoginTime[1], LoginTime[2], LoginTime[0],
	  ((LoginTime[3] > 12) ? (LoginTime[3] - 12) : LoginTime[3]),
	  LoginTime[4], LoginTime[5], ((LoginTime[3]<12) ? "am" : "pm") );

	sprintf( ConnNumString, "%i", ConnNum );

	rc = NWGetInternetAddress( ConnID, ConnNum, NWNetAddress );
	if( rc ){
		strcpy( Network, "" );
		strcpy( Node, "" );
	}else{
		NetAddr = (NET_ADDRESS_T*)NWNetAddress;
		sprintf( Network, "%2.2X%2.2X%2.2X%2.2X", NetAddr->network[0],
		  NetAddr->network[1], NetAddr->network[2], NetAddr->network[3] );
		sprintf( Node, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
		  NetAddr->node[0], NetAddr->node[1], NetAddr->node[2],
		  NetAddr->node[3], NetAddr->node[4], NetAddr->node[5] );
	}

	rc = NWGetConnNumberByConnID( ConnID, &MyConnNum );

	if( Flags & NAMES_ONLY ){
		nprintf( ":153:%s\n", UserName );
	}else if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":244:%4.4s %1.1s%-29.29s[%8.8s][%12.12s] %19.19s\n",
		  ConnNumString,
		  ((ConnNum == MyConnNum) ? gettxt(":245", "*") : gettxt(":246", " ")),
		  UserName, Network, Node, LoginTimeString );
	}else{
		nprintf( ":247:User: %s\n", UserName );
		nprintf( ":248:\tNetwork: %s\n", Network );
		nprintf( ":249:\tNode: %s\n", Node );
		nprintf( ":250:\tConnection Number: %i\n", ConnNum );
		nprintf( ":251:\tLogin Time: %s\n", LoginTimeString );
	}
	return( SUCCESS );
}

uint32
BinderyDisplayQueue( NWCONN_HANDLE ConnID, char* QueueName, uint32 Flags )
{
	char					QueueDir[NWMAX_SEGMENT_DATA_LENGTH];
	uint32					rc;
	NWFLAGS					MoreSegs;
	NWOBJ_ID				ObjID;

	rc = NWGetObjectID( ConnID, (uint8*)QueueName, OT_PRINT_QUEUE, &ObjID );
	if( rc ){
		return( rc );
	}

	rc = NWReadPropertyValue( ConnID, (uint8*)QueueName, OT_PRINT_QUEUE,
	  (uint8*)"Q_DIRECTORY", 1, (uint8*)QueueDir, &MoreSegs, NULL);
	if( rc ){
		strcpy( QueueDir, "" );
	}

	if( !(Flags & PRINT_DETAIL) ){
		nprintf( ":252:%40.40s\n", QueueName );
	}else{
		nprintf( ":253:Name: %s\n", QueueName );
		nprintf( ":254:/tObject ID: %8.8X\n", ObjID );
		if( (strlen(QueueDir)) != 0 ){
			nprintf( ":255:/tQueue Directory: %s\n", QueueDir );
		}
		nprintf( ":256:/tUsers:\n" );
		PrintPropertyObjList( ConnID, "Q_USERS", OT_PRINT_QUEUE, QueueName,
		  NULL );
		nprintf( ":257:/tOperators:\n" );
		PrintPropertyObjList( ConnID, "Q_OPERATORS", OT_PRINT_QUEUE, QueueName,
		  NULL );
		nprintf( ":258:/tServers:\n" );
		PrintPropertyObjList( ConnID, "Q_SERVERS", OT_PRINT_QUEUE, QueueName,
		  NULL );
	}
	return( SUCCESS );
}

uint32
PrintPropertyObjList(	NWCONN_HANDLE	ConnID,
						char*			PropName,
						NWOBJ_TYPE		ObjType, 
						char*			ObjName,
						char*			PropTitle )
{
	uint32				rc;
	void*				Node;
	SL_LIST_T*			List;

	rc = InitSLList( &List, free, strcmp, NULL, 0 );
	if( rc ){
		return( rc );
	}

	rc = GetPropertyObjList( ConnID, PropName, ObjType, ObjName, List );
	if( rc == SUCCESS ){
		if( PropTitle != NULL ){
			nprintf( ":259:\t%s\n", PropTitle );
		}
		if( isSLListEmpty(List) == FALSE ){
			Node = GetSLListHead( List );
			while( TRUE ){
				nprintf( ":260:\t\t%s\n", GetSLListData(Node) );
				if( isSLListLastNode( Node ) ){
					break;
				}
				Node = GetSLListNextNode( Node );
			}
		}
	}
	FreeSLList( &List, FREE_DATA );

	return( rc );
}

uint32
GetPropertyObjList(	NWCONN_HANDLE	ConnID,
					char*			PropName,
					NWOBJ_TYPE		ObjType, 
					char*			ObjName,
					SL_LIST_T*		List )
{
	int						i=0;
	int						n_Objs;
	char					TmpObjName[NWMAX_OBJECT_NAME_LENGTH];
	NWCCODE					rc;
	NWFLAGS					MoreSegs;
	NWOBJ_TYPE				MemberType;
	NWOBJ_ID				ObjList[OBJECT_LIST_LENGTH];
	NWSEGMENT_NUM			SegNum=0;

	while( TRUE ){
		SegNum++;
		rc = NWReadPropertyValue( ConnID, (uint8*)ObjName, ObjType,
		  (uint8*)PropName, SegNum, (uint8*)ObjList, &MoreSegs, NULL );
		if( rc ){
			return( rc );
		}
		for( i=0; ObjList[i] && (i < OBJECT_LIST_LENGTH); i++ ){
			rc = NWGetObjectName( ConnID, ObjList[i], (uint8*)TmpObjName,
			  &MemberType);
			if( !rc ){
				rc = AppendToSLList( List, strdup(TmpObjName), 0 );
				if( rc ){
					Error( ":150:An unexpected error occurred. (%i:%#8.8x)\n",
					  12, rc );
					MoreSegs = 0;	/* stop reading for this object */
					break;	/* no more room for list - print what we got */
				}
			}
		}

		/*	If there aren't any more segments
		 *	break out of this loop and finish up.
		 */
		if( !MoreSegs ){
			break;
		}
	}

	return( rc );
}

NWOBJ_TYPE
ParseObjType( char* ObjTypeString )
{
	NWOBJ_TYPE		ObjType;

	if( !strcmpi(ObjTypeString, gettxt(":261", "GROUP")) ){
		strcpy( ObjTypeString, gettxt(":261", "GROUP") );
		ObjType = OT_USER_GROUP;
	}else if( !strcmpi(ObjTypeString, gettxt(":262", "QUEUE")) ){
		strcpy( ObjTypeString, gettxt(":262", "QUEUE") );
		ObjType = OT_PRINT_QUEUE;
	}else if( !strcmpi(ObjTypeString, gettxt(":263", "SERVER")) ){
		strcpy( ObjTypeString, gettxt(":263", "SERVER") );
		ObjType = OT_FILE_SERVER;
	}else if( !strcmpi(ObjTypeString, gettxt(":264", "USER")) ){
		strcpy( ObjTypeString, gettxt(":264", "USER") );
		ObjType = OT_USER;
	}else if( !strcmpi(ObjTypeString, gettxt(":265", "VOLUME")) ){
		strcpy( ObjTypeString, gettxt(":265", "VOLUME") );
		ObjType = OT_VOLUME;
	}else{
		ObjType = 0;
	}

	return( ObjType );
}

int
CheckAuthentication( NWCONN_HANDLE ConnID, NWOBJ_TYPE ObjType )
{
	int			rc = FALSE;

	switch( ObjType ){
		case OT_FILE_SERVER:
		case OT_VOLUME:
			rc = TRUE;
			break;
		default:
			rc = isAuthenticated( ConnID );
			break;
	}
	return( rc );
}
