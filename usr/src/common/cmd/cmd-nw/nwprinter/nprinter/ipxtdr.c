/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/ipxtdr.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/ipxtdr.c,v 1.2 1994/03/25 21:44:03 vtag Exp $";
#endif

#include "ipxapi.h"
#include "ipx.h"
#include "nwprint_tdr.h"
#include "ipxtdr.h"

void
TDRGetSocketFromIPXAddress(
	IPXAddress_ta ipxAddress,
	uint16		 *socket)
{
	uint8 *packetPtr;

	packetPtr = ipxAddress + IPX_ADDRESS_NETWORK_LENGTH +
		IPX_ADDRESS_NODE_LENGTH;
	HI_LO_UINT16_TO( *socket );
}


void
TDRSetSocketInIPXAddress(
	IPXAddress_ta ipxAddress,
	uint16		  socket)
{
	uint8 *packetPtr;

	packetPtr = ipxAddress + IPX_ADDRESS_NETWORK_LENGTH +
		IPX_ADDRESS_NODE_LENGTH;
	HI_LO_UINT16_FROM( socket );
}


void
TDRSetSAPBroadcastInIPXAddress(
	IPXAddress_ta ipxAddress)
{
	int	   i;
	uint8 *packetPtr;

	packetPtr = ipxAddress;

	for (i = 0; i < IPX_ADDRESS_NETWORK_LENGTH; i++)
		COPY_UINT8_FROM( 0x00 );

	for (i = 0; i < IPX_ADDRESS_NODE_LENGTH; i++)
		COPY_UINT8_FROM( 0xFF );

	HI_LO_UINT16_FROM( IPX_SAP_SERVICE_ADVERTISING_SOCKET );
}


void
TDRSAPGeneralServiceQuery(
	uint16	serverType,
	uint8	queryPacket[])
{
	uint8 *packetPtr;

	packetPtr = queryPacket;

	HI_LO_UINT16_FROM( IPX_SAP_GENERAL_SERVICE_QUERY );
	HI_LO_UINT16_FROM( serverType );
}


void
TDRSAPGeneralServiceResponse(
	uint8				responsePacket[],
	int					responseLength,
	IPXsapGSResponse_t *responseInfo)
{
	int	  i;
	uint8 *packetPtr;
	IPXsapServerInfo_t *respInfoPtr;

	packetPtr = responsePacket;

	if (responseLength < sizeof( uint16 ))
		responseInfo->sapType = (uint16)-1;
	else
		HI_LO_UINT16_TO( responseInfo->sapType );

	if (responseInfo->sapType != IPX_SAP_GENERAL_SERVICE_RESPONSE) {
		responseInfo->serverCount = 0;
		return;
	}

	responseInfo->serverCount = (responseLength - 2) /
		IPX_SAP_SERVER_INFO_LENGTH;

	for (i = 0; i < (int)(responseInfo->serverCount); i++) {
		respInfoPtr = &responseInfo->servers[i];

		HI_LO_UINT16_TO( respInfoPtr->serverType );
		COPY_ASCIIZ_TO( respInfoPtr->serverName,
			IPXMAX_SERVER_NAME_LENGTH, IPXMAX_SERVER_NAME_LENGTH );
		COPY_BYTES_TO( respInfoPtr->ipxAddress,
			IPX_ADDRESS_LENGTH, IPX_ADDRESS_LENGTH );
		HI_LO_UINT16_TO( respInfoPtr->hops );
	}
}
