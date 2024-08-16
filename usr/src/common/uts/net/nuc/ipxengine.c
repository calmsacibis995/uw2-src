/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengine.c	1.28"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengine.c,v 2.62.2.10 1995/02/13 20:58:53 doshi Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE:
 *		ipxengine.c
 *
 *	ABSTRACT:
 *		The main body of the ipxeng package.  These functions represent
 *		the service interface operations loaded into the NWgtsOpsSw[].
 *		
 *
 *	Functions declared in this module:
 *
 *	Public Functions:
 *		IPXEngInitialize
 *		IPXEngOpenEndpoint
 *		IPXEngCloseEndpoint
 *		IPXEngConnect
 *		IPXEngDisconnect
 *		IPXEngGetEndpointState
 *		IPXEngPreviewPacket
 *		IPXEngSendPacket
 *		IPXEngGetPacket
 *		IPXEngRegisterAsyncHandler
 *		IPXEngInterruptHandler
 *		IPXEngInterruptHandlerBurst
 *		IPXEngMaintainFragmentList
 *		IPXEngObtainTransportInfo
 *
 *	Private functions:
 */

#ifdef _KERNEL_HEADERS
#include <util/types.h>
extern char qrunflag;
#include <io/strsubr.h>
#include <util/param.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <io/stream.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengtune.h>
#include <net/nuc/ipxengcommon.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gipcmacros.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/requester.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>

#else _KERNEL_HEADERS

extern char qrunflag;
#include <sys/strsubr.h>
#include <sys/time.h>
#include <sys/ipx_app.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nuctool.h>
#include <sys/ipxengine.h>
#include <sys/ipxengtune.h>
#include <sys/ipxengcommon.h>
#include <sys/gtscommon.h>
#include <sys/gipccommon.h>
#include <sys/gipcchannel.h>
#include <sys/gtsendpoint.h>
#include <sys/gipcmacros.h>
#include <sys/nucerror.h>
#include <sys/ncpconst.h>
#include <sys/ncpiopack.h>
#include <sys/requester.h>
#include <sys/nuc_prototypes.h>
#include <sys/debug.h>

#endif _KERNEL_HEADERS

#define NTR_ModMask	NTRM_ipxeng

/*
 *	External variables
 */

extern rwlock_t	*nucTSLock;

/*
 *	global variables
 */
uint32	writeBurstTimeoutTicks = 0;

/*
 * Forward Reference Functions
 */
extern	void_t	IPXEngConnectionTimeout();
extern ccode_t IPXEngMaintainFragmentList();
extern void_t IPXEngWriteBurstClockInterrupt();


/*
 * BEGIN_MANUAL_ENTRY(IPXEngInitialize(3K), \
 *		./man/kernel/ts/ipxeng/Initialize)
 * NAME
 *	IPXEngInitialize -	Initialize IPXEngine
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngInitialize(mode)
 *
 *	uint32	mode;
 *
 * INPUT
 *	mode	- GTS_START requests IPX to prepare for client usage.  GTS_STOP
 *		  requests IPX to stop client usage.  
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0	- Success Completion.
 *
 * DESCRIPTION
 *	Used to either start or stop the IPXengine layer.  GTS bleeds the 
 *	start or stop down to this dependent TS layer.  Represents the the
 *	IPX dependent portion of a NWtsInitializeTransportService(3K).
 *
 * NOTES
 *	The GTS_STOP is ignored at this time.  It is assumed that simply
 *	reinitializing the tables will cause a GTS_START to begin clean
 *	if necessary.  Also, the ipxenhalt() is used to make the IPX layer
 *	ready to die at this time.
 *
 * SEE ALSO
 *	NWtsInitializeTransportService(3K), NWtsIntroduction(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngInitialize(mode)
uint32	mode;
{
	NTR_ENTER( 1, mode, 0, 0, 0, 0);

	if (mode == GTS_START) {
		IPXEngInitTables();
	}
#ifdef UNDEF
	else {
		IPXEngReInitTables();
	}
#endif UNDEF

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngOpenEndpoint(3K), \
 *		./man/kernel/ts/ipxeng/OpenEndpoint)
 * NAME
 *	IPXEngOpenEndpoint -	Open IPX transport endpoint
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngOpenEndpoint( serviceMode, localAddress, eventMode, credPtr, 
 *				endPoint, diagnostic )
 *	uint32		serviceMode;
 *	uint8		*localAddress;	
 *	uint32		eventMode;
 *	opaque_t	*credPtr;
 *	GTS_ENDPOINT_T	**endPoint;
 *	int32		*diagnostic;
 *
 * INPUT
 *	serviceMode	- Ingnored, assumed to be GTS_REQUESTOR.
 *	localAddress	- Ignored, assumed to be 0 for ephemeral.
 *	eventMode	- Ignored, assumed to be GTS_SYNC.
 *	credPtr
 *      credPtr         - Credentials of the client opening the the End Point.
 *                        This is an opaque data strucuture which
 *                        is managed by the 'nuctools'.
 *
 * OUTPUT
 *      endPoint	 - A pointer to the GTS Transport Endpoint
 *                          object opened.
 *
 *      diagnostic        - Set to one of the following  when return
 *                          value <0.
 *
 *                        [NWD_GTS_ADDRESS_INUSE] - The 'localAddress' is
 *                                                  currently in use.
 *
 *                        [NWD_GTS_BAD_ADDRESS]   - The 'localAddress' is
 *                                                  not valid with the
 *                                                  'realStack'.
 *
 *                        [NWD_GTS_NO_STACK]      - The IPX Stack can not be
 *						    connected with.
 *
 *                        [NWD_GTS_NO_RESOURCE]   - An EndPoint could not be
 *                                                  opened due to a resource
 *                                                  shortage.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'IPXEngOpenEndpoint' opens a endpoint with the IPX stack for
 *	client (requestor) service by NCP.  The endpoint represents a logical
 *	connection to IPX (Note: IPX is actually connectionless), which is
 *	multiplexed into a socket set for the client (i.e. UNIX Credentials).
 *	The endpoint will be connected when a destination IPX address is loaded
 *	into it via the IPXEngConnect(3K) operation.  This endpoint is used
 *	to send and receive NCP messages with a NetWare Server.  If a client 
 *	socket set has not been allocated, one will be allocated at this time.
 *	Represents the the IPX dependent portion of a
 *	NWtsOpenTransportEndpoint(3K).
 *
 * NOTES
 *	Currently, serviceMode, eventMode, and localAddress are ignored
 *	As the assumption will be:
 *
 *	serivceMode = GTS_REQUESTOR 
 *	eventMode = GTS_SYNC 
 *	localAddress = NULL
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *      NWtsIntroduction(3K), IPXEngCloseEndpoint(3K), IPXEngConnect(3K),
 *      NWtsOpenTransportEndPoint(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngOpenEndpoint( serviceMode, localAddress, eventMode, credPtr, 
			endPoint, diagnostic )
uint32		serviceMode;
uint8		*localAddress;	
uint32		eventMode;
opaque_t	*credPtr;
GTS_ENDPOINT_T	**endPoint;
int32		*diagnostic;
{
	ccode_t		ccode;
	ipxTask_t	*taskHandle;
	ipxClient_t	*client;
	pl_t		pl;

	NTR_ENTER( 5, serviceMode, localAddress, eventMode, credPtr, endPoint);

	*diagnostic = SUCCESS;

	/*
	 *	If the client structure is not allocated for this
	 *	process' credentials, allocate a new one
	 */

	pl = RW_WRLOCK (nucTSLock, plstr);
	if (IPXEngFindClient_l ( clientList, credPtr, &client )) {
		if (ccode = IPXEngAllocClient_l ( clientList, &client )) {
			RW_UNLOCK (nucTSLock, pl);
			*diagnostic = NWD_GTS_NO_RESOURCE;
			NTR_PRINTF( "IPXEngOpenEndpoint: cant alloc CLIENT\n", 0, 0, 0 );
			NTR_LEAVE( NWD_GTS_NO_RESOURCE);
			return( FAILURE );
		}

		client->numTasks = 0;
		if (((nwcred_t *)credPtr)->flags & NWC_OPEN_PRIVATE) {
			client->state |= IPX_CLIENT_PRIVATE;
		}
		/*
		 *	Duplicate the credentials structure so a copy of it can
		 *	be kept in this layer
		 */
		if (NWtlDupCredStruct (credPtr,
				(nwcred_t **)(&(client->credentialsPtr)))) {
			NTR_PRINTF("IPXEng; DupCred failed due to mem exhaustion!\n",0,0,0);
			IPXEngFreeClient_l ( clientList, client );
			RW_UNLOCK (nucTSLock, pl);

			*diagnostic = NWD_GTS_NO_RESOURCE;
			NTR_LEAVE( NWD_GTS_NO_RESOURCE);
			return( FAILURE );
		}

		/*
		 *	Call specific IPC here to load the sockets
		 */
		RW_UNLOCK (nucTSLock, pl);
		if (ccode = IPXEngAllocateSocketSet ( client )) {
			pl = RW_WRLOCK (nucTSLock, plstr);
			IPXEngFreeClient_l ( clientList, client );
			RW_UNLOCK (nucTSLock, pl);
			*diagnostic = ccode;
			NTR_PRINTF( "IPXEngOpenEndpoint: alloc socket failed\n", 0, 0, 0 );
			NTR_LEAVE( ccode);
			return( FAILURE );
		}
		pl = RW_WRLOCK (nucTSLock, plstr);

	}

	if (ccode = IPXEngAllocTask_l (client->taskList, &taskHandle)) {
		RW_UNLOCK (nucTSLock, pl);
		NTR_PRINTF( "IPXEngOpenEndpoint: cant alloc task\n", 0, 0, 0 );
		*diagnostic = ccode;
		NTR_LEAVE( ccode);
		return( FAILURE );
	}

	/*
	 *	Increment task counter for this client
	 */
	client->numTasks++;
	RW_UNLOCK (nucTSLock, pl);

	/*
	 *	Attach the task (endpoint) to the client socket set
	 */

	pl = LOCK (taskHandle->taskLock, plstr);

	taskHandle->clientPtr = client;
#ifdef	NUC_DEBUG
	IPXENG_CMN_ERR(CE_CONT, 
		"IPXEngOpenEndpoint: ipxTask_t at %x\n", taskHandle);
#endif
	NTR_PRINTF( "IPXEngOpenEndpoint: ipxTask_t at %x\n", taskHandle, 0, 0 );

	/*
	 * Return the GTS EndPoint component
	 */
	*endPoint = &(taskHandle->gtsEndPoint);

	UNLOCK (taskHandle->taskLock, pl);

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngCloseEndpoint(3K), \
 *		./man/kernel/ts/ipxeng/CloseEndpoint)
 * NAME
 *	IPXEngCloseEndpoint -	Terminate connection.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngCloseEndpoint( endPoint, diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint	 - A pointer to the GTS Transport EndPoint
 *                         object to close.
 *
 * OUTPUT
 *	diagnostic	- Not used at this time, always returns success.
 *
 * RETURN VALUES
 *	0		- Successful.
 *
 * DESCRIPTION
 *      The 'IPXEngCloseEndpoint' closes the specified EndPoint when it
 *      is no longer in use.  The EndPoint will be dismantled, and is longer
 *      useable upon return.  This encompases unlinking the endpoint from the
 *	client socket set it was multiplexed into.  Represents the the IPX
 *	dependent portion of a NWtsCloseTransportEndpoint(3K).
 *
 * NOTES
 *	Check the status of the task and cancel any outstanding requests
 *	at the transport.  If the endpoint is the last one for the
 *	last client, disable the skulker as well.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		channel->connRWLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		channel->connRWLock
 *
 * SEE ALSO
 *      NWtsIntroduction(3K), IPXEngOpenEndpoint(3K), IPXEngDisconnect(3K),
 *      NWtsCloseTransportEndPoint(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngCloseEndpoint (GTS_ENDPOINT_T *endPoint, int32 *diagnostic )
{
	ipxClient_t	*clientPtr;
	ipxTask_t	*taskHandle;
	pl_t		pl;

	NTR_ENTER( 2, endPoint, diagnostic, 0, 0, 0);

	*diagnostic = SUCCESS;

	taskHandle = (ipxTask_t *) endPoint->realEndPoint;
	clientPtr = taskHandle->clientPtr;

#ifdef	NUC_DEBUG
	IPXENG_CMN_ERR(CE_CONT, 
			"IPXEngCloseEndpoint: Closing ipxTask_t at %x\n", taskHandle);
#endif
	NTR_PRINTF("IPXEngCloseEndpoint: Closing ipxTask_t at %x\n",
			taskHandle,0,0);

	pl = RW_WRLOCK (nucTSLock, plstr);

	IPXEngFreeTask_l ( clientPtr->taskList, taskHandle );
	
	--(clientPtr->numTasks);

	if ( clientPtr->numTasks <= 0) {
		/*
		 *	No more clients, clean up...
		 */
		RW_UNLOCK (nucTSLock, pl);
		IPXEngFreeSocketSet ( clientPtr );
		pl = RW_WRLOCK (nucTSLock, plstr);
		NWtlFreeCredStruct( clientPtr->credentialsPtr );
		IPXEngFreeClient_l ( clientList, clientPtr );

	}

	RW_UNLOCK (nucTSLock, pl);

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngConnect(3K), \
 *		./man/kernel/ts/ipxeng/Connect)
 * NAME
 *	IPXEngConnect -	Perform logical connection to endpoint
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngConnect( endPoint, address, diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	uint8		*address;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint       - A pointer to the GTS Transport EndPoint
 *                         object which is to be connected to the
 *                         specified NetWare Server.
 *
 *      address          - A pointer NET:NODE:SOCKET of the NetWare Server
 *			   to be connected with.
 *
 * OUTPUT
 *      diagnostic      - Set to one of the following  when return
 *                         value <0.
 *
 *                        [NWD_GTS_CONNECTED]     - The 'IPXEndPoint' is
 *                                                  currently connected to a
 *                                                  NetWare Server.
 *
 * RETURN VALUES
 *      0       - Successful completion.
 *
 *      -1      - Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'IPXEngConnect' is used to connect the NUC NCP layer with a 
 *	specified NetWare Server. This entails loading the specified NetWare
 *	Server's address into the endpoint which emulates connection oriented
 *	service. Represents the the IPX dependent portion of a
 *	NWtsConnectToPeer(3K).
 *
 * NOTES
 *	Load the destination endpoint for use later during send and
 *	receipt.
 *
 * SEE ALSO
 *      NWtsIntroduction(3K), IPXEngDisconnect(3K), IPXEngOpenEndpoint(3K),
 *      NWtsConnectToPeer(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngConnect (GTS_ENDPOINT_T *endPoint, uint8 *address, int32 *diagnostic )
{
	ipxTask_t	*taskHandle;
	ipxTask_t	*existTask;
	pl_t		pl;

	NTR_ENTER( 3, endPoint, address, diagnostic, 0, 0);

	*diagnostic = SUCCESS;


	taskHandle = (ipxTask_t *) endPoint->realEndPoint;
	pl = RW_WRLOCK (nucTSLock, plstr);

	/*
	 *	Connect sets the address, which is the unique indicator for
	 *	each task.  If there is already a task with this address,
	 *	the connect will fail which should cause the caller to dismantle
	 *	the endpoint associated with taskHandle.
	 */
	if (IPXEngFindTask_l (taskHandle->clientPtr->taskList,
			(ipxAddress_t *)address, &existTask)){
		bcopy ( address, taskHandle->address.ipxAddr, IPX_ADDR_SIZE );
		taskHandle->state |= IPX_TASK_CONNECTED;
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXEngConnect: ipxTask_t at %x\n", taskHandle);
#endif	NUC_DEBUG

		NTR_PRINTF( "IPXEngConnect: ipxTask_t at %x\n", taskHandle, 0, 0 );
		RW_UNLOCK (nucTSLock, pl);

		return( NTR_LEAVE( SUCCESS ) );
	}

	RW_UNLOCK (nucTSLock, pl);

	NTR_PRINTF( "IPXEngConnect: task already exists at %x\n", existTask, 0, 0 );
	*diagnostic = NWD_GTS_CONNECTED;

	NTR_LEAVE( NWD_GTS_CONNECTED);
	return( FAILURE );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngDisconnect(3K), \
 *		./man/kernel/ts/ipxeng/Disconnect)
 * NAME
 *	IPXEngDisconnect -	Perform logical disconnection to endpoint
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngDisconnect( endPoint, diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint       - A pointer to the GTS Transport EndPoint
 *                         object which is to be dis-connected from a
 *                         NetWare Server.
 *
 * OUTPUT
 *	diagnostic	- Not used at this time, always returns success.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 * DESCRIPTION
 *	The 'IPXEngDisconnect' is used to disconnect the NUC NCP layer from a 
 *	NetWare Server. This entails clearing the specified NetWare
 *	Server's address from the endpoint which emulates connection oriented
 *	service. Represents the the IPX dependent portion of a
 *	NWtsDisConnectFromPeer(3K).
 *
 * NOTES
 *
 * SEE ALSO
 *      NWtsIntroduction(3K), IPXEngConnect(3K), IPXEngCloseEndpoint(3K),
 *      NWtsDisConnectFromPeer(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngDisconnect (	GTS_ENDPOINT_T	*endPoint,
					int32			*diagnostic )
{
	ipxTask_t	*taskHandle;
	pl_t		pl;

	NTR_ENTER( 2, endPoint, diagnostic, 0, 0, 0);

	*diagnostic = SUCCESS;

	pl = RW_WRLOCK (nucTSLock, plstr);

	taskHandle = (ipxTask_t *)endPoint->realEndPoint;

#ifdef	NUC_DEBUG
	IPXENG_CMN_ERR(CE_CONT, 
		"IPXEngDisconnect: ipxTask_t at %x\n", taskHandle);
#endif	NUC_DEBUG

	NTR_PRINTF( "IPXEngDisconnect: ipxTask_t at %x\n", taskHandle, 0, 0 );
	bzero ( (char *)(taskHandle->address.ipxAddr), IPX_ADDR_SIZE );
	taskHandle->state &= ~(IPX_TASK_CONNECTED);

	RW_UNLOCK (nucTSLock, pl);

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngPreviewPacket(3K), \
 *		./man/kernel/ts/ipxeng/PreviewPacket)
 * NAME
 *	IPXEngPreviewPacket	- Peek at a received NCP Packet.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngPreviewPacket( endPoint, frag1, frag2, msgType, diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	NUC_IOBUF_T	*frag1;
 *	NUC_IOBUF_T	*frag2;
 *	int32		*msgType;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint       - A pointer to the GTS Transport EndPoint
 *                         object which is to used to preview a message on.
 *
 * OUTPUT
 *	frag1		  - Not Used.
 *	frag2		  - Not Used.
 *	msgType		  - Not Used.
 *      diagnostic        - Always set to.
 *
 *                        [NWD_GTS_NO_MESSAGE]   - The operation is not
 *					    	   supported.
 *
 * RETURN VALUES
 *	-1	- Unsuccessful completion.
 *
 * DESCRIPTION
 *      The 'NWtsPreviewMessageFromPeer' maps the first two contiguous Generic
 *      InterProcess Communication blocks of the message into the fragment
 *      points specified by the caller.  Represents the the IPX dependent
 *	portion of a NWtsPreviewMessageFromPeer(3K).
 *
 *
 * NOTES
 *	This is not supported currently as async is not supported by IPX Engine
 *	currently.
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngGetPacket(3K), IPXEngSendPacket(3K),
 *	NWtsPreviewMessagesFromPeer(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngPreviewPacket(	ipxTask_t	*taskHandle,
						NUC_IOBUF_T	*frag1,
						NUC_IOBUF_T	*frag2,
						int32		*msgType,
						int32		*diagnostic )
{
	NTR_ENTER( 5 , taskHandle, frag1, frag2, msgType, diagnostic);

	*diagnostic = NWD_GTS_NO_MESSAGE;
	NTR_LEAVE( NWD_GTS_NO_MESSAGE);
	return( FAILURE );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngSendPacket(3K), \
 *		./man/kernel/ts/ipxeng/SendPacket)
 * NAME
 *		IPXEngSendPacket -	Pass the packet data to the IPX
 *					transport STACK for placement on the
 *					wire.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngSendPacket( endPoint, frag1, frag2, msgType, diagnostic )
 *	GTS_ENDPOINT_T	*endPoint;
 *	NUC_IOBUF_T	*frag1,
 *	NUC_IOBUF_T	*frag2;
 *	int32		msgType;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint            - A pointer to the GTS Transport Endpoint
 *                              connected to the NetWare server the NCP
 *				Request is for.
 *
 *      frag1->buffer         - A pointer to a contiguous block to send.
 *
 *      frag1->bufferLength   - Size in bytes of 'frag1->buffer'.
 *
 *      frag1->kernelResident - Set to TRUE for buffers which are resident
 *                              in kernel space, and FALSE for buffers which
 *                              are resident in USER space.
 *
 *      frag2->buffer         - A pointer to a contigous block which is to be
 *                              concatenated with 'frag1->buffer'.  Not
 *                              Used if 'frag2' is NULL.
 *
 *      frag2->bufferLength   - Size in bytes of 'frag2->buffer'.  Not
 *                              Used if 'frag2' is NULL.
 *
 *      frag2->kernelResident - Set to TRUE for buffers which are resident
 *                              in kernel space, and FALSE for buffers which
 *                              are resident in USER space.
 *
 *      msgType               - Set to GTS_IN_BAND for Data Messages, and to
 *                              GTS_OUT_OF_BAND for Out of Band Messages.
 *
 * OUTPUT
 *      diagnostic      - Set to one of the following  when return
 *                        value <0.
 *
 *                        [NWD_GTS_BUF_FAULT]     - The 'fragment1' or 'fragment
2'
 *                                                  located in the USER Memory
 *                                                  Space generated a EFAULT.
 *
 *                        [NWD_GTS_NO_RESOURCE]    - The NCP packet could not
 *						     be sent to a critical 
 *						     resource shortage in GIPC.
 *
 *                        [NWD_GTS_TIMED_OUT]     - The 'IPXEndPoint' timed out
 *						    attempting to deliver 
 *						    NCP packet to NetWare
 *						    Server.
 *
 * RETURN VALUES
 *      0       - Successful completion.
 *
 *      -1      - Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'IPXEngSendPacket' attempts to send a NCP request from the UNIX
 *	Client to the NetWare Server over the IPX Endpoint.  The NCP request
 *	may be contained in two buffers, frag1 normally is the NCP header, 
 *	and frag2 is the NCP data.  The two buffers will be coallesced into
 *	one NCP packet.  Represents the the IPX dependent portion of a
 *	NWtsSendMessageToPeer(3K).
 *
 * NOTES
 *	Check the state of the task to see if the skulker has changed
 *	it due to watchdog failure, or timeout.  If so, error out here
 *	indicating to the caller that they should dismantle the channel
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		ncp_channel->connRWLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		ncp_channel->connRWLock
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngGetPacket(3K), IPXEngConnect(3K),
 *	NWtsSendMessageToPeer(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngSendPacket (	GTS_ENDPOINT_T *endPoint,
					NUC_IOBUF_T *frag1,
					NUC_IOBUF_T *frag2,
					uint32 checksumTrigger,
					int32 msgType,
					int32 *diagnostic,
					uint8 *signature )
{
	ccode_t		ccode;
	ipxTask_t	*taskHandle;
	ipxSocket_t	*socket;
	ipxAddress_t	destAddress;
	pl_t		pl;

	NTR_ENTER( 5, endPoint, frag1, frag2, checksumTrigger, msgType);

	*diagnostic = SUCCESS;

	taskHandle = (ipxTask_t *)endPoint->realEndPoint;

	pl = LOCK (taskHandle->taskLock, plstr);

	if (taskHandle->state & IPX_TASK_TIMEDOUT) {
		*diagnostic = NWD_GTS_TIMED_OUT;
		taskHandle->clientPtr->stats.ipxengWriteErrorCount++;
		/*
		 * Make sure the GTS task is not in connected mode.
		 */
		taskHandle->state &= ~(IPX_TASK_CONNECTED);
		UNLOCK (taskHandle->taskLock, pl);
		NTR_LEAVE( NWD_GTS_TIMED_OUT);
		return( FAILURE );
	}

	if (!(taskHandle->state & IPX_TASK_CONNECTED) ) {
		*diagnostic = NWD_GTS_NOT_CONNECTED;
		taskHandle->clientPtr->stats.ipxengWriteErrorCount++;

		UNLOCK (taskHandle->taskLock, pl);
		NTR_LEAVE( NWD_GTS_NOT_CONNECTED );
		return( FAILURE );
	}

	/*	If the state is IPX_REPLY_TIMEDOUT then is is a retransmission
	 *	and we don't want to restart the round trip timer.
	 */
	if( taskHandle->state & IPX_REPLY_TIMEDOUT ){
		taskHandle->state &= ~IPX_REPLY_TIMEDOUT;
	}else{
		GET_HRESTIME_EXACT (&taskHandle->roundTripStart);

		/*
		 *	Change task state to indicate that a packet is currently
		 *	on the wire.  
		 */
		taskHandle->state |= IPX_TASK_TRANSMIT;
		if( ((ncp_channel_t*)taskHandle->channel)->flags &
		  PACKET_BURST_TRANSACTION ){
			taskHandle->state |= IPX_TASK_BURST;
		}
	}

	if( taskHandle->state & IPX_TASK_BURST ){
		socket = &(taskHandle->clientPtr->socketSet.packetBurstSocket);
	} else {
		socket = &(taskHandle->clientPtr->socketSet.serviceSocket);
	}

	bcopy (taskHandle->address.ipxAddr, destAddress.ipxAddr, 
				IPX_ADDR_SIZE);

	if (taskHandle->reTransBeta < 1) {
		taskHandle->reTransBeta = 1;
	}

	/*	IPXTASK_HOLD (taskHandle);	*/

	UNLOCK (taskHandle->taskLock, pl);

	/*
	 *	Call the IPC to transmit the packet to the wire.
	 */
	if ( ccode = IPXEngSendIPCPacket(socket->ipcChannel, destAddress.ipxAddr, 
			frag1, frag2, IPX_NOVELL_PACKET_TYPE, checksumTrigger, msgType, 
			&(taskHandle->ipcMsgHandle), signature)) {
		NTR_PRINTF( "IPXEngSendPacket: Error %x from SendIPC\n", ccode, 0, 0 );

		pl = LOCK (taskHandle->taskLock, plstr);
		/*	IPXTASK_RELE (taskHandle);	*/
		taskHandle->state &= ~(IPX_TASK_TRANSMIT | IPX_TASK_BURST);
		UNLOCK (taskHandle->taskLock, pl);

		*diagnostic = ccode;
		return( NTR_LEAVE( FAILURE ) );
	}

	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngGetPacket(3K), \
 *		./man/kernel/ts/ipxeng/GetPacket)
 * NAME
 *	IPXEngGetPacket -	Retrieve packet data from a transport endpoint
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngGetPacket( endPoint, frag1, frag2, msgType, diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	NUC_IOBUF_T	*frag1;
 *	NUC_IOBUF_T	*frag2;
 *	int32		*msgType;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint            - A pointer to the GTS Transport Endpoint
 *                              connected to the NetWare server the NCP
 *				Response is coming from.
 *
 *      frag1->buffer         - A pointer to a contiguous receive block.
 *
 *      frag1->bufferLength   - Size in bytes of 'frag1->buffer'.
 *
 *      frag1->kernelResident - Set to TRUE for buffers which are resident
 *                              in kernel space, and FALSE for buffers which
 *                              are resident in USER space.
 *
 *      frag2->buffer         - A pointer to a contigous block which is to be
 *                              copied into upon filling 'frag1->buffer'.
 *
 *      frag2->bufferLength   - Size in bytes of 'frag2->buffer'.  Not
 *                              Used if 'frag2' is NULL.
 *
 *      frag2->kernelResident - Set to TRUE for buffers which are resident
 *                              in kernel space, and FALSE for buffers which
 *                              are resident in USER space.
 *
 *
 * OUTPUT
 *      frag1->Length         - Size in bytes of message copied into
 *                              'frag1->buffer'.
 *
 *      frag2->Length         - Size in bytes of message copied into
 *                              'frag2->buffer'.
 *
 *      msgType               - Set to GTS_IN_BAND for Data Messages, and to
 *                              GTS_OUT_OF_BAND for Out of Band Messages.
 *
 *      diagnostic            - Set to one of the following  when return
 *                              value <0.
 *
 *                        [NWD_GTS_BUF_FAULT]     - The 'frag1' or 'frag2'
 *                                                  located in the USER Memory
 *                                                  Space generated a EFAULT.
 *
 *                        [NWD_GTS_MSG_OVERFLOW]  - The message size exceeded
 *                                                  the receive fragments
 *					  	    combined size, the fragments
 *						    have been filled, the
 *						    residual has been discarded.
 *
 *                        [NWD_GTS_NO_MESSAGE]    - There is no message
 *						    available on the
 *						    'IPXEndpoint'.
 *
 *                        [NWD_GTS_TIMED_OUT]     - The 'IPXEndpoint'
 *                                                  connection has timed out,
 *                                                  peer is assumed dead, the
 *                                                  EndPoint is left open for
 *						    subsequent attempts.
 *
 * RETURN VALUES
 *      0       - Successful completion.
 *
 *      -1      - Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'IPXEngGetPacket' receives NCP responses from a NetWare Server
 *	over the IPX Endpoint.  The NCP response may be fragmented into two
 *	buffers, frag1 normally is the NCP header, and frag2 is the NCP data.
 *	Represents the the IPX dependent portion of a
 *	NWtsReceiveMessageFromPeer(3K).
 *
 * NOTES
 *	Check the state of the task to see if the skulker has changed
 *	it due to watchdog failure, or timeout.  If so, error out here
 *	indicating to the caller that they should dismantle the channel
 *
 *	Call down to the IPC to see if the packet has arrived. If not,
 *	and this is running in SYNC, block the process and return.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngSendPacket(3K), IPXEngConnect(3K),
 *	NWtsReceiveMessageFromPeer(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngGetPacket (	GTS_ENDPOINT_T	*endPoint,
					NUC_IOBUF_T		*frag1,
					NUC_IOBUF_T		*frag2,
					uint32			*msgType,
					boolean_t		is_it_pb,
					int32			*diagnostic )
{
	ccode_t				ccode;
	ipxTask_t			*taskHandle;
	register	int32	responseDelta, currentRoundTrip;
	pl_t				pl;
	int32				tt;

	NTR_ENTER( 5, endPoint, frag1, frag2, msgType, diagnostic);

	*diagnostic = SUCCESS;

	taskHandle = (ipxTask_t *)endPoint->realEndPoint;

	pl = LOCK (taskHandle->taskLock, plstr);

	if (taskHandle->state & IPX_TASK_TIMEDOUT) {
		*diagnostic = NWD_GTS_TIMED_OUT;
		taskHandle->clientPtr->stats.ipxengWriteErrorCount++;
		/*
		 * Make sure the GTS task is not in connected mode.
		 */
		taskHandle->state &= ~(IPX_TASK_CONNECTED);
		UNLOCK (taskHandle->taskLock, pl);
		NTR_LEAVE( NWD_GTS_TIMED_OUT);
		return( FAILURE );
	}

	if (!(taskHandle->state & IPX_TASK_CONNECTED) ) {
		*diagnostic = NWD_GTS_NOT_CONNECTED;
		taskHandle->clientPtr->stats.ipxengWriteErrorCount++;
		UNLOCK (taskHandle->taskLock, pl);
		NTR_LEAVE( NWD_GTS_NOT_CONNECTED );
		return( FAILURE );
	}

	/*
	 *	Reset the timeout values for the receive sequence
	 */
	taskHandle->waitTime = 0;
	taskHandle->curBackOffDelta = 0;
	taskHandle->timerTicks = taskHandle->reTransBeta;

	for (;;) { /* Retransmit loop */

		/*
		 *	Let us set the timeout routine only if it is not already set
		 */
		if (is_it_pb == B_TRUE)
			tt = taskHandle->timerTicks * 4;
		else
			tt = taskHandle->timerTicks * 2;

		if( taskHandle->callOutID == 0 ){
			taskHandle->callOutID = itimeout( IPXEngConnectionTimeout,
				(caddr_t)taskHandle, tt, plstr);
		}
		if( taskHandle->callOutID == 0 ){
			/* Very bad news, the callout list has overflowed
			 */
			UNLOCK (taskHandle->taskLock, pl);
			*diagnostic = NWD_GTS_NO_RESOURCE;
			NTR_LEAVE( NWD_GTS_NO_RESOURCE);
			return( FAILURE );
		}

		NTR_PRINTF( "IPXEngGetPacket: ticks=%d beta=%d\n",
			taskHandle->timerTicks, taskHandle->reTransBeta, 0 );

		/*
		 *	Synchronize with response
		 */
#ifdef	INLINE_SEMA

		(void)LOCK(taskHandle->syncSemaphore->psuedoSemaLock, NUCPLHI);
	
		while (taskHandle->syncSemaphore->psuedoSemaDraining && 
		    (taskHandle->syncSemaphore->psuedoSemaValue == 0)) {
		    NTR_PRINTF( "NWtlPPsuedoSema: Waiting.\n", 0, 0, 0 );
		    UNLOCK(taskHandle->taskLock, NUCPLHI);
		    SV_WAIT(taskHandle->syncSemaphore->psuedoSemaSV, 
		    		primed, 
		    		taskHandle->syncSemaphore->psuedoSemaLock );
		    pl = LOCK(taskHandle->taskLock, plstr);
		    (void)LOCK(taskHandle->syncSemaphore->psuedoSemaLock,
		    		NUCPLHI );
		}
	
		while (taskHandle->syncSemaphore->psuedoSemaValue == 0) {
	
		    ++taskHandle->syncSemaphore->psuedoSemaWaitCount;
		    NTR_PRINTF("NWtlPPsuedoSema: Waiting.\n", 0, 0, 0 );

		    UNLOCK(taskHandle->taskLock, NUCPLHI);
		    SV_WAIT(taskHandle->syncSemaphore->psuedoSemaSV, 
		    		primed, 
		    		taskHandle->syncSemaphore->psuedoSemaLock );
		    pl = LOCK(taskHandle->taskLock, plstr);
		    (void)LOCK(taskHandle->syncSemaphore->psuedoSemaLock,
		    		NUCPLHI);
	
		    NTR_ASSERT(
		        taskHandle->syncSemaphore->psuedoSemaWaitCount > 0);
	
		    --taskHandle->syncSemaphore->psuedoSemaWaitCount;
	
		    if (taskHandle->syncSemaphore->psuedoSemaDraining) {
		    	if (taskHandle->syncSemaphore->psuedoSemaWaitCount 
					== 0) {
		    		taskHandle->syncSemaphore->psuedoSemaDraining 
						= 0;
				SV_BROADCAST(
				   taskHandle->syncSemaphore->psuedoSemaSV, 0);
			}
		    	goto PPsuedoSemaDone;
		    }
		}
		--taskHandle->syncSemaphore->psuedoSemaValue;

PPsuedoSemaDone:
		UNLOCK(taskHandle->syncSemaphore->psuedoSemaLock, plstr);

#else
		UNLOCK( taskHandle->taskLock, pl);
		NWtlPPsuedoSema(taskHandle->syncSemaphore);	
		pl = LOCK( taskHandle->taskLock, plstr);
#endif

		/*	If a retransmission timer is pending turn it off.
		 */
		if( taskHandle->callOutID ){
			NTR_PRINTF( "IPXEngGetPacket: cancelling timer %x\n",
				taskHandle->callOutID, 0, 0 );
			UNLOCK( taskHandle->taskLock, pl);
			untimeout(taskHandle->callOutID);
			pl = LOCK( taskHandle->taskLock, plstr);
			taskHandle->callOutID = 0;
		}

		if( taskHandle->state & IPX_REPLY_TIMEDOUT ){


			/*	Retransmitt Packet.
			 */
			if( taskHandle->state & IPX_TASK_BURST ){
				UNLOCK (taskHandle->taskLock, pl);
				ccode =
				  NCPdiPacketBurstRetransmissionUpCall (taskHandle->channel);
			}else{
				UNLOCK (taskHandle->taskLock, pl);
				ccode = NCPdiRetransmissionUpCall (taskHandle->channel);
			}

			if( ccode ){
				pl = LOCK (taskHandle->taskLock, plstr);
				taskHandle->state &= ~(IPX_TASK_TRANSMIT | IPX_TASK_BURST);
				UNLOCK (taskHandle->taskLock, pl);

				*diagnostic = 0;
				return( NTR_LEAVE( SUCCESS ) );
			}
			pl = LOCK (taskHandle->taskLock, plstr);
			continue;
		}
		break;
	}  /* Retransmit loop */

	/*
	 * We are done with transmit or retransmit.  So make sure it is
	 * no longer in the burst mode.
	 */
	taskHandle->state &= ~(IPX_TASK_TRANSMIT | IPX_TASK_BURST);

	if ( taskHandle->state & IPX_TASK_DYING ) {
		/*
		 * This connection has been detached by the NetWare server!!
		 */
		taskHandle->state |= IPX_TASK_TIMEDOUT;
	}

	if ( taskHandle->state & IPX_TASK_TIMEDOUT ) {
		taskHandle->clientPtr->stats.ipxengTimeoutCount++;
		/*
		 * Make sure the GTS task is not in connected mode.
		 */
		taskHandle->state &= ~(IPX_TASK_CONNECTED);
		UNLOCK (taskHandle->taskLock, pl);

		*diagnostic = NWD_GTS_TIMED_OUT;
		NTR_LEAVE( NWD_GTS_TIMED_OUT);
		return( FAILURE );
	}

	/*
	 * Update the Smoothed Round Trip time, and retransmission variance
	 * for this connection.  Use the modified DARPA RFC 793 implmented on
	 * BSD 4.3 Tahoe TCP as the model.  This model uses a dynamic estimated
	 * round trip, which in our case is the time between NCP request and 
	 * response.  The retransmission time is based on this smoothed out 
	 * time plus a smoothed out variance between round trip times.  The 
	 * latest actual round trip time in HZ ticks (10 -16 millisecond
	 * granularity) is factored into the previous smoothed round trip
	 * time at a rate of .125, which leaves .875 of the old smoothed time
	 * to smooth delta's out.  The smoothed variace (mean delta) is
	 * produced by factoring the lastest delta into the previous smoothed
	 * variance at a rate of .25, which leaves .75 or the old smoothed 
	 * variance.  This model produces an efficient convergence on both
	 * short delay (local LANS) and long delay (LANS internetworked, with
	 * LANS and WANS).  Refer to "The Design and Implementation of the 4.3
	 * BSD UNIX Operating System"; Leffler, McKusick, Karels, Quarterman;
	 * Addison Wesley Publishers (1989), ISBN 0-201-06196-1.
	 *
	 * The formulas are as follws:
	 *   Where
	 *	ALPHA	; .875 Smoothing Factor
	 *	BETA	; Base retransmission timeout (dynamically determined)
	 *	DELTA	; delta between current round trip and old smoothed
	 *		  round trip
	 *
	 *   DELTA = currentRoundTrip - smoothedRoundTrip;
	 *   smoothedRoundTrip = (smoothedRoundTrip * ALPHA) +
	 *		(currentRoundTrip * (1 - ALPHA) );
	 *   smoothedVariance = (smoothedVariance * .75) + (DELTA * .25);
	 *	Note: Smoothed Variance is at least 1.
	 *   BETA = smoothedRoundTrip + (2 * smoothedVariance);
	 *
	 * Note:
	 *	The smoothedRoundTrip, and smoothedVariance are stored in fixed
	 *	point to utilized shifts and therefore avoid costly floating
	 *	point operations.  The smoothedRoundTrip is stored <<3 (Scaled
	 *	by 8) normalized, and the smoothedVariance is stored <<2
	 *	(Scaled by 4) normalized.
	 */

	/*	For normal NCPs the time between the request and response is the
	 *	round trip time used for these calculations.
	 *
	 *	For Packet Burst the time between the last packet sent in the request
	 *	and the first packet received in the reply.
	 */

	/*
	 *	calculate currentRoundTrip time in ticks
	 */
	currentRoundTrip = drv_usectohz(1000000 *
	  taskHandle->roundTripTime.tv_sec +
	  (taskHandle->roundTripTime.tv_nsec / 1000) );

	if (currentRoundTrip > ((taskHandle->smoothedRoundTrip >> 3) + 1
				<< EXP_SHIFT_CLIP) ) {
		/*
		 * Clip at our maximmal believable limit, this
		 * response arrvied in a severly degrated connection.
		 */
		currentRoundTrip = ((taskHandle->smoothedRoundTrip >> 3) + 1
				<< EXP_SHIFT_CLIP);
	}

	/*
	 *   DELTA = currentRoundTrip - smoothedRoundTrip;
	 */
	responseDelta = currentRoundTrip - (taskHandle->smoothedRoundTrip >> 3);

	/*
	 *   smoothedRoundTrip = min ((smoothedRoundTrip * ALPHA) +
	 *		(currentRoundTrip * (1 - ALPHA)), minRoundTrip);
	 */
	if ( (taskHandle->smoothedRoundTrip +=  responseDelta)
			< ipxEngTune.minRoundTripTicks<<3 ) {
		/*
		 * Must be at least 1 HZ tick
		 */
		taskHandle->smoothedRoundTrip = ipxEngTune.minRoundTripTicks<<3;
	}

	/*
	 *   smoothedVariance =
	 *	min((smoothedVariance * .75) + (DELTA * .25), minVariance);
	 */
	if ( responseDelta < 0 ) {
		/*
		 * Make it an absolute value
		 */	
		responseDelta = -responseDelta;
	}
	responseDelta -= (taskHandle->smoothedVariance >> 2);
	if ( (taskHandle->smoothedVariance += responseDelta) <
			ipxEngTune.minVariance<<2 ) {
		/*
		 * Must be at least 1 HZ tick
		 */
		taskHandle->smoothedVariance = ipxEngTune.minVariance<<2;
	}

	/*
	 *   BETA = smoothedRoundTrip + (2 * smoothedVariance);
	 */
	taskHandle->reTransBeta = (taskHandle->smoothedRoundTrip >> 3) +
		(taskHandle->smoothedVariance >> 1);

	UNLOCK (taskHandle->taskLock, pl);

	NTR_TRACE( NVLTT_ipxeng_times, taskHandle->reTransBeta,
	  taskHandle->smoothedRoundTrip, taskHandle->smoothedVariance,
	  currentRoundTrip);

	*diagnostic = 0;
	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngRegisterAsyncHandler(3K), \
 *		./man/kernel/ts/ipxeng/RegisterAsyncHandler)
 * NAME
 *	IPXEngRegisterAsyncHandler -	Register async hander to IPX for
 *					managing callback on ASYNC channels.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngRegisterAsyncHandler( endPoint, callbackFunc, callbackHandle, 
 *						diagnostic )
 *
 *	GTS_ENDPOINT_T	*endPoint;
 *	int32		(*callbackFunc)();
 *	opaque_t	*callbackHandle;
 *	int32		*diagnostic;
 *
 * INPUT
 *      endPoint       - A pointer to the GTS Transport Endpoint
 *                         connected to a NetWare server.
 *
 *      callbackFunc     - A pointer to an opaque function which is to
 *                         service asynchronous events for the IPX Transport
 *                         EndPoint.  This function is expected to have the
 *                         following declaration:
 *
 *                              opaque_t
 *                              asyncFunction(endPoint, callbackHandle,
 *                                              eventMsgType)
 *
 *                              opaque_t        *endPoint;
 *                              private_t       *callbackHandle;
 *                              int32           eventMsgType;
 *
 *                         where the 'endPoint' and the supplied
 *                         'callbackHandle' are set to the values specified on
 *		  	   the call to IPXEngRegisterAsyncHandler(), and the
 *                         'eventMsgType' will be set to GTS_OUT_OF_BAND.
 *
 *      callbackHandle   - A pointer to opaque information which is managed by
 *                         by the 'callbackFunc' which is to be supplied on
 *                         asynchronous call backs.
 *
 * OUTPUT
 *	diagnostic	- Not used at this time, always returns success.
 *
 * RETURN VALUES
 *	0		- Successful completion.
 *
 * DESCRIPTION
 *      The 'IPXEngRegisterAsyncHandler' arranges for a function to be called
 *      when asychronous events occur on the IPX message socket. The
 *      function will be passed the IPX Transport EndPoint handle, a
 *      handle the function registered, and the event messsage type of
 *	GTS_OUT_OF_BAND.  Upon return from the asynchronously called function
 *	it is assumed the event which caused the asynchronous call has been
 *	completely serviced.
 *
 * NOTES
 *	This call back mechanism has been placed into the IPX Engine which 
 *	normally runs in the synchronous mode, to accomodate messages, and
 *	cache consistency protocol events.  When the NCP layer registers
 *	a call back handler, it will be called to service these events.  If no
 *	call back handler is registered, the message will be discarded by IPX
 *	Engine.  Represents the the IPX dependent portion of a
 *	NWtsRegisterAsyncEventHandler(3K).
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngGetPacket(3K),
 *	NWtsRegisterAsyncEventHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngRegisterAsyncHandler( endPoint, callbackFunc, callbackHandle, diagnostic )

GTS_ENDPOINT_T	*endPoint;
void_t		(*callbackFunc)();
opaque_t	*callbackHandle;
int32		*diagnostic;
{

	NTR_ENTER( 4, endPoint, callbackFunc, callbackHandle, diagnostic, 0);

	*diagnostic = SUCCESS;

	((ipxTask_t *)endPoint->realEndPoint)->asyncHandler = callbackFunc;
	((ipxTask_t *)endPoint->realEndPoint)->asyncContextHandle = callbackHandle;
	return( NTR_LEAVE( SUCCESS ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngInterruptHandler(3K), \
 *		./man/kernel/ts/ipxeng/InterruptHandler)
 * NAME
 *	IPXEngInterruptHandler -	Handle async events coming from
 *					service	socket.
 *
 * SYNOPSIS
 *	static void_t
 *	(*ipcChannel->callBackFunction)(ipcChannel, clientHandle,
 *				eventMsgType)
 *
 *	opaque_t	*ipcChannel;
 *	ipxClient_t	*clientHandle;
 *	uint32		eventMsgType;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel associated with the
 *			  Service Socket.
 *	clientHandle	- A pointer to the IPX Client Socket Set attached to
 *			  the GIPC Channel.
 *	msgType		- Set to GIPC_NORMAL_MSG, since only IN BAND messages
 *			  can arrive on the service socket (they in fact are
 *			  all Responses to outstanding requests).
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'IPXEngInterruptHandler' is called when a response arrives on a
 *	service socket.  This handler is register with the GIPC layer via the
 *	NWpcRegisterAsyncEventHandler(3K) to handle all received messages on
 *	the GIPC Channel associated with the service socket for a client socket
 *	set.  This handler runs out of process context, and its job to wakeup
 *	(schedule) the blocked NCP process the response belongs to.
 *
 * NOTES
 *	Handles interrupts from the transport provider causing a context
 *	to be scheduled if one is waiting for the packet.
 *
 *	Need to scan the IPX Header source address and use it as the
 *	search handle of the task table to find the correct context to
 *	wakeup.
 *
 *	The idea here is to catch all incoming responses for a client socket
 *	set and multiplex them to the correct IPX EndPoint with a outstanding
 *	NCP request.
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngGetPacket(3K),
 *	NWpcRegisterAsyncEventHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
void_t
IPXEngInterruptHandler (opaque_t *ipcChannel, ipxClient_t *clientHandle,
								uint32 msgType)
{
	ccode_t			ccode;
	ipxAddress_t	ipxAddress;
	ipxTask_t		*taskPtr;
	NUC_IOBUF_T		buf;
	pl_t			pl;
	NUC_IOBUF_T		*frag1, *frag2;
	uint32			tmpMsgType;
	uint32			rc;
	boolean_t		timerFound = B_FALSE;

	NTR_ENTER (3, ipcChannel, clientHandle, msgType, 0, 0);

	/*
		Retrieve the IPX address and a pointer to the packet.
		Leave the party early if this retrieval returns 
		other than a handful of expected return codes.
	*/
	ccode = IPXEngGetIPCPacketPointer (ipcChannel, &buf, &ipxAddress );

	switch (ccode) {
		case SUCCESS:
		case NWD_GTS_NOT_CONNECTED:
		case NWD_GTS_TIMED_OUT:
		case NWD_GTS_BLOCKING:
		case NWD_GTS_BAD_ADDRESS: {

			/*
				Find the task associated with this packet and flush the
				packet if no task can be found.
			*/
			pl = RW_WRLOCK (nucTSLock, plstr);
			rc = IPXEngFindTask_l (clientHandle->taskList, &ipxAddress,
									&taskPtr);
			RW_UNLOCK (nucTSLock, pl);

			if (rc) {
				NTR_PRINTF ("IPXEngInterruptHandler: no NCP connection found\n",
				  0, 0, 0);
				break;
			}

			pl = LOCK (taskPtr->taskLock, plstr);

#ifdef TEST_IPXTASK_HOLD
			if (IPXTASK_IS_HELD(taskPtr)) {
				/*	The GTS_RECEIVE routine has not been reached yet
				 *	for this task - we got the reply even before we
				 *	can get to SV_WAIT in IPXEngGetPacket!! So let us
				 *	discard the reply. A retransmit will clear the connection
				 */
				if (taskPtr->callOutID == 0 ){
					taskPtr->callOutID = itimeout( IPXEngConnectionTimeout,
						(caddr_t)taskPtr, 5, plstr );
				}
				UNLOCK (taskPtr->taskLock, pl);
				NTR_PRINTF ("IPXEngInterruptHandler: taskPtr->channel->channelHold = 0x%x\n",
						((ncp_channel_t *)taskPtr->channel)->channelHold, 0, 0);
				break;
			}
#endif TEST_IPXTASK_HOLD

			if ( ccode == NWD_GTS_NOT_CONNECTED ) {
				taskPtr->state |= IPX_TASK_DYING;
				NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			/*
				If we are not currently transmitting (IPX_TASK_TRANSMIT),
				this packet is a duplicate or a result of a
				"sorcerers apprentice" retransmission and will be flushed.
			 *
			 *	If we are in the midst of a Burst (IPX_TASK_BURST), throw away
				this packet and don't touch the timer since it now belongs to
				Packet Burst.
			 *
				If the connection is timed out (IPX_TASK_TIMEDOUT), the
				connection is invalid and the packet will be flushed.
			 */

			if( !(taskPtr->state & IPX_TASK_TRANSMIT) ||
			  (taskPtr->state & IPX_TASK_BURST) ||
			  (taskPtr->state & IPX_REPLY_TIMEDOUT) ||
			  (taskPtr->state & IPX_TASK_TIMEDOUT) ){

				NTR_PRINTF( "IPXEngInterruptHandler: taskPtr->state = 0x%x\n",
				  taskPtr->state, 0, 0 );
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			/*
				Turn off the retransmission timer.  It will be re-started if
				it is determined that the current response is to be ignored
				and an eventual correct response can be expected.
			*/
			if (taskPtr->callOutID) {
				NTR_PRINTF("IPXEngInterruptHandler: canceling ID=%x\n",
					taskPtr->callOutID, 0,0);
				UNLOCK( taskPtr->taskLock, pl);
				if( untimeout_r(taskPtr->callOutID) ){

					timerFound = TRUE;
				}
				pl = LOCK( taskPtr->taskLock, plstr);
				taskPtr->callOutID = 0;
			} else {
				NTR_PRINTF( "IPXEngInterruptHandler: No timer to cancel?\n",
				  0,0,0);
			}

			/*
				If a route to the server is not available, set the appropriate
				task state and wake up the task waiting for the response.
			*/
			if( ccode ){
				if (ccode == NWD_GTS_BAD_ADDRESS) {
					taskPtr->state |= IPX_TASK_TIMEDOUT;
					taskPtr->state &= ~(IPX_TASK_TRANSMIT);
					NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				}

				if ( ccode == NWD_GTS_TIMED_OUT ) {
					taskPtr->state |= IPX_TASK_DYING;
					NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				}

				if( ccode == NWD_GTS_BLOCKING ) {
					/*	A SERVER BUSY packet (type=0x9999) was received,
					 *	don't reset the roundTripStart as the SERVER BUSY
					 *	should be accumulated in the current roundTripQuantum.
					 *	Restart the retransmission timer and flush the packet,
					 *	the callout handler will resend our request.
					 */
					taskPtr->waitTime = 0;
					if (timerFound) {
						taskPtr->callOutID = itimeout (IPXEngConnectionTimeout,
							(caddr_t)taskPtr, taskPtr->timerTicks, plstr);
					}
				}
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			ccode = NCPdiReceivePacketUpCall (taskPtr->channel, &buf,
				(void_t*)&frag1, (void_t *)&frag2, ipcChannel);

			if (ccode == NWD_GTS_TIMED_OUT) {
				taskPtr->state |= IPX_TASK_DYING;
				NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}else if (ccode) {
				taskPtr->waitTime = 0;
				if (timerFound) {
					taskPtr->callOutID = itimeout (IPXEngConnectionTimeout,
						(caddr_t)taskPtr, taskPtr->timerTicks, plstr);
				}
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			/*
			 *	Drain the packet off the stream head into the arguments
			 *	passed in
			 */
			if ((ccode = IPXEngGetIPCPacket (
					taskPtr->clientPtr->socketSet.serviceSocket.ipcChannel,
					ipxAddress.ipxAddr, frag1, frag2, &tmpMsgType)) != 0) {

				NTR_PRINTF("IPXEngInterruptHandler: GetIPC failed w/cc=%x\n",
					ccode, 0,0);

				taskPtr->state |= IPX_TASK_DYING;
				NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			/*
				A correctly sequenced packet has been received on a
				transmitting channel and the connection is still valid.  
				
				Set the ipxtask_t state to NOT transmitting.
			*/
			taskPtr->state &= ~(IPX_TASK_TRANSMIT);

			/*
			 * Compute the real round trip for this response
			 */
			/* taskPtr->roundTripTime = hrestime; */
			GET_HRESTIME (&taskPtr->roundTripTime);
			taskPtr->roundTripTime.tv_sec -= taskPtr->roundTripStart.tv_sec;
			taskPtr->roundTripTime.tv_nsec -=
			  taskPtr->roundTripStart.tv_nsec;


			NWtlVPsuedoSema (taskPtr->syncSemaphore);
			UNLOCK (taskPtr->taskLock, pl);

			NTR_LEAVE( 0 );
			return;
		}
		break;
	default:
#ifdef NUC_DEBUG
		cmn_err(CE_PANIC, 
			"IPXEngInterruptHandler: GetSourceAddress FAILED\n");
#endif
		/*
			Here would be a good place to call a function to 
			increment a bad packet counter for this socket.
		*/
		break;
	}

	NTR_PRINTF("IPXEngInterruptHandler: timer ID=%x\n", 
			taskPtr->callOutID, 0,0);

	NTR_PRINTF( "IPXEngInterruptHandler: Flushing message\n", 0, 0, 0 );

	IPXEngFlushCurrentIPCPacket( ipcChannel );
	NTR_LEAVE( 0 );
	return;
}




/*
 * BEGIN_MANUAL_ENTRY(IPXEngInterruptHandlerBurst(3K), \
 *		./man/kernel/ts/ipxeng/InterruptHandlerBurst)
 * NAME
 *	IPXEngInterruptHandlerBurst -	Handle async events coming from
 *		packet burst socket.
 *
 * SYNOPSIS
 *	static void_t
 *	(*ipcChannel->callBackFunction)(ipcChannel, clientHandle,
 *				eventMsgType)
 *
 *	opaque_t	*ipcChannel;
 *	ipxClient_t	*clientHandle;
 *	uint32		eventMsgType;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel associated with the
 *			  packet burst Socket.
 *	clientHandle	- A pointer to the IPX Client Socket Set attached to
 *			  the GIPC Channel.
 *	msgType		- Set to GIPC_NORMAL_MSG, since only IN BAND messages
 *			  can arrive on the packet burst socket (they in fact are
 *			  all Responses to outstanding requests).
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'IPXEngInterruptHandlerBurst' is called when a response arrives on a
 *	packet burst socket.  This handler is register with the GIPC layer via the
 *	NWpcRegisterAsyncEventHandler(3K) to handle all received messages on
 *	the GIPC Channel associated with the packet burst socket for a client 
 *	socket set.  This handler runs out of process context, and its job 
 *	to wakeup (schedule) the blocked process the response belongs to.
 *
 * NOTES
 *	Handles interrupts from the transport provider causing a context
 *	to be scheduled if one is waiting for the packet.
 *
 *	Need to scan the IPX Header source address and use it as the
 *	search handle of the task table to find the correct context to
 *	wakeup.
 *
 *	The idea here is to catch all incoming responses for a client socket
 *	set and multiplex them to the correct IPX EndPoint with a outstanding
 *	packet burst request.
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngGetPacket(3K),
 *	NWpcRegisterAsyncEventHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
void_t
IPXEngInterruptHandlerBurst(	opaque_t		*ipcChannel,
								ipxClient_t		*clientHandle,
								uint32			msgType )
{
	ccode_t			ccode;
	ccode_t			ccode2;
	ipxAddress_t	ipxAddress;
	ipxTask_t		*taskPtr;
	NUC_IOBUF_T		buf;
	boolean_t		timerFound = B_FALSE;
	pl_t			pl;
	NUC_IOBUF_T		frag1, frag2;
	uint32			tmpMsgType;

	NTR_ENTER( 3, ipcChannel, clientHandle, msgType, 0, 0);

	/*
		Retrieve the IPX address and a pointer to the packet.
		Leave the party early if this retrieval returns 
		other than a handful of expected return codes.
	*/
	ccode = IPXEngGetIPCPacketPointer ( ipcChannel, &buf, &ipxAddress );

	switch (ccode) {
		case SUCCESS:
		case NWD_GTS_NOT_CONNECTED:
		case NWD_GTS_TIMED_OUT:
		case NWD_GTS_BLOCKING:
		case NWD_GTS_BAD_ADDRESS: {

			/*
				Find the task associated with this packet and flush the
				packet if no task can be found.
			*/
			pl = RW_WRLOCK (nucTSLock, plstr);
			if(IPXEngFindTask_l(clientHandle->taskList, &ipxAddress, &taskPtr)){
				RW_UNLOCK (nucTSLock, pl);
				NTR_PRINTF( "IPXEngInterruptHandlerBurst: "
				  "no BURST connection found for response\n", 0, 0, 0 );
				break;
			}
			RW_UNLOCK (nucTSLock, pl);

			pl = LOCK (taskPtr->taskLock, plstr);

			/*	This packet will be flushed if:
			 *
			 *	We are not currently transmitting (IPX_TASK_TRANSMIT) this
			 *	is usually the result of a duplicate packet or a
			 *	"sourcerers apprentice".
			 *
			 *	We are not currently bursting (IPX_TASK_BURST).
			 *	NOTE:	We don't want to touch the timer in this case
			 *			because it could be in use by standard NCP.
			 *
			 *	We are currently retransmitting.
			 *
			 *	The connection is timed out.
			 */
#ifdef TEST_IPXTASK_HOLD
			if (IPXTASK_IS_HELD(taskPtr)) {
				/*	The GTS_RECEIVE routine has not been reached yet
				 *	for this task - we got the reply even before we
				 *	can get to SV_WAIT in IPXEngGetPacket!! So let us
				 *	discard the reply. A retransmit will clear the connection
				 */
				if (taskPtr->callOutID == 0 ){
					taskPtr->callOutID = itimeout( IPXEngConnectionTimeout,
						(caddr_t)taskPtr, 10, plstr );
				}
				UNLOCK (taskPtr->taskLock, pl);
				NTR_PRINTF ("IPXEngInterruptHandlerBurst: taskPtr->channel->channelHold = 0x%x\n",
						((ncp_channel_t *)taskPtr->channel)->channelHold, 0, 0);
				break;
			}
#endif TEST_IPXTASK_HOLD

			if (!(taskPtr->state & IPX_TASK_TRANSMIT) ||
				!(taskPtr->state & IPX_TASK_BURST) ||
				(taskPtr->state & IPX_REPLY_TIMEDOUT) ||
				(taskPtr->state & IPX_TASK_TIMEDOUT) ){
				UNLOCK (taskPtr->taskLock, pl);
				NTR_PRINTF ("IPXEngInterruptHandlerBurst: taskPtr->state = 0x%x\n",
							taskPtr->state, 0, 0 );
				break;
			}

			/*
				Turn off the retransmission timer.  It will be re-started if
				it is determined that the current response is to be ignored
				and an eventual correct response can be expected.
			*/
			if (taskPtr->callOutID) {
				NTR_PRINTF( "IPXEngInterruptHandlerBurst: canceling ID=%x\n",
				  taskPtr->callOutID, 0, 0 );
				UNLOCK( taskPtr->taskLock, pl);
				if( untimeout_r(taskPtr->callOutID) ){
					timerFound = TRUE;
				}
				pl = LOCK( taskPtr->taskLock, plstr);
				taskPtr->callOutID = 0;
			} else {
				NTR_PRINTF("IPXEngInterruptHandlerBurst: No timer to cancel?\n",
				  0, 0, 0 );
			}

			if( ccode ){
				switch( ccode ){
					case NWD_GTS_NOT_CONNECTED:
						/*	Looks like we received a SIGHUP message from ipx.
						 *	We need to start the cleanup procedures for each
						 *	of the open tasks
						 */
						taskPtr->state |= IPX_TASK_DYING;
						NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
						UNLOCK (taskPtr->taskLock, pl);
						break;

					case NWD_GTS_BAD_ADDRESS:
						/*	If a route to the server is not available, set the
						 *	appropriate task state and wake up the task waiting
						 *	for the response.
						 */
						taskPtr->state |= IPX_TASK_TIMEDOUT;
						taskPtr->state &= ~(IPX_TASK_TRANSMIT | IPX_TASK_BURST);
						NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
						UNLOCK (taskPtr->taskLock, pl);
						break;

					case NWD_GTS_BLOCKING:
						/*
						 * A Server Busy was received, reset the
						 * retransmission state, and let call out
						 * handler resend our request.  Don't reset
						 * roundTripStart as the Server Busy should be
						 * accumulated in the current roundTripQuantum.
						 */
						if( timerFound ){
							taskPtr->waitTime = 0;
							taskPtr->callOutID = itimeout(
							  IPXEngConnectionTimeout, (caddr_t)taskPtr,
							  taskPtr->timerTicks, plstr);
							NTR_PRINTF( "IPXEngInterruptHandlerBurst: "
							  "timer ID=%x, ticks=%x\n", 
							  taskPtr->callOutID, taskPtr->timerTicks, 0 );
						}
						UNLOCK (taskPtr->taskLock, pl);
						break;

					case NWD_GTS_TIMED_OUT:
						taskPtr->state |= IPX_TASK_DYING;
						NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
						UNLOCK (taskPtr->taskLock, pl);
						break;
				}
				break;
			}

			ccode = NCPdiPacketBurstReceiveUpCall( taskPtr->channel, &buf,
			  &frag1, &frag2, ipcChannel);

			if( ccode != SUCCESS && ccode != NWD_GTS_PB_ALL_FRAGS_RECEIVED ){
				switch( ccode ){
					case NWD_GTS_REPLY_TIMED_OUT:
						taskPtr->state |= IPX_REPLY_TIMEDOUT;
						NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
						UNLOCK (taskPtr->taskLock, pl);
						break;
					case NWD_GTS_TIMED_OUT:
						/*	A valid packet has been received on a valid channel.
						 *	This packet indicated that the server connection is
						 *	no longer valid.  Set the ipxTask_t state to
						 *	IPX_TASK_DYING so future requests to this channel
						 *	will be denied.
						 */
						taskPtr->state |= IPX_TASK_DYING;
						NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
						UNLOCK (taskPtr->taskLock, pl);
						break;
					case NWD_GTS_BLOCKING:
						/*
						 * A Server Busy was received, reset the
						 * retransmission state, and let call out
						 * handler resend our request.  Don't reset
						 * roundTripStart as the Server Busy should be
						 * accumulated in the current roundTripQuantum.
						 */
						if( timerFound ){
							taskPtr->waitTime = 0;
						}
						/*  Fall through  */
					default:
						if( timerFound ){
							taskPtr->callOutID = itimeout(
							  IPXEngConnectionTimeout, (caddr_t)taskPtr,
							  taskPtr->timerTicks, plstr);
							NTR_PRINTF( "IPXEngInterruptHandlerBurst: "
							  "timer ID=%x, ticks=%x\n", 
							  taskPtr->callOutID, taskPtr->timerTicks, 0 );
						}
						UNLOCK (taskPtr->taskLock, pl);
						break;
				}
				break;
			}

			/*
			 *	Drain the packet off the stream head into the arguments
			 *	passed in
			 */
			if ( (ccode2 = IPXEngGetIPCPacket(
				taskPtr->clientPtr->socketSet.packetBurstSocket.ipcChannel,
				ipxAddress.ipxAddr, &frag1, &frag2, &tmpMsgType )) != 0 ) {

				NTR_PRINTF("IPXEngInterruptHandlerBurst: GetIPC failed w/cc=%x\n",
					ccode2, 0,0);

				taskPtr->state |= IPX_TASK_DYING;
				NWtlWakeupPsuedoSema (taskPtr->syncSemaphore);
				UNLOCK (taskPtr->taskLock, pl);
				break;
			}

			if( taskPtr->roundTripStart.tv_sec != 0 ||
			  taskPtr->roundTripStart.tv_nsec != 0 ){
				/*
				 * Compute the real round trip for this response
				 */
				/* taskPtr->roundTripTime = hrestime; */
				GET_HRESTIME_EXACT (&taskPtr->roundTripTime);
				taskPtr->roundTripTime.tv_sec -= taskPtr->roundTripStart.tv_sec;
				taskPtr->roundTripTime.tv_nsec -=
				  taskPtr->roundTripStart.tv_nsec;

				taskPtr->roundTripStart.tv_sec = 0;
				taskPtr->roundTripStart.tv_nsec = 0;
			}

			if( ccode == NWD_GTS_PB_ALL_FRAGS_RECEIVED ){

				taskPtr->state &= ~(IPX_TASK_TRANSMIT | IPX_TASK_BURST);

				NWtlVPsuedoSema (taskPtr->syncSemaphore);
				UNLOCK (taskPtr->taskLock, pl);
				NTR_PRINTF( 
				 "IPXEngInterruptHandlerBurst: exit SUCCESS\n",
				  0, 0, 0 );

			} else {
				/*	Since Packet Burst Reads will receive many packets for
				 *	a single request we need to reset the timeout values
				 *	after each successfully received packet.
				 */
				taskPtr->waitTime = 0;
				taskPtr->curBackOffDelta = 0;

				/*	Is there an old timer active?
				 */
				NTR_ASSERT( !(taskPtr->callOutID) );

				if (timerFound) {
					taskPtr->callOutID = itimeout (IPXEngConnectionTimeout,
						(caddr_t)taskPtr, taskPtr->timerTicks, plstr);
				}

				UNLOCK (taskPtr->taskLock, pl);

				NTR_PRINTF( "IPXEngInterruptHandlerBurst: "
				  "timer ID=%x, ticks=%x\n", taskPtr->callOutID,
				  taskPtr->timerTicks, 0 );
			}


			NTR_LEAVE(0);
			return;
		}
		default:
			/*	GetSourceAddress FAILED!
			 */
			cmn_err (CE_PANIC, "IPXEngInterruptHandlerBurst:Terrible ccode\n");
	}

	NTR_PRINTF( "IPXEngInterruptHandlerBurst: Flushing message\n", 0, 0, 0 );

	IPXEngFlushCurrentIPCPacket( ipcChannel );
	NTR_LEAVE( 0 );
	return;
}

/*
 * BEGIN_MANUAL_ENTRY(IPXEngObtainTransportInfo(3K), \
 *		./man/kernel/ts/ipxeng/ObtainTransportInfo)
 * NAME
 *	IPXEngObtainTransportInfo - Update the transport socket
 *			number in the ncp_channel_t.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngObtainTransportInfo( channel )
 *	ncp_channel_t	*channel;
 *
 * INPUT
 *      ncp_channel_t - a pointer to the NCP channel object defining this
 *				connection.
 *
 * OUTPUT
 *		ncp_channel_t - updated with the packet burst socket number
 *			obtained from GTS.
 *
 * RETURN VALUES
 *	SPI_SUCCESS		- Successful.
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *      NCPdplAllocChannel(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngObtainTransportInfo (ncp_channel_t *channel )
{
	ipxTask_t	*ipxTask;
	pl_t			pl;

	NTR_ENTER( 1, channel, 0, 0, 0, 0);

	ipxTask = (ipxTask_t *)((GTS_ENDPOINT_T *)
					channel->transportHandle)->realEndPoint;
	channel->packetBurstSocket = *(uint16 *) 
					ipxTask->clientPtr->socketSet.packetBurstSocket.socketID;

	pl = LOCK (ipxTask->taskLock, plstr);

	ipxTask->channel = channel;
	UNLOCK (ipxTask->taskLock, pl);


	return( NTR_LEAVE( SUCCESS ) );
}
