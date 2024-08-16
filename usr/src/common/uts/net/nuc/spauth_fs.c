/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spauth_fs.c	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spauth_fs.c,v 2.53.2.2 1994/12/21 02:50:07 ram Exp $"

/*
 *  Netware Unix Client
 */

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/debug.h>
#include <svc/time.h>

#define NVLT_ModMask	NVLTM_ncp
extern void_t *NCPbroadcastMessageQueue;

/*
 * BEGIN_MANUAL_ENTRY(NCPspGetServerDateAndTime.3k)
 * NAME
 *		NCPspGetServerDateAndTime -	Send the Create Service Connection NCP
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspGetServerDateAndTime_l ( ncp_channel_t*  channel,
 *                                  NCP_DATETIME_T* dateTime ) 
 *
 * INPUT
 *    channel         - Channel structure
 *
 * OUTPUT
 *    NCP_DATETIME_T  - a structure to hold the date and time from the server
 *
 * RETURN VALUES
 *    SUCCESS         - Successful completion.
 *    FAILURE
 *
 * DESCRIPTION
 *    Formats and sends the Get Date and Time NCP request to a server and
 *    return the response
 *
 * NCP
 *    0x2222 20 --
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspGetServerDateAndTime_l (	ncp_channel_t	*channel,
								NCP_DATETIME_T	*dateTime ) 
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	Function;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	year;
		uint8	month;
		uint8	day;
		uint8	hour;
		uint8	minute;
		uint8	second;
		uint8	dayOfWeek;
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (2);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	/*
	 *	Format the packet here
	 */
	NCPdplBuildNCPHeader_l (channel, request);
	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;	
	requestPacket->Function = FNGET_SERVER_DATE_TIME; 
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof (struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	check connection status on every request to make sure
		 *	a window wasn't hit
		 */
		if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
		} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
			ccode = SPI_SERVER_DOWN;
		} else {
			
			/*
			 *	Check the completion code and act accordingly.
			 */
			switch (replyPacket->completionCode) {

				case SUCCESS:
					dateTime->year = replyPacket->year;
					dateTime->month = replyPacket->month;
					dateTime->day = replyPacket->day;
					dateTime->hour = replyPacket->hour;
					dateTime->minute = replyPacket->minute;
					dateTime->second = replyPacket->second;
					dateTime->dayOfWeek = replyPacket->dayOfWeek;
					ccode = SUCCESS;
					break;

				default:
					ccode = SPI_GENERAL_FAILURE;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}
