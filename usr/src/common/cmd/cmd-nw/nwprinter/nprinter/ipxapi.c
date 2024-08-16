/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/ipxapi.c	1.2"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)cmd/rprinter/ipxapi.c 1.3 (Novell) 7/30/91";
#endif

#include <ctype.h>
#include <fcntl.h>
#include <tiuser.h>
#ifdef OS_AIX
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* OS_AIX */
#include <errno.h>
#include "ipxapi.h"
#include "ipx.h"
#include "ipxtdr.h"

#define MAX_SAP_TIMEOUT					200			/* msec */

#define MAX_IPX_ADDRESS_DISPLAY_LENGTH	30
#define MAX_ERRNO_DISPLAY_LENGTH		18


extern int	errno;
extern int	t_errno;


static void uppercase();


int
IPXOpenTransport(
	IPXHandle_t	*ipxHandle,
	int16		 socket)
{
	struct t_bind *tbind;

	if ((ipxHandle->fd =
		t_open( IPX_DRIVER_NAME, O_RDWR, (struct t_info *) 0 )) == -1)
	{
		ipxHandle->errno = t_errno;
		ipxHandle->errnoType = IPXET_T_ERRNO;
		return FAILURE;
	}

	/* FIX?: We are not checking that the address we are binding
	** to is actually what we asked for.
	* NOTE: Don't think we should worry, this is dynamic socket alloc.
	* However, we should allocate memory for bind struct. MJW 2/24/92
	*/
     
    if((tbind = (struct t_bind *)t_alloc(ipxHandle->fd, T_BIND, T_ALL))==NULL)
    {   
        ipxHandle->errno = t_errno;
        ipxHandle->errnoType = IPXET_T_ERRNO;
        t_close( ipxHandle->fd );
        return FAILURE;
    }
	TDRSetSocketInIPXAddress( ipxHandle->ipxAddress, socket );
	tbind->addr.len = IPX_ADDRESS_LENGTH;
	tbind->addr.maxlen = IPX_ADDRESS_LENGTH;
	tbind->addr.buf = (char *) ipxHandle->ipxAddress;
	tbind->qlen = 0;

	if (t_bind( ipxHandle->fd, tbind, tbind ) == -1) {
		ipxHandle->errno = t_errno;
		ipxHandle->errnoType = IPXET_T_ERRNO;
		t_close( ipxHandle->fd );
		return FAILURE;
	}

	ipxHandle->errno = 0;
	return 0;
}


int
IPXCloseTransport( IPXHandle_t	*ipxHandle)
{
	if (t_close( ipxHandle->fd ) == -1) {
		ipxHandle->errno = t_errno;
		ipxHandle->errnoType = IPXET_T_ERRNO;
		return FAILURE;
	}

	return 0;
}


int
IPXIsServerAdvertising(
	IPXHandle_t			*ipxHandle,
	IPXsapServerInfo_t	*sapInfo)
{
	int	   i;
	int	   flags;
	int	   pollrc;
	uint8  ipxType = 0;
	uint8  queryPacket[IPX_SAP_GSQ_PACKET_LENGTH];
	uint8  responsePacket[IPXMAX_SAP_GSR_PACKET_LENGTH];
	char   serverName[IPXMAX_SERVER_NAME_LENGTH];
	unsigned long numOfFds;
	IPXAddress_ta broadcastAddr;
	IPXAddress_ta responseAddr;
	IPXsapGSResponse_t responseInfo;
	IPXsapServerInfo_t *respInfoPtr;
	struct t_unitdata *queryTud;
	struct t_unitdata *responseTud;
	struct pollfd fds[2];

	strncpy( serverName, sapInfo->serverName,
		IPXMAX_SERVER_NAME_LENGTH );
	uppercase( serverName );

	TDRSetSAPBroadcastInIPXAddress( broadcastAddr );
	TDRSAPGeneralServiceQuery( sapInfo->serverType, queryPacket );

	/* FIX?: The unitdata structures should really have be allocated with
	** t_alloc.  
	*  True!  T_alloc should be here!  MJW 2/24/92
    */
    if((queryTud = (struct t_unitdata *)t_alloc(ipxHandle->fd,
                        T_UNITDATA, T_ALL))==NULL)
    {
        ipxHandle->errno = t_errno;
        ipxHandle->errnoType = IPXET_T_ERRNO;
        return FALSE;
    }
	queryTud->addr.len = IPX_ADDRESS_LENGTH;
	queryTud->addr.buf = (char *) broadcastAddr;
	queryTud->opt.len = sizeof( uint8 );
	queryTud->opt.buf = (char *) &ipxType;
	queryTud->udata.len = IPX_SAP_GSQ_PACKET_LENGTH;
	queryTud->udata.buf = (char *) queryPacket;

	if (t_sndudata( ipxHandle->fd, queryTud ) == -1) {
		if (t_errno == TLOOK) {
			ipxHandle->errno = t_look( ipxHandle->fd );
			ipxHandle->errnoType = IPXET_TLOOK;
		} else {
			ipxHandle->errno = t_errno;
			ipxHandle->errnoType = IPXET_T_ERRNO;
		}
		return FALSE;
	}

	/* FIX?: The unitdata structures should really have be allocated with
	** t_alloc.  
    *  True!  t_alloc should be here!  MJW 2/24/92
    */
    if((responseTud = (struct t_unitdata *)t_alloc(ipxHandle->fd,
                        T_UNITDATA, T_ALL))==NULL)
    {
        ipxHandle->errno = t_errno;
        ipxHandle->errnoType = IPXET_T_ERRNO;
        return FALSE;
    }
	responseTud->addr.len = IPX_ADDRESS_LENGTH;
	responseTud->addr.maxlen = IPX_ADDRESS_LENGTH;
	responseTud->addr.buf = (char *) responseAddr;
	responseTud->opt.len = sizeof( uint8 );
	responseTud->opt.maxlen = sizeof( uint8 );
	responseTud->opt.buf = (char *) &ipxType;
	responseTud->udata.len = IPXMAX_SAP_GSR_PACKET_LENGTH;
	responseTud->udata.maxlen = IPXMAX_SAP_GSR_PACKET_LENGTH;
	responseTud->udata.buf = (char *) responsePacket;

	fds[0].fd = ipxHandle->fd;
	fds[0].events = POLLIN;
	numOfFds = 1;

	while (pollrc = poll( fds, numOfFds, MAX_SAP_TIMEOUT )) {
		if (pollrc == -1) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			ipxHandle->errno = errno;
			ipxHandle->errnoType = IPXET_ERRNO;
			return FALSE;
		}

		/* FIX?: flags should be checked upon return, since, if there
		** is more data to be recv'd than can be held in a given 
		** buffer, flags would be set to T_MORE.
        ** NOTE: Don't think we should worry.  This is just getting a
        ** General Service Response and thats what we allocate.
        ** MJW 2/24/92
		*/
		if (t_rcvudata( ipxHandle->fd, responseTud, &flags )
		== -1) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			if (t_errno == TLOOK) {
				ipxHandle->errno = t_look( ipxHandle->fd );
				ipxHandle->errnoType = IPXET_TLOOK;
			} else {
				ipxHandle->errno = t_errno;
				ipxHandle->errnoType = IPXET_T_ERRNO;
			}
			return FALSE;
		}

		TDRSAPGeneralServiceResponse( (uint8 *)responseTud->udata.buf,
			responseTud->udata.len, &responseInfo);

		for (i = 0; i < (int)responseInfo.serverCount; i++) {
			respInfoPtr = &responseInfo.servers[i];

			if (!strcmp( serverName, respInfoPtr->serverName )
			&& (sapInfo->serverType == respInfoPtr->serverType
			|| sapInfo->serverType == IPX_SERVER_TYPE_WILD_CARD)) {
				memcpy( (char *) sapInfo->ipxAddress,
					(char *) respInfoPtr->ipxAddress,
					IPX_ADDRESS_LENGTH );
				sapInfo->serverType = respInfoPtr->serverType;
				sapInfo->hops = respInfoPtr->hops;

				ipxHandle->errno = 0;
				return TRUE;
			}
		}
	}

	ipxHandle->errno = 0;
	return FALSE;
}


void
IPXGetSocketFromAddress(
	IPXAddress_ta ipxAddress,
	uint16		 *socket)
{
	TDRGetSocketFromIPXAddress( ipxAddress, socket );
}


char *
IPXDisplayAddress(
	IPXAddress_ta ipxAddress)
{
	int    i;
	uint8 *sp;
	char  *tp;
	static char dispString[MAX_IPX_ADDRESS_DISPLAY_LENGTH];

	sp = ipxAddress;
	tp = dispString;

	for (i = 0; i < IPX_ADDRESS_NETWORK_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	*tp++ = ':';

	for (i = 0; i < IPX_ADDRESS_NODE_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	*tp++ = ':';

	for (i = 0; i < IPX_ADDRESS_SOCKET_LENGTH; i++, sp++ )
		tp += sprintf( tp, "%02X", *sp & 0xFF );

	return dispString;
}


char *
IPXDisplayErrno(
	IPXHandle_t	*ipxHandle)
{
	char *dispTemplate;
	static char dispString[MAX_ERRNO_DISPLAY_LENGTH];

	if (!ipxHandle->errno) {
		*dispString = '\0';
		return dispString;
	}

	switch (ipxHandle->errnoType) {
		case IPXET_T_ERRNO:
			dispTemplate = "t_errno = %d";
			break;
		case IPXET_TLOOK:
			dispTemplate = "TLOOK = 0x%04X";
			break;
		case IPXET_ERRNO:
			dispTemplate = "errno = %d";
			break;
		default:
			dispTemplate = "error = %d";
			break;
	}

	sprintf( dispString, dispTemplate, ipxHandle->errno );

	return dispString;
}


static void
uppercase(char string[])
{
	char *ptr;

	for (ptr = string; *ptr; ptr++)
		*ptr = toupper( *ptr );
}
