/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxestruct.c	1.17"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxestruct.c,v 2.56.2.4 1995/02/11 04:47:09 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxestruct.c
 *
 *	ABSTRACT: Structure manipulation routines.  
 *
 *	Functions declared in this module:
 *	Public functions:
 *		IPXEngAllocClient
 *		IPXEngFreeClient
 *		IPXEngFindClient
 *		IPXEngAllocTask
 *		IPXEngFreeTask
 *		IPXEngFindTask
 *		IPXEngAllocateSocketSet
 *		IPXEngFreeSocketSet
 *
 */

#ifdef _KERNEL_HEADERS
#include <io/stream.h>
#include <mem/kmem.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/param.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengtune.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/requester.h>

#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/time.h>
#include <sys/ipx_app.h>

#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nuctool.h>
#include <sys/ipxengine.h>
#include <sys/ipxengtune.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/gtsconf.h>
#include <sys/nucerror.h>
#include <sys/requester.h>

#include <sys/nuc_hier.h>

#endif /* ndef _KERNEL_HEADERS */

LKINFO_DECL(taskLockInfo, "NETNUC:NUC:taskLock", 0);

#define NTR_ModMask	NTRM_ipxeng
#define IPXE_SOCKET_ALLOC_RETRY 3	/* give it 3 chances */

extern rwlock_t	*nucTSLock;
extern int32 IPXEngInterruptHandler();
extern int32 IPXEngInterruptHandlerBurst();
extern int32 IPXEngWDSocketInterruptHandler();
extern int32 IPXEngOOBSocketInterruptHandler();


/*
 * BEGIN_MANUAL_ENTRY(IPXEngAllocClient(3K), \
 *		./man/kernel/ts/ipxeng/AllocClient)
 * NAME
 *	IPXEngAllocClient -	Find free slot in the client list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngAllocClient( clientList, clientPtr )
 *
 *	ipxClient_t	*clientList;
 *	ipxClient_t	**clientPtr;
 *
 * INPUT
 *	clientList	- A pointer to a vector of client contexts.
 *
 * OUTPUT
 *	clientPtr	- A pointer to a client context which has been
 *			  allocated to create a virtual work station.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_RESOURCE	- All client contexts are in use, the UNIX
 *				  UID can't be created a virtual work station.
 *
 * DESCRIPTION
 *	Find a free slot in the client table, and allocate for use by a new
 *	UNIX UID.  
 *
 * NOTES
 *	This function creates a new client context.  A client context is
 *	associated with a UNIX credential (i.e. UID).  The UNIX Client is
 *	architected as a replicated set of virtual client machines to NetWare.
 *	Each virtual client machine is in reality a collection of all 
 *	processes being used by a UNIX UID which are using NetWare.  The idea
 *	here is to create one socket set (i.e. Ephemeral= Service Socket,
 *	Ephemeral+1= Watch Dog Socket, and Ephemeral+2= Message Socket) for a
 *	UNIX UID as a virtual DOS/Windows or OS/2 machine, and thread each
 *	NetWare Server Connections (i.e. different destination IPX address)
 *	through the socket set.  In this model the client uses the same local
 *	IPX address with each NetWare Server it is consuming resources on.  
 *	Each different client has a unique local address, thus appearing as
 *	multiple client work stations to NetWare servers.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucTSLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		nucTSLock
 *
 * SEE ALSO
 *	IPXEngOpenEndpoint(3K) 
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngAllocClient_l (ipxClient_t *clientList, ipxClient_t **clientPtr)
{
	register int	i;

	NTR_ENTER(2, clientList, clientPtr, 0, 0, 0);

	for ( i = 0; i < ipxEngTune.maxClients; i++ ) {
		if (clientList[i].state == IPX_CLIENT_FREE) {
			*clientPtr = &(clientList[i]);
			clientList[i].state = IPX_CLIENT_INUSE;

			NTR_LEAVE((uint_t) *clientPtr);
			return( SUCCESS  );
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFreeClient(3K), \
 *		./man/kernel/ts/ipxeng/FreeClient)
 * NAME
 *	IPXEngFreeClient -	Free slot in the client list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFreeClient( clientList, clientPtr )
 *
 *	ipxClient_t	*clientList;
 *	ipxClient_t	*clientPtr;
 *
 * INPUT
 *	clientList	- A pointer to a vector of client contexts.
 *	clientPtr	- A pointer to a client context which released from
 *			  a virtual work station, and made available for a
 *			  new virtual work station when needed.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_ENDPOINT	- The 'clientPtr' is not asscociated with an
 *				  allocated virtual work station context.
 *
 * DESCRIPTION
 *	Return a virtual work staion context to a free state for future 
 *	binding to a new client context.
 *
 * NOTES
 *	See notes pertaining to IPXEngAllocClient.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucTSLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		nucTSLock
 *
 * SEE ALSO
 *	IPXEngCloseEndpoint(3K), IPXEngAllocClient(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFreeClient_l (ipxClient_t *clientList, ipxClient_t *clientPtr )
{
	register int i;

	NTR_ENTER(2, clientList, clientPtr, 0, 0, 0);

	/*
	 *	Change this to use math to determine the index into
	 *	the array instead of searching it.
	 */

	for (i = 0; i < ipxEngTune.maxClients; i++) {
		if (clientList[i].state == IPX_CLIENT_FREE)
			continue;

		if (clientPtr == &(clientList[i])) {
			clientPtr->credentialsPtr = (opaque_t *)NULL;
			clientPtr->state = IPX_CLIENT_FREE;

			return( NTR_LEAVE( SUCCESS ) );
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_ENDPOINT ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFindClient(3K), \
 *		./man/kernel/ts/ipxeng/FindClient)
 * NAME
 *	IPXEngFindClient -	Find free slot in the client list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFindClient( clientList, credPtr, clientPtr )
 *	ipxClient_t		*clientList;
 *	opaque_t		*credPtr;
 *	ipxClient_t		**clientPtr;
 *
 * INPUT
 *	clientList	- A pointer to a vector of client contexts.
 *	credPtr		- A pointer to a UNIX UID contained in a UNIX kernel
 *			  centric structure managed by the 'nuctool' library.
 *			  The UID represents the client virtual work station
 *			  to search for.
 *
 * OUTPUT
 *	clientPtr	- A pointer to a client virtual work station context
 *			  associated with this UNIX UID.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_ENDPOINT	- The 'credPtr' UNIX UID is not asscociated
 *				  with an allocated virtual work station
 *				  context.
 *
 * DESCRIPTION
 *	Search for a client virtual work station context for the specified
 *	UNIX UID.
 *
 * NOTES
 *	See notes on IPXEngAllocClient(3K).
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucTSLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		nucTSLock
 *
 *
 * SEE ALSO
 *	IPXEngOpenEndpoint(3K), IPXEngAllocClient(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFindClient_l (ipxClient_t clientList[],
					opaque_t *credPtr, ipxClient_t **clientPtr)
{
	register int	i;

	NTR_ENTER(3, clientList, credPtr, clientPtr, 0, 0);

	for (i = 0; i < ipxEngTune.maxClients; i++) {
		if (clientList[i].state == IPX_CLIENT_FREE)
			continue;

		if (((nwcred_t *)credPtr)->flags & NWC_OPEN_PRIVATE) {
			if (clientList[i].state & IPX_CLIENT_PRIVATE) {
				if ( (NWtlCredMatch( credPtr, clientList[i].credentialsPtr, CRED_PID)) ) {
					*clientPtr = &(clientList[i]);	

					NTR_LEAVE((uint_t) *clientPtr);
					return( SUCCESS );
				}
			}
		}
		else {
			if ((clientList[i].state & IPX_CLIENT_PRIVATE) == 0) {
				if ( (NWtlCredMatch( credPtr, clientList[i].credentialsPtr, CRED_UID)) ) {
					*clientPtr = &(clientList[i]);	

					NTR_LEAVE((uint_t) *clientPtr);
					return( SUCCESS );
				}
			}
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_ENDPOINT ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngAllocTask(3K), \
 *		./man/kernel/ts/ipxeng/AllocTask)
 * NAME
 *	IPXEngAllocTask -	Find free slot in the Task list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngAllocTask( taskList, taskPtr )
 *
 *	ipxTask_t	*taskList;
 *	ipxTask_t	**taskPtr;
 *
 * INPUT
 *	taskList	- A pointer to a vector of IPX EndPoint contexts
 *			  for a client virtual work station context.
 *
 * OUTPUT
 *	taskPtr		- A pointer to a IPX EndPoint context which has been
 *			  allocated to on a virtual work station for a 
 *			  connection to a NetWare Server.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_RESOURCE	- All IPX EndPoints for the client context are
 *				  in use, the UNIX UID can't create a
 *				  connection to a NetWare Server without
 *				  disconnecting for another NetWare Server
 *				  first.
 *
 * DESCRIPTION
 *	Find a free slot in the IPX EndPoint connection table for a client
 *	virtual workstation context, and allocate for use by a new NetWare
 *	Server connection.
 *
 * NOTES
 *	This function creates a IPX EndPoint for use a connection on a 
 *	client virtual workstation context.  This is analogous to a connection
 *	table entry on a DOS/Windows or OS/2 machine.  The new IPX EndPoint
 *	be threaded into the Socket Set associated with the client context, 
 *	and will be used to connect the UNIX UID to a NetWare Server.
 *	See notes on IPXEngAllocClient for a complete discussion of the
 *	client context virtual workstation architecture.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	IPXEngOpenEndpoint(3K), IPXEngAllocClient(3K), IPXEngFreeTask(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngAllocTask_l (ipxTask_t *taskList, ipxTask_t **taskPtr)
{
	register int32	i;

	NTR_ENTER(2, taskList, taskPtr, 0, 0, 0);

	for ( i = 0; i < ipxEngTune.maxClientTasks; i++ ) {
		if (taskList[i].state == IPX_TASK_FREE) {
			*taskPtr = &(taskList[i]);
			bzero((char *)(*taskPtr), sizeof(ipxTask_t));
			if ( NWtlCreateAndSetPsuedoSema( &((*taskPtr)->syncSemaphore), 0)
							== SUCCESS ) {
				(*taskPtr)->state = IPX_TASK_INUSE;
				(*taskPtr)->smoothedRoundTrip = 
					(IPX_STARTING_ROUND_TRIP << 3);
				(*taskPtr)->smoothedVariance = (1 << 2);
				(*taskPtr)->reTransBeta = ((*taskPtr)->smoothedRoundTrip>>3) +
							  ((*taskPtr)->smoothedVariance>>1);
				(*taskPtr)->gtsEndPoint.realTsOps = NWgtsOpsSw[NOVELL_IPX];
				(*taskPtr)->gtsEndPoint.realEndPoint = (opaque_t *)*taskPtr;

				if (((*taskPtr)->taskLock = LOCK_ALLOC (NUCTASKLOCK_HIER, plstr,
							&taskLockInfo, KM_NOSLEEP)) == NULL) {
					cmn_err(CE_WARN,
							"IPXEngAllocTask: taskLock alloc failed");
					bzero((char *)(*taskPtr), sizeof(ipxTask_t));
					return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
				}

				NTR_LEAVE((uint_t) *taskPtr);
				return( SUCCESS );
			} else {
				return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
			}
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFreeTask(3K), \
 *		./man/kernel/ts/ipxeng/FreeTask)
 * NAME
 *	IPXEngFreeTask -	Find free slot in the client list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFreeTask( taskList, taskPtr )
 *	ipxTask_t	*taskList;
 *	ipxTask_t	*taskPtr;
 *
 * INPUT
 *	taskList	- A pointer to a vector of IPX EndPoint contexts
 *			  for a client virtual work station context.
 *	taskPtr		- A pointer to a IPX EndPoint context which is to be
 *			  released and made available for a new connection 
 *			  when needed.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_ENDPOINT	- The 'tasktPtr' is not asscociated with an
 *				  allocated connection on the virtual work
 *				  station context.
 *
 * DESCRIPTION
 *	Return a IPX EndPoint connecton on a virtual work staion context to a
 *	free state for future binding to a new NetWare Server connection.
 *
 * NOTES
 *	See notes on IPXEngAllocTask(3K).
 *
 *	I will most likely change this to check to ensure that the
 *	task pointer falls within the range of the task table, and
 *	de-reference the structure based upon offset instead of
 *	index.
 *
 * SEE ALSO
 *	IPXEngOpenEndpoint(3K), IPXEngAllocClient(3K), IPXEngAllocTask(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFreeTask_l (ipxTask_t *taskList, ipxTask_t *taskPtr)
{
	register int	i;
	pl_t			pl;

	NTR_ENTER(2, taskList, taskPtr, 0, 0, 0);

	for (i = 0; i < ipxEngTune.maxClientTasks; i++) {
		if (taskList[i].state == IPX_TASK_FREE) {
			continue;
		}

		if (taskPtr == &(taskList[i])) {
			pl = LOCK (taskPtr->taskLock, plstr);
			if (taskPtr->callOutID) {
				UNLOCK (taskPtr->taskLock, pl);
				untimeout (taskPtr->callOutID);
				pl = LOCK (taskPtr->taskLock, plstr);
				taskPtr->callOutID = 0;
			}
			NWtlDestroyPsuedoSema(&(taskPtr->syncSemaphore));
			taskList[i].state = IPX_TASK_FREE;
			UNLOCK (taskList[i].taskLock, pl);
			LOCK_DEALLOC (taskList[i].taskLock);
			return( NTR_LEAVE( SUCCESS ) );
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_ENDPOINT ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFindTask(3K), \
 *		./man/kernel/ts/ipxeng/FindTask)
 * NAME
 *	IPXEngFindTask -Find free slot in the client list.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFindTask( taskList, address, taskPtr )
 *
 *	ipxTask_t	*taskList;
 *	ipxAddress_t	*address;
 *	ipxTask_t	**taskPtr;
 *
 * INPUT
 *	taskList	- A pointer to a vector of IPX EndPoint contexts
 *			  for a client virtual work station context.
 *	address		- A pointer to a IPX NET:NODE:SOCKET address to find
 *			  a connection on the client virtual workstation 
 *			  context.
 *
 * OUTPUT
 *	taskPtr		- A pointer to a IPX EndPoint connection which is 
 *			  associated with the specified NetWare Server.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_NO_ENDPOINT	- The 'address' is not asscociated a connected
 *				  IPX EndPoint on the virtual work station
 *				  context.
 *
 * DESCRIPTION
 *	Search for a NetWare Server connection of the specified address on a
 *	client virtual workstation context.
 *
 * NOTES
 *	See notes on IPXEngAllocTask(3K).
 *
 * SEE ALSO
 *	IPXEngOpenEndpoint(3K), IPXEngAllocClient(3K), IPXEngAllocTask(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFindTask_l (ipxTask_t *taskList, ipxAddress_t *address,
					ipxTask_t **taskPtr)
{
	register int	i;

	NTR_ENTER(3, taskList, address, taskPtr, 0, 0);

	for (i = 0; i < ipxEngTune.maxClientTasks; i++) {
		if (!((taskList[i].state & IPX_TASK_INUSE) &&
			(taskList[i].state & IPX_TASK_CONNECTED)) )
			continue;

		/*
		 * NOTE!!
		 *	The original comparison was on NET:NODE:SOCKET.  This
		 *	proved to be a problem for watch dogs, which originate
		 *	on 0x4003 and not the service socket 0x0451.  Thus we assume
		 *	that only one NET:NODE per client customer (one connection on
		 *	a ipxClient_t to a NetWare Server) in the new check of only
		 *	NET:NODE.
		 */
		if (!bcmp((caddr_t)address->ipxAddr,
			(caddr_t)taskList[i].address.ipxAddr, (IPX_ADDR_SIZE-2)) ) {

			*taskPtr = &(taskList[i]);

			NTR_LEAVE((uint_t) *taskPtr);
			return( SUCCESS );
		}
	}

	return( NTR_LEAVE( NWD_GTS_NO_ENDPOINT ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngAllocateSocketSet(3K), \
 *		./man/kernel/ts/ipxeng/AllocateSocketSet)
 * NAME
 *	IPXEngAllocateSocketSet -	Allocate 3 IPX sockets for use by NCP.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngAllocateSocketSet( clientPtr )

 *	ipxClient_t	*clientPtr;
 *
 * INPUT
 *	clientPtr	- A pointer to the client virtual workstation context
 *			  containing an empty socket set structure.
 *
 * OUTPUT
 *	clientPtr->socketSet.serviceSocket	- Set to an ephemeral socket.
 *	clientPtr->socketSet.watchdogSocket	- Set to serviceSocket + 1;
 *	clientPtr->socketSet.outOfBandSocket	- Set to serviceSocket + 2;
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *	NWD_GTS_ADDRESS_IN_USE	- All ephemeral attempts were unable to
 *				  get the 3 ordinal sockets bound.
 *	*			- A GIPC Failure mapped to GTS diagnostic.
 *
 * DESCRIPTION
 *	Calls the ipc library routines to allocate the standard
 *	IPX socket set necessary to communicate with an IPX/NCP
 *	file server.  
 *
 * NOTES
 *	The Socket Set of a virtual workstation is a follows:
 *		Service Socket (Ephemeral; 0x4000 - 0x8000)
 *			Used to issue all IN-BAND NCP Requests from client work
 *			station to NetWare Server, and receive NCP responses
 *			from NetWare Server.
 *		Watch Dog Socket(Service+1)
 *			Used by NetWare Server to unsolicit ping client work
 *			station to determine if a idle connection should be
 *			kept alive.
 *		MessageSocket(Service+2)
 *			Used by NetWare Server to unsolicit a OUT-OF-BAND event
 *			notification to a client work station.  The event may
 *			be notification a message needs to be retrieved by 
 *			the client work station, a SFT III server redirect which
 *			instructs the client workstation to move its 
 *			connection to the backup fault tolerant server clone,
 *			or may be a cache consistency indication for cache
 *			synchronization.
 *		Packet Burst Socket (Service+3)
 *			Used to issue all IN-BAND NCP Packet Burst requests.
 *
 *	See notes on IPXEngAllocClient(3K) and IPXEngAllocTask(3K) for more
 *	information on the socket set relationship to a client virtual work
 *	station and its connections.
 *
 *	Persistance is not a tuneable parameter.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	IPXEngAllocateIPCSocket(3K), IPXEngAllocClient(3K), IPXEngAllocTask(3K)
 *	IPXEngFreeSocketSet(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngAllocateSocketSet (ipxClient_t *clientPtr )
{
	ccode_t		ccode;
	register ipxSocket_t	*ss,*ws,*os,*ps;
	uint16		socketID;
	register	int32	tryCount = 0;
	pl_t		pl;

	NTR_ENTER(1, clientPtr, 0, 0, 0, 0);

	/*
	 *	Allocate the three sockets needed to make this thing work
	 *	ss = Service Socket
	 *	ws = Watch Dog Socket
	 *	os = Message (Out-Of-Band) Socket
	 */
	ss = &(clientPtr->socketSet.serviceSocket);
	ws = &(clientPtr->socketSet.watchdogSocket);
	os = &(clientPtr->socketSet.outOfBandSocket);
	ps = &(clientPtr->socketSet.packetBurstSocket);

	/*
	 *	First pass, allow the IPX driver to find a free on for
	 *	us to start with.
	 */
	ss->socketID[0] =  ss->socketID[1] = 0;

	/*
	 * Create a synchronization semaphore for socket binding
	 */
	pl = RW_WRLOCK (nucTSLock, plstr);
	if (NWtlCreateAndSetSemaphore((int *)&(clientPtr->syncSemaphore), 0)
									!= SUCCESS) {
		RW_UNLOCK (nucTSLock, pl);
		return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
	}
	RW_UNLOCK (nucTSLock, pl);

	/*
	 *	Now, loop in here until the sockets are successfully allocated,
	 *	or the retry count is exceeded
	 */
	do {
		/*
		 *	Allow IPX the chance to look for one for us.
		 */
		ccode = IPXEngAllocateIPCSocket(&(ss->ipcChannel), clientPtr, 
				ss->socketID, (void_t (*)())IPXEngInterruptHandler);
		if ( ccode ) {		 /* if we can't get the first, quit */
			NWtlDestroySemaphore(clientPtr->syncSemaphore);
			return( NTR_LEAVE( NWD_GTS_NO_RESOURCE ) );
		}
	
		IPXEngResetSocketStats( ss );
	
		/*
		 *	Watchdog has to be one plus service socket
		 */
		socketID = *(uint16 *)ss->socketID;
	
		socketID++;
		*(uint16 *)ws->socketID = socketID;
		ccode = IPXEngAllocateIPCSocket(&(ws->ipcChannel), clientPtr, 
				ws->socketID, (void_t (*)())IPXEngWDSocketInterruptHandler);
		if ( ccode ) {
			IPXEngFreeIPCSocket( ss->ipcChannel );
			goto socketRetry;
		}

		IPXEngResetSocketStats( ws );
	
		/*
		 *	Message socket has to be two plus service socket
		 */
	
		socketID++;
		*(uint16 *)os->socketID = socketID;
		ccode = IPXEngAllocateIPCSocket(&(os->ipcChannel), clientPtr, 
				os->socketID, (void_t (*)())IPXEngOOBSocketInterruptHandler);
		if ( ccode ) {
			IPXEngFreeIPCSocket( ss->ipcChannel );
			IPXEngFreeIPCSocket( ws->ipcChannel );
			goto socketRetry;
		}

		IPXEngResetSocketStats( os );

		/*
		 *	Packet Burst socket has to be three plus service socket
		 */
	
		socketID++;
		*(uint16 *)ps->socketID = socketID;
		ccode = IPXEngAllocateIPCSocket(&(ps->ipcChannel), clientPtr, 
				ps->socketID, (void_t (*)())IPXEngInterruptHandlerBurst);
		if ( ccode ) {
			IPXEngFreeIPCSocket( ss->ipcChannel );
			IPXEngFreeIPCSocket( ws->ipcChannel );
			IPXEngFreeIPCSocket( os->ipcChannel );
			goto socketRetry;
		}

		IPXEngResetSocketStats( ps );

		goto socketContinue;

socketRetry:
	/*
	 *	If the allocation of one of the sockets fails, try it again
	 *	with a number in a higher range
	 */
		++tryCount;
		socketID = *(uint16 *)ss->socketID;
		socketID+=4;
		*(uint16 *)ss->socketID = socketID;
socketContinue:
		;
	}
	while ((ccode != SUCCESS) && (tryCount < IPXE_SOCKET_ALLOC_RETRY));
	
	/*
	 * Destroy the socket binding synchronization semaphore
	 */
	NWtlDestroySemaphore(clientPtr->syncSemaphore);

	return( NTR_LEAVE( ccode ) );
}


/*
 * BEGIN_MANUAL_ENTRY(IPXEngFreeSocketSet(3K), \
 *		./man/kernel/ts/ipxeng/FreeSocketSet)
 * NAME
 *	IPXEngFreeSocketSet -	Free socket set allocated for a client.
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngFreeSocketSet( clientPtr )
 *
 *	ipxClient_t	*clientPtr;
 *
 * INPUT
 *	clientPtr	- A pointer to the client virtual workstation context
 *			  containing an bound socket set structure.
 *
 * OUTPUT
 *	clientPtr->socketSet.serviceSocket	- Set to 0.
 *	clientPtr->socketSet.watchdogSocket	- Set to 0. 
 *	clientPtr->socketSet.outOfBandSocket	- Set to 0.
 *	clientPtr->socketSet.packetBurstSocket	- Set to 0.
 *
 * RETURN VALUES
 *	0			- Successful completion.
 *
 * DESCRIPTION
 *	Calls the ipc library routines to free the standard
 *	IPX socket set necessary to communicate with an IPX/NCP
 *	file server.  
 *
 * NOTES
 *	See notes on IPXEngAllocateSocketSet(3K).
 *
 *	When all client tasks are gone, free the client and the sockets
 *	that he is using as well.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	IPXEngFreeIPCSocket(3K), IPXEngAllocClient(3K), IPXEngAllocTask(3K)
 *	IPXEngAllocateSocketSet(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngFreeSocketSet (ipxClient_t *clientPtr )
{
	ipxSocket_t	*wds,*oobs,*ss,*ps;	

	NTR_ENTER(1, clientPtr, 0, 0, 0, 0);

	ss = &(clientPtr->socketSet.serviceSocket);
	wds = &(clientPtr->socketSet.watchdogSocket);
	oobs = &(clientPtr->socketSet.outOfBandSocket);
	ps = &(clientPtr->socketSet.packetBurstSocket);

	IPXEngFreeIPCSocket( wds->ipcChannel );
	IPXEngFreeIPCSocket( oobs->ipcChannel );
	IPXEngFreeIPCSocket( ss->ipcChannel );
	IPXEngFreeIPCSocket( ps->ipcChannel );

	return( NTR_LEAVE( SUCCESS ) );
}
