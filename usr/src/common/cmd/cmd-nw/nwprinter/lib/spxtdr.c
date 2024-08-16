/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/spxtdr.c	1.2"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/spxtdr.c,v 1.3 1994/08/18 16:26:57 vtag Exp $";
#endif

#include "spxapi.h"
#include "spx.h"
#include "spxtdr.h"
#include "nwprint_tdr.h"

void
TDRGetSocketFromSPXAddress(
	SPXAddress_ta spxAddress,
	uint16		 *socket)
{
	uint8 *packetPtr;

	packetPtr = spxAddress + SPX_ADDRESS_NETWORK_LENGTH +
		SPX_ADDRESS_NODE_LENGTH;
	HI_LO_UINT16_TO( *socket );
}


void
TDRSetSocketInSPXAddress(
	SPXAddress_ta spxAddress,
	uint16		  socket)
{
	uint8 *packetPtr;

	packetPtr = spxAddress + SPX_ADDRESS_NETWORK_LENGTH +
		SPX_ADDRESS_NODE_LENGTH;
	HI_LO_UINT16_FROM( socket );
}
 

void
TDRGetTCallOptionsInfo(
	uint8			  spxOpt[],
	SPXOptionsInfo_t *spxOptions)
{
	uint8 *packetPtr;

	packetPtr = spxOpt;

	HI_LO_UINT16_TO( spxOptions->connectionID );
	HI_LO_UINT16_TO( spxOptions->allocationNumber );
}
 

void
TDRGetSPXHeader(
	uint8		 packet[],
	SPXIIHeader_t *spxHeader)
{
	uint8 *packetPtr;

	packetPtr = packet;

	HI_LO_UINT16_TO( spxHeader->checksum );
	HI_LO_UINT16_TO( spxHeader->length );
	COPY_UINT8_TO( spxHeader->transportControl );
	COPY_UINT8_TO( spxHeader->packetType );
	COPY_BYTES_TO( spxHeader->destinationAddress, SPX_ADDRESS_LENGTH,
		SPX_ADDRESS_LENGTH );
	COPY_BYTES_TO( spxHeader->sourceAddress, SPX_ADDRESS_LENGTH,
		SPX_ADDRESS_LENGTH );
	COPY_UINT8_TO( spxHeader->connectionControl );
	COPY_UINT8_TO( spxHeader->dataStreamType );
	HI_LO_UINT16_TO( spxHeader->sourceConnID );
	HI_LO_UINT16_TO( spxHeader->destinationConnID );
	HI_LO_UINT16_TO( spxHeader->sequenceNumber );
	HI_LO_UINT16_TO( spxHeader->acknowledgeNumber );
	HI_LO_UINT16_TO( spxHeader->allocationNumber );
	if (spxHeader->connectionControl & SPX2_CC_FLAG) {
#ifdef HARD_DEBUG_1
		printf("TDRGetSPXHeader SPXII Packet\n");
#endif
		HI_LO_UINT16_TO( spxHeader->negotiationSize );
	}
		
}
