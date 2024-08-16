/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxipc.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxipc.c,v 2.59.2.2 1995/01/12 00:47:25 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxipc.c
 *
 *	ABSTRACT: IPXEngine interface to the GIPC layer. 
 *
 *	This module works on top of UnixWare STREAMS.
 *	and will have to be modified to accomodate BSD Sockets.
 *
 *	Functions declared in this module:
 *	Public functions:
 *		IPXEngSendIPCPacket
 *		IPXEngGetIPCPacket
 *		IPXEngAllocateIPCSocket
 *		IPXEngStreamsTPIBind
 *		IPXEngFreeIPCSocket
 *		IPXEngGetLocalAddress
 *		IPXEngGetIPCPacketPointer
 *		IPXEngFlushCurrentIPCPacket
 *		IPXEngReSendIPCPacket
 *		IPXEngFreeIPCPacket
 *	Private functions:
 */

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <io/stream.h>
#include <svc/time.h>
#include <net/tihdr.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucmachine.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcmacros.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/requester.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/debug.h>

#else _KERNEL_HEADERS

#include <sys/time.h>
#include <sys/tihdr.h>
#include <sys/ipx_app.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nuctool.h>
#include <sys/nucmachine.h>
#include <sys/ipxengine.h>
#include <sys/gipccommon.h>
#include <sys/gipcmacros.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/nucerror.h>
#include <sys/ncpconst.h>
#include <sys/ncpiopack.h>
#include <sys/requester.h>
#include <sys/nuc_prototypes.h>

#endif _KERNEL_HEADERS

#define NTR_ModMask	NTRM_ipxeng

extern char transportDevicePath[];	/* Path to IPX inode */
extern struct free_rtn NCPesr;

/*
 *	Forward declaration
 */
void_t	IPXEngMapIPCDiagnostic();

void_t
IPXEngFreeBufferESR ()
{
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngSendIPCPacket(3K), \
 *			./man/kernel/ts/ipxeng/SendIpcPacket)
 * NAME
 *	IPXEngSendIPCPacket -	Send Packet via IPC to IPX endpoint
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngSendIPCPacket( ipcChannel, targetAddress, frag1, frag2, 
 *				packetType, msgType, ipcMsgHandle )
 *	opaque_t	*ipcChannel;
 *	uint8		*targetAddress;
 *	NUC_IOBUF_T	frag1;
 *	NUC_IOBUF_T	frag2;
 *	int32		packetType;
 *	int32		msgType;
 *	opaque_t	**ipcMsgHandle;
 *			
 * INPUT
 *	ipcChannel 	- Ipc channel returned by gipc.OpenTransportEndpoint.
 *			  The channel is assoicated with a IPX socket.
 *	targetAddress 	- The NET:NODE:SOCKET encoded in machine order, of
 *			  the NetWare Server to send the NCP Request to.
 *	frag1		- First packet fragment (usually NCP protocol
 *			  header.
 *	frag2		- Second fragment (usually packet data)
 *	packetType	- Type of packet, either NCP (type 17) or zero for watchdog
 *	msgType		- Type of message GIPC_NORMAL_MSG for IN-BAND, or
 *			  GIPC_HIPRI_MSG for OUT-OF-BAND.
 *
 * OUTPUT
 *	ipcMsgHandle	- Handle for re-transmission of message.  Used when
 *			  response timeout occurs to re-transmit the message
 *			  out of process context.
 *
 * RETURN VALUES
 *	SUCCESS		- Packet was dispatched successfully
 *	*		- GIPC error mapped to GTS error.
 *
 * DESCRIPTION
 *	Passes packet fragments to the GIPC handle specified for
 *	transmission onto the network.  
 *
 * NOTES
 *	Handles the UnixWare TPI T_UNITDATA_REQ service, which sends a NCP
 *	packet to the IPX Stack for delivery to the NetWare Server over
 *	the wire.
 *
 * SEE ALSO
 *	IPXEngGetIPCPacket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngSendIPCPacket (opaque_t *ipcChannel, opaque_t *targetAddress,
					 NUC_IOBUF_T *frag1, NUC_IOBUF_T *frag2,
					 int32 packetType, uint32 checksumTrigger,
					 int32 msgType, opaque_t **ipcMsgHandle,
					 uint8 *signature )
{
	NUC_IOBUF_T	controlMsg;
	int32		diagnostic;
	struct sendBuf {
		struct T_unitdata_req	strBuf;
		ipxAddress_t		ipxAddr;	
#pragma pack(1)
		struct Opt {
			uint8			ipxPacketType;
			uint16			ipxChecksum;
		} ipxOpts;
#pragma pack()
	} sbuf;

	NTR_ENTER(5, ipcChannel, targetAddress, frag1, frag2, packetType);

	/*
	 *	Build the TPI Header first
	 */
	sbuf.strBuf.PRIM_type = T_UNITDATA_REQ;
	sbuf.strBuf.DEST_length = IPX_ADDR_SIZE;
	sbuf.strBuf.DEST_offset = sizeof(struct T_unitdata_req);
	sbuf.strBuf.OPT_length = 3;		/* should be macro'ed to IPX_OPTS_SIZE */
	sbuf.strBuf.OPT_offset = sizeof(struct T_unitdata_req)+IPX_ADDR_SIZE;

	bcopy( targetAddress, (caddr_t)sbuf.ipxAddr.ipxAddr, IPX_ADDR_SIZE ); 

	/*
	 *	Set the packet type.  This value is now being passed in from
	 *	the caller to allow the watchdog interrupt routine to pass
	 *	a zero as the packet type.  Normally the packet type is
	 *	IPX_NOVELL_PACKET_TYPE.
	 *
	 */
	sbuf.ipxOpts.ipxPacketType = (uint8)packetType;
	if (checksumTrigger & NWC_SECURITY_LEVEL_CHECKSUM)
		sbuf.ipxOpts.ipxChecksum = IPX_CHKSUM_TRIGGER;


	/*
	 *	Build the GIPC header now...
	 */
	controlMsg.buffer = (opaque_t *)&sbuf;
	controlMsg.bufferLength = sizeof(struct sendBuf);
	controlMsg.memoryType = IOMEM_KERNEL;
	controlMsg.esr = &NCPesr;

	NTR_ASSERT( *(uint16 *)frag1->buffer != 0x3333 );

	GIPC_SEND( ipcChannel, &controlMsg, frag1, 
			frag2, msgType, ipcMsgHandle, &diagnostic, signature );

	if (diagnostic) {
#ifdef NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"SendIPCMessaged failed with diag=%x\n",diagnostic);
#endif
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	}

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngGetIPCPacket(3K), \
 *			./man/kernel/ts/ipxeng/GetIpcPacket)
 * NAME
 *	IPXEngGetIPCPacket -	Get packet from the head of IPC queue
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngGetIPCPacket( ipcChannel, sourceAddress, frag1, frag2, msgType )
 *
 *	opaque_t	*ipcChannel;
 *	uint8		*sourceAddress;
 *	opaque_t	*frag1;
 *	opaque_t	*frag2;
 *	int32		*msgType;
 *		
 *
 * INPUT
 *	ipcChannel	- 	Channel returned by call to 
 *				gipc.OpenTransportEndpoint
 *
 * OUTPUT
 *	sourceAddress	- Source IPX address of packet originator
 *	frag1		- First data fragment of packet (usu. sproto header)
 *	frag2		- Second data fragment of packet (usu. packet data)
 *	msgType		- Message type (IN_BAND or OUT_OF_BAND)
 *
 * RETURN VALUES
 *	SUCCESS		- Packet was received successfully
 *	*		- GIPC error mapped to GTS error.
 *
 * DESCRIPTION
 *	Formats the control buffer used by IPC in order to receive the
 *	source address that is returned to the caller. IPC function handles
 *	all data movement.
 *
 * NOTES
 *	This function blocks the calling process if the channel was opened
 *	in synchronus mode, otherwise, the get request is posted, and 
 *	the function will return immediately.  The calling process will
 *	be interrupted when the stream head calls the registered async handler
 *		
 *
 *	This function should be called when the IPC does, in fact, have
 *	a message waiting at the head.  If it is called with no message,
 *	just return an error.
 *
 * SEE ALSO
 *	IPXEngSendIPCPacket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngGetIPCPacket (opaque_t *ipcChannel, uint8 *sourceAddress,
					NUC_IOBUF_T *frag1, NUC_IOBUF_T *frag2, uint32 *msgType)
{
	int32		diagnostic;
	int32		residualBytes;
	NUC_IOBUF_T  controlMsg;
	struct getBuf {
		struct T_unitdata_ind 	stBuf;
		ipxAddress_t		ipxAddress;
		uint8			ipxPacketType;
	} gbuf;

	NTR_ENTER(5, ipcChannel, sourceAddress, frag1, frag2, msgType);
	
	controlMsg.buffer = (opaque_t *)&gbuf;
	controlMsg.bufferLength = sizeof(struct getBuf);
	controlMsg.memoryType = IOMEM_KERNEL;

	GIPC_RECEIVE(ipcChannel, &controlMsg, frag1, 
			frag2, msgType, &diagnostic, &residualBytes);

	if (diagnostic) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"GetIPCMessaged failed with diag=%x(hex)\n",diagnostic);
#endif	NUC_DEBUG
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	}

	if (residualBytes) {
		/*
		 * May want to add some stuff here to return back a condition
		 * code telling the caller that more data exists.
		 */
		GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &diagnostic );
	}

	bcopy( (caddr_t)gbuf.ipxAddress.ipxAddr,
			   (caddr_t)sourceAddress, IPX_ADDR_SIZE);

	return( NTR_LEAVE( SUCCESS ) );
}



/*
 * BEGIN_MANUAL_ENTRY(IPXEngAllocateSocket(3K), \
 *			./man/kernel/ts/ipxeng/AllocateSocket)
 * NAME
 *	IPXEngAllocateSocket -	Allocate an IPX socket from the driver
 *				for use as a transport endpoint.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngAllocateIPCSocket( ipcChannel, clientHandle, socketNumber, 
 *				asyncFunction)
 *	opaque_t	**ipcChannel;
 *	ipxTask_t	*clientHandle;
 *	uint8		*socketNumber;
 *	int32		(*asyncFunction)();
 *
 * INPUT
 *	clientHandle	- IPXEng client structure
 *	socketNumber	- Socket number to allocate (0 to have one selected
 *			  ephemerally)
 *	asyncFunction	- Async handler that will be registered for this 
 *						  socket
 * OUTPUT
 *	ipcChannel	- Handle used to reference this socket on subsequent
 *			  calls
 *	socketNumber	- Socket number allocated
 *
 * RETURN VALUES
 *	SUCCESS			- Successful completion.
 *	NWD_GTS_ADDRESS_INUSE	- Socket number requested is allocated to
 *				  another client.
 *	*			- GIPC error mapped to GTS error.
 *
 * DESCRIPTION
 *	Open a GIPC endpoint and send TPI messages to the protocol stack
 *	to allocate the socket number specified.  In order to open an
 *	ephemeral socket, the socket number is passed as a 0.  Otherwise
 *	the stack will attempt to allocate the socket number specified.
 *
 * NOTES
 *	This function assumes STREAMS.
 *
 *	Given a socket number (or potentially 0) open the IPX streams driver
 *	and give them an ioctl to allocate the specified socket.  If the
 *	the socket passed was 0, IPX will allocate one from the ephemeral
 *	socket pool.  
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	IPXEngFreeIPCSocket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngAllocateIPCSocket (opaque_t **ipcChannel, ipxClient_t *clientHandle,
					uint8 *socketNumber, void_t (*asyncFunction)())
{
	int32	diagnostic, filler, residualBlocks;
	uint32	msgType;
	NUC_IOBUF_T	control,frag1;
	struct tpiBindReq {
		struct T_bind_req 	tbind;
		ipxAddress_t		ipxAdd;
	} bindReq;
	struct tpiBindAck {
		struct T_bind_ack	tbind;
		ipxAddress_t		ipxAdd;
	} *bindAckPtr;

	opaque_t	*reTransHandle;

	NTR_ENTER(4, ipcChannel, clientHandle, socketNumber, asyncFunction, 0);
	
	/*
	 *	Call the ipc to open the transport inode
	 */
	GIPC_OPEN( RITCHIE_STREAMS, transportDevicePath, 
		GIPC_ASYNC, ipcChannel, &diagnostic );
	if ( diagnostic ) {
#ifdef	NUC_DEBUG
		cmn_err (CE_CONT, "IPXEng;OpenIpcChannel failed with diag=%x \n",
					diagnostic);
#endif	NUC_DEBUG
		IPXEngMapIPCDiagnostic (&diagnostic);
		return (NTR_LEAVE (diagnostic));
	}

	/*
	 *	Register this endpoint and async handler.  To start out 
	 *	register the bind call back.
	 */
	GIPC_REGISTER_CALLBACK( *ipcChannel, IPXEngStreamsTPIBind, 
		clientHandle, &diagnostic );

	bindReq.tbind.PRIM_type = T_BIND_REQ;
	bindReq.tbind.ADDR_length = sizeof(ipxAddress_t);
	bindReq.tbind.ADDR_offset = sizeof(struct T_bind_req);

	/*
	 *	Setup the IPX header
	 *	Byte swap socket number to NET order if necessary
	 */
	*(uint16 *)(bindReq.ipxAdd.ipxComp.socket) =
		GETINT16(*(uint16 *)socketNumber);

	control.bufferLength = sizeof(struct tpiBindReq);
	control.buffer = (opaque_t *)&bindReq;
	control.memoryType = IOMEM_KERNEL;
	control.esr = &NCPesr;

	frag1.bufferLength = 0;
	frag1.memoryType = IOMEM_KERNEL;
	frag1.esr = &NCPesr;

	msgType = GIPC_NORMAL_MSG; 

	GIPC_SEND(*ipcChannel, &control, &frag1, 
		NULL, msgType, &reTransHandle, &diagnostic, NULL);
	if (diagnostic) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXEng:AllocSocket,gipcSendMessage failed with cc=%x\n",
					diagnostic );
#endif	/* NUC_DEBUG	*/
		/*
		 *	Something wrong with the channel we just opened???
		 */
		GIPC_CLOSE(*ipcChannel, &filler);
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	}

	/*
	 *	Synchronize with Ack of Bind Request
	 *	Note:
	 *		Since in all cases but socket binding, the GIPC
	 *		channel will be running asyncrhonously, event driven,
	 *		we opened the channel GIPC_ASYNC, but must now block
	 *		ourselves until the T_BIND_ACK is received.  We have
	 *		a temporary call back handler IPXEngStreamsTPIBind().
	 */
	NWtlPSemaphore(clientHandle->syncSemaphore);

	GIPC_PREVIEW( *ipcChannel, &control, &frag1,
		&msgType, TRUE, &diagnostic, &residualBlocks );
	if (diagnostic) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXEngAllocateSocket; PreviewMessaged failed diag=%x\n",
			diagnostic);
#endif	/* NUC_DEBUG	*/
		GIPC_CLOSE(*ipcChannel, &filler);
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	} else {
		/*
		 *	Take a look at what we received...
		 */
		bindAckPtr = (struct tpiBindAck *)control.buffer;
		if (bindAckPtr->tbind.PRIM_type == T_BIND_ACK) {
			/*
			 *	Byte swap the socket number back to machine order
			 *	as it comes back from IPX in NET order
			 */
			*(uint16 *)socketNumber = 
				GETINT16( *(uint16 *)(bindAckPtr->ipxAdd.ipxComp.socket));
		} else {
			/*
			 *	Received a message type other than what we wanted
			 *	(probably T_ERROR_ACK) so we'll tear-down and fail..
			 */
#ifdef	NUC_DEBUG
			IPXENG_CMN_ERR(CE_CONT, 
				"IPXEng;Bind Failed prim=%x \n", bindAckPtr->tbind.PRIM_type);
#endif	/* NUC_DEBUG	*/
			GIPC_CLOSE(*ipcChannel, &diagnostic);
			diagnostic = NWD_GTS_ADDRESS_INUSE;
			return( NTR_LEAVE( diagnostic ) );
		}
	}

	/*
	 *	Now, register the real event handler for this socket
	 */
	GIPC_FLUSH(*ipcChannel, GIPC_FLUSH_RALL, &diagnostic);
	GIPC_REGISTER_CALLBACK( *ipcChannel, asyncFunction, clientHandle, &diagnostic );

	return( NTR_LEAVE( SUCCESS ) );
}



/*
 * BEGIN_MANUAL_ENTRY(IPXEngStreamsTPIBind(3K), \
 *			./man/kernel/ts/ipxeng/StreamsTPIBind)
 * NAME
 *	IPXEngStreamsTPIBind - Async event handler for the T_BIND_REQ message 
 *
 * SYNOPSIS
 *
 *	ccode_t
 *	IPXEngStreamsTPIBind( ipcChannel, clientPtr, eventMsgType )
 *
 *	opaque_t	*ipcChannel;
 *	ipxClient_t	*clientPtr;
 *	int32		msgType;
 *
 * INPUT
 *	ipcChannel	- IPC channel used to reference the endpoint opened
 *			  by the gipc.OpenTransportEndpoint
 *	clientPtr	- Client structure used as the wakeup channel
 *	msgType		- Type of message returned by the IPC callback
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	Asynch event handler for TPI socket binding through streams.
 *	This is esentially waiting for a response from streams that
 *	the request was processed, and that a requested socket has 
 *	been allocated by IPX.
 *
 * NOTES
 *	This async event handler simply wakes up (schedules) the process
 *	which blocked itself in IPXEngAllocateIPCSocket(3K), waiting for
 *	the bind ack.
 *
 *	This handler is loaded into the GIPC Channel only until the socket
 *	is bound, and then is replaced by the appropriate socket call
 *	back handler.
 *
 * SEE ALSO
 *	IPXEngAllocateIPCSocket(3k), IPXEngInterruptHandler(3K), 
 *	IPXEngWDSocketInterruptHandler(3K), IPXEngOOBSocketInterruptHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
void_t
IPXEngStreamsTPIBind (opaque_t *ipcChannel, ipxClient_t *clientPtr,
					int32 msgType)
{
	NTR_ENTER(3, ipcChannel, clientPtr, msgType, 0, 0);

	/*
	 *	Synchronize with Binding Context
	 */
	NWtlVSemaphore_l (clientPtr->syncSemaphore);

	NTR_LEAVE( SUCCESS );
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFreeIPCSocket(3K), \
 *			./man/kernel/ts/ipxeng/FreeIPCSocket)
 * NAME
 *	IPXEngFreeIPCSocket - Free previously allocate IPX socket
 *
 * SYNOPSIS
 *
 *	ccode_t
 *	IPXEngFreeIPCSocket( ipcChannel, clientPtr )
 *
 *	opaque_t	*ipcChannel;
 *	ipxClient_t	*clientPtr;
 *	
 *
 * INPUT
 *	ipcChannel	- Channel representing previously allocated socket
 *	clientPtr	- Client structure which icpChannel is attached to.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	Closes the Transport endpoint, and thus, causes the IPX driver to
 *	free the socket allocated by this channel.
 *
 * NOTES
 *	Assumes that IPX gives up all the channel's resources when the
 *	endpoint is closed.
 *
 * SEE ALSO
 *	IPXEngAllocIPCSocket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFreeIPCSocket (opaque_t	*ipcChannel)
{
	int32	diagnostic;

	NTR_ENTER(1, ipcChannel, 0, 0, 0, 0);

	GIPC_CLOSE( ipcChannel, &diagnostic );


	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngGetIPCPacketPointer(3K), \
 *			./man/kernel/ts/ipxeng/GetIPCPacketPointer)
 * NAME
 *	IPXEngGetIPCPacketPointer
 *
 * SYNOPSIS
 *
 *	ccode_t
 *	IPXEngGetIPCPacketPointer( ipcChannel, buffer, ipxAddress )
 *
 *	opaque_t	*ipcChannel;
 *	NUC_IOBUF_T	*buffer;
 *	uint8		*ipxAddress;
 *
 * INPUT
 *	ipcChannel	- Ipc channel associated with an IPX socket
 *
 * OUTPUT
 *	buffer - NUC_IOBUF_T which points to the packet
 *	ipxAddress	- NET:NODE:SOCKET of the server sending NCP response.
 *
 * RETURN VALUES
 *	0			- Successful Completion.
 *	NWD_GTS_NO_MESSAGE	- No NCP response is on the read head.
 *	*			- GIPC layer error, mapped to GTS error.
 *	
 * DESCRIPTION
 *	Previews the current packet at the IPC head to get the IPX 
 *	source address and the NCP sequence number for validation 
 *	purposes.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngGetIPCPacketPointer (opaque_t *ipcChannel, NUC_IOBUF_T *buffer,
							ipxAddress_t *ipxAddress)
{
	NUC_IOBUF_T	control;
	int32		diagnostic, filler, residualBlocks;
	uint32		msgType;

	typedef	struct	{
		uint16	replyType;
		uint8	sequenceNumber;
		uint8	connectionNumberLowOrder;
		uint8	taskNumber;
		uint8	connectionNumberHighOrder;
		uint8	completionCode;
		uint8	connectionStatus;
	}NCP_REPLY_HEADER_T;

	struct getBuf {
		struct T_unitdata_ind 	stBuf;
		ipxAddress_t		ipxAddress;
		uint8			ipxPacketType;
	} *gbuf;

	typedef	struct	{
		struct	T_uderror_ind	erBuf;
		ipxAddress_t			ipxAddress;
	}T_UD_ERROR_HDR_T;

	NTR_ENTER(3, ipcChannel, buffer, ipxAddress, 0, 0);

	GIPC_PREVIEW( ipcChannel, &control, buffer, &msgType, TRUE, &diagnostic,
					&residualBlocks);

	if (diagnostic) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXEng; GetIPCPackSourceAddr, Preview FAILED! \n");
#endif	NUC_DEBUG
		GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &filler );
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	}

	gbuf = (struct getBuf *)control.buffer;

	/*
	 *	Now that the buffers have been mapped, make sure the reply is
	 *	a T_UNITDATA_IND
	 */
	switch (gbuf->stBuf.PRIM_type) {
	case T_UNITDATA_IND:

		*ipxAddress = (gbuf->ipxAddress);

		/*
		 * Trap out NCP_SERVER_BUSY here, so that we can hide this
		 * behaviour, and let our retransmission strategy account
		 * for this problem.
		 */

		if ( ((NCP_REPLY_HEADER_T *)buffer->buffer)->replyType == 
			NCP_SERVER_BUSY ) {
#ifdef NUC_DEBUG
			IPXENG_CMN_ERR(CE_CONT, "IPXEng: Server busy received! \n");
#endif NUC_DEBUG
			diagnostic = NWD_GTS_BLOCKING;
			break;
		}
		/*
		if ( (((NCP_REPLY_HEADER_T *)buffer->buffer)->replyType == 
			BIG_FILE_SERVICE_REQUEST) &&
			(((struct packetBurstHeader *)
			buffer->buffer)->connectionControl & SYSTEM_PROCESSING_BIT) ) {
#ifdef NUC_DEBUG
			IPXENG_CMN_ERR(CE_CONT, "IPXEng: Burst Server busy received! \n");
#endif NUC_DEBUG
			diagnostic = NWD_GTS_BLOCKING;
			break;
		}
		*/
		/*
			Packet Burst Server Busy packets are detected in 
			IPXEngInterruptHandlerBurst.
		*/
		if ( ((NCP_REPLY_HEADER_T *)buffer->buffer)->replyType == 
			BIG_FILE_SERVICE_REQUEST) {
			diagnostic = SUCCESS;
			break;
		}
		if ((((NCP_REPLY_HEADER_T *) buffer->buffer)->connectionStatus & 
				(NCP_LOST_CONNECTION))
#ifdef IS_THIS_WRONG_THING_TO_DO
			|| (((NCP_REPLY_HEADER_T *) buffer->buffer)->connectionStatus &
				(NCP_SERVER_GOING_DOWN))
#endif IS_THIS_WRONG_THING_TO_DO
			) {
			/*
			 * Reply indicates that the connection is no longer
			 * valid.
			 */
#ifdef NUC_DEBUG
			IPXENG_CMN_ERR(CE_CONT, "IPXEng: Connection not valid received! \n");
#endif NUC_DEBUG
			diagnostic = NWD_GTS_TIMED_OUT;
			break;
		}

		/*
		 * Packet Looks Good
		 */
		diagnostic = SUCCESS;
		break;

	case T_UDERROR_IND:
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"ipxEng: T_UDERROR_IND tpi prim received in packet\n");
#endif	NUC_DEBUG
		*ipxAddress = (((T_UD_ERROR_HDR_T *)control.buffer)->ipxAddress);
		diagnostic = NWD_GTS_BAD_ADDRESS;
		break;

	default:
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"ipxEng: Unknown tpi prim = %d(dec) received in packet\n");
#endif	NUC_DEBUG
		diagnostic = NWD_GTS_NO_MESSAGE;
		break;
	}

	return( NTR_LEAVE( diagnostic ) );
}



/*
 * BEGIN_MANUAL_ENTRY(IPXEngFlushCurrentIPCPacket(3K), \
 *			./man/kernel/ts/ipxeng/FLushCurrentIPCPacket)
 * NAME
 *	IPXEngFlushCurrentIPCPacket
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFlushCurrentIPCPacket( ipcChannel )
 *
 *	opaque_t	*ipcChannel;
 *
 * INPUT
 *	ipcChannel	- Ipc channel associated with an IPX socket
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	Calls GIPC with this channel and parameters informing it to 
 *	free the current message at the head of the ipc for this socket
 *
 * NOTES
 *	Only flushes the packet on the read head.
 *
 * SEE ALSO
 *	IPXEngOpenIPCChannel(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFlushCurrentIPCPacket (opaque_t *ipcChannel )
{
	int32	diagnostic;

	NTR_ENTER(1, ipcChannel, 0, 0, 0, 0);

	GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &diagnostic );
	return( NTR_LEAVE( SUCCESS ) );
}

#ifdef UNDEF

/*
 * BEGIN_MANUAL_ENTRY(IPXEngReSendIPCPacket(3K), \
 *			./man/kernel/ts/ipxeng/ReSendIPCPacket)
 * NAME
 *	IPXEngReSendIPCPacket - Re-transmit a previously sent packet 
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngReSendIPCPacket( ipcChannel, ipcPacketHandle )
 *
 *	opaque_t	*ipcChannel;
 *	opaque_t	*ipcPacketHandle;
 *
 * INPUT
 *	ipcChannel	- Ipc channel associated with an IPX socket
 *	ipcPacketHandle	- Handle to a GIPC message containing the
 *			  NCP request to retransmit.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	SUCCESS			- Successful completion.
 *	*			- GIPC error mapped to GTS error.
 *
 * DESCRIPTION
 *	Re-sends a packet previously sent via the IPXEngSendIPCPacket()
 *	function by means of the ipcPacketHandle that was returned by
 *	the IPXEngSendIPCPacket() function when the packet was initially
 *	transmitted.
 *
 * NOTES
 *	This function operates out of process context.  It runs in the 
 *	call out context off the timer experation of a outstanding NCP
 *	request.
 *
 * SEE ALSO
 *	IPXEngSendIPCPacket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngReSendIPCPacket (opaque_t *ipcChannel, opaque_t *ipcPacketHandle )
{
	int32	diagnostic;

	NTR_ENTER(2, ipcChannel, ipcPacketHandle, 0, 0, 0);

	GIPC_RESEND( ipcChannel, ipcPacketHandle, &diagnostic);
	if (diagnostic) {
		IPXEngMapIPCDiagnostic(&diagnostic);
		return( NTR_LEAVE( diagnostic ) );
	} else {
		return( NTR_LEAVE( SUCCESS ) );
	}
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFreeIPCPacket(3K), \
 *			./man/kernel/ts/ipxeng/FreeIPCPacket)
 * NAME
 *	IPXEngFreeIPCPacket - Free packet data allocated by IPC mechanism
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFreeIPCPacket( ipcChannel, ipcPacketHandle )
 *	opaque_t	*ipcChannel;
 *	opaque_t	*ipcPacketHandle;
 *
 * INPUT
 *	ipcChannel	- Ipc channel associated with an IPX socket
 *	ipcPacketHandle	- Handle to a GIPC message to free.
 *
 * OUTPUT
 *	Nothing
 *
 * RETURN VALUES
 *	SUCCESS		- Successful completion.
 *
 * DESCRIPTION
 *	Frees IPC memory resources that were allocated when a message
 *	is sent to the IPC.  The IPC mechanism is required to duplicate
 *	the message prior to transmission and return the handle to the
 *	caller in order to provide a means of re-transmission if no
 *	acknowledgement is received on an in-band socket.
 *
 * NOTES
 *
 * SEE ALSO
 *	IPXEngSendIPCPacket(3k), IPXEngReSendIPCPacket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFreeIPCPacket (opaque_t *ipcChannel, opaque_t *ipcPacketHandle)
{
	int32	diagnostic;

	NTR_ENTER(2, ipcChannel, ipcPacketHandle, 0, 0, 0);

	GIPC_FREE( ipcChannel, ipcPacketHandle, &diagnostic );


	return( NTR_LEAVE( SUCCESS ) );
}
#endif UNDEF


/*
 * BEGIN_MANUAL_ENTRY(IPXEngMapIPCDiagnostic(3K), \
 *			./man/kernel/ts/ipxeng/MapIPCDiagnostic)
 * NAME
 *		IPXEngMapIPCDiagnostic - Map a GIPC layer diagnostic into a
 *					 GTS layer diagnostic.
 *
 * SYNOPSIS
 *	void_t
 *	IPXEngMapIPCDiagnostic( diagnositc )
 *	int32	*diagnositc;
 *
 * INPUT
 *	diagnostic	- A GIPC diagnostic. (See nucerror.h).
 *
 * OUTPUT
 *	diagnostic	- A GTS diagnostic. (See nucerror.h).
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	Maps a GIPC Layer diagnostic into a GTS layer diagnostic for 
 *	subsequent return to the GTS consumer.  The NUC architecture requires
 *	a layer to return only its advertised diagnostics in order to preserve
 *	the abstractions between layers.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
IPXEngMapIPCDiagnostic (int32 *diagnostic )
{

	switch ( *diagnostic ) {

	case NWD_GIPC_BAD_IPC:
		*diagnostic = NWD_GTS_NO_STACK;
		break;

	case NWD_GIPC_BAD_PEER:
		*diagnostic = NWD_GTS_NO_STACK;
		break;

	case NWD_GIPC_BIG_MSG:
		*diagnostic = NWD_GTS_MSG_OVERFLOW;
		break;

	case NWD_GIPC_BLOCKING:
		*diagnostic = NWD_GTS_BLOCKING;
		break;

	case NWD_GIPC_BUF_FAULT:
		*diagnostic = NWD_GTS_BUF_FAULT;
		break;

	case NWD_GIPC_IOCTL_FAIL:
		*diagnostic = NWD_GTS_NO_RESOURCE;
		break;

	case NWD_GIPC_NO_CHANNEL:
		*diagnostic = NWD_GTS_NOT_CONNECTED;;
		break;

	case NWD_GIPC_NO_IPC:
		*diagnostic = NWD_GTS_NO_STACK;;
		break;

	case NWD_GIPC_NO_MESSAGE:
		*diagnostic = NWD_GTS_NO_MESSAGE;;
		break;

	case NWD_GIPC_NO_RESOURCE:
		*diagnostic = NWD_GTS_NO_RESOURCE;
		break;

	case NWD_GIPC_PEER_REJECT:
		*diagnostic = NWD_GTS_NO_RESOURCE;
		break;

	default:
		*diagnostic = NWD_GTS_NO_RESOURCE;
		break;

	}
}
