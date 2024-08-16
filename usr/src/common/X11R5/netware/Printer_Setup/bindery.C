#ident	"@(#)prtsetup2:bindery.C	1.13"
/*----------------------------------------------------------------------------
 *	bindery.c
 *
 *	This file contains functions to access data from NetWare binderies. This
 *	includes: file servers, print queues, and printers.
 */

#define OWNER_OF_BINDERY

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include <Xm/Xm.h>
#include <nw/nwcalls.h>
extern "C" {
#include <sys/sap_app.h>
}

#include "bindery.h"
#include "ps_hdr.h"
#include "ps_i18n.h"

#define NUMQUEUES 				512
#define	MAX_SERVERS_AT_ONCE		256
//#define	FAILURE					0x000000FFL

/*typedef struct sapi {
	unsigned int serverType;
	unsigned char serverName[48];
}SAPI;*/

//--------------------------------------------------------------
//	Get a list of servers from the  SAP table that match the argumented
//	sapType.
//	Parameters:	listCount     load with the amount of servers found
//    			list          load with names of servers found
//
//	Callee is responsible for freeing the server list space.
//--------------------------------------------------------------
static void
getSappingServerList (const int sapType, char*** list, int* listCount)
{
	SAPI						ServerBuf[MAX_SERVERS_AT_ONCE];
	char**						temp;
	int							ServerEntry = 0;
	int							MaxEntries = MAX_SERVERS_AT_ONCE;
	int							ServerBufSize;
	int							i;
	int							x;

	ServerBufSize = MaxEntries;
	*listCount = 0;
	temp = (char**)calloc ((sizeof (char*)) * MaxEntries, 1);
	*list = (char**)temp;
	do {
		i = SAPGetAllServers (sapType, &ServerEntry, ServerBuf, MaxEntries);
		if (i <= 0) {
			break;
		}
		for (x = 0 ; x < i ; x++) {
			temp[*listCount] = (char*)strdup ((const char*)ServerBuf[x].serverName);
			*listCount += 1;
		}
		if (i == MaxEntries) {
			ServerBufSize += MaxEntries;
			temp = (char**)realloc ((char*)temp,
									sizeof (char*) * ServerBufSize);
			*list = (char**)temp;
		}
	} while (i == MaxEntries);
}

/*----------------------------------------------------------------------------
 *	This function compares 2 strings and returns values as strcmp does.
 */
static int
cmpr (const void* str1, const void* str2)
{
	static char**				s1;
	static char**				s2;

	s1 = (char**)str1;
	s2 = (char**)str2;
	return (strcmp (*s1, *s2));
}

/*----------------------------------------------------------------------------
 *	This function returns a list of NetWare file servers.
 */
char**
GetNetWareFileServers ()
{
	static char**				serverList = 0;	
	int				 			cnt;

	if (!serverList) {
		getSappingServerList (0x4, &serverList, &cnt);
		serverList = (char**)XtRealloc ((char*)serverList,
										(cnt + 1) * sizeof (char*));

		if (serverList) {									// Sort the list
			serverList[cnt] = 0;
			qsort (serverList, cnt, sizeof (char*), cmpr);
		}
	}	
	return (serverList);
}

/*----------------------------------------------------------------------------
 *	This function returns a list of NetWare print queues.
 *
 *	***** THIS NEEDS TO RETURN ERROR NUMBERS.  One for No Connection and the
 *		  Other for no PrintQueues and not malloc memory unless needed.
 *	*************************************************************************
 */
XmString*
GetNetWarePrintQueues (char* name, short* retCnt)
{
	NWCONN_HANDLE 				connID;
	NWOBJ_ID NWFAR  			objectID = -1;
	NWOBJ_TYPE NWFAR			objType;
	NWFLAGS NWFAR				hasPropertiesFlag;
	NWFLAGS NWFAR				objectFlags;
	NWFLAGS NWFAR				objectSecurity;
	NWCConnString				string;
	char NWFAR					objectName[48 + 1];  // Can't find define
	XmString*					printQueues = 0;
	int							cnt = 0;

	string.pString = name;
	string.uStringType = NWC_STRING_TYPE_ASCII;
	string.uNameFormatType = NWC_NAME_FORMAT_BIND;	

	if (NWOpenConnByName (NULL,
						  &string,
						  "NCP_SERVER",
						  NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED,
						  NWC_TRAN_TYPE_WILD,
						  (NWCONN_HANDLE NWPTR)&connID) != SUCCESSFUL) {
		return (0);
	}

	printQueues = (XmString*)XtMalloc ((NUMQUEUES * sizeof (XmString)));

	for (;;cnt++) {
		if (NWScanObject (connID,
						  "*",
						  OT_PRINT_QUEUE,
						  &objectID,
						  (pnstr8)objectName,
						  &objType,
						  &hasPropertiesFlag,
						  &objectFlags,
						  &objectSecurity) != SUCCESSFUL) {
			break;
		}
 
		if ((cnt % NUMQUEUES) == (NUMQUEUES - 2)) {
			printQueues = (XmString*)XtRealloc ((char*)printQueues,
												((cnt + NUMQUEUES) *
												sizeof (XmString)));
		}

		printQueues[cnt] = XmStringCreateLocalized (objectName);
		printQueues[cnt + 1] = 0;
	}

	NWCloseConn (connID);

	*retCnt = cnt;
	return (printQueues);
}

/*----------------------------------------------------------------------------
 *	This function returns a list of NetWare print servers.
 *
 *	***** THIS NEEDS TO RETURN ERROR NUMBERS.  One for No Connection and the
 *		  Other for no PrintQueues and not malloc memory unless needed.
 *	*************************************************************************
 */
XmString*
GetNetWarePrintServers (char* name, short* retCnt)
{
	NWCONN_HANDLE NWFAR 		connID;
	NWOBJ_ID NWFAR				sequence = -1;
	NWOBJ_TYPE NWFAR			objType;
	NWFLAGS NWFAR				hasPropertiesFlag;
	NWFLAGS NWFAR				objectFlags;
	NWFLAGS NWFAR				objectSecurity;
	NWCConnString				string;
	char NWFAR					objectName[48 + 1]; // Can't find define
	XmString*					printServers = 0;
	int							cnt = 0;

	string.pString = name;
	string.uStringType = NWC_STRING_TYPE_ASCII;
	string.uNameFormatType = NWC_NAME_FORMAT_BIND;	

	if (NWOpenConnByName (NULL,
						  &string,
						  "NCP_SERVER",
						  NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED,
						  NWC_TRAN_TYPE_WILD,
						  (NWCONN_HANDLE NWPTR)&connID) != SUCCESSFUL) {
		return (0);
	}

	printServers = (XmString*)XtMalloc ((NUMQUEUES * sizeof (XmString)));

	for (;;cnt++) {
		if (NWScanObject (connID,
						  "*",
						  OT_PRINT_SERVER,
						  &sequence,
						  (pnstr8)objectName,
						  &objType,
						  &hasPropertiesFlag,
						  &objectFlags,
						  &objectSecurity) != SUCCESSFUL) {
			break;
		}

		if ((cnt % NUMQUEUES) == (NUMQUEUES - 2)) {
			printServers = (XmString*)XtRealloc ((char*)printServers,
												 ((cnt + NUMQUEUES) *
												 sizeof (XmString)));
		}

		printServers[cnt] = XmStringCreateLocalized (objectName);
		printServers[cnt + 1] = 0;
	}

	NWCloseConn (connID);

	*retCnt = cnt;
	return (printServers);
}

/*----------------------------------------------------------------------------
 *	Function to deallocate an array of XmStrings
 */
void
FreeArrayOfXmStrings (XmString* arr)
{
	for (int i = 0; arr[i] != NULL; i++) {
		XmStringFree (arr[i]);
	}
	XtFree ((char*)arr);
}

//--------------------------------------------------------------
//	This function determines a user is authenticated to a NetWare file server.
//
//	True if authenticated False if not authenticated
//--------------------------------------------------------------
Boolean
IsAuthenticated (char* name)
{
	char*						buf;
	char						xyz[64];
	struct passwd*				pwd;	
	Boolean						retCode;
	uid_t						uid;
	char						tbuf[64];

	uid = getuid ();
	pwd = getpwuid (uid);

	sprintf (tbuf, "%d", pwd->pw_uid);
	buf = (char*)XtMalloc (strlen (xauto_path) + 
							strlen (space) + 2 +
							strlen (space) + strlen (name) + 
							strlen (space) + 2 + 
							strlen (space) + strlen (pwd->pw_name) + 
							+ 74);

	strcpy (buf, xauto_path);
	strcat (buf, space);
	strcat (buf, "-s");
	strcat (buf, space);
	strcat (buf, name);           /* server's name */
	strcat (buf, space);
	strcat (buf, "-u");
	strcat (buf, space);
	strcat (buf, pwd->pw_name);   /* user's name */
	strcat (buf, space);
	strcat (buf, "-i");
	strcat (buf, space);
	sprintf (xyz, "%d", pwd->pw_uid);
	strcat (buf, xyz);

	if (IsSelectable (name) != SUCCESSFUL) {
		if (system (buf) == -1) {
			retCode = False;
		}
	 	else {
			if (IsSelectable (name) != SUCCESSFUL) {
				retCode = False;
			}
			else {
				retCode = True;
			}
		}
	}
	else {
		retCode = True;
	}

	XtFree (buf);
	return (retCode);
}

//--------------------------------------------------------------
// This function determines a user is selecable.
//
// True if selectable False if not selectable
//--------------------------------------------------------------
static NWCCODE
IsSelectable (char* name)
{
	NWCCODE						retCode;
	static NWCONN_HANDLE 		pConnHandle;		
	NWCConnString				string;

	string.pString = name;
	string.uStringType = NWC_STRING_TYPE_ASCII;
	string.uNameFormatType = NWC_NAME_FORMAT_BIND;	

#ifdef DEBUG
	printf("In IsSelectable\n");
#endif
	retCode = NWOpenConnByName (NULL,
								&string,
								"NCP_SERVER",
								NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED, 
								NWC_TRAN_TYPE_WILD, 
								(NWCONN_HANDLE NWPTR)&pConnHandle);
#ifdef DEBUG
	printf( "retCode is %d\n", retCode );
#endif
	if (retCode == SUCCESSFUL) {
		uint32 					rc;
		uint32 					authFlags;

		rc = NWGetConnInformation (pConnHandle,
								   NWC_CONN_INFO_AUTH_STATE,
								   sizeof (uint32),
								   &authFlags);
		NWCloseConn (pConnHandle);
		if (rc) {
			return( FAILURE );
		}
		if (authFlags == NWC_AUTH_STATE_NONE) {
			return (FAILURE);
		}
		return (SUCCESSFUL);
	}
	NWCloseConn (pConnHandle);
	return( retCode );
} 

