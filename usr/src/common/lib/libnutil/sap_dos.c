/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/sap_dos.c	1.5"
#ident	"$Id: sap_dos.c,v 1.8 1994/07/22 20:52:16 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "memmgr_types.h"
#include "sap_lib.h"
#include <sys/sap_dos.h>
#include <util_proto.h>

extern void StopSapList(void);

/******************************************************************/
/*
**	The AdvertiseService function advertises a service on the network
**  This function is present for Native compatibility with the
**  following exceptions:
**  - A socket number of 0 may not be used for dynamic allocation. The
**    application must open the socket it wishes to use.	
*/
int
AdvertiseService(
	uint16	serverType,
	char	*serverName,
	uint8	*serverSocket)
{
	int		ccode = 0;
	uint16	socketNum;

/*
**	Check for a 0 in the serverSocket field
*/
	socketNum = PGETINT16( serverSocket );
	if( socketNum == 0 )
		return( -SAPL_INVALSOCK );

	ccode = SAPAdvertiseMyServer(serverType, (uint8 *)serverName,
									socketNum, SAP_ADVERTISE );

	return(ccode);
}

/******************************************************************/
/*
**	The ShutdownSAP function informs sapd to stop advertising a process's
**  list of services.
*/
int
ShutdownSAP( void )
{
	StopSapList();

	return( 0 );
}


/******************************************************************/
/*
**	The QueryServices function allows the user to query for a 
**	specified number of servers of a specified type, or the nearest
**	server.
*/
int
QueryServices(
	uint16	queryType,
	uint16	serverType,
	uint16	returnSize,
	SAP_ID_PACKET *serviceBuffer)
{
	SAPI			*serverEntry, *currSAPI;
	SAP_ID_PACKET	*currSAP_ID;
	int  			ccode, maxEntries, i;
	int 			index = 0; 			/* Index for QueryServices */

/*
**	Determine how many elements of SAP_ID_PACKET we can return
*/
	maxEntries = (returnSize / sizeof(SAP_ID_PACKET));
	if(maxEntries == 0)
		return( -3 );

/*
**	Allocate the same number of SAPI structures so we can get that number
**	of entries
*/
	serverEntry = (SAPI *)malloc((unsigned)(sizeof(SAPI) * maxEntries));
	if(serverEntry == NULL)
		return (-SAPL_ENOMEM );
 
/*
**	Switch on query type.
*/
	if(queryType == GENERAL_SERVICE_REQUEST)	/* 0x0001 */
	{
		ccode = SAPGetAllServers( serverType, &index, serverEntry,
									maxEntries );
		if(!ccode) /* We didn't find anything */
			return( -1 );
		if(ccode < 0)
			return(ccode);
	}
	else if(queryType == NEAREST_SERVER_REQUEST)	/* 0x0003 */
	{
		ccode = SAPGetNearestServer( serverType, serverEntry );
		if(!ccode) /* We didn't find anything */
			return( -1 );
		if(ccode < 0)
			return(ccode);
	}
	else /* Invalid Request */
	{
		return( -2 );
	}

/*
**	Load the reply
*/
	currSAPI = serverEntry;
	currSAP_ID = serviceBuffer;

	for(i=0; i<ccode; i++)
	{
		currSAP_ID->serverType = currSAPI->serverType;
		strcpy(currSAP_ID->serverName,
						(char *)currSAPI->serverName);
		memcpy(currSAP_ID->network,
					currSAPI->serverAddress.net, IPX_NET_SIZE);
		memcpy(currSAP_ID->node,
					currSAPI->serverAddress.node, IPX_NODE_SIZE);
		currSAP_ID->socket = PGETINT16(currSAPI->serverAddress.sock);
		currSAP_ID->hops = currSAPI->serverHops;
		currSAP_ID++;
		currSAPI++;
	}
	
	(void) free( serverEntry );
	return( ccode );
}
