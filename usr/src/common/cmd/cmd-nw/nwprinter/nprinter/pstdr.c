/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/pstdr.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/pstdr.c,v 1.4 1994/08/18 16:27:17 vtag Exp $";
#endif

#include "spxapi.h"
#include "psapi.h"
#include "nwprint_tdr.h"
#include "pstdr_proto.h"

/*********************************************************************
** BEGIN_MANUAL_ENTRY( TDRRequestRPrinterQuery (PS TDR), \
**                     api/utils/rprinter/pstdr/requestq )
**			A transparent data representation routine for the PSERVER
**			client protocol to build a query packet to request that a
**			specified printer slot be assigned to us
**
** SYNOPSIS
**			uint8 printerNumber;
**			uint8 packet[REQUEST_RPRINTER_QUERY_LENGTH];
**
**			TDRRequestRPrinterQuery( printerNumber, packet );
**
** INPUT
**			printerNumber		the number of the printer slot
**								desired on the PSERVER
**
** OUTPUT
**			packet				receives the exact byte format
**								of what must be sent on the wire
**								for this query
**
** DESCRIPTION
**			Creates the Request Remote Printer query packet as
**			documented in "NETWARE PRINT SERVER". The format is as
**			follows:
**
**				OFFSET		FIELD				TYPE		VALUE
**				0			command code		uint8		0x81
**				1			printer number		uint8
**
** SEE ALSO
**			TDRRequestRPrinterResponse
**
** END_MANUAL_ENTRY ( 11/10/90 )
**/

void
TDRRequestRPrinterQuery(
	uint8 printerNumber,
	uint8 queryPacket[])
{
	queryPacket[0] = PSRP_REQUEST_REMOTE_PRINTER;
	queryPacket[1] = printerNumber;
}


/*********************************************************************
** BEGIN_MANUAL_ENTRY( TDRRequestRPrinterResponse (PS TDR), \
**                     api/utils/rprinter/pstdr/requestq )
**			A transparent data representation routine for the PSERVER
**			client protocol to unpack a response packet from a
**			request that a specified printer slot be assigned to us
**
** SYNOPSIS
**			uint8 packet[REQUEST_RPRINTER_RESPONSE_LENGTH];
**			int length;
**			int ccode;
**			PSRPrinterInfo_t rprinterInfo;
**
**			TDRRequestRPrinterResponse( packet, length, &ccode,
**				&rprinterInfo );
**
** INPUT
**			packet				the response as received off of
**								the wire
**
**			length				the length of the packet
**
** OUTPUT
**			ccode				receives the completion code
**								from the packet
**
**			rprinterInfo		receives all other information
**								from the packet
**
** DESCRIPTION
**			Unpacks the information from the Request Remote Printer
**			response packet as documented in "NETWARE PRINT SERVER"
**			and puts it into the rprinterInfo structure. All fields
**			are in LOW/HIGH format except the socket number. The
**			format of the packet is as follows:
**
**				OFFSET	FIELD						TYPE	ORDER
**				0		commpletion code			uint16	LH
**				2		printer type				uint16	LH
**				4		use interrupts				uint16	LH
**				6		IRQ number					uint16	LH
**				8		number of 512 byte blocks	uint16	LH
**				10		use XON XOFF				uint16	LH
**				12		baud rate					uint16	LH
**				14		data bits					uint16	LH
**				16		stop bits					uint16	LH
**				18		parity						uint16	LH
**				20		PSERVER/RPRINTER socket		uint16	HL
**
**			If the completion code is not 0, only it is unpacked.
**			Possible non-zero values for the completion code are:
**				PSE_NOT_ENOUGH_MEMORY		0x0301
**				PSE_NO_SUCH_PRINTER			0x0302
**				PSE_ALREADY_IN_USE			0x0308
**				PSE_DOWN					0x030C
**
** SEE ALSO
**			TDRRequestRPrinterQuery
**
** END_MANUAL_ENTRY ( 11/10/90 )
**/

void
TDRRequestRPrinterResponse(
	uint8   responsePacket[],
	int	    length,
	uint16 *ccode,
	PSRPrinterInfo_t *rprinterInfo)
{
	uint8 *packetPtr;

	if (length < sizeof( uint16 )) {
		*ccode = INVALID_RESPONSE_PACKET_LENGTH;
		return;
	}

	packetPtr = responsePacket;
	LO_HI_UINT16_TO( *ccode );
	if (*ccode)
		return;

	if (length < REQUEST_RPRINTER_RESPONSE_LENGTH) {
		*ccode = INVALID_RESPONSE_PACKET_LENGTH;
		return;
	}

	LO_HI_UINT16_TO( rprinterInfo->printerType );
	LO_HI_UINT16_TO( rprinterInfo->useInterrupts );
	LO_HI_UINT16_TO( rprinterInfo->irqNumber );
	LO_HI_UINT16_TO( rprinterInfo->numBlocks );
	LO_HI_UINT16_TO( rprinterInfo->useXonXoff );
	LO_HI_UINT16_TO( rprinterInfo->baudRate );
	LO_HI_UINT16_TO( rprinterInfo->dataBits );
	LO_HI_UINT16_TO( rprinterInfo->stopBits );
	LO_HI_UINT16_TO( rprinterInfo->parity );
	HI_LO_UINT16_TO( rprinterInfo->socket );
}
