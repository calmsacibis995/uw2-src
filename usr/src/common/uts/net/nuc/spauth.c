/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spauth.c	1.23"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spauth.c,v 2.57.2.10 1995/02/13 07:54:36 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE:   spauth.c
 *	ABSTRACT:   Generic authentication/connection manipulation NCP requests.
 *
 *	NOTE:
 *      On the use of SendPacket and GetPacket: These functions handle the
 *      channel access semaphore.  As a result, if SendPacket completes
 *      successfully, GetPacket MUST be called to free the semaphore. See
 *      NCPdiSendPacket(3k) and NCPdiGetPacket(3k) for details
 *
 *	Functions declared in this module:
 *		NCPspCreateConnection
 *		NCPspDestroyServiceConnection
 *		NCPspNegotiateBufferSize
 *		NCPspMapNameToID
 *		NCpspMapIDtoName ?
 *		NCPspGetServerVersion
 *		NCPspRawNCP
 *		NCPspEndOfTask
 *		NCPspMessageSocketEventHandler
 *		NCPspGetBroadcastMessage
 *		NCPspInitiatePacketBurstConnection
 *
 */ 

#ifdef _KERNEL_HEADERS
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
#include <util/debug.h>
#include <svc/time.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/ncpconst.h>
#include <sys/nucmachine.h>	
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/spilcommon.h>
#include <sys/ncpiopack.h>
#include <sys/nucerror.h>
#include <sys/time.h>

#endif /* ndef _KERNEL_HEADERS */

#define NVLT_ModMask	NVLTM_ncp

extern void_t *NCPbroadcastMessageQueue;
extern sv_t *NCPbroadcastMessageQueueSV;

/*
 * BEGIN_MANUAL_ENTRY(NCPspCreateConnection.3k)
 * NAME
 *    NCPspCreateConnection - Send the Create Service Connection NCP.
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspCreateConnection_l (ncp_channel_t	 *channel ) 
 *
 * INPUT
 *    channel -	Opaque channel structure
 *
 * OUTPUT
 *    Nothing
 *
 * RETURN VALUES
 *    SUCCESS
 *    FAILURE
 *    SPI_CLINET_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *    Formats and sends the Create Service Connection NCP for establishing
 *    service to a server.
 *
 * NCP
 *	1111
 *
 * SEE ALSO
 *		NCPspDestroyConnection(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspCreateConnection_l (ncp_channel_t	 *channel ) 
{
	ccode_t		ccode = SUCCESS;
	uint8		hiByte;
	uint8		lowByte;
	iopacket_t	*request;
	iopacket_t	*reply;

#pragma pack (1)
	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack ()

	NVLT_ENTER (1);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));

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
	request->ncpU.ncpHdr.type = CREATE_A_SERVICE_CONNECTION;
	request->ncpU.ncpHdr.lowByteConnectionNumber = NEW_CONNECTION;

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE+1; /* just normal params */
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			NVLT_PRINTF ("NCPspCreateServiceConnection: "
				"Bad completionCode or PROBLEM_WITH_CONNECTION\n", 0, 0, 0);
			ccode = FAILURE;
		} else {
			lowByte = reply->ncpU.ncpHdr.lowByteConnectionNumber;
			hiByte = reply->ncpU.ncpHdr.highByteConnectionNumber;
			channel->connectionNumber = lowByte | (hiByte<<8); 
			ccode = SUCCESS;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspDestroyConnection.3k)
 * NAME
 *    NCPspDestroyConnection - Inform the server the connection is no longer
 *                             needed.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPspDestroyServiceConnection ( void_t *channel )
 *
 * INPUT
 *    channel	- Opaque pointer to the channel structure.
 *
 * OUTPUT
 *    Nothing
 *
 * RETURN VALUES
 *    SUCCESS
 *    FAILURE
 *
 * DESCRIPTION
 *    Formats and dispatches packets telling the server to terminate the
 *    service for this instance.
 * 
 * NCP
 *    5555
 *
 * SEE ALSO
 *    NCPspCreateServiceConnection(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspDestroyServiceConnection_l ( ncp_channel_t	*channel )
{
	ccode_t			ccode = SUCCESS;
	iopacket_t*		request;
	iopacket_t*		reply;

#pragma pack (1)
	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack ()

	NVLT_ENTER (1);

	/*
	 *	Get a free I/O buffer from the pool, else block until
	 *	one becomes available
	 */
	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = DESTROY_A_SERVICE_CONNECTION;
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + 1; 
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Now setup for receipt 
	 */
	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/* time to stop signing packets */
	if (channel->spiTaskPtr != NULL) {
		if (((SPI_TASK_T *)(channel->spiTaskPtr))->md4 != NULL) {
			kmem_free (((SPI_TASK_T *)(channel->spiTaskPtr))->md4,
					sizeof(md4_t));

			NVLT_PRINTF (
				"NCPspDestroyServiceConnection_l: free md4_t 0x%x",
				((SPI_TASK_T *)(channel->spiTaskPtr))->md4, 0, 0 );

			((SPI_TASK_T *)(channel->spiTaskPtr))->md4 = NULL;
		}
	}

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the connection status
		 *	to make sure the server has free connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspNegotiateBufferSize.3k)
 * NAME
 *    NCPspNegotiateBufferSize - Query the server to determine the maximum
 *                               packet size that can be transferred.  
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspNegotiateBufferSize_l (	void_t  *channel,
 *                                  uint16  *bufferSize )
 *
 * INPUT
 *    channel     - Opaque channel pointer
 *
 * OUTPUT
 *    bufferSize  - Max buffer size in bytes
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *    Pings the server in an attempt at determining the maximum data packet
 *    size allowable over the intervening media.
 *
 * NCP
 *    0x2222  33 --
 *
 * NOTES
 *    The result value returned from this request will be a multiple of 256. 
 *	
 *    This call allows the client to negotiate the buffer size that it will
 *    use when sending reqular file read and write requests to the server.
 *
 *    This particular request is unique from most NCP's in that it has
 *    dependencies upon the transmission media.  The request is used primarily
 *    to allow fragmentation of data read/written to the server during file I/O.
 *    Due to NetWare's current archetecture, and the behavior of the IPX
 *    transport protocol, data fragmentation must be performed by the
 *    application layer instead of the transport.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspNegotiateBufferSize_l (	ncp_channel_t	*channel,
								uint16			*bufferSize )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;
	uint16		workBufferSize;

#pragma pack(1)
	struct reqStruct {
		uint8	function;
		uint8	proposedBufferSize[2];
		uint8	securityFlag;
	} *requestStruct;

	struct repStruct {
		uint8	compCode;
		uint8	connStatus;
		uint16	buffSize;
	} *replyStruct;
#pragma pack()

	NVLT_ENTER (2);

	/*
	 *	Grab a free packet from the pool
	 */
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

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqStruct);

	/*
	 *	Fill out the request
	 */
	requestStruct = (struct reqStruct *)request->ncpU.ncpHdr.data;
	requestStruct->function = FNNEGOTIATE_BUFFER_SIZE;
    workBufferSize = GETINT16 (MAX_NEGOTIATED_BUFFER_SIZE);
	bcopy ((caddr_t)&workBufferSize, (caddr_t)requestStruct->proposedBufferSize,
			  sizeof(uint16));
	requestStruct->securityFlag = 0x80;		/* This bit disables LIP	*/

	/*
	 *	This request does not utilize a second fragment
	 */
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repStruct);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Block while we wait for the packet to be returned from the
	 *	server
	 */
	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyStruct = (struct repStruct *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyStruct->compCode != SUCCESS) ||
				(replyStruct->connStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		} else {
			/*
			 *	perform a 16 bit byte swap if necessary because
			 *	the data comes across in HIGH/LOW (Motorola) byte
			 *	order (at least in this call)
			 */
			*bufferSize = GETINT16 (replyStruct->buffSize);
			if (*bufferSize > MAX_NEGOTIATED_BUFFER_SIZE)
				*bufferSize = MAX_NEGOTIATED_BUFFER_SIZE;
			ccode = SUCCESS;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * NCP 
 *    97
 */
/*
 * BEGIN_MANUAL_ENTRY(NCPspMaxPacketSize_l.3k)
 * NAME
 *    NCPspMaxPacketSize_l - 
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspMaxPacketSize_l ( void_t  *channel,
 *                           uint16  *packetSize,
 *                           uint8   *securityFlags )
 *
 * INPUT
 *    channel     - Opaque channel pointer
 *
 * NCP
 *    0x2222  97 --
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspMaxPacketSize_l (	void_t*	channel,
						uint16* packetSize,
						uint8* securityFlags )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t	*request;
	iopacket_t	*reply;

#pragma pack(1)
	struct reqStruct {
		uint8	function;
		uint8	proposedPacketSize[2];
		uint8	proposedSecurityFlags;
	} *requestStruct;

	struct repStruct {
		uint8	compCode;
		uint8	connStatus;
		uint8	acceptedPacketSize[2];
		uint8	echoSocket[2];
		uint8	acceptedSecurityFlags;
	} *replyStruct;
#pragma pack()

	uint16	n;

	NVLT_ENTER (3);

	/*
	 *	Grab a free packet from the pool
	 */
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
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqStruct);

	/*
	 *	This request does not utilize a second fragment
	 */
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Fill out the request
	 */
	requestStruct = (struct reqStruct *)request->ncpU.ncpHdr.data;
	requestStruct->function = FNMAX_PACKET_SIZE;

	n = GETINT16 (*(uint16 *)packetSize);

	bcopy ((caddr_t)&n, (caddr_t)requestStruct->proposedPacketSize,
			sizeof(uint16));
	requestStruct->proposedSecurityFlags = *securityFlags;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repStruct);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Block while we wait for the packet to be returned from the server.
	 */
	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyStruct = (struct repStruct *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyStruct->compCode != SUCCESS) ||
				(replyStruct->connStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		} else {
			*packetSize = GETINT16 (*(uint16 *)replyStruct->acceptedPacketSize);
			*securityFlags = replyStruct->acceptedSecurityFlags;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPspGetServerVersion.3k)
 * NAME
 *		NCPspGetServerVersion
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspGetServerVersion_l ( void_t     *hannel,
 *                              version_t  *majorVersion,
 *                              version_t  *minorVersion,
 *                              uint8      *serverName )
 *
 * INPUT
 *    channel
 *
 * OUTPUT
 *    majorVersion
 *    minorVersion
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *    Query the NCP server to determine its version in order to provide switch
 *    information to NCP's that vary from version to version
 *
 * NCP
 *    23 17
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspGetServerVersion_l (	ncp_channel_t	*channel,
							version_t		*majorVersion,
							version_t		*minorVersion,
							uint8			*serverName )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	subFunctionLength[2];
		uint8	subFunction;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		char	serverName[NCP_MAX_OBJECT_NAME_LENGTH];
		uint8	majorVersion;
		uint8	minorVersion;
		uint16	maxConn;
		uint8	connInUse;
		uint8	numVolumes;
		uint8	rev;
		uint8	SFTLevel;
		uint8	TTSLevel;
		uint8	maxConnectUsed;
		uint8	acctVersion;
		uint8	vapVersion;
		uint8	qmsVersion;
		uint8	printVersion;
		uint8	vconsoleVersion;
		uint8	restrictLevel;
		uint8	bridge;
		uint8	reserved[60];
	} *replyPacket;
#pragma pack()

	uint16	subFunctLen;

	NVLT_ENTER (4);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->function = FNGENERAL_SERVICES;
	requestPacket->subFunction = SFGET_SERVER_INFO;
	subFunctLen = PUTINT16 (1);
	bcopy ((caddr_t)&subFunctLen, (caddr_t)requestPacket->subFunctionLength,
			   sizeof(subFunctLen));

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof (struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Now setup for receipt 
	 */
	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the connection status
		 *	to make sure the server has free connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		} else {
			*majorVersion = replyPacket->majorVersion;
			*minorVersion = replyPacket->minorVersion;
			bcopy (replyPacket->serverName, serverName,
					NCP_MAX_OBJECT_NAME_LENGTH);
			ccode = SUCCESS;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPspRawNCP.3k)
 * NAME
 *    NCPspRawNCP - Send a raw NCP request
 *
 * SYNOPSIS
 *
 * INPUT
 *    channel
 *
 * OUTPUT
 *    Packet data received off the wire via the packetBuffer.	
 *
 * RETURN VALUES
 *    SUCCESS
 *    FAILURE
 *
 * DESCRIPTION
 *    Allows pre-formatted NCP request to passed to transport.  Primary purpose
 *    of this routine is for use by API and other services that need
 *    un-registered NCP's.
 *
 * NOTES
 *    hkResFlag and dkResFlag are set to TRUE if the buffers are already kernel
 *    resident, otherwise, they are assumed to be user address space and are
 *    copied in.
 *
 *    Task is a new argument passed in after registering raw requests in order
 *    to protect file system handles and locks from being manipulated by the
 *    API.  Registering raw interface returns a task number that is used on all
 *    requests through the device minor the request was registered to.
 *
 *    The header is copied in because it must be manipulated to reflect the
 *    current state of the connection (connection Number, task, etc...).  The
 *    data buffer is managed by the IPC package layer which will interpret the
 *    memoryType flag to determine whether the data will by bcopy'd or
 *    copyout'd to the caller supplied buffer.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspRawNCP_l (	ncp_channel_t	*channel,
				uint8			*task,
				char			*hdr,
				int32			sHdrLen,
				int32			*rHdrLen,
				int32			hkResFlag,
				char			*data,
				int32			sDataLen,
				int32			*rDataLen,
				int32			dkResFlag )
{
	ccode_t		ccode;
	iopacket_t*	request;
	iopacket_t*	reply;
	char*		ncpHeaderPointer;
	char*		ncpDataPointer;
	struct {
		NUC_IOBUF_T				req_iobuf;
		NUC_IOBUF_T				rep_iobuf;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (10);
	NVLT_PRINTF ("NCPspRawNCP_l: sHdrLen=0x%x, sDataLen=0x%x\n", 
			sHdrLen, sDataLen, 0);

	/*
	 *	Save these off before messing with them so we have pointers
	 *	to copy the data back into
	 */
	ncpHeaderPointer = hdr;
	ncpDataPointer = data;

	if (NCPdplGetFreePacket_l (channel, &request)) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	/*
	 *	Verify that the packet header length is within bounds
	 */
	if (sHdrLen > NCP_HEADER_SIZE) {
		NVLT_PRINTF ("NCPspRawNCP_l: hdr size of %d is too big, max is %d\n", 
			sHdrLen, NCP_HEADER_SIZE, 0);
		NCPdplFreePacket (request);	/* done with it, give it back to pool */
		NCPdplFreePacket (reply);	/* done with it, give it back to pool */

		ccode = SPI_GENERAL_FAILURE;
		goto done;
	}

	/*
	 *	Set the transmission header length 
	 */
	request->ncpHeaderSize = sHdrLen;

	/*
	 *	If header is kernel resident, bcopy it, otherwise copyin from user
	 *	address space.
	 */
	if (hkResFlag) {
		bcopy (hdr, request->ncpU.ncpHdrData, sHdrLen);
	} else {
		/*
		 *	Now, copy in the NCP header's worth of data so the header
		 *	can be built
		 */
		if (copyin (hdr, request->ncpU.ncpHdrData, sHdrLen)) {
			NCPdplFreePacket (request);	/* done with it, give it back to pool */
			NCPdplFreePacket (reply);	/* done with it, give it back to pool */

			ccode = SPI_USER_MEMORY_FAULT;
			goto done;
		}
	}

	/*
	 *	Fill in the particulars here regarding sequence, task and
	 *	connection information
	 */
	NCPdplBuildNCPHeader_l (channel, request);
	request->ncpU.ncpHdr.currentTask = *task;

	/*
	 *	If transmission data length is nothing, null out the
	 *	databuf pointer, otherwise, setup the IOBUF structure 
	 *	for use below
	 */
	if (sDataLen <= 0)
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;
	else {
		request->ncpDataBuffer = &KAlloc->req_iobuf;
		/*
		 *	Memory in this part of the buffer resides in 
		 *	the user address space
		 */
		KAlloc->req_iobuf.buffer = (void_t *)data; 
		KAlloc->req_iobuf.memoryType = dkResFlag;
		KAlloc->req_iobuf.bufferLength = sDataLen;
	}

	/* NVLT_ASSERT (request->ncpU.ncpHdr.type == 0x2222); */

	NVLT_PRINTF("NCPspRawNCP_l: before transaction rHdrLen=0x%x\n",
		*rHdrLen, 0, 0);
	reply->ncpHeaderSize = *rHdrLen; 

	/*
	 *	Tell the transport to copyout directly into
	 *	the user's buffer
	 */
	if (*rDataLen <= 0) {
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;
	} else {
		reply->ncpDataBuffer = &KAlloc->rep_iobuf;
		KAlloc->rep_iobuf.buffer = (void_t *)ncpDataPointer;
		KAlloc->rep_iobuf.bufferLength = *rDataLen;
		KAlloc->rep_iobuf.memoryType = dkResFlag;
	}

	/*
	 *	Wait for the packet to get here
	 */
	NVLT_PRINTF (
		"NCPspRawNCP_l: before transaction req_iobuf.bufferLength=0x%x\n",
		KAlloc->req_iobuf.bufferLength, 0, 0);
	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		switch (reply->ncpU.ncpHdrData[7]) {
			case 1:
				ccode = SPI_BAD_CONNECTION;;
				break;

			case 4:
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
				break;

			case 16:
				ccode = SPI_SERVER_DOWN;
				break;

			case 64:	/*	We have a broadcast message waiting for us	*/
			default:
				break;

	   	}

		if (hkResFlag) {
			bcopy (reply->ncpU.ncpHdrData, ncpHeaderPointer, *rHdrLen);
		} else {
			if (copyout (reply->ncpU.ncpHdrData, ncpHeaderPointer, *rHdrLen))
				ccode = SPI_USER_MEMORY_FAULT;
		}
		if (*rDataLen > 0) {
			*rDataLen = KAlloc->rep_iobuf.bufferLength;
		}
		*rHdrLen = reply->ncpHeaderSize;
		NVLT_PRINTF ("NCPspRawNCP_l: after transaction rHdrLen=0x%x\n",
			*rHdrLen, 0, 0);
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPspEndOfTask.3k)
 * NAME
 *    NCPspEndOfTask - Send the End of task NCP	
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspEndOfTask_l ( void_t  *channel,
 *                       uint8   *task )
 *
 * INPUT
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *    SUCCESS
 *    FAILURE
 *
 * DESCRIPTION
 *    Formats and sends the Create Service Connection NCP for establishing
 *    service to a server.
 *
 * NCP
 *    24
 *
 * NOTES
 *    Used primarily by Raw NCP calls when relinquishing a task number
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspEndOfTask_l (	ncp_channel_t	*channel,
					uint8			*task )
{
	ccode_t		ccode;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
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

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;	
	requestPacket->function = FNEND_OF_JOB; 
	request->ncpU.ncpHdr.currentTask = *task;

	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;
	request->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	/*
	 *	Wait for the packet to get here
	 */
	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspMessageSocketEventHandler.3k)
 * NAME
 *    NCPspMessageSocketEventHandler - called when a packet is received on
 *                                     a message socket.
 *
 * SYNOPSIS
 *    void_t
 *    NCPspMessageSocketEventHandler ( void_t   *gtsHandle,
 *                                     void_t   *taskPtr,
 *                                     uint32   eventType )
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *    When called by the IPX Async Event Handler, moves the passed taskPtr to
 *    NCPbroadcastMessageQueue and wakes up any tasks sleeping on
 *    NCPbroadcastMessageQueue
 *
 * NOTES
 *    This function registered as an asynchronous event handler by
 *    NCPdplAllocChannel when the ncp_task_t/ncp_channel_t structures are
 *    created and unregistered by NCPdplFreeChannel.
 *
 * SEE ALSO
 *    NCPdplAllocChannel, NCPdplFreeChannel, NCPsiGetBroadcastMessage,
 *    NCPspGetBroadcastMessage
 *
 * END_MANUAL_ENTRY
 */
void_t
NCPspMessageSocketEventHandler (	void_t		*gtsHandle,
									void_t		*taskPtr,
									uint32		eventType )
{
	pl_t			pl;
	ccode_t			ccode = FAILURE;

	extern lock_t	*nucLock;

	/*
	 * TODO:  One message only in the queue?  Failures?
	 *
	 * The LWP registering this callout has held the spiltask embedded in
	 * taskPtr, and the LWP cancelling the callout will release it.  We add
	 * a hold for NCPbroadcastMessageQueue.  The lwp taking it off the
	 * queue will release the queue's hold when it is done with the
	 * spiltask (nwmpGetServiceMessage).  We go through a little dance
	 * here to deal with lock hierarchy problems.
	 */
	if (taskPtr && ((ncp_task_t *)taskPtr)->spilTaskPtr) {
		ccode = NWslSetTaskInUse_l(((ncp_task_t *)taskPtr)->spilTaskPtr);
	}
	pl = LOCK(nucLock, plstr);
	if (NCPbroadcastMessageQueue) {
		UNLOCK(nucLock, pl);
		SV_BROADCAST(NCPbroadcastMessageQueueSV, 0);
		if (ccode == SUCCESS) {
			NWslSetTaskNotInUse_l(((ncp_task_t *)taskPtr)->spilTaskPtr);
		}
	} else if (ccode == SUCCESS) {
		NCPbroadcastMessageQueue = (ncp_task_t *)taskPtr;
		UNLOCK(nucLock, pl);
		SV_BROADCAST(NCPbroadcastMessageQueueSV, 0);
	} else {
		UNLOCK(nucLock, pl);
	}
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspGetBroadcastMessage.3k)
 * NAME
 *    NCPspGetBroadcastMessage - get a broadcast message from the server
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspGetBroadcastMessage_l ( ncp_channel_t*   channel,
 *                                 NWSI_MESSAGE_T*  message ) 
 *
 * INPUT
 *    channel
 *
 * OUTPUT
 *    message;
 *    messageLength;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Returns a broadcast message from the server via NCP 21/1.
 *
 * NCP
 *    21 01
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspGetBroadcastMessage_l (	ncp_channel_t*	channel,
								NWSI_MESSAGE_T*	message ) 
{
	ccode_t		ccode = SUCCESS;
	iopacket_t	*request;
	iopacket_t	*reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunctionStrucLen[2];
		uint8	SubFunctionCode;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;

		uint8	MessageLength;
		uint8	Message[128];
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
	requestPacket->FunctionCode = FNMESSAGE_SERVICES; 
	requestPacket->SubFunctionStrucLen[0] = 0;
	requestPacket->SubFunctionStrucLen[1] = 1;
	requestPacket->SubFunctionCode = SFGET_BROADCAST_MESSAGE;
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		PRINT_REPLY (replyPacket);

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 *
		 *	We do not check for CS_SERVER_DOWN here since the
		 *	server might be sending a broadcast message saying the
		 *	server is down.  The message should be available, however.
		 */
		if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
		}

		if (ccode == SUCCESS) {
			if (replyPacket->MessageLength > 0) {
				NVLT_PRINTF ("NCPspGetBroadcastMessage_l: message Len = %d\n",
						replyPacket->MessageLength, 0, 0);
				NVLT_STRING ((char *)replyPacket->Message);
				if (replyPacket->MessageLength > SPI_MAX_MESSAGE_LENGTH) {
					bcopy (replyPacket->Message, message->messageText, 
							SPI_MAX_MESSAGE_LENGTH-1);
					message->messageLength = SPI_MAX_MESSAGE_LENGTH;
					message->messageText[SPI_MAX_MESSAGE_LENGTH] = '\0';
				} else {
					bcopy (replyPacket->Message, message->messageText, 
							replyPacket->MessageLength);
					message->messageLength = replyPacket->MessageLength;
					message->messageText[message->messageLength] = '\0';
				}
			} else {
				ccode = NWD_GTS_NO_MESSAGE;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPspInitiatePacketBurstConnection.3k)
 * NAME
 *    NCPspInitiatePacketBurstConnection - sets up a packet burst session on
 *                                         this connection.
 *
 * SYNOPSIS
 *    ccode_t
 *    NCPspInitiatePacketBurstConnection_l ( ncp_channel_t* channel )
 *
 * DESCRIPTION
 *    Returns SPI_SUCCESS if the server supports packet burst on this connection
 *    and updates the ncp_channel_t.
 *
 *	packetBurstConnectionRequest - defines the structure of the Packet
 *	Burst request header sent by the workstation.  It is defined as follows:
 *
 *		RequestType - 0x2222
 *
 *		SequenceNumber - last NCP sequence number plus 1.
 *
 *		ConnectionNumberLow - service connection number low byte.
 *
 *		TaskNumber - current task number.
 *
 *		ConnectionNumberHigh - service connection number high byte.
 *
 *		FunctionNumber - 101 decimal.
 *
 *		LocalConnectionNumberID - unique local connection number.
 *
 *		LocalMaxPacketSize - negotiated buffer size (HI-LO).
 *
 *		LocalTargetSocket - from IPX (local socket number, HI-LO).
 *
 *		LocalMaxSendSize - client maximum burst send size (HI-LO).
 *
 *		LocalMaxRecvSize - client maximum burst receive size (HI-LO).
 *
 *
 *	packetBurstConnectionReply - defines the structure of the Packet
 *	Burst request header received by the workstation.  It is defined as follows:
 *
 *		RequestType - 0x3333
 *
 *		SequenceNumber - last NCP sequence number plus 1.
 *
 *		ConnectionNumberLow - service connection number low byte.
 *
 *		TaskNumber - current task number.
 *
 *		ConnectionNumberHigh - service connection number high byte.
 *
 *		CompletionCode - result of the request.
 *
 *		RemoteTargetID - unique remote connection number.
 *
 *		RemoteMaxPacketSize - negotiated buffer size (HI-LO).
 *
 *		RemoteMaxSendSize - server maximum burst send size (HI-LO).
 *
 *		RemoteMaxRecvSize - server maximum burst receive size (HI-LO).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspInitiatePacketBurstConnection_l ( ncp_channel_t* channel )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint32	LocalConnectionID;
		uint32	LocalMaxPacketSize;
		uint16	LocalTargetSocket;
		uint32	LocalMaxSendSize;
		uint32	LocalMaxRecvSize;
	} *requestPacket;
#pragma pack()

#pragma pack(1)
	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint32	RemoteTargetID;
		uint32	RemoteMaxPacketSize;
		uint32	RemoteMaxSendSize;		/* No longer used (3.12 & 4.x) */
		uint32	RemoteMaxRecvSize;		/* No longer used (3.12 & 4.x) */
	} *replyPacket;
#pragma pack()

	struct packetBurstInfoStruct*	burst = NULL;
	extern timestruc_t				hrestime;    /* GMT time here on SVR4 */
	extern int						PacketBurstIPG;
	uint32							packetSize;
	time_t							tempTime;
	uint32							PacketBurstSendSize = 4096;
	uint32							PacketBurstRecvSize = 4096;
	uint32							PacketBurstMaxSendSize = 0x00181000;
	uint32							PacketBurstMaxRecvSize = 0x000c1000;

	NVLT_ENTER (1);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE(SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	channel->burstInfo = (struct packetBurstInfoStruct *)NULL;

	tempTime = hrestime.tv_sec;

	/*
	 *	Format the packet here
	 */
	NCPdplBuildNCPHeader_l (channel, request);
	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;	
	requestPacket->FunctionCode = FNBURST_CONNECTION_REQUEST; 
	requestPacket->LocalConnectionID = PUTINT32 (tempTime);
	requestPacket->LocalMaxPacketSize =
			PUTINT32 (channel->negotiatedBufferSize);
	requestPacket->LocalTargetSocket = PUTINT16 (channel->packetBurstSocket);
	requestPacket->LocalMaxSendSize = PUTINT32 (PacketBurstMaxSendSize);
	requestPacket->LocalMaxRecvSize = PUTINT32 (PacketBurstMaxRecvSize);

    request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_BAD_CONNECTION)) {
			ccode = FAILURE;
		} else {

			/*
			 *	If a portable server doesn't like this request, disable burst
			 */
			packetSize = GETINT32((uint32)(replyPacket->RemoteMaxPacketSize));
			if (packetSize == 0) {
				ccode = FAILURE;
			} else {

				burst = (struct packetBurstInfoStruct *)kmem_alloc ( 
						sizeof (struct packetBurstInfoStruct), KM_NOSLEEP);
				if (burst == (struct packetBurstInfoStruct *)NULL) {
					NVLT_PRINTF("Packet Burst Info struct allocation failure\n",
							0, 0, 0);
					ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
				} else {
					channel->burstInfo = burst;
					burst->localConnectionID = tempTime;
					burst->localTargetSocket = channel->packetBurstSocket;
					burst->localPacketSequenceNumber = 0;
					burst->burstSequenceNumber = 0;
					burst->ackSequenceNumber = 0;
					burst->localRecvIPG = PacketBurstIPG;
					burst->localSendIPG = PacketBurstIPG * 100;
					burst->minSendIPG = PacketBurstIPG * 100;
					burst->remoteTargetID = 
							GETINT32 (replyPacket->RemoteTargetID);
					burst->remoteMaxPacketSize =
							GETINT32 (replyPacket->RemoteMaxPacketSize);
					if (burst->remoteMaxPacketSize >
							channel->negotiatedBufferSize) {
						burst->maxPacketSize = channel->negotiatedBufferSize;
					} else {
						burst->maxPacketSize = burst->remoteMaxPacketSize;
					}

					/*	maxFragmentLength = maxPacketSize - 
					 *			(sizeof(IPXHeader) + sizeof(PakcetBurstHeader));
					 */
					burst->maxFragmentLength = burst->maxPacketSize -
						(48 + sizeof(PacketBurstHeader_T));

					burst->rawSendSize = PacketBurstSendSize;
					burst->currentSendSize = (burst->rawSendSize /
						burst->maxFragmentLength) * burst->maxFragmentLength;
					burst->rawRecvSize = PacketBurstRecvSize;
					burst->currentRecvSize = (burst->rawRecvSize /
						burst->maxFragmentLength) * burst->maxFragmentLength;

					burst->minWindowSize = burst->maxFragmentLength * 2;
				}
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * NCP
 *    23 29
 */
ccode_t
NCPspLicenseConnection_l (	ncp_channel_t	*channel,
							uint32			flags ) 
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	Function;
		uint8	SubFunctionStrucLen[2];
		uint8	SubFunctionCode;
		uint32	licenseFlag;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
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
	requestPacket->Function = FNGENERAL_SERVICES; 
	requestPacket->SubFunctionStrucLen[0] = 0;
	requestPacket->SubFunctionStrucLen[1] = 5;
	requestPacket->SubFunctionCode = SFLICENSE_CONNECTION;
	requestPacket->licenseFlag = 1;
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

ccode_t
NCPspLogout_l ( ncp_channel_t *channel ) 
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
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (1);

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
	requestPacket->Function = FNLOGOUT; 
	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Check completion code for success, and check the 
		 *	connection status to make sure the server has free 
		 *	connection slots and is up.
		 */
		if ((replyPacket->completionCode != SUCCESS) ||
				(replyPacket->connectionStatus & CS_PROBLEM_WITH_CONNECTION)) {
			ccode = FAILURE;
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}
