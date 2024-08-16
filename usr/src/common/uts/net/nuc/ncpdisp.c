/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ncpdisp.c	1.23"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ncpdisp.c,v 2.57.2.6 1995/02/13 07:54:20 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpdisp.c
 *	ABSTRACT: Dispatch routines for sending NCP packets to
 *			  the transport.
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPdiSendPacket
 *		NCPdiReceiveHandler
 *		NCPdiGetPacket
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
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/debug.h>
#include <util/nuc_tools/trace/nwctrace.h>

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
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/gtsconf.h>
#include <sys/gtsmacros.h>
#include <sys/nucerror.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */

void_t MD4Encode (uint8 *, uint32 *, int);

#define NVLT_ModMask	NVLTM_ncp

void_t
NCPesrFreeBufferInstr ()
{
	NVLT_ENTER (0);
	NVLT_LEAVE (0);
	return;
}

void_t
NCPesrFreeBuffer ()
{
	return;
}

void_t
NCPdiFreeBufferESR ( ncp_channel_t* channel )
{
	NVLT_ENTER (1);

	NWtlVSemaphore_l (channel->freeBufferSemaphore);

	NVLT_LEAVE (SUCCESS);
}

struct free_rtn NCPesrInstr = { NCPesrFreeBufferInstr, NULL};
struct free_rtn NCPesr = { NCPesrFreeBuffer, NULL};

ccode_t
NCPdiSendRequest_l (	ncp_channel_t	*channel,
						iopacket_t		*packet,
						uint8			*signature )
{
	ccode_t		ccode = SUCCESS;
	int32		diagnostic;
	void_t		*transportHandle;
	struct {
		NUC_IOBUF_T	ncpHdrBuf;
		NUC_IOBUF_T	ncpDataBuf;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (3);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));

	/*
	 *	Call the transport to dispatch the packet. 
	 *
	 *	Setup the header fragment of the packet data.  
	 * 	this portion is always kernel resident.
	 */
	KAlloc->ncpHdrBuf.bufferLength = packet->ncpHeaderSize; 
	KAlloc->ncpHdrBuf.buffer = (opaque_t *)packet->ncpU.ncpHdrData; 
	KAlloc->ncpHdrBuf.memoryType = IOMEM_KERNEL;
	KAlloc->ncpHdrBuf.esr = &NCPesrInstr;

	/*
	 *	The data portion of the request is passed down in a
	 *	NUC_IOBUF_T	structure that is part of the iopacket_t
	 *	structure
	 */
	if (packet->ncpDataBuffer == (NUC_IOBUF_T *)NULL) {
		KAlloc->ncpDataBuf.bufferLength = 0;
		KAlloc->ncpDataBuf.memoryType = IOMEM_KERNEL;
		KAlloc->ncpDataBuf.buffer = (opaque_t *)NULL;
	} else {
		if (packet->ncpDataBuffer->memoryType != IOMEM_KERNEL) {
			KAlloc->ncpDataBuf.buffer =
				kmem_zalloc( packet->ncpDataBuffer->bufferLength, KM_SLEEP );
			if (copyin(packet->ncpDataBuffer->buffer,
						KAlloc->ncpDataBuf.buffer,
						packet->ncpDataBuffer->bufferLength)) {
				ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
				goto done;
			}
		} else {
			KAlloc->ncpDataBuf.buffer = packet->ncpDataBuffer->buffer;
		}
		KAlloc->ncpDataBuf.bufferLength = packet->ncpDataBuffer->bufferLength;
		KAlloc->ncpDataBuf.memoryType = IOMEM_KERNEL;
	}
	NVLT_PRINTF ("NCPdiSendRequest_l: ncpDataBuf.bufferLength=0x%x\n",
				  KAlloc->ncpDataBuf.bufferLength, 0, 0 );

	KAlloc->ncpDataBuf.esr = &NCPesr;

	/*
	 *	Get the GTS handle from the channel structure
	 *	and pass the buffers on to GTS for processing and 
	 *	eventual transmission on the wire
	 */
	transportHandle = channel->transportHandle;

	if (channel->spiTaskPtr != NULL) {
		GTS_SEND (transportHandle, &KAlloc->ncpHdrBuf,
					&KAlloc->ncpDataBuf,
					((SPI_TASK_T *)(channel->spiTaskPtr))->ipxchecksum,
					GTS_IN_BAND, &diagnostic, signature);
	} else {
		GTS_SEND (transportHandle, &KAlloc->ncpHdrBuf,
					&KAlloc->ncpDataBuf, 0,
					GTS_IN_BAND, &diagnostic, signature);
	}

	if (diagnostic) {
		switch(diagnostic) {
			case NWD_GTS_BUF_FAULT:
				NVLT_PRINTF (
						"NCPdiSendRequest: Bad User buffer passed buf=%x\n",
						packet->ncpDataBuffer, 0, 0 );
				ccode = SPI_USER_MEMORY_FAULT;
				break;

			case NWD_GTS_TIMED_OUT:
			case NWD_GTS_NOT_CONNECTED:
				NVLT_PRINTF ("NCPdiSendRequest: Timeout detected\n", 0, 0, 0);
				ccode = SPI_SERVER_UNAVAILABLE;
				break;

			default:
				NVLT_PRINTF ("NCPdiSendRequest: Error %x returned from GTS\n",
						diagnostic, 0, 0);
				ccode = SPI_GENERAL_FAILURE;
				break;
		}
	}

done:
	if (KAlloc->ncpDataBuf.buffer != NULL &&
	    packet->ncpDataBuffer->memoryType != IOMEM_KERNEL) {

		kmem_free (KAlloc->ncpDataBuf.buffer,
				   packet->ncpDataBuffer->bufferLength);
	}
	kmem_free(KAlloc, sizeof(*KAlloc));
	return (NVLT_LEAVE (ccode));
}

ccode_t
NCPdiTransaction_l (	ncp_channel_t	*channel,
						iopacket_t		*request,
						iopacket_t		*reply )
{
	int32			diagnostic;
	uint32			msgType;
	ccode_t			ccode;
	void_t*			transportHandle;
	uint8*			signature = NULL;
	extern uint8	maxSignatureRetries;
	md4_t*			md4;
	uint32			messageDigest[4];
	struct {
		NUC_IOBUF_T		tempBuffer;
		NUC_IOBUF_T		ncpHdrBuf;
		NUC_IOBUF_T		replyBuffer;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (3);

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));

	/*
	 *	Sleep Lock on the semaphore to get to the wire
	 */
	NWtlPSemaphore (channel->wireSemaphore);

	/*
	 *	Set the status of this packet so we know who to wakeup
	 *	and stuff the current Sequence Number in the outgoing
	 *	packet.
	 */
	channel->sequenceNumber++;
	request->ncpU.ncpHdr.sequenceNumber = channel->sequenceNumber;

	KAlloc->replyBuffer.buffer = reply->ncpU.ncpHdrData;
	KAlloc->replyBuffer.bufferLength = reply->ncpHeaderSize;
	KAlloc->replyBuffer.memoryType = IOMEM_KERNEL;
	KAlloc->replyBuffer.esr = &NCPesr;
	KAlloc->replyBuffer.next = reply->ncpDataBuffer;

	/*	We need to make sure that our receive buffers are all kernel
	 *	resident.
	 */
	if( KAlloc->replyBuffer.next == NULL ){
		KAlloc->tempBuffer.buffer = (opaque_t *)NULL;
		KAlloc->tempBuffer.bufferLength = 0;
		KAlloc->tempBuffer.memoryType = IOMEM_KERNEL;
		KAlloc->replyBuffer.next = &KAlloc->tempBuffer;
	}else{
		if( KAlloc->replyBuffer.next->memoryType != IOMEM_KERNEL ){
			KAlloc->tempBuffer.buffer =
			  kmem_alloc( reply->ncpDataBuffer->bufferLength, KM_SLEEP );
			KAlloc->tempBuffer.memoryType = IOMEM_KERNEL;
			KAlloc->replyBuffer.next = &KAlloc->tempBuffer;
		}
		KAlloc->tempBuffer.bufferLength = reply->ncpDataBuffer->bufferLength;
	}
	KAlloc->replyBuffer.next->esr = &NCPesr;

	NVLT_PRINTF ("tempBuffer = %x tempBuffer.bufferLength = %d \n",
			&KAlloc->tempBuffer, KAlloc->tempBuffer.bufferLength, 0);

	/*
	 *
	 *	Set the channel's current request packet pointer 
	 */
	channel->currRequest = request;
	channel->currReply = &KAlloc->replyBuffer;

	if (channel->spiTaskPtr != NULL) {
		md4 = ((SPI_TASK_T *)(channel->spiTaskPtr))->md4;
		if (md4) {
			uint8	block[64];
			uint32	headerLength;
			uint32	dataLength;
			uint32	length;

			headerLength = request->ncpHeaderSize;

			if (request->ncpDataBuffer)
				dataLength = request->ncpDataBuffer->bufferLength;
			else
				dataLength = 0;

			length = headerLength + dataLength;

			bzero(block, 64);
			bcopy(md4->sessionKey, block, 8);
			bcopy(&length, block + 8, 4);

			if (headerLength - 6 < 52) {
				bcopy(request->ncpU.ncpHdrData + 6, block + 12,
					headerLength - 6);
				if (dataLength != 0) {
					if ((headerLength - 6 + dataLength) > 52)
						bcopy(request->ncpDataBuffer->buffer,
							block + 12 + (headerLength - 6),
							52 - (headerLength - 6));
					else
						bcopy(request->ncpDataBuffer->buffer,
							block + 12 + (headerLength - 6),
							dataLength);
				}

			} else
				bcopy(request->ncpU.ncpHdrData + 6, block + 12, 52);

			bcopy(md4->currentMessageDigest, messageDigest, 16);
			BuildMessageDigest(messageDigest, block);
			signature = (uint8 *)&messageDigest[0];

			bcopy(md4->currentMessageDigest, md4->previousMessageDigest, 16);
			bcopy(&messageDigest[0], md4->currentMessageDigest, 16);
		
			/* Set max tries for bad signatures */
			((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries =
								maxSignatureRetries;
		
		}
	}

	ccode = NCPdiSendRequest_l( channel, request, signature );
	if( ccode == SUCCESS ){

		NVLT_PRINTF("NCPdiTransaction_l: before GTS_RECEIVE reply->ncpHeaderSize=0x%x\n",
			reply->ncpHeaderSize, 0, 0);

		/*
		 *	Get the GTS handle from the NCP channel structure and 
		 *	call GTS to load our buffers with the data received
		 *
		 *	NOTE: reply->ncpDataBuffer is already formatted in 
		 *	a NUC_IOBUF_T	structure.
		 */
		transportHandle = channel->transportHandle;
		GTS_RECEIVE(transportHandle, &KAlloc->ncpHdrBuf,
					&KAlloc->tempBuffer, &msgType, B_FALSE, &diagnostic);

		if( diagnostic ){
			switch( diagnostic ){
				case NWD_GTS_TIMED_OUT:
				case NWD_GTS_NOT_CONNECTED:
					NVLT_PRINTF(
						"NCPdiTransaction: timeout detected, GTS rc=0x%x\n",
						diagnostic, 0, 0 );
					ccode = SPI_SERVER_UNAVAILABLE;
					break;

				default:
					NVLT_PRINTF( "NCPdiTransaction: failed, GTS rc=0x%x\n",
						diagnostic, 0, 0 );
					ccode = SPI_GENERAL_FAILURE;
					break;
			}
		}else{
			ccode = SUCCESS;
		}

		reply->ncpHeaderSize = KAlloc->ncpHdrBuf.bufferLength;

	}

	if( ccode ){
		/*
		 *	Reset the sequence so another request can go out on
		 *	this connection
		 */
		--channel->sequenceNumber;
	}

	reply->ncpHeaderSize = KAlloc->replyBuffer.bufferLength;

	if( reply->ncpDataBuffer != NULL ){
		if( reply->ncpDataBuffer->memoryType != IOMEM_KERNEL ){
			copyout ( KAlloc->tempBuffer.buffer, reply->ncpDataBuffer->buffer,
			  KAlloc->tempBuffer.bufferLength );
			kmem_free( KAlloc->tempBuffer.buffer,
						reply->ncpDataBuffer->bufferLength );
		}
		reply->ncpDataBuffer->bufferLength = KAlloc->tempBuffer.bufferLength;
	}

	/*
	 *	Set the channel's current request component to NULL
	 *	indicating that there are no loger any pending requests
	 *	on this channel
	 */
	channel->currRequest = NULL;
	channel->currReply = NULL;

	/*
	 *	Open up the channel for the next request to be handled when
	 *	we return here...
	 */
	NWtlVSemaphore_l (channel->wireSemaphore);
done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return( NVLT_LEAVE(ccode) );
}

void_t
NCPdplGetChannelCurrentReply_l (ncp_channel_t* channelPtr, void_t** packetPtr)
{
	*packetPtr = channelPtr->currReply;
	return;
}


ccode_t
NCPdiReceivePacketUpCall(	ncp_channel_t	*channel,
							NUC_IOBUF_T		*buf,
							NUC_IOBUF_T		**frag1,
							NUC_IOBUF_T		**frag2,
							opaque_t		*ipcChannel )
{
	ccode_t			ccode = FAILURE;
	ncp_reply_t		*ncpHeader;

	NVLT_ENTER (5);

	ncpHeader = (ncp_reply_t *)buf->buffer;
	if ((ncpHeader->responseType != 0x3333) &&
			(ncpHeader->responseType != 0x5555)) {
		/*
		 *	Unknown packet type!
		 */
		cmn_err (CE_NOTE,
			"NCPdiReceivePacketUpCall: Received a bad packet respType = 0x%x\n",
			ncpHeader->responseType);
		return (NVLT_LEAVE (ccode));
	}

	if (channel->spiTaskPtr != NULL) {
		if (((SPI_TASK_T *)(channel->spiTaskPtr))->md4) {
			if (!NWstrValidateSignature (ipcChannel,
					((SPI_TASK_T *)(channel->spiTaskPtr))->md4, 6, 0)) {

				((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries -= 1;
				if (((SPI_TASK_T *)(channel->spiTaskPtr))->badSignatureRetries
									== 0) {
					NVLT_PRINTF ("NCPspReceivePacketUpCall: "
						"Received max number of bad signatures.\n", 0,0,0);
					ccode = NWD_GTS_TIMED_OUT;
				}
				return (NVLT_LEAVE (ccode));
			}
		}
	}

	/*
	 *	If we are transmitting on a valid channel but the sequence 
	 *	number is invalid, this packet is out of sequence and will
	 *	be flushed.  Since the channel is still active, restart the 
	 *	retransmission timer.
	 *	Since the channel->sequenceNumber is incremented immediately
	 *	after the packet is sent, we should be comparing the reply
	 *	sequenceNumber with (channel->sequenceNumber - 1)
	 */
	if ( ncpHeader->sequenceNumber != channel->sequenceNumber ) {
		NVLT_PRINTF("NCPspReceivePacketUpCall: "
		  "Out of sequence packet received. channel=%d, packet=%d.\n",
		  channel->sequenceNumber, ncpHeader->sequenceNumber, 0 );
		return (NVLT_LEAVE (ccode));
	}

	NCPdplGetChannelCurrentReply_l( channel, (void_t *)frag1 );

	if( frag1 == NULL ){
		return (NVLT_LEAVE (FAILURE));
	}
	NVLT_PRINTF("NCPspReceivePacketUpCall: frag1 != NULL.\n", 0, 0, 0);

	*frag2 = (*frag1)->next;

	return (NVLT_LEAVE (SUCCESS));
}

ccode_t
NCPdiRetransmissionUpCall( ncp_channel_t* channel )
{
	ccode_t			ccode;
	md4_t*			md4;
	uint8*			signature = NULL;
	iopacket_t*		request;

	NVLT_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));

	if (channel->spiTaskPtr != NULL) {
		md4 = ((SPI_TASK_T *)(channel->spiTaskPtr))->md4;
		if( md4 ){
			signature = (uint8*)(md4->currentMessageDigest);
		}
	}

	request = channel->currRequest;
	if( request == NULL ){
		return( NVLT_LEAVE(FAILURE) );
	}

	ccode = NCPdiSendRequest_l( channel, request, signature );

	return( NVLT_LEAVE(ccode) );
}

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD4 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD4 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
*/

#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

/* F, G and H are basic MD4 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s) { \
    (a) += F ((b), (c), (d)) + (x); \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define GG(a, b, c, d, x, s) { \
    (a) += G ((b), (c), (d)) + (x) + (uint32)0x5a827999; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define HH(a, b, c, d, x, s) { \
    (a) += H ((b), (c), (d)) + (x) + (uint32)0x6ed9eba1; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }

/*
 * Convention: when stored in memory, always keep MD4 state in lo-hi
 * byte order.  Let BuildMessageDigest() convert to/from lo-hi <-> native
 * byte order when using state for calculation.
 */

void_t
InitMD4State (uint32 *state)
{
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
	MD4Encode ((uint8 *)state, state, 16);
}

/* Encodes input (uint32) into output (uint8). Assumes len is
 * a multiple of 4.
 */
void_t
MD4Encode (uint8 *output, uint32 *input, int len)
{
  uint32 i, j, tmp;

  for (i = 0, j = 0; j < len; i++, j += 4) {
	tmp = input[i];
    output[j]   = (uint8)(tmp & 0xff);
    output[j+1] = (uint8)((tmp >> 8) & 0xff);
    output[j+2] = (uint8)((tmp >> 16) & 0xff);
    output[j+3] = (uint8)((tmp >> 24) & 0xff);
  }
}

/*
 * Decodes input (uint8) into output (uint32). Assumes len is
 * a multiple of 4.
 */
void_t
MD4Decode (uint32 *output, uint8 *input, int len)
{
  int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint32)input[j]) | (((uint32)input[j+1]) << 8) |
      (((uint32)input[j+2]) << 16) | (((uint32)input[j+3]) << 24);
}

void_t
BuildMessageDigest (uint32 *state, uint8 *block)
{
  uint32 a, b, c, d, x[16];

#ifdef DEBUG_LATER
	NCP_CMN_ERR(CE_CONT, "BuildMessageDigest: enter\n");
	NCP_CMN_ERR(CE_CONT, "\tstate: "); PrintHex((uint8 *)state, 16);
	NCP_CMN_ERR(CE_CONT, "\tblock: "); PrintHex(block +  0, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(block + 16, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(block + 32, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(block + 48, 16);
#endif

#ifdef HI_LO_MACH_TYPE
	MD4Decode(state, state, 16);
	MD4Decode(x, block, 64);
#else
	bcopy((caddr_t)block, (caddr_t)x, 64);
#endif

#ifdef DEBUG_LATER
	NCP_CMN_ERR(CE_CONT, "\tetats: "); PrintHex((uint8 *)state, 16);
	NCP_CMN_ERR(CE_CONT, "\tkcolb: "); PrintHex(x +  0, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(x +  4, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(x +  8, 16);
	NCP_CMN_ERR(CE_CONT, "\t       "); PrintHex(x + 12, 16);
#endif

  a = state[0], b = state[1], c = state[2], d = state[3];

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11); /* 1 */
  FF (d, a, b, c, x[ 1], S12); /* 2 */
  FF (c, d, a, b, x[ 2], S13); /* 3 */
  FF (b, c, d, a, x[ 3], S14); /* 4 */
  FF (a, b, c, d, x[ 4], S11); /* 5 */
  FF (d, a, b, c, x[ 5], S12); /* 6 */
  FF (c, d, a, b, x[ 6], S13); /* 7 */
  FF (b, c, d, a, x[ 7], S14); /* 8 */
  FF (a, b, c, d, x[ 8], S11); /* 9 */
  FF (d, a, b, c, x[ 9], S12); /* 10 */
  FF (c, d, a, b, x[10], S13); /* 11 */
  FF (b, c, d, a, x[11], S14); /* 12 */
  FF (a, b, c, d, x[12], S11); /* 13 */
  FF (d, a, b, c, x[13], S12); /* 14 */
  FF (c, d, a, b, x[14], S13); /* 15 */
  FF (b, c, d, a, x[15], S14); /* 16 */

  /* Round 2 */
  GG (a, b, c, d, x[ 0], S21); /* 17 */
  GG (d, a, b, c, x[ 4], S22); /* 18 */
  GG (c, d, a, b, x[ 8], S23); /* 19 */
  GG (b, c, d, a, x[12], S24); /* 20 */
  GG (a, b, c, d, x[ 1], S21); /* 21 */
  GG (d, a, b, c, x[ 5], S22); /* 22 */
  GG (c, d, a, b, x[ 9], S23); /* 23 */
  GG (b, c, d, a, x[13], S24); /* 24 */
  GG (a, b, c, d, x[ 2], S21); /* 25 */
  GG (d, a, b, c, x[ 6], S22); /* 26 */
  GG (c, d, a, b, x[10], S23); /* 27 */
  GG (b, c, d, a, x[14], S24); /* 28 */
  GG (a, b, c, d, x[ 3], S21); /* 29 */
  GG (d, a, b, c, x[ 7], S22); /* 30 */
  GG (c, d, a, b, x[11], S23); /* 31 */
  GG (b, c, d, a, x[15], S24); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 0], S31); /* 33 */
  HH (d, a, b, c, x[ 8], S32); /* 34 */
  HH (c, d, a, b, x[ 4], S33); /* 35 */
  HH (b, c, d, a, x[12], S34); /* 36 */
  HH (a, b, c, d, x[ 2], S31); /* 37 */
  HH (d, a, b, c, x[10], S32); /* 38 */
  HH (c, d, a, b, x[ 6], S33); /* 39 */
  HH (b, c, d, a, x[14], S34); /* 40 */
  HH (a, b, c, d, x[ 1], S31); /* 41 */
  HH (d, a, b, c, x[ 9], S32); /* 42 */
  HH (c, d, a, b, x[ 5], S33); /* 43 */
  HH (b, c, d, a, x[13], S34); /* 44 */
  HH (a, b, c, d, x[ 3], S31); /* 45 */
  HH (d, a, b, c, x[11], S32); /* 46 */
  HH (c, d, a, b, x[ 7], S33); /* 47 */
  HH (b, c, d, a, x[15], S34); /* 48 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

#ifdef HI_LO_MACH_TYPE
  MD4Encode(state, state, 16);
#endif

#ifdef DEBUG_LATER
	NCP_CMN_ERR(CE_CONT,"\ta=%#8x b=%#8x c=%#8x d=%#8x\n", a, b, c, d);
	NCP_CMN_ERR(CE_CONT,"\tstate: "); PrintHex((uint8 *)state, 16);
#endif
}
