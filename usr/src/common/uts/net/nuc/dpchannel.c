/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/dpchannel.c	1.22"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/dpchannel.c,v 2.57.2.3 1995/02/12 23:36:45 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpchannel.c
 *	ABSTRACT: Manage the channel data structure
 *
 *
 *	Functions contained in this module:
 *
 *	NCPdplAllocChannel
 *	NCPdplFreeChannel
 *	NCPdplGetChannelPacketPoolHandle
 *	NCPdplGetAndIncChannelSequenceNumber
 *	NCPdplGetChannelCurrentRequest
 *	NCPdplSetChannelCurrentRequest
 *	NCPdplResetChannelSequenceNumber
 *	NCPdplLockChannelWireSemaphore
 *	NCPdplUnlockChannelWireSemaphore
 *	NCPdplSetChannelConnectionStatus
 *	NCPdplGetChannelConnectionStatus
 *	
 */ 

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <io/stream.h>
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/cmn_err.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/nwncpconf.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <util/debug.h>
#include <net/nuc/requester.h>

#include <util/ksynch.h>
#include <mem/kmem.h>
#include <net/nuc/nuc_hier.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/nucmachine.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/spilcommon.h>
#include <sys/nwncpconf.h>
#include <sys/ncpconst.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/gtsconf.h>
#include <sys/gtsmacros.h>
#include <sys/ncpiopack.h>
#include <sys/nucerror.h>
#include <sys/requester.h>
#include <sys/debug.h>

#include <sys/nuc_hier.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ncp

LKINFO_DECL(connSleepLockInfo, "NETNUC:NUC:connSleepLock", 0);
extern lock_t		*nucLock;
extern rwlock_t	*nucTSLock;

/*
 *	ANSI function prototypes
 */
#if defined(__STDC__)
#endif


/*
 *	Forward declarations
 */
void_t NCPspMessageSocketEventHandler();
void_t NCPdiFreeBufferESR();

/*
 * BEGIN_MANUAL_ENTRY(NCPdplAllocChannel.3k)
 * NAME
 *		NCPdplAllocChannel -	Allocate an I/O channel
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplAllocChannel( credStruct, transProto, address, addressLength, 
 *			bufferSize, taskPtr, channelPtr )
 *		void_t			*credStruct;
 *		uint32			transProto;
 *		uint8			*address;
 *		int32			addressLength;
 *		uint32			bufferSize;
 *		void_t			*taskPtr;
 *		ncp_channel_t	**channelPtr;
 *
 * INPUT
 *		credStruct		- Pointer to opaque credentials structure
 *		transProto		- Transport protocol identifier
 *		address			- Endpoint address encode in transProto form
 *		addressLength	- Length of the address field
 *		bufferSize		- negotiated buffer size for server
 *		taskPtr			- ncp_task_t of the owning task
 *
 * OUTPUT
 *		channelPtr		- Opaque pointer to channel structure used in 
 *						  subsequent calls to pass data to the transport
 *						  endpoint.
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Allocates the channel structure, allocates a buffer pool of packets
 *		to allow multiple threads to pre-format packets while waiting for
 *		the wire to come free.	Calls transport to open a transport endpoint.
 *		Allocates a semaphore used to synchronize access to the transport.
 *
 * NOTES
 *		The wire access semaphore is needed as a result of the single
 *		request/response model utilized by the current generation of
 *		Netware Core Protocol service protocols.
 *
 *		Had to implement the cheesy stage code in order to free all
 *		of the various resources in the event of failure.  Could have used
 *		setjmp/longjmp.
 *
 * SEE ALSO
 *		NCPdplFreeChannel(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplAllocChannel (	void_t			*credStruct,
						uint32			transProto,
						struct	netbuf	*address,
						uint32			bufferSize,
						void_t			*ncpTaskPtr,
						void_t			*spiTaskPtr,
						ncp_channel_t	**channelPtr )
{
	ccode_t			ccode;
	ncp_channel_t	*tmpPtr;
	int 			stage = 0;
	int32			diagnostic = SUCCESS;
	pl_t			pl;

	extern uint32 connectionReference;

	NTR_ENTER (5, credStruct, transProto, address, bufferSize, ncpTaskPtr);

	/*
	 *	Stage 1:
	 *
	 *	Allocate a block and initialize it.
	 */
	tmpPtr = (ncp_channel_t *)kmem_zalloc (sizeof(ncp_channel_t), KM_SLEEP);	

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: NCPdplAllocChannel: alloc channel * at 0x%x, size = 0x%x",
		tmpPtr, sizeof(ncp_channel_t), 0 );
#endif

	stage++;

	/*
	 *	Create the semaphores that synch access to the wire, and
	 *	for requests that require multiple NCP's to be sent in sequence
	 */
	ccode = NWtlCreateAndSetSemaphore( (int *)(&(tmpPtr->wireSemaphore)), 1 );
	if (ccode) {
		ccode = FAILURE;
		goto backOut;
	}
	stage++;


	/*
	 *	Stage 2:
	 *
	 *	This semaphore is used to signal when streams is finished with
	 *	our buffers.
	 */
	ccode = NWtlCreateAndSetSemaphore((int *)(&(tmpPtr->freeBufferSemaphore)),
			0);
	if (ccode) {
		ccode = FAILURE;
		goto backOut;
	}
	stage++;

	/*
	 *	Stage 3:
	 *
	 *	Initialize other fields in the structure
	 */
	tmpPtr->connectionNumber = 0;
	tmpPtr->sequenceNumber = 0xff;
	tmpPtr->taskNumber = 2;	/* one is used by connect/filesystem */
	tmpPtr->connectionStatus = 0;
	tmpPtr->currRequest = (void_t *)NULL;
	tmpPtr->negotiatedBufferSize = bufferSize;
	tmpPtr->taskPtr = ncpTaskPtr;
	tmpPtr->spiTaskPtr = spiTaskPtr;
	tmpPtr->securityFlags = 0;
	tmpPtr->connectionReference = connectionReference++;
	tmpPtr->broadcastState = NWC_BCAST_PERMIT_NONE;
	tmpPtr->flags = 0;
	tmpPtr->freeBufferStruct.free_func = NCPdiFreeBufferESR;
	tmpPtr->freeBufferStruct.free_arg = (void_t*)tmpPtr;

	tmpPtr->connSleepLock = SLEEP_ALLOC (NUCCONNSLOCK_HIER,
				&connSleepLockInfo, KM_SLEEP);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: NCPdplAllocChannel: alloc sleep_t * at 0x%x, size = 0x%x",
		tmpPtr->connSleepLock, sizeof(sleep_t), 0 );
#endif

	stage++;

	/*
	 *	Stage 4
	 *
	 *	Allocate ioBuffers for the pool
	 *	ncpConf structure is allocated in space.c of the nwncp package
	 */
	SLEEP_LOCK (tmpPtr->connSleepLock, NUCCONNSLEEPPRI);

	ccode = NCPdplAllocatePool(	ncpConf.ioBuffersPerTask, 
								&(tmpPtr->packetPool)); 
	if (ccode) {
		ccode = FAILURE;
		goto backOut;
	}
	stage++;

	/*
	 *	Stage 5:
	 *
	 *	Call gipc (Generic Inter-Process Communiction) to allocate a 
	 *	transport endpoint.  This will be the interface structure that is
	 *	passed to gain access to the wire.
	 *
	 *	Fills in TransportHandle
	 */

	GTS_OPEN(	transProto, 
				GTS_REQUESTOR,
	 			NULL, 
				GTS_SYNC,
				credStruct, 
				&(tmpPtr->transportHandle), 
				&diagnostic );

	if (diagnostic) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR(CE_CONT, 
			"NCPAllChan; gtsOpenTransportEndpoint failed diag=%d\n",
			diagnostic);
#endif
		ccode = diagnostic;
		goto backOut;
	}

	/*
	 *	Perform a connect operation to formalize the transport 
	 *	session.  In datagram protocols, this merely saves off the 
	 *	destination address, otherwize the virtual circuit based procols
	 *	will create and endpoint instantiation.
	 */
	GTS_CONNECT( tmpPtr->transportHandle, address->buf, &diagnostic);

	pl = RW_WRLOCK (nucTSLock, plstr);
	*channelPtr = tmpPtr;
	RW_UNLOCK (nucTSLock, pl);

	if (diagnostic) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR(CE_CONT, "NCPAllChan; gtsConnectToPeer failed diag=%d\n",
			diagnostic);
#endif
		ccode = diagnostic;
		goto backOut;
	}

	/*
	 *	Packet Burst uses its socket number in its request packets.  
	 *	Retrieve it from the ipxClient_t and store it in my ncp_channel_t.
	 *	Also, save a pointer to my ncp_channel_t in the associated
	 *	ipxTask_t so the IPXEng Packet Burst Interrupt routines can
	 *	find it.
	 */
	IPXEngObtainTransportInfo(tmpPtr);

	/*
	 *	If this connection will involve an authenticated task, register a
	 *	handler to indicate that broadcast messages are waiting on the server
	 */
	if (ncpTaskPtr != NULL) {
		GTS_REGISTER (tmpPtr->transportHandle, NCPspMessageSocketEventHandler, 
						ncpTaskPtr, &diagnostic);
		if (diagnostic) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR(CE_CONT, 
			"NCPspAllocChannel: RegisterAsyncEventHandler failed diag=%d\n",
			diagnostic);
#endif NUC_DEBUG
			ccode = diagnostic;
			goto backOut;
		}	
	}

#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	/*
	 *	Build an object validation stamp to ensure what we get is sane
	 *	when debugging
	 */
	if (ccode == SUCCESS) {
		(*channelPtr)->tag[0] = 'C';
		(*channelPtr)->tag[1] = 'P';
	}
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))

	return(NTR_LEAVE (ccode));

backOut:

	/*
	 *	Back-out all that's been done to free up the resource
	 */
	switch (stage) {
		case 5:
			NCPdplFreePool(tmpPtr->packetPool);
			SLEEP_UNLOCK (tmpPtr->connSleepLock);
		case 4:
			SLEEP_DEALLOC (tmpPtr->connSleepLock);

#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM_FREE: NCPdplAllocChannel:free sleep_t 0x%x, size=0x%x",
				tmpPtr->connSleepLock, sizeof(sleep_t), 0 );
#endif NUCMEM_DEBUG
		case 3:
			NWtlDestroySemaphore(tmpPtr->freeBufferSemaphore);
		case 2:
			NWtlDestroySemaphore(tmpPtr->wireSemaphore);
		case 1:
			kmem_free (tmpPtr, sizeof(ncp_channel_t));
#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM_FREE: NCPdplAllocChannel:free channel 0x%x, size=0x%x",
				tmpPtr, sizeof(ncp_channel_t), 0 );
#endif NUCMEM_DEBUG
			*channelPtr = (ncp_channel_t *)NULL;
			break;
	}

	return(NTR_LEAVE (ccode));

}


/*
 * BEGIN_MANUAL_ENTRY(NCPdplFreeChannel.3k)
 * NAME
 *		NCPdplFreeChannel -	Free channel structure and assocated
 *							resources
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplFreeChannel( channelPtr )
 *		ncp_channel_t	*channelPtr;
 *
 * INPUT
 *		channelPtr	- Pointer to a channel structure
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Frees resources associated with the I/O channel used by
 *		NCP to communicate with the world.
 *
 * NOTES
 *		Assumes the caller is trusted. 
 *
 * SEE ALSO
 *		NCPsplAllocChannel(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplFreeChannel_l (	ncp_channel_t	*channelPtr )
{
	int32	diagnostic;

	NTR_ASSERT ((channelPtr->tag[0] == 'C') && (channelPtr->tag[1] == 'P'));

	NTR_ENTER (1, channelPtr, 0, 0, 0, 0);

	NWtlDestroySemaphore( channelPtr->wireSemaphore );
	NWtlDestroySemaphore( channelPtr->freeBufferSemaphore );
	NCPdplFreePool( channelPtr->packetPool );

	/*
	 *	Call the transport to give back the handle
	 */
	if ( channelPtr->transportHandle != NULL ) {
		GTS_REGISTER( channelPtr->transportHandle, NULL, NULL, &diagnostic ); 
		GTS_DISCONNECT( channelPtr->transportHandle, &diagnostic );
		GTS_CLOSE( channelPtr->transportHandle, &diagnostic );
	}
	
	if (channelPtr->burstInfo != NULL) {
		kmem_free (channelPtr->burstInfo,
					sizeof (struct packetBurstInfoStruct));
#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NCPdplFreeChannel_l:free Burst * at 0x%x, size=0x%x",
			channelPtr->burstInfo, sizeof(struct packetBurstInfoStruct), 0 );
#endif NUCMEM_DEBUG

	}

	SLEEP_UNLOCK (channelPtr->connSleepLock);
	SLEEP_DEALLOC (channelPtr->connSleepLock);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: NCPdplFreeChannel_l: free sleep_t * at 0x%x, size = 0x%x",
		channelPtr->connSleepLock, sizeof(sleep_t), 0 );
#endif

	kmem_free ( (char *)channelPtr, sizeof (ncp_channel_t));

#ifdef NUCMEM_DEBUG
	NTR_PRINTF(
		"NUCMEM_FREE:NCPdplFreeChannel_l:free channel * at 0x%x, size=0x%x",
		channelPtr, sizeof(ncp_channel_t), 0 );
#endif NUCMEM_DEBUG

	channelPtr = NULL;

#ifdef MCPOWERS_DEBUG
	NCP_CMN_ERR(CE_CONT, "requester: channel free\n");
#endif

	return (NTR_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPdplGetFreeChannelTaskNumber.3k)
 * NAME
 *		NCPdplGetFreeChannelTaskNumber - Get the task number of this channel.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplGetFreeChannelTaskNumber( channelPtr, taskNumber )
 *		ncp_channel_t	*channelPtr;
 *		uint8			*taskNumber;
 *
 * INPUT
 *		channelPtr
 *
 * OUTPUT
 *		taskNumber
 *
 * RETURN VALUES
 *		None
 *
 * DESCRIPTION
 *		Returns the current value of a free task number for this 
 *		channel.
 *
 * NOTES
 *		This call is used by RAW registery only.
 *		It enforces the assumption that only 254 tasks can be
 *		operating concurrently.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucUpperLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 *
 * SEE ALSO
 *		NCPdplClearChannelTaskNumber
 *
 * END_MANUAL_ENTRY
*/
ccode_t
NCPdplGetFreeChannelTaskNumber_l (	ncp_channel_t	*channelPtr,
									uint8			*taskNumber,
									uint32			*connectionReference )
{
	register int	i;
	register uint8	*bptr = channelPtr->taskMask;

	NTR_ASSERT ((channelPtr->tag[0] == 'C') && (channelPtr->tag[1] == 'P'));

	/*
	 *	Tasks 0 and 1 are not available to raw
	 *	request
	 */
	for (i = 2; i < NCP_MAX_TASKS; i++)
	{
		if ( !(bptr[i/8] & (1 << (i%8))) )
		{
			bptr[i/8] |= 1 << (i%8);
			*taskNumber = (unsigned char)i;
			channelPtr->referenceCount++;
			*connectionReference = channelPtr->connectionReference;
#ifdef MCPOWERS_DEBUG
			NCP_CMN_ERR(CE_CONT, "requester: reference count = %d\n",
				channelPtr->referenceCount);
#endif
			return(SUCCESS);
		}
	}

	*taskNumber = (unsigned char)0;
#ifdef NUC_DEBUG
	NCP_CMN_ERR(CE_CONT, "GetFreeChannelTaskNumber FAILED! \n");
#endif
	return(FAILURE);

}


ccode_t
NCPdplClearChannelTaskNumber_l (	ncp_channel_t	*channelPtr,
									uint8			*taskNumber )
{
	int i;
	unsigned char *bptr = channelPtr->taskMask;

	NTR_ASSERT ((channelPtr->tag[0] == 'C') && (channelPtr->tag[1] == 'P'));

	/*
	 *	Flip the bit associated with this task
	 */
	i = (int)*taskNumber;
	bptr[i/8] &= ~( 1 << (i%8) );
	channelPtr->referenceCount--;
	if (channelPtr->referenceCount < 0)
	{
		channelPtr->referenceCount = 0;
	}
#ifdef MCPOWERS_DEBUG
	else NCP_CMN_ERR(CE_CONT, "requester: reference count = %d\n",
		channelPtr->referenceCount);
#endif
	return(SUCCESS);
}
