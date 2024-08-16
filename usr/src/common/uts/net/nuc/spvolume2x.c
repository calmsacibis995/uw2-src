/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spvolume2x.c	1.11"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spvolume2x.c,v 2.52.2.2 1994/12/21 02:52:42 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: spvolume.c
 *	ABSTRACT: NetWare V2.1X NCP's for manipulating Volume objects 
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPsp2XGetVolumeInfo
 *	Private functions:
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

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

/*
 *	Manifest constant for byte/sector returned by servers
 */
#define NCP_BYTES_PER_SECTOR	512


/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XGetVolumeInfo.3k)
 * NAME
 *		NCPsp2XGetVolumeInfo - Get volume statistics on a NetWare
 *							   V2.1X server.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XGetVolumeInfo( channel, volumeNumber, totalBytes, 
 *							freeBytes, totalSlots, freeSlots )
 *		void_t	*channel;
 *		int32	volumeNumber;
 *		uint32	*totalBytes;
 *		uint32	*freeBytes;
 *		uint32	*totalSlots;
 *		uint32	*freeSlots;
 *
 * INPUT
 *		void_t	*channel;
 *		int32	volumeNumber;
 *
 * OUTPUT
 *		uint32	*totalBytes;
 *		uint32	*freeBytes;
 *		uint32	*totalSlots;
 *		uint32	*freeSlots;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_ACCESS_DENIED
 *
 * DESCRIPTION
 *		Query a NetWare V2.X server for information on the volume
 *		denoted by volumeNumber argument.
 *
 * NCP
 *	18
 *
 * NOTES
 *		Converts between sector/cluster and bytes
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XGetVolumeInfo_l (	ncp_channel_t	*channel,
							int32			volumeNumber,
							uint32			*totalBytes,
							uint32			*freeBytes,
							uint32			*totalSlots,
							uint32			*freeSlots )
{
	ccode_t		ccode;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	volumeNumber;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint16	sectorsPerCluster;
		uint16	totalClusters;
		uint16	freeClusters;
		uint16	totalSlots;
		uint16	freeSlots;
		uint8	volumeName[16];
		uint16	removableFlag;
	} *replyPacket;
#pragma pack()

	uint16 spc, tc, fc;

	NVLT_ENTER (6);

	if( NCPdplGetFreePacket_l(channel, &request) ){
		return( NVLT_LEAVE(SPI_CLIENT_RESOURCE_SHORTAGE) );
	}
	if( NCPdplGetFreePacket_l(channel, &reply) ){
		NCPdplFreePacket( request );
		return( NVLT_LEAVE(SPI_CLIENT_RESOURCE_SHORTAGE) );
	}

	NCPdplBuildNCPHeader_l ( channel, request );

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	/*
	 *	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	requestPacket->function = FNGET_VINFO_WITH_NUM;
	requestPacket->volumeNumber = (uint8)volumeNumber;

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l( channel, request, reply );
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		if( replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE ) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if( replyPacket->connectionStatus & CS_BAD_CONNECTION ) {
			ccode = SPI_BAD_CONNECTION;
		} else if( replyPacket->connectionStatus & CS_SERVER_DOWN ) {
			ccode = SPI_SERVER_DOWN;
		}else{
			switch( replyPacket->completionCode ) {
				case SUCCESS:
				/*
				 *	Munge the data into useful numbers
				 */
					spc = GETINT16(replyPacket->sectorsPerCluster);
					tc = GETINT16(replyPacket->totalClusters);
					fc = GETINT16(replyPacket->freeClusters);
					*totalSlots = GETINT16(replyPacket->totalSlots);
					*freeSlots = GETINT16(replyPacket->freeSlots);
					*totalBytes = spc * NCP_BYTES_PER_SECTOR * tc;
					*freeBytes = spc * NCP_BYTES_PER_SECTOR * fc;
					ccode = SUCCESS;
					break;
				default:
					ccode = SPI_ACCESS_DENIED;
					break;
			}
		}
	}

	NCPdplFreePacket( request );	/* done with it, give it back to pool */
	NCPdplFreePacket( reply );		/* done with it, give it back to pool */

	return( NVLT_LEAVE(ccode) );
}
