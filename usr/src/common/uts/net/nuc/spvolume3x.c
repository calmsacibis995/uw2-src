/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spvolume3x.c	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spvolume3x.c,v 2.51.2.2 1994/12/21 02:52:45 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: spvolume3x.c
 *	ABSTRACT: Netware V3.X NCP requests 
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

/* NCP
 *	18
 *	22 44
 */
ccode_t
NCPsp3XGetVolumeInfo_l(	ncp_channel_t	*channel,
						int32			volumeNumber,
						uint32			*totalBytes,
						uint32			*freeBytes,
						uint32			*totalSlots,
						uint32			*freeSlots )
{
#pragma pack(1)
	struct request_1 {
		uint8	functionCode;
		uint8	volumeNumber;
	} *request_1;

	struct reply_1 {
		uint8	completionCode;
		uint8	connectionStatus;
		uint16	sectorsPerCluster;
		uint16	totalClusters;
		uint16	freeClusters;
		uint16	totalSlots;
		uint16	freeSlots;
		uint8	volumeName[16];
		uint16	removableFlag;
	} *reply_1;

	struct request_2 {
		uint8	functionCode;
		uint16	subFunctionStrucLen;
		uint8	subFunctionCode;
		uint8	volumeNumber;
	} *request_2;

	struct reply_2 {
		uint8	completionCode;
		uint8	connectionStatus;
		uint32	totalBlocks;
		uint32	freeBlocks;
		uint32	purgeableBlocks;
		uint32	notYetPurgeableBlocks;
		uint32	totalDirEntries;
		uint32	availableDirEntries;
		uint8	reserved[4];
		uint8	sectorsPerBlock;
		uint8	volumeNameLength;
		uint8	volumeName[16];
	} *reply_2;
#pragma pack()

	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;
	uint32		totalBlocks, freeBlocks, volumeBlockSize, purgeableBlocks=0;
	uint16		len;

	NVLT_ENTER (6);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	/*
	 *  Two NCPs are needed.
	 *
	 *  The first gets the number of 512 byte sectors
	 *  per block and the second gets available and
	 *  free blocks.  Although the first NCP does return available
	 *  and free blocks, it uses 16 bits values which are not
	 *  enough if the volume is larger than 256 MB.
	 *
	 *  The second NCP doesn't return the volume's block size.
	 *  
	 */

	request_1 = (struct request_1 *)request->ncpU.ncpHdr.data;

	request_1->functionCode = FNGET_VINFO_WITH_NUM;
	request_1->volumeNumber = (uint8)volumeNumber;

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof (struct request_1);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof (struct reply_1);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		reply_1 = (struct reply_1 *)reply->ncpU.ncpHdr.data;

		if( reply_1->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE ) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if (reply_1->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
		} else if (reply_1->connectionStatus & CS_SERVER_DOWN) {
			ccode = SPI_SERVER_DOWN;
		} else {
			switch (reply_1->completionCode) {
				case SUCCESS:
					volumeBlockSize = GETINT16(reply_1->sectorsPerCluster) << 9;
					ccode = SUCCESS;
					break;
				default:
					ccode = SPI_ACCESS_DENIED;
					break;
			}
		}
	}

	if (ccode == SUCCESS) {

		NCPdplBuildNCPHeader_l( channel, request );

		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

		request_2 = (struct request_2 *)request->ncpU.ncpHdr.data;
		request_2->functionCode = FNDIRECTORY_SERVICES;

		len = PUTINT16 (1);
		bcopy (&len, &(request_2->subFunctionStrucLen), sizeof(uint16));

		request_2->subFunctionCode = SFGET_VOL_AND_PURGE_INFO;
		request_2->volumeNumber = (uint8)volumeNumber;

		request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE +
			sizeof(struct request_2);
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof(struct reply_2);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode == SUCCESS) {
			reply_2 = (struct reply_2 *)reply->ncpU.ncpHdr.data;

			if (reply_2->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			} else if (reply_2->connectionStatus & CS_BAD_CONNECTION) {
				ccode = SPI_BAD_CONNECTION;
			} else if (reply_2->connectionStatus & CS_SERVER_DOWN) {
				ccode = SPI_SERVER_DOWN;
			} else {
				switch (reply_2->completionCode) {
					case SUCCESS:
						totalBlocks = REVGETINT32 (reply_2->totalBlocks);
						freeBlocks = REVGETINT32 (reply_2->freeBlocks);
						purgeableBlocks =
								REVGETINT32 (reply_2->purgeableBlocks);
						*totalBytes = totalBlocks * volumeBlockSize;
						*freeBytes = (freeBlocks + purgeableBlocks) *
								volumeBlockSize;
						*totalSlots = REVGETINT32 (reply_2->totalDirEntries);
						*freeSlots = REVGETINT32 (reply_2->availableDirEntries);
						ccode = SUCCESS;
						break;
					default:
						ccode = SPI_ACCESS_DENIED;
						break;
				}
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}
