/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:nwslist.c	1.10"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/nwslist.c,v 1.14 1994/09/29 20:21:27 renuka Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL 							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <sys/sap_app.h>
#include <nw/nwcalls.h>
#include <nwbinderyprops.h>
#include <nct.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "scroll.h" 
#include "main.h" 

/************************************************************
		global defines
************************************************************/
#ifndef uint32
#define uint32	unsigned long
#endif

#define CONN_HANDLE_LIST_SIZE 		255

struct ConnData 
{
	char *server;
	char *user;
};

/************************************************************
		extern declarations	
************************************************************/
extern void		GUIError (Widget, char *);
extern char		*GetStr  (char *);
extern void		qsort();

/************************************************************
		static declarations	
************************************************************/
static int		compr();
void 			GetConnInfo(NWCONN_HANDLE ConnID, 
				struct ConnData *connData);
NWCONN_HANDLE 		ConnList[CONN_HANDLE_LIST_SIZE];
NWNUMBER        	NumConns = 0;

/************************************************************
		Read the servers routine
		returns the list of servers using Hashem's
		NWGetNetWareServerList routine
************************************************************/
int
ReadServers (Widget parent, ServerList *servers)
{
	NWCONN_HANDLE			serverconnID, connectionID;
	int				j, index, slistIndex;
	uint32				ccode;
	char				buffer[50];
	SLIST_STRUCT			slist[MAXSERVER];
	struct ConnData 		*connData = NULL;
	unsigned int 			connRef;
	int 				i, ServerEntry = 0;
	char				**Servers;	
	NET_ADDRESS_T			*NetAddr;
	NWFLAGS				MoreSegs;
	NWSEGMENT_DATA			SegData[NWMAX_SEGMENT_DATA_LENGTH];
	SAPI 				ServerBuf;
	uid_t				uid;
	
	slistIndex = 0;
	uid = geteuid ();
	
	/* Initialize the requester before getting the list of servers
	 */
#ifdef DEBUG
	printf ("initializing the requester \n");
#endif
	if(ConnList[0] != 0)
		NWFreeConnectedConnIDs(ConnList, &NumConns);

	ccode = NWCallsInit (NULL, NULL);
	if (ccode) {
		GUIError (parent, GetStr (TXT_initfail));
		return(0);
	}

	/* Use the new api from Hashem to get a server list	
	 */
#ifdef DEBUG
	printf ("getting the server list from Hashem\n");
#endif
	if ((Servers = (char **)NWGetNetWareServerList (uid, &slistIndex)) 
		== NULL) {
		GUIError (parent, GetStr (TXT_noserver));
		return(0);
	}
#ifdef DEBUG
	printf ("reading the server list from Hashem %d\n", slistIndex);
	for (i = 0; i < slistIndex; i++) 
		printf ("%s\n", Servers[i]);
#endif

	/* Store the server names in the slist structure we have and
	 * get the Network and Node address here.  
	 */
#ifdef DEBUG
	printf ("for each server in the server list from Hashem\n");
	printf ("getting the network and node address\n");
#endif
	for (i = 0; i < slistIndex; i++) {

		/* this is where the junk bug was - add null to string
		 */
		strncpy (slist[i].ObjectName, Servers[i], 
				strlen (Servers[i]) + 1);

		/* Try to get the node/network address using the SAP api
		 * first
		 */
		ServerEntry = NULL;
		if ((SAPGetServerByName ((NWsp) slist[i].ObjectName,  
					FILE_SERVER_TYPE, &ServerEntry, 
					&ServerBuf, 1)) >= 0) {
#ifdef DEBUG
			printf ("network, node address via SAP %s\n",
				slist[i].ObjectName);
#endif
			memcpy( slist[i].Network,
				ServerBuf.serverAddress.net, 4 );
			memcpy( slist[i].NodeAddress, 
				ServerBuf.serverAddress.node, 6 );
		}
		/* If that does not work then use the read property value
		 * NW call that hits the wire and gets the network/node
		 * address
		 */
		else {
#ifdef DEBUG
			printf ("network, node address via NWcalls %s\n",
				slist[i].ObjectName);
#endif
			ccode = NWGetConnIDByName (Servers[i], &serverconnID);
			if (serverconnID == True) {
				ccode = NWReadPropertyValue(serverconnID, (NWsp)
					Servers[i], OT_FILE_SERVER, 
					(NWsp)"NET_ADDRESS", 1, (NWsp)SegData, 
					&MoreSegs, NULL );

				if( !ccode ) {
					NetAddr = (NET_ADDRESS_T*) SegData;
					memcpy(slist[i].Network, 
						NetAddr->network, 4);
					memcpy(slist[i].NodeAddress, 
						NetAddr->node, 6);
        			}
				else {
#ifdef DEBUG
					printf ("did not read prop value %s\n",
						Servers[i]);
#endif
					/* did not get the address */
					for (index = 0; index < 4; index++)
						slist[i].Network[index] = 0;
					for (index = 0; index < 6; index++)
						slist[i].NodeAddress[index] = 0;
				}
				/* Close the connection
				 */
				NWCloseConn(serverconnID);

			} /* got the connid by name */

			else {
#ifdef DEBUG
				printf ("did not get connid by name %s\n",
					Servers[i]);
#endif
				/* did not get the address */
				for (index = 0; index < 4; index++)
					slist[i].Network[index] = 0;
				for (index = 0; index < 6; index++)
					slist[i].NodeAddress[index] = 0;

			} /* did not get the connection id by name */

		} /* SAP api did not work so hit the wire */

	} /* for all servers get the network/node address */

	if (!slistIndex) {
		GUIError (parent, GetStr (TXT_noserver));
		return(0);
	}
	else 
		NWFreeNetWareServerList (Servers, slistIndex);

	/* Sort the slist table using qsort */
	qsort( (char *)slist, slistIndex, sizeof(SLIST_STRUCT), compr );

	/*
	 * Get the default connection id
	 * If that succeeds then we are probably logged into a server
	 * so query for a list a user names.
	 */
	ccode = NWGetPrimaryConnRef((pnuint32)&connRef);
	if ( ccode == 0) {
		ccode = NWOpenConnByReference(connRef, NWC_OPEN_PUBLIC | 
					NWC_OPEN_UNLICENSED,&connectionID);
		NWCloseConn (connectionID);
		if ( ccode == 0) {
			/* Get list of servers we are connected to 
		 	 * and the user name 
		 	 */
			/*getConnectionList(ConnList,&NumConns); */
			NWGetConnectedConnIDs(ConnList, &NumConns);
    			if( NumConns > 0 ) {
				connData = (struct ConnData *) malloc 
					(sizeof(struct ConnData) * NumConns);
        			for( i=0; i < (int)NumConns; i++ ) {
            				GetConnInfo( ConnList[i],&connData[i]);
       				}
   			}
		}
	}

	servers->cnt = servers->allocated = 0;
	servers->list = 0;
	servers->shortlist = 0;

	/* print the sorted slist */
	for( j = 0; j < slistIndex; j++) {

		/**********allocating space for 50 servers at a time ******/
		if (servers->cnt >= servers->allocated) { 
		 	servers->allocated += SERVER_ALLOC_SIZE;
			servers->list = (FormatData *)
				XtRealloc ((char *) servers->list,
			   	servers->allocated * sizeof (FormatData));
			servers->shortlist = (ShortFormatData *)
			XtRealloc ((char *) servers->shortlist,
		  	servers->allocated * sizeof (ShortFormatData));
		}
	
		/*****FILE SERVERS NAMES **********************/
		if (strlen (slist[j].ObjectName) > 20) { 
			slist[j].ObjectName[38] = '.';
			slist[j].ObjectName[39] = '.';
			slist[j].ObjectName[40] = '\0';
		}
		
		servers->list [servers->cnt].FileServers = 
				(XtPointer) strdup (slist[j].ObjectName);
		servers->shortlist [servers->cnt].FileServers = 
				(XtPointer) strdup (slist[j].ObjectName);

		/******NETWORK ADDRESS ***********************/
		for (index = 0 ; index < 4; index++) {
			sprintf (buffer,"%02x",slist[j].Network[index]);
			servers->list [servers->cnt].NwAddress[index] = 
				(XtPointer) strdup (buffer);
		}

		/****************NODE ADDRESS ***************/
		for (index = 0 ; index < 6; index++) {
			sprintf (buffer,"%02x",slist[j].NodeAddress[index]);
			servers->list [servers->cnt].NodeAddress[index] = 
				(XtPointer) strdup (buffer); 
		}
		/****************user names *****************/
		strcpy (buffer, " ");
		for ( i = 0; i < NumConns; i++ ) {
			if (strcmp(slist[j].ObjectName,connData[i].server) 
				== NULL ) {
				strcpy (buffer, connData[i].user);
				break;
			}
		}
		 servers->list [servers->cnt].userName = 
				(XtPointer) strdup (buffer);
		 servers->shortlist [servers->cnt++].userName = 
				(XtPointer) strdup (buffer);
	}
	if ( connData )
		free(connData);

    return(1);

}	/* End of ReadServers () */

/*******************************************************
	comparison routine for the servers names
*******************************************************/
static int 
compr(server1, server2)
SLIST_STRUCT *server1, *server2;
{
	int i;

	i = strcmp(server1->ObjectName, server2->ObjectName);
	return i;
}

/*******************************************************
 * Get connection information. If user is authenticated
 * to the connection id, get user name from connid.
*******************************************************/
void
GetConnInfo(NWCONN_HANDLE ConnID, struct ConnData *connData)
{
	char user[48];
	char server[48];
	
	connData->user = NULL;
	connData->server = NULL;
		
   	if( isAuthenticated(ConnID) ) {
		if (NWGetUserNameByConnID(ConnID, user) == 0) {
				connData->user = strdup(user);
		}
		else {
			connData->user = strdup(" ");
		}
    	}
	if (NWGetServerNameByConnID(ConnID,server) == 0) {
		connData->server = strdup(server);
   	}
}
	
/*******************************************************
 * Get connection list.  Scan the connection information
 * and return. 
*******************************************************/
getConnectionList(NWCONN_HANDLE *connListBuffer, short * numConns)
{
    	int 		connListSize = CONN_HANDLE_LIST_SIZE;
	nuint32     	scanIndex = 0;
	nuint       	curConn = 0;
	nuint32     	connRef = 0, tmp;
	NWCCODE     	ccode;
	NWCONN_HANDLE 	conn;

	*numConns = 0;
	while (connListSize > 0) {
		ccode = NWScanConnInformation(	&scanIndex,
					NWC_CONN_INFO_RETURN_ALL,
	                      		0, NULL, NWC_RETURN_PUBLIC, 
	                      		NWC_CONN_INFO_CONN_REF,
	                      		sizeof(nuint32),
	                      		&connRef, &tmp);
	
		if (ccode == SCAN_COMPLETE)
			break;
		if (ccode)
			return(ccode);
		ccode = NWOpenConnByReference(connRef,
				NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED,&conn);
		if (ccode)
			return(ccode);
		connListBuffer[curConn++] = conn;
		connListSize--;
   		*numConns = curConn;
		NWDetach (conn);
   	}
   	*numConns = curConn;
   	return(0);
}
