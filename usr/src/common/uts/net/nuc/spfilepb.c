/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spfilepb.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spfilepb.c,v 2.58.2.13 1995/02/13 07:55:41 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: spfilepb.c
 *	ABSTRACT: Packet Burst File manipulation NCP's 
 *
 *	Functions declared in this module:
 *	Public Functions:
 *		NCPspPacketBurstReadFile
 *		NCPspPacketBurstWriteFile
 *	Private Functions:
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/cmn_err.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/spfilepb.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/ipxengine.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>
#include <svc/psm.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

extern struct free_rtn NCPesrInstr;
extern struct free_rtn NCPesr;

/*
 *	ANSI function prototypes
 */
#if defined(__STDC__)
#endif

/*
 *	Request Header for Packet Burst READ:
 *
 *		functionNumber	- 0x00000001 (LO-HI).
 *		fileHandle		- handle returned from the OPEN NCP.
 *		readAheadOffset - Hint for the server (HI-LO).
 *		readAheadLength	- Hint for the server (HI-LO).
 *		dataOffset		- offset into file to begin the read (HI-LO).
 *		dataLength		- number of bytes to read at dataOffset (HI-LO).
 *
 *
 *	Reply Header for Packet Burst READ:
 *		
 *		resultCode		- result of the READ operation (LO-HI).
 *							Possible return values are:
 *								0 - No error
 *								1 - Initial error
 *								2 - I/O error
 *								3 - No data read
 *
 *		bytesRead		- number of bytes actually read (LO-HI).
 *		( data bytes follows bytesRead )
 */
ccode_t
NCPspPacketBurstReadFile_l(	ncp_channel_t*	channel,
							uint8*			fileHandle,
							uint32			offset,
							NUC_IOBUF_T*	buffer )
{
#pragma pack(1)
	struct reqPack {
		struct	packetBurstHeader	header;
		uint32						functionNumber;
		uint32						fileHandle;
		uint32						readAheadOffset;
		uint32						readAheadLength;
		uint32						dataOffset;
		uint32						dataLength;
	} *requestPacket;

	struct repPack {
		struct	packetBurstHeader	header;
		uint32						resultCode;
		uint32						bytesRead;
	} *replyPacket;
#pragma pack()

	ccode_t			ccode = SUCCESS;
	int32			burstLength;
	int32			burstOffset = 0;
	int32			bytesReadByServer;
	int32			eof = FALSE;
	iopacket_t*		request;
	iopacket_t*		reply;
	struct {
		NUC_IOBUF_T		tempBuffer;
		NUC_IOBUF_T		tempBuffer2;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (4);

	/*	Get packet buffers from the pool for this channel
	 */
	if( ccode = NCPdplGetFreePacket_l(channel, &request) ) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}
	if( ccode = NCPdplGetFreePacket_l(channel, &reply) ) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	KAlloc->tempBuffer2.memoryType = IOMEM_KERNEL;
	KAlloc->tempBuffer2.buffer = NULL;
	KAlloc->tempBuffer2.bufferLength = 0;

	KAlloc->tempBuffer.memoryType = buffer->memoryType;
	request->ncpHeaderSize = sizeof(struct reqPack);
	request->ncpDataBuffer = &KAlloc->tempBuffer2;
	reply->ncpHeaderSize = sizeof(struct repPack);
	reply->ncpDataBuffer = &KAlloc->tempBuffer;

	/*	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdrData;
	replyPacket = (struct repPack *)reply->ncpU.ncpHdrData;

	/*	Make as many bursts as necessary to fill all of the
	 *	caller's buffer.
	 */
	while( !eof ){
		/*	If we have completely satisfied the callers request,
		 *	return SUCCESS.
		 */
		if( burstOffset == buffer->bufferLength ){
			ccode = SUCCESS;
			break;
		}
		NVLT_ASSERT( burstOffset < buffer->bufferLength );

		burstLength = buffer->bufferLength - burstOffset;
		if( burstLength > channel->burstInfo->currentRecvSize -
		 (sizeof(struct repPack) - sizeof(struct packetBurstHeader)) ){
			burstLength = channel->burstInfo->currentRecvSize -
				(sizeof(struct repPack) - sizeof(struct packetBurstHeader));
		}
		NVLT_PRINTF( "NCPspPacketBurstReadFile: "
		  "offset = %d length = %d bufferLength = %d.\n",
		  burstOffset, burstLength, buffer->bufferLength );

		requestPacket->functionNumber = PACKET_BURST_READ_REQUEST;
		NCPpbTranslateFileHandle( requestPacket->fileHandle, fileHandle );
		requestPacket->readAheadOffset = PUTINT32( offset );
		requestPacket->readAheadLength = PUTINT32( buffer->bufferLength );
		requestPacket->dataOffset = PUTINT32( (offset + burstOffset) );
		requestPacket->dataLength = PUTINT32( burstLength );

		/*	The NetWare Server will pad the Packet Burst message
		 *	(or transaction) so that the offset into the file
		 *	is aligned on a 32 bit boundary.  This means there
		 *	could be up to 3 bytes of padding at the begining of
		 *	the data in the first packet (read reply).  These
		 *	need to be discarded.
		 */
		reply->ncpHeaderSize = sizeof(struct repPack) +
		  ((offset + burstOffset) & 3);

		KAlloc->tempBuffer.buffer = (uint8*)buffer->buffer + burstOffset;
		KAlloc->tempBuffer.bufferLength = burstLength;

		ccode = NCPdiPacketBurstTransaction( channel, request, reply, 0,
		  buffer->bufferLength );
		if( ccode ){
			break;
		}

		/*	Exit if the read was failed by the server
		 */
		if( GETINT32( replyPacket->resultCode ) ){
			switch (GETINT32( replyPacket->resultCode )) {
				case 0x93000000:
					/*
					 *	Error returned by server when we try to read
					 *	at the end of file
					 */
					buffer->bufferLength = 0;
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					ccode = SPI_ACCESS_DENIED;
					break;
			}
			break;
		}

		/*	If resultCode == 0 and bytesRead < burstLength then
		 *	we have reached the EOF.
		 */
		bytesReadByServer = GETINT32( replyPacket->bytesRead );
		if( bytesReadByServer < burstLength ){
			NVLT_PRINTF("NCPspPacketBurstReadFile: EOF %d bytes read\n",
			  bytesReadByServer, 0, 0 );

			buffer->bufferLength = burstOffset + bytesReadByServer;
			eof = TRUE;
		}

		NVLT_PRINTF("bytesRead = %d, burstTotalLength = %d.\n",
		  bytesReadByServer, burstLength, 0 );

		burstOffset += burstLength;
	}

	NCPdplFreePacket( request );
	NCPdplFreePacket( reply );
done:
	kmem_free(KAlloc, sizeof(*KAlloc));

	return( NVLT_LEAVE(ccode) );
}


/*
 * BEGIN_MANUAL_ENTRY(NCPspPacketBurstWriteFile.3k)
 * NAME
 *		NCPspPacketBurstWriteFile - Write buffer contents to the file
 *						            signified by fileHandle
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPspPacketBurstWriteFile(	ncp_channel_t*	channel,
 *									uint8*			fileHandle,
 *									uint32			offset,
 *									NUC_IOBUF_T*	buffer )
 *
 * INPUT
 *		ncp_channel_t		*channel;
 *		uint8		*fileHandle;
 *		uint32		offset;
 *		void_t		*buffer;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_ACCESS_DENIED
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Writes the contents of the buffer signified by the buffer
 *		argument to the file signified by the fileHandle argument.
 *		The data is written to the file at the offset specfied.
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp2XOpenFile(3k), NCPsp2XReadFile(3k)
 *
 * END_MANUAL_ENTRY
 */
/*	Request Header for Packet Burst WRITE:
 *
 *		functionNumber	- 0x00000002 (LO-HI).
 *		fileHandle		- handle returned from the OPEN NCP.
 *		reserved{1,2}	- reserved fields set to zero.
 *		dataOffset		- offset into file to begin the write (HI-LO).
 *		dataLength		- number of bytes to write at dataOffset (HI-LO).
 *		( data bytes follows dataLength )
 *
 *	Reply Header for Packet Burst WRITE:
 *		
 *		resultCode - result of the WRITE operation (LO-HI).
 *							Possible return values are:
 *								0 - No error
 *								4 - Write error
 */
ccode_t
NCPspPacketBurstWriteFile_l(	ncp_channel_t*	channel,
								uint8*			fileHandle,
								uint32			offset,
								NUC_IOBUF_T*	buffer )
{
#pragma pack(1)
	struct reqPack {
		struct packetBurstHeader	header;
		uint32						functionNumber;
		uint32						fileHandle;
		uint32						reserved1;
		uint32						reserved2;
		uint32						dataOffset;
		uint32						dataLength;
	} *requestPacket;

	struct repPack {
		struct packetBurstHeader	header;
		uint32						resultCode;
	} *replyPacket;
#pragma pack()

	ccode_t			ccode = SUCCESS;
	iopacket_t*		request;
	iopacket_t*		reply;
	uint32			burstOffset = 0;
	uint32			burstLength;
	uint32			resultCode;
	struct {
		NUC_IOBUF_T		tempBuffer;
		NUC_IOBUF_T		tempBuffer2;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (4);

	NVLT_PRINTF( "IPXEngWriteBurst: called to write %d bytes at offset %d\n", 
			buffer->bufferLength, offset, 0 );
	
	/*
	 *	Get packet buffers from the pool for this channel
	 */
	if( ccode = NCPdplGetFreePacket_l(channel, &request) ) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}
	if( ccode = NCPdplGetFreePacket_l(channel, &reply) ) {
		NCPdplFreePacket( request );
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	KAlloc->tempBuffer.memoryType = buffer->memoryType;
	request->ncpHeaderSize = sizeof(struct reqPack);
	request->ncpDataBuffer = &KAlloc->tempBuffer;

	KAlloc->tempBuffer2.memoryType = IOMEM_KERNEL;
	KAlloc->tempBuffer2.buffer = NULL;
	KAlloc->tempBuffer2.bufferLength = 0;

	reply->ncpHeaderSize = sizeof(struct repPack);
	reply->ncpDataBuffer = &KAlloc->tempBuffer2;

	/*	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdrData;
	replyPacket = (struct repPack *)reply->ncpU.ncpHdrData;

	/*
	 *	Initialize variables for burst sequence.
	 */

	/*	Make as many bursts as necessary to write all of the
	 *	caller's buffer.
	 */
	while( burstOffset < buffer->bufferLength ){  /* burst loop */

		burstLength = buffer->bufferLength - burstOffset;
		if( burstLength > channel->burstInfo->currentSendSize -
		  (sizeof(struct reqPack) - sizeof(struct packetBurstHeader)) ){
			burstLength = channel->burstInfo->currentSendSize -
				(sizeof(struct reqPack) - sizeof(struct packetBurstHeader));
		}

		requestPacket->functionNumber = PACKET_BURST_WRITE_REQUEST;
		NCPpbTranslateFileHandle( requestPacket->fileHandle, fileHandle );
		requestPacket->dataOffset = PUTINT32( offset + burstOffset );
		requestPacket->dataLength = PUTINT32( burstLength); 
		requestPacket->reserved1 = PUTINT32( offset );
		requestPacket->reserved2 = PUTINT32( buffer->bufferLength );

		KAlloc->tempBuffer.buffer = (uint8*)buffer->buffer + burstOffset;
		KAlloc->tempBuffer.bufferLength = burstLength;

		NVLT_PRINTF( "IPXEngWriteBurst: burst %d\n",
					channel->burstInfo->burstSequenceNumber, 0, 0 );

		ccode = NCPdiPacketBurstTransaction( channel, request, reply,
		  buffer->bufferLength, 0 );
		if( ccode ){
			break;
		}

		/*	An ack was received.  Check for errors, indicating the 
		 *	volume is full.  Exit if the write failed.
		 */
		resultCode = REVGETINT32( replyPacket->resultCode );
		if( resultCode != SUCCESS ){
			NVLT_PRINTF( "NCPspPacketBurstWriteFile: resultCode=0x%x\n",
					resultCode, 0, 0 );
			switch (resultCode) {
				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				case E_OUT_OF_DISK_SPACE:
					ccode = SPI_OUT_OF_DISK_SPACE;
					break;

				default:
					ccode = SPI_FILE_TOO_BIG;
					break;
			}

			break;
		}

		/*	Adjust burstOffset for next burst.
		 */
		burstOffset += burstLength;
		NVLT_ASSERT( burstOffset <= buffer->bufferLength );

	}	/* end burst write WHILE loop */

	NCPdplFreePacket( request );
	NCPdplFreePacket( reply );
done:
	kmem_free(KAlloc, sizeof(*KAlloc));

	return( NVLT_LEAVE(ccode) );
}


/*
 * BEGIN_MANUAL_ENTRY(NCPdiRemovePBFragmentFromList(3K), \
 *		./man/kernel/ts/ipxeng/NCPdiRemovePBFragmentFromList)
 * NAME
 *	NCPdiRemovePBFragmentFromList -	Remove fragment (offset, length) from
 *									fragment list.
 *
 * SYNOPSIS
 *	ccode_t
 *	NCPdiRemovePBFragmentFromList(fragmentList,fragmentListCount,offset,length)
 *	packetBurstMissingFragmentList	*fragmentList;
 *	uint32			*fragmentListCount;
 *	uint32			offset;
 *	uint32			length;
 *
 * INPUT
 *		fragmentList - a pointer to an array of packetBurstMissingFragmentList
 *			entries.
 *		fragmentListCount - a pointer to the number of fragments in the list.
 *		offset - the offset within the fragment list that is to be removed.
 *		length - the length of the fragment that is to be removed.
 *
 * OUTPUT
 *		fragmentListCount updated to reflect the new number of fragments in
 *		the list.
 *
 * RETURN VALUES
 *		SUCCESS		- Specified fragment was successfully removed from the list.
 *		FAILURE		- Specified fragment is not represented within the list.
 *
 * DESCRIPTION
 *      NCPdiRemovePBFragmentFromList removes from the specified fragment list
 *		the fragment described by offset and length.  It returns SUCCESS if
 *		the fragment was removed.  It returns FAILURE if the fragment doesn't
 *		exist in the fragment list or spans two or more exising fragments.
 *
 * NOTES
 *		This function is called by IPXEngReadFileBurst while receiving
 *		fragments from the server to maintain an up-to-date list of
 *		fragments still outstanding.  It is also called by 
 *		IPXEngWriteFileBurst after receiving a missing fragment list from the
 *		server.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdiRemovePBFragmentFromList(	pb_fragment_t*	f_list,
								uint16*			fragmentCount,
								uint32			offset,
								uint16			length )
{
	uint16 i, j;
	ccode_t	ccode;

	NVLT_ENTER (4);

	NVLT_PRINTF( "fragmentCount = %d, offset = %d, length = %d.\n",
	  *fragmentCount, offset, length );
		
	/*	Make sure a fragment exists. */
	if( *fragmentCount == 0 ){
		return( NVLT_LEAVE(FAILURE) );
	}

	/*	Find a fragment entry that contains some of my fragment
	 */
	for (i = 0; i < *fragmentCount; i++ ) {
		/*	Find a fragment in the list with fragmentOffset within it.
		 */
		if( (offset < f_list[i].f_offset) ||
		  (offset > (f_list[i].f_offset + f_list[i].f_length)) ){
			continue;
		}
		NVLT_PRINTF( "found Frag: Offset = 0x%x, Length = %d\n",
		  f_list[i].f_offset, f_list[i].f_length, 0 );

		if( offset == f_list[i].f_offset ){
			/*	NOTE:  This check is first since it is the most common. */
			/*	The fragment is smaller than the list entry and positioned
			 *	at the begining.  Add the fragment length to the entry's
			 *	offset and subtract the fragment length from the entry's
			 *	length and return SUCCESS.
			 */
			if( (offset + length) < (f_list[i].f_offset + f_list[i].f_length) ){
				NVLT_PRINTF("The received Frag at the begining of found Frag\n",
					0, 0, 0 );
				f_list[i].f_offset += length;
				f_list[i].f_length -= length;
				ccode = SUCCESS;
				NVLT_PRINTF( "New Frag: Offset = 0x%x, Length = %d\n",
				  f_list[i].f_offset, f_list[i].f_length, 0 );
				break;
			}

			/*	If the list entry we found is exactly equal to the fragment,
			 *	delete the list entry and return SUCCESS
			 */
			if( length == f_list[i].f_length ){
				NVLT_PRINTF( "The received Frag == found Frag.\n", 0, 0, 0 );
				ccode = SUCCESS;
				(*fragmentCount) --;
				if( *fragmentCount > 0 ){
					for( ; i < (uint16)(*fragmentCount); i++ ){
						f_list[i] = f_list[i+1];
					}
				}
				break;
			}
		}else{
			/*	The fragment is smaller than the list entry and positioned
			 *	at the end of the list entry.  Subtract the fragment length
			 *	from the list entry's and return SUCCESS.
			 */
			if( (offset + length) == (f_list[i].f_offset + f_list[i].f_length)){
				NVLT_PRINTF( "The received Frag at the end of found Frag.\n",
				  0, 0, 0 );
				f_list[i].f_length -= length;
				ccode = SUCCESS;
				NVLT_PRINTF( "New Frag: Offset = 0x%x, Length = %d\n",
				  f_list[i].f_offset, f_list[i].f_length, 0 );
				break;
			}

			/* The fragment is smaller than the list entry and positioned
			 *	in the middle of the list entry.  Split the list entry in
			 *	two and remove the fragment from them.
			 */
			if( (offset + length) < (f_list[i].f_offset + f_list[i].f_length) ){
				/*	Move the list entries above this one up one to
				 *	make room for the new fragment.
				 */
				for( j=*fragmentCount; j > i; j-- ){
					f_list[j] = f_list[j-1];
				}
				f_list[i+1].f_offset = offset + length;
				f_list[i+1].f_length = f_list[i].f_length - 
					(f_list[i+1].f_offset - f_list[i].f_offset);
				f_list[i].f_length -= (length + f_list[i+1].f_length);
				(*fragmentCount)++;
				ccode = SUCCESS;
				NVLT_PRINTF( "Old Frag: Offset =0x%x, Length = %d\n",
				  f_list[i].f_offset, f_list[i].f_length, 0 );
				NVLT_PRINTF( "New Frag: Offset = 0x%x, Length = %d\n",
				  f_list[i+1].f_offset, f_list[i+1].f_length, 0 );
				break;
			}
		}

		/*
			My fragment spans two list entries (which means a 
			fragment was received that included other fragments already
			received).  This should never happen, so just exit
			this FOR loop.
		*/
		ccode = FAILURE;
		break;
	}

	NVLT_PRINTF( "fragmentCount = %d, offset = %d, length = %d.\n",
	  *fragmentCount, offset, length );

	return( NVLT_LEAVE( ccode ) );

}


ccode_t
NCPdiPacketBurstTransaction(	ncp_channel_t*	channel,
								iopacket_t*		request,
								iopacket_t*		reply,
								uint32			desiredRequestLength,
								uint32			desiredReplyLength )
{
	ccode_t						ccode = SUCCESS;
	boolean_t					resetBufferFlag=FALSE;
	int32						diagnostic;
	struct {
		NUC_IOBUF_T					headerBuffer;
		NUC_IOBUF_T					tempBuffer;
		NUC_IOBUF_T					tempBuffer2;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);
	NUC_IOBUF_T*				oldDataBuff;
	void_t*						transportHandle;
	uint32						msgType;
	uint32						requestLength;
	extern uint8				maxSignatureRetries;
	md4_t*						md4;
	uint32						messageDigest[4];
	uint8*						signature = NULL;

	NVLT_ENTER (5);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));
	NVLT_ASSERT (channel->spiTaskPtr != NULL);

	NWtlPSemaphore (channel->wireSemaphore);
	channel->flags |= PACKET_BURST_TRANSACTION;

	/*	Get the GTS handle from the channel structure.
	 */
	transportHandle = channel->transportHandle;

	/*	We need to make sure that our receive buffers are all kernel
	 *	resident.
	 */
	if( reply->ncpDataBuffer == NULL ){
		KAlloc->tempBuffer.buffer = (opaque_t *)NULL;
		KAlloc->tempBuffer.bufferLength = 0;
		KAlloc->tempBuffer.memoryType = IOMEM_KERNEL;
		oldDataBuff = reply->ncpDataBuffer;
		reply->ncpDataBuffer = &KAlloc->tempBuffer;
		resetBufferFlag = TRUE;
	}else{
		if( reply->ncpDataBuffer->memoryType != IOMEM_KERNEL ){
			KAlloc->tempBuffer.buffer =
			  kmem_alloc( reply->ncpDataBuffer->bufferLength, KM_SLEEP );
			KAlloc->tempBuffer.bufferLength =
							reply->ncpDataBuffer->bufferLength;
			KAlloc->tempBuffer.memoryType = IOMEM_KERNEL;
			oldDataBuff = reply->ncpDataBuffer;
			reply->ncpDataBuffer = &KAlloc->tempBuffer;
			resetBufferFlag = TRUE;
		}
	}
	reply->ncpDataBuffer->esr = &NCPesr;

	if( request->ncpDataBuffer == NULL ){
		request->ncpDataBuffer = &KAlloc->tempBuffer2;
		KAlloc->tempBuffer2.buffer = (opaque_t *)NULL;
		KAlloc->tempBuffer2.bufferLength = 0;
		KAlloc->tempBuffer2.memoryType = IOMEM_KERNEL;
	}

	channel->currRequest = request;
	channel->currReply = reply;

	/*
	 *	Initialize the request fragment list
	 */
	requestLength = (request->ncpHeaderSize - PACKET_BURST_HEADER_SIZE) +
	  request->ncpDataBuffer->bufferLength;

	channel->burstInfo->requestFragmentCount = 1;
	channel->burstInfo->requestFragmentList[0].f_offset = 0;
	channel->burstInfo->requestFragmentList[0].f_length = (uint16)requestLength;

	/*	Initialize the packetReceived flag so the retransmission up call and
	 *	the receive up call will know no packets have been received for this
	 *	burst.
	 */
	channel->burstInfo->receivedCount = 0;
	channel->burstInfo->transmittedCount = 0;
	channel->burstInfo->retransmissionFlag = SUCCESS;

	/*	Get the signature from the channel structure.
	 */
	md4 = ((SPI_TASK_T *)(channel->spiTaskPtr))->md4;
	if( md4 ){
		uint8	block[64];
		uint32	headerLength;
		uint32	dataLength;
		uint32	length;
		uint32	dataStreamOffset = PACKET_BURST_HEADER_SIZE;

		bzero( block, 64 );
		bcopy( md4->sessionKey, block, 8 );

		headerLength = request->ncpHeaderSize - dataStreamOffset;
		dataLength = request->ncpDataBuffer->bufferLength;

		length = GETINT32( headerLength + dataLength );
		bcopy( &length, block + 8, 4 );

		if( headerLength < 52 ){
			bcopy(((uint8*)request->ncpU.ncpHdrData) + dataStreamOffset,
			  block + 12, headerLength );
			if( dataLength != 0 ){
				if( dataLength > (52 - headerLength) ){
					bcopy( request->ncpDataBuffer->buffer,
						block + 12 + headerLength, 52 - headerLength );
				}else{
					bcopy( request->ncpDataBuffer->buffer,
					  block + 12 + headerLength, dataLength );
				}
			}
		}else{
			bcopy(((uint8*)request->ncpU.ncpHdrData) + dataStreamOffset,
			  block + 12, 52);
		}

		bcopy(md4->currentMessageDigest, messageDigest, 16);
		BuildMessageDigest(messageDigest, block);

		bcopy(md4->currentMessageDigest, md4->previousMessageDigest, 16);
		bcopy(&messageDigest[0], md4->currentMessageDigest, 16);
		
		/* Set max tries for bad signatures */
		((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries =
				maxSignatureRetries;

		signature = (uint8 *)(md4->currentMessageDigest);
	}  /*  md4  */

	/*	Send the request
	 */
	ccode = NCPdiSendRequestBurst( channel, request, signature );
	if( ccode ){
		channel->currRequest = NULL;
		channel->flags &= ~PACKET_BURST_TRANSACTION;
		NWtlVSemaphore_l (channel->wireSemaphore);
		goto done;
	}

	/*	Call GTS to load our buffers with the data received
	 */
	GTS_RECEIVE(transportHandle, &KAlloc->headerBuffer, &KAlloc->tempBuffer,
				&msgType, B_TRUE, &diagnostic);

	if (diagnostic) {
		switch (diagnostic) {
			case NWD_GTS_TIMED_OUT:
			case NWD_GTS_NOT_CONNECTED:
				NVLT_PRINTF( "NCPdiPacketBurstTransaction: "
				  "timeout, GTS rc=%x\n", diagnostic, 0, 0 );
				ccode = SPI_SERVER_UNAVAILABLE;
				break;
			default:
				NVLT_PRINTF( "NCPdiPacketBurstTransaction: "
				  "GTS_RECEIVE failed, GTS rc=%x\n", diagnostic, 0, 0 );
				ccode = SPI_GENERAL_FAILURE;
				break;
		}
	}

	if( resetBufferFlag ){
		reply->ncpDataBuffer = oldDataBuff;
		if( oldDataBuff != NULL ){
			copyout ( KAlloc->tempBuffer.buffer, reply->ncpDataBuffer->buffer,
			  reply->ncpDataBuffer->bufferLength );
			kmem_free( KAlloc->tempBuffer.buffer,
					   reply->ncpDataBuffer->bufferLength );
		}
	}

	/*	Set the channel's current request component to NULL
	 *	indicating that there are no loger any pending requests
	 *	on this channel
	 */
	channel->currRequest = NULL;
	channel->currReply = NULL;

	NCPdiAdjustPacketBurstWindow( channel, desiredRequestLength,
	  desiredReplyLength );

	channel->flags &= ~PACKET_BURST_TRANSACTION;
	NWtlVSemaphore_l (channel->wireSemaphore);
done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return( NVLT_LEAVE(ccode) );
}


ccode_t
NCPdiSendRequestBurst(	ncp_channel_t	*channel,
						iopacket_t		*request,
						uint8			*signature )
{
	ccode_t						ccode = SUCCESS;
	int32						diagnostic;
	void_t*						transportHandle;
	uint16						fragmentCount;
	uint16						fragmentLength;
	uint32						fragmentOffset;
	pb_fragment_t*				tmpFragmentList;
	uint32						functionLength;
	PacketBurstHeader_T*		requestHeader;
	psmtime_t					startTime;
	psmtime_t					totalTime;
	dl_t						dl_time;
	uint32						elapsedTime;
	uint8*						tmpSignature;
	uint32						tmpIPG;
	struct {
		NUC_IOBUF_T				headerBuffer;
		NUC_IOBUF_T				tempBuffer;
	} *KAlloc, SAlloc;
	int							didKmemAlloc;

	NVLT_ENTER (3);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));
	NVLT_ASSERT (channel->spiTaskPtr != NULL);

	/*
	 * Allocate memory from the stack only if running at interrupt level.
	 */
	KAlloc = &SAlloc;
	didKmemAlloc = 0;
	if (getpl() == plbase) {
		KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);
		didKmemAlloc = 1;
	}

	/*	Get the GTS handle from the channel structure.
	 */
	transportHandle = channel->transportHandle;

	/*	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestHeader = (PacketBurstHeader_T*)request->ncpU.ncpHdrData;

	/*	Initialize variables for burst sequence.
	 */
	functionLength = request->ncpHeaderSize - PACKET_BURST_HEADER_SIZE;
	KAlloc->headerBuffer.buffer = request->ncpU.ncpHdrData;
	KAlloc->headerBuffer.memoryType = IOMEM_KERNEL;

	/*	Copy the fragment list to use while transmitting the request
	 */
	fragmentCount = channel->burstInfo->requestFragmentCount;
	tmpFragmentList = kmem_alloc( MAX_FRAGMENTS * sizeof (pb_fragment_t),
			KM_NOSLEEP );
	if (tmpFragmentList == NULL) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}
	bcopy( channel->burstInfo->requestFragmentList, tmpFragmentList,
		(fragmentCount * sizeof(pb_fragment_t)) );

	/*	Set up the header information which doesn't change between
	 *	packets.
	 */
	NCPdplBuildPacketBurstHeader( channel, request->ncpU.ncpHdrData );
	if( request->ncpDataBuffer->bufferLength == 0 ){
		requestHeader->sendDelayTime =
		  PUTINT32( channel->burstInfo->localRecvIPG );
	}else{
		/*	It's not necessary to fill this in with newer versions of
		 *	Packet Burst.  It's here just for backwards compatability.
		 */
		tmpIPG = channel->burstInfo->localSendIPG / 100;
		requestHeader->sendDelayTime = PUTINT32 (tmpIPG);
	}
	requestHeader->totalLength = PUTINT32( functionLength +
	  request->ncpDataBuffer->bufferLength );
	NVLT_PRINTF( "NCPdiSendRequestBurst: BurstLength = %d FragCount = %d\n",
				requestHeader->totalLength, fragmentCount, 0);

	for (;;) {

		psm_time_get( &startTime );

		fragmentOffset = tmpFragmentList[0].f_offset;
		fragmentLength = min( channel->burstInfo->maxFragmentLength,
			tmpFragmentList[0].f_length );
		/*	We need to leave room in the packet for the signature.
		 */
		if( fragmentOffset == 0 && signature != NULL ){
			if( (fragmentLength + 8) > channel->burstInfo->maxFragmentLength ){
				fragmentLength -= 8;  /*  signature length = 8  */
			}
		}

		++((channel->burstInfo)->localPacketSequenceNumber);
		requestHeader->packetSequenceNumber =
		  PUTINT32(channel->burstInfo->localPacketSequenceNumber );

		requestHeader->dataOffset = PUTINT32( fragmentOffset );
		requestHeader->dataLength = PUTINT16( fragmentLength );

		if( fragmentCount == 0 ){
			requestHeader->connectionControl |= END_OF_MESSAGE_BIT;
		}

		/*	Set up the buffers to be transmitted.
		 */
		if( fragmentOffset == 0 ){
			/*	The first fragment in the burst includes the request and
			 *	is the only part of the request burst which is signed
			 *	when NCP signatures are used.
			 */
			tmpSignature = signature;

			KAlloc->tempBuffer.buffer = (uint8*)request->ncpDataBuffer->buffer;
			KAlloc->tempBuffer.bufferLength = fragmentLength - functionLength;
			KAlloc->headerBuffer.bufferLength = request->ncpHeaderSize;
		}else{
			KAlloc->tempBuffer.buffer = (uint8*)request->ncpDataBuffer->buffer +
			  fragmentOffset - functionLength;
			KAlloc->tempBuffer.bufferLength = fragmentLength;
			KAlloc->headerBuffer.bufferLength = PACKET_BURST_HEADER_SIZE;

			tmpSignature = NULL;
		}
		KAlloc->tempBuffer.memoryType = request->ncpDataBuffer->memoryType;
		KAlloc->headerBuffer.esr = &(channel->freeBufferStruct);
		KAlloc->tempBuffer.esr = &NCPesr;

		NVLT_PRINTF( "NCPdiSendRequestBurst: "
		  "packet = %d, offset = %d, length = %d.\n",
		  (channel->burstInfo)->localPacketSequenceNumber,
		  fragmentOffset, fragmentLength );

		/*	Pass the buffers on to GTS for processing and 
		 *	eventual transmission on the wire
		 */
		GTS_SEND( transportHandle, &KAlloc->headerBuffer, &KAlloc->tempBuffer,
		  ((SPI_TASK_T *)(channel->spiTaskPtr))->ipxchecksum, GTS_IN_BAND,
		  &diagnostic, tmpSignature);


		if( diagnostic ){
			switch( diagnostic ){
				case NWD_GTS_BUF_FAULT:
					NVLT_PRINTF(
					  "NCPdpPacketBurstTransaction: Bad buffer=%x\n", 
					  request->ncpDataBuffer, 0, 0 );
					ccode = SPI_USER_MEMORY_FAULT;
					break;

				case NWD_GTS_TIMED_OUT:
				case NWD_GTS_NOT_CONNECTED:
					NVLT_PRINTF( "NCPdpSendPacket; Timeout detected\n",
						0, 0, 0 );
					ccode = SPI_SERVER_UNAVAILABLE;
					break;

				default:
					NVLT_PRINTF(
					  "NCPdiSendpacket; Error %x returned from GTS\n",
					  diagnostic, 0, 0 );
					ccode = SPI_GENERAL_FAILURE;
					break;
			}
			break;
		} else {
			NWtlPSemaphore( channel->freeBufferSemaphore );
		}

		ccode = NCPdiRemovePBFragmentFromList( tmpFragmentList,
			&fragmentCount, fragmentOffset, fragmentLength );

		++channel->burstInfo->transmittedCount;

		if( fragmentCount <= 0 ){
			break;
		}

		psm_time_get( &totalTime );
		psm_time_sub( &totalTime, &startTime );
		psm_time_cvt( &dl_time, &totalTime );
		if( dl_time.dl_hop > 0 ){
			continue;
		}

		elapsedTime = dl_time.dl_lop;

		NVLT_PRINTF( "NCPdiSendRequestBurst: send time=%d\n", elapsedTime,0,0 );

		if( elapsedTime < channel->burstInfo->minSendIPG ||
		  (channel->burstInfo->minSendIPG == 0) ){
			channel->burstInfo->minSendIPG = elapsedTime;
		}
			
		if( (elapsedTime + 40) < channel->burstInfo->localSendIPG ){
			YieldWithDelay_usec(channel->burstInfo->localSendIPG - elapsedTime);
		}

	}  /*  fragmentCount > 0  */

	kmem_free( tmpFragmentList, (MAX_FRAGMENTS * sizeof (pb_fragment_t)) );
done:
	if (didKmemAlloc)
		kmem_free(KAlloc, sizeof(*KAlloc));

	return( NVLT_LEAVE(ccode) );
}


void
YieldWithDelay_usec ( uint32 delayTime )
{
	uint32		elapsedTime = 0;
	psmtime_t	startTime;
	psmtime_t	ptime2;
	dl_t		dl_time;

	NVLT_ENTER (1);

	psm_time_get( &startTime );
	while( elapsedTime < delayTime ){
		yield();
		psm_time_get( &ptime2 );
		psm_time_sub( &ptime2, &startTime );
		psm_time_cvt( &dl_time, &ptime2 );

		if( dl_time.dl_hop > 0 ){
			break;
		}else{
			elapsedTime = dl_time.dl_lop;
		}
	}
	NVLT_LEAVE( SUCCESS );
	return;
}

ccode_t
NCPdiPacketBurstReceiveUpCall(	ncp_channel_t*	channel,
								NUC_IOBUF_T*	previewBuf,
								NUC_IOBUF_T*	frag1,
								NUC_IOBUF_T*	frag2,
								opaque_t*		ipcChannel )
{
#pragma pack(1)
	struct replyPack {
		struct packetBurstHeader	header;
		pb_fragment_t				fragmentList[MAX_FRAGMENTS];
	} *replyPacket;
#pragma pack()

	ccode_t			ccode = FAILURE;
	uint32			burstNumber;
	iopacket_t*		reply;
	uint16			fragmentCount;
	uint32			fragmentOffset;
	uint16			fragmentLength;

	NVLT_ENTER (5);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));
	NVLT_ASSERT (channel->spiTaskPtr != NULL);

	replyPacket = (struct replyPack*)previewBuf->buffer;

	if( channel->burstInfo == NULL ){
		NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: no Info struct\n", 0, 0, 0 );
		return( NVLT_LEAVE(FAILURE) );
	}

	/*	Make sure this is a Packet Burst packet.
	 */
	if ( replyPacket->header.packetType != 0x7777) {
		NVLT_PRINTF("NCPdiPacketBurstReceiveUpCall:Not a packet burst response\n",
		  0, 0, 0 );
		return( NVLT_LEAVE(FAILURE) );
	} 

	/*
	 *	Validate signature if needed.
	 */
	NVLT_PRINTF ("NCPdiPacketBurstReceiveUpCall: connectionControl = 0x%x\n",
					replyPacket->header.connectionControl, 0, 0);
	if( ((SPI_TASK_T *)(channel->spiTaskPtr))->md4 &&
	  !(replyPacket->header.connectionControl & SYSTEM_PACKET_BIT) &&
	  (GETINT32(replyPacket->header.dataOffset) == 0)) {

		uint32 length;

		length = replyPacket->header.totalLength;

		if (!NWstrValidateSignature(ipcChannel,
				((SPI_TASK_T *)(channel->spiTaskPtr))->md4, 36, length)) {
			((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries -= 1;
			NVLT_PRINTF(
				"NCPdiPacketBurstReceiveUpCall: bad signature, retries=%d\n",
				((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries,
				0, 0 );
			if( ((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries
					== 0 ) {
				NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: "
				  "Received max number of bad signatures.\n", 0, 0, 0 );
			}
			return( NVLT_LEAVE(FAILURE) );
		}
	}

	/*	If the system bit is on and the fragment list is empty 
	 *	this is a busy indication packet.  We'll just drop the
	 *	packet and reset the timer.
	 */
	if( replyPacket->header.connectionControl & SYSTEM_PACKET_BIT ){
		fragmentCount = GETINT16( replyPacket->header.fragmentCount );
		if( fragmentCount == 0 ){
			NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: burst BUSY received\n",
				0, 0, 0 );
			return( NVLT_LEAVE(NWD_GTS_BLOCKING) );
		}else{
			channel->burstInfo->requestFragmentCount = fragmentCount;
			GETBE_FRAGMENT_LIST( channel->burstInfo->requestFragmentList,
				replyPacket->fragmentList, fragmentCount );
			return( NVLT_LEAVE(NWD_GTS_REPLY_TIMED_OUT) );
		}
	}

	/*	
	 *	Verify that the ack is for the current burst
	 */
	burstNumber = GETINT16( replyPacket->header.burstSequenceNumber );
	if (channel->burstInfo->burstSequenceNumber != burstNumber) {
		if (channel->burstInfo->burstSequenceNumber == (burstNumber + 1) &&
			(replyPacket->header.connectionControl & END_OF_MESSAGE_BIT)) {
			/*	Server is finally acknowledging the burst we sent out
			 *	before. However, we have started the retransmissin of the
			 *	entire burst. If we throw this packet away, we can never mark
			 *	our taskPtr to ~IPX_TASK_BURST and any NCP replies from
			 *	now on will be discarded since taskPtr->state==IPX_TASK_BURST
			 *	and eventually, we will set taskPtr->state to IPX_TASK_TIMEDOUT
			 *	and it will appear to our service consumers that the server
			 *	went away, eventhough it still alive  -- Ram M
			 *
			 *	The IPX_TASK_BURST and IPX_TASK_TRANSMIT are turned off in the
			 *	IPXEngGetPacket after IPXEngInterruptHandler is done.
			 */
			NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: "
				"We got the ack for this burst with this EOM reply: "
				"got %x expected %x - flushing\n",
				burstNumber, channel->burstInfo->burstSequenceNumber, 0 );
			return( NVLT_LEAVE(FAILURE) );
		} else {
			NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: "
				"got %x expected %x - flushing\n",
				burstNumber, channel->burstInfo->burstSequenceNumber, 0 );
			return( NVLT_LEAVE(FAILURE) );
		}
	}

	/*	The fragment list for the reply is not initialized until we receive
	 *	the first packet because the server may not send us as much data as
	 *	we requested.
	 *
	 *	NOTE:	channel->burstInfo->packetReceived is also used to
	 *			determine state for retransmissions.
	 */
	if( channel->burstInfo->receivedCount == 0 ){
		channel->burstInfo->replyFragmentCount = 1;
		channel->burstInfo->replyFragmentList[0].f_offset = 0;
		channel->burstInfo->replyFragmentList[0].f_length =
		  GETINT32( replyPacket->header.totalLength );
		NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: "
		  "Initializing Frag List, f_length = %d.\n",
		  channel->burstInfo->replyFragmentList[0].f_length,0,0 );
	}

	fragmentOffset = GETINT32( replyPacket->header.dataOffset );
	fragmentLength = GETINT16( replyPacket->header.dataLength );

	/*	Check to make sure this is a fragment we requested.
	 */
	ccode = NCPdiRemovePBFragmentFromList(channel->burstInfo->replyFragmentList,
	  &channel->burstInfo->replyFragmentCount, fragmentOffset, fragmentLength );
	if (ccode != SUCCESS) {
		NVLT_PRINTF( "NCPdiPacketBurstReceiveUpCall: Duplicate packet received\n",
		  0, 0, 0 );
		return( NVLT_LEAVE(FAILURE) );
	}

	reply = (iopacket_t *)channel->currReply;
	if( reply == NULL ){
		return( NVLT_LEAVE(FAILURE) );
	}

	++channel->burstInfo->receivedCount;

	if( fragmentOffset == 0 ){
		frag1->buffer = reply->ncpU.ncpHdrData;
		frag1->bufferLength = reply->ncpHeaderSize;
		frag1->memoryType = IOMEM_KERNEL;

		frag2->buffer = reply->ncpDataBuffer->buffer;
		frag2->bufferLength = fragmentLength -
		  (reply->ncpHeaderSize - PACKET_BURST_HEADER_SIZE);
		frag2->memoryType = reply->ncpDataBuffer->memoryType;
	}else{
		frag1->buffer = reply->ncpU.ncpHdrData;
		frag1->bufferLength = PACKET_BURST_HEADER_SIZE;
		frag1->memoryType = IOMEM_KERNEL;

		frag2->buffer = ((uint8*)reply->ncpDataBuffer->buffer) +
		  (fragmentOffset - (reply->ncpHeaderSize - PACKET_BURST_HEADER_SIZE));
		frag2->bufferLength = fragmentLength;
		frag2->memoryType = reply->ncpDataBuffer->memoryType;
	}
	frag1->esr = &NCPesrInstr;
	frag2->esr = &NCPesr;

abort_retransmission:
	/* NVLT_ASSERT( channel->burstInfo->replyFragmentCount < 0 ); */

	/*	If we have received all of the reply fragments
	 */
	if( channel->burstInfo->replyFragmentCount == 0 ){
		/*	Set the burst and ack numbers for the next burst cycle.
		 *	We are suppose to set them to the same
		 *	value.
		 */
		channel->burstInfo->burstSequenceNumber =
		  GETINT16( replyPacket->header.ackSequenceNumber );
		channel->burstInfo->ackSequenceNumber =
		  channel->burstInfo->burstSequenceNumber;

		return( NVLT_LEAVE(NWD_GTS_PB_ALL_FRAGS_RECEIVED) );
	}

	return( NVLT_LEAVE(SUCCESS) );
}


ccode_t
NCPdiPacketBurstRetransmissionUpCall ( ncp_channel_t* channel )
{
	ccode_t			ccode = SUCCESS;
	iopacket_t*		request;

#pragma pack(1)
	struct ackPack {
		struct packetBurstHeader	header;
		pb_fragment_t				fragmentList[MAX_FRAGMENTS];
	} *ackPacket;
#pragma pack()

#ifdef OLD
	uint16			lengthOfFragments;
#endif OLD
	int32			diagnostic;
	NUC_IOBUF_T		headerBuffer;
	NUC_IOBUF_T		tempBuffer;
	void_t*			transportHandle;
	md4_t*			md4;
	uint8*			signature = NULL;

	NVLT_ENTER (1);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));
	NVLT_ASSERT (channel->spiTaskPtr != NULL);

	/*	If no packets have been received just resend the
	 *	original packet.  If packets have been received,
	 *	send a system packet with a missing fragments list.
	 */
	if( channel->burstInfo->receivedCount ){

#ifdef OLD
		lengthOfFragments = channel->burstInfo->replyFragmentCount *
		  sizeof(pb_fragment_t);
#endif OLD

		channel->burstInfo->retransmissionFlag |= REPLY_RETRANSMISSION;

		ackPacket = (struct ackPack*) kmem_alloc( NCP_HEADER_SIZE, KM_SLEEP );

		NCPdplBuildPacketBurstHeader( channel, ackPacket );

		++channel->burstInfo->localPacketSequenceNumber;
		ackPacket->header.packetSequenceNumber =
		  PUTINT32(channel->burstInfo->localPacketSequenceNumber );
		ackPacket->header.sendDelayTime =
		  PUTINT32( channel->burstInfo->localRecvIPG );
		ackPacket->header.connectionControl = SYSTEM_PACKET_BIT;
#ifdef OLD
		ackPacket->header.totalLength = PUTINT32( lengthOfFragments );
		ackPacket->header.dataOffset = PUTINT32( 0 );
		ackPacket->header.dataLength = PUTINT16( lengthOfFragments );
#else
		ackPacket->header.totalLength = 0;
		ackPacket->header.dataOffset = 0;
		ackPacket->header.dataLength = 0;
#endif OLD

		ackPacket->header.fragmentCount =
		  PUTINT16( channel->burstInfo->replyFragmentCount );
		MOVEBE_FRAGMENT_LIST( ackPacket->fragmentList,
		  channel->burstInfo->replyFragmentList,
		  channel->burstInfo->replyFragmentCount );

		headerBuffer.buffer = ackPacket;
#ifdef OLD
		headerBuffer.bufferLength = sizeof(struct packetBurstHeader) +
		  lengthOfFragments;
#else
		headerBuffer.bufferLength = sizeof(struct packetBurstHeader) +
		  (channel->burstInfo->replyFragmentCount * sizeof(pb_fragment_t));
#endif OLD
		headerBuffer.memoryType = IOMEM_KERNEL;
		headerBuffer.esr = &(channel->freeBufferStruct);

		tempBuffer.buffer = NULL;
		tempBuffer.bufferLength = 0;
		tempBuffer.memoryType = IOMEM_KERNEL;
		tempBuffer.esr = &NCPesr;

		/*	Get the GTS handle from the channel structure.
		 */
		transportHandle = channel->transportHandle;

		GTS_SEND( transportHandle, &headerBuffer, &tempBuffer,
				((SPI_TASK_T *)(channel->spiTaskPtr))->ipxchecksum,
				GTS_IN_BAND, &diagnostic, NULL);

		ccode = diagnostic;

		NWtlPSemaphore( channel->freeBufferSemaphore );
		kmem_free( ackPacket, NCP_HEADER_SIZE );
	}else{

		channel->burstInfo->retransmissionFlag |= REQUEST_RETRANSMISSION;

		request = channel->currRequest;
		if( request == NULL ){
			return( NVLT_LEAVE(FAILURE) );
		}

		/*
		 *	Get the signature from the channel structure.
		 */
		md4 = ((SPI_TASK_T *)(channel->spiTaskPtr))->md4;
		if( md4 ){
			signature = (uint8 *)(md4->currentMessageDigest);
		}

		NVLT_PRINTF( "Retransmitting burst %d.\n", 
					channel->burstInfo->burstSequenceNumber, 0, 0 );
		ccode = NCPdiSendRequestBurst( channel, request, signature );
	}

	return( NVLT_LEAVE(ccode) );
}


ccode_t
NCPdiAdjustPacketBurstWindow(	ncp_channel_t*	channel,
								uint32			requestLength,
								uint32			replyLength )
{
	uint32							min_ipg = 0; /* in 100 usec */
	uint32							ipx_rtt;
	uint32							ipg_d;
	struct packetBurstInfoStruct*	b;
	uint32							tmp;

	NVLT_ENTER (3);

	b = channel->burstInfo;

	ipx_rtt = (((ipxTask_t*)channel->transportHandle)->smoothedRoundTrip >> 3);

	/*	If two consecutive bursts were unsuccessful
	 *	adjust the window size.
	 * 
	 *	If the burst was successful, add 1/8th of rawSendSize to its size, 
	 *	otherwise decrease the burst size to 7/8th of rawSendSize.
	 */
	switch( b->retransmissionFlag ){
		case SUCCESS:
			b->consecutiveRetransmissionCount = 0;
			if( b->transmittedCount > 1 ){
				if( b->currentSendSize < requestLength ){
					b->rawSendSize += b->rawSendSize >> 3;
					tmp = b->rawSendSize;
					if( tmp < 0x10000 ){
						b->rawSendSize = (uint16)tmp;
					}else{
						b->rawSendSize = 0xffff;
					}
					if( b->rawSendSize >
					  (b->currentSendSize + b->maxFragmentLength) ){
						b->currentSendSize += b->maxFragmentLength;
					}
				}else{
					if( b->localSendIPG > b->minSendIPG ){
						ipg_d = b->localSendIPG >> 3;
						if( ipg_d == 0 ){
							ipg_d = 1;
						}
						if( b->localSendIPG > ipg_d ){
							b->localSendIPG -= ipg_d;
						}
					}
				}
			}
			if( b->receivedCount > 1 ){
				if( b->currentRecvSize < replyLength ){
					b->rawRecvSize += b->rawRecvSize >> 3;
					tmp = b->rawRecvSize;
					if( tmp < 0x10000 ){
						b->rawRecvSize = (uint16)tmp;
					}else{
						b->rawRecvSize = 0xffff;
					}
					if( b->rawRecvSize >
					  (b->currentRecvSize + b->maxFragmentLength) ){
						b->currentRecvSize += b->maxFragmentLength;
					}
				}else{
					if( b->localRecvIPG > min_ipg ){
						ipg_d = b->localRecvIPG >> 3;
						if( ipg_d == 0 ){
							ipg_d = 1;
						}
						if( b->localRecvIPG > ipg_d ){
							b->localRecvIPG -= ipg_d;
						}
					}
				}
			}
			break;
		case REQUEST_RETRANSMISSION:
			if( b->consecutiveRetransmissionCount ){
				if( b->localSendIPG < (ipx_rtt * 10000) ){
					ipg_d = b->localSendIPG >> 3;
					if( ipg_d == 0 ){
						ipg_d = 1;
					}
					b->localSendIPG += ipg_d;
				}else{
					if( b->currentSendSize > b->minWindowSize ){
						b->rawSendSize -= b->rawSendSize >> 3;
						if( b->rawSendSize <
						  (b->currentSendSize - b->maxFragmentLength) ){
							b->currentSendSize -= b->maxFragmentLength;
						}
					}
				}
			}
			++b->consecutiveRetransmissionCount;
			break;
		case REPLY_RETRANSMISSION:
			if( b->consecutiveRetransmissionCount ){
				if( b->localRecvIPG < (ipx_rtt * 100) ){
					ipg_d = b->localRecvIPG >> 3;
					if( ipg_d == 0 ){
						ipg_d = 1;
					}
					b->localRecvIPG += ipg_d;
				}else{
					if( b->currentRecvSize > b->minWindowSize ){
						b->rawRecvSize -= b->rawRecvSize >> 3;
						if( b->rawRecvSize <
						  (b->currentRecvSize - b->maxFragmentLength) ){
							b->currentRecvSize -= b->maxFragmentLength;
						}
					}
				}
			}
			++b->consecutiveRetransmissionCount;
			break;
		default:
			NVLT_PRINTF( "NCPdiAdjustPacketBurstWindow: "
			  "bad retransmissionFlag\n", 0, 0, 0 );
			break;
	}

	b->retransmissionFlag = SUCCESS;

	return( NVLT_LEAVE(SUCCESS) );
}


void
NCPdplBuildPacketBurstHeader( ncp_channel_t* channel, void_t* headerData )
{
	PacketBurstHeader_T*	header;

	header = (PacketBurstHeader_T*)headerData;

	header->packetType = BIG_FILE_SERVICE_REQUEST;
	header->connectionControl = 0;
	header->streamType = BIG_SEND_MESSAGE;
	header->sourceConnectionID =
	  PUTINT32( channel->burstInfo->localConnectionID );
	header->destinationConnectionID =
	  PUTINT32( channel->burstInfo->remoteTargetID );
	header->packetSequenceNumber = 0;
	
	header->burstSequenceNumber =
	  PUTINT16( channel->burstInfo->burstSequenceNumber );
	header->ackSequenceNumber =
	  PUTINT16( channel->burstInfo->ackSequenceNumber );
	header->fragmentCount = 0;
}


void
ConvertFragmentListToBE (	pb_fragment_t*	to,
							pb_fragment_t*	from,
							uint16			fc )
{
	int i;

	NVLT_ENTER (3);

	for (i = 0; i < (int)fc; i++ ) {
		NVLT_PRINTF( "Converting fragment offset = %d, length = %d.\n",
			from[i].f_offset, from[i].f_length, 0 );
		to[i].f_offset = PUTINT32( from[i].f_offset );
		to[i].f_length = PUTINT16( from[i].f_length );
	}

	NVLT_LEAVE( SUCCESS );
	return;
}
