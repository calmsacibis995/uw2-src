/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengine.h	1.16"
#ifndef _NET_NUC_IPXENG_IPXENGINE_H
#define _NET_NUC_IPXENG_IPXENGINE_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengine.h,v 2.53.2.3 1994/12/21 02:46:53 ram Exp $"

/*
 *  Netware Unix Client 
 *
 *	  MODULE: ipxengine.h
 *	ABSTRACT: Define data structures associated with the IPX protocol
 *		  engine.
 */


#ifdef _KERNEL_HEADERS
#include <net/nw/ipx_app.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/ncpiopack.h>
#else  _KERNEL_HEADERS
#include <sys/ipx_app.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/nuctool.h>
#endif _KERNEL_HEADERS



/*
 *	HZ is Ticks per second
 */

/*
 *	Task state constants (IPX Connection States)
 *
 *
 *	IPX_TASK_FREE		- Task node is currently on the free list
 *	IPX_TASK_INUSE		- Task has been allocated for use
 *	IPX_TASK_TRANSMIT	- Packet has been transmitted (SendPacket)
 *	IPX_TASK_TIMEDOUT	- Task has been disabled due to watchdog or
 *				  retry fail
 *	IPX_TASK_DYING		- Last NCP Response had Bad Connection Status
 *				  Bit (Server Gone or Connection Gone).
 *	IPX_TASK_CONNECTED	- Task is connected by address to the server
 *	IPX_TASK_BURST		- A Packet Burst transaction is in progress
 *
 */
#define	IPX_TASK_FREE		0	/* Not in use			*/
#define IPX_TASK_INUSE		(1<<0)	/* In use			*/
#define IPX_TASK_TRANSMIT	(1<<1)	/* Packet on the wire		*/
#define IPX_TASK_TIMEDOUT	(1<<2)	/* Connection is timed out	*/	
#define	IPX_TASK_DYING		(1<<3)	/* Connection no longer valid	*/
#define	IPX_TASK_CONNECTED	(1<<4)	/* Connection in place 		*/
#define	IPX_TASK_BURST		(1<<5)	/* Packet Burst in progress	*/
#define IPX_REPLY_TIMEDOUT	(1<<6)	/* Reply not received by specified time	*/	

/*
 *	Client structure state variables ( Virtual Workstations )
*/
#define IPX_CLIENT_FREE		0
#define	IPX_CLIENT_INUSE	(1<<0)
#define	IPX_CLIENT_PRIVATE	(1<<1)	/* private connection		*/

/*
 * Retransmission Constants
 */
#define	MAX_EXP_SHIFT	10	/* Maximun shift in exponential backoff  */
#define	EXP_SHIFT_CLIP	4	/* Max beliveable shifts per request/response */
#define	IPX_STARTING_ROUND_TRIP	(HZ / 2)	/* 1/2 second	*/
#define	MAX_RETRANS_QUANTUM	(HZ * 30)	/* 1/2 minute	*/

/*
 * NAME
 *	ipxAddress	- The format of an IPX address.	
 *
 * DESCRIPTION
 *	IPX uses an address format to identify the network, computer (client
 *	or server), and application on the machine to send a packet to.  This
 *	address uniquely identifies the recepient of a message (NCP Request/
 *	Response).  It is comprised of a triplet of the following components.
 *	
 *	network		- 32 bit logical network address.
 *	node		- 48 bit physical address of machine on network.
 *	socket		- 16 bit logical address of application on machine.
 */
typedef union {
	struct {
		uint8	network[IPX_NET_SIZE];
		uint8	node[IPX_NODE_SIZE];
		uint8	socket[IPX_SOCK_SIZE];
	} ipxComp;
	uint8	ipxAddr[IPX_ADDR_SIZE];
} ipxAddress_t;

/*
 * NAME
 *	socketStat	- Statistics of a IPX socket.
 *
 * DESCRIPTION
 *	The statistics of a IPX socket in use by a ipxSocket in a
 *	ipxSocketSet in a ipxClient virtual work station.
 *
 *	packetsSent		- Total number of Requests ( including
 *				  retransmitted sent on socket.
 *	packetsReceived		- Total number of Responses ( including
 *				  duplicates) received on socket.
 *	badPacketsReceived	- Total number of spurious responses 
 *				  received on socket.
 */
typedef struct {
	int32		packetsSent;
	int32		packetsReceived;
	int32		badPacketsReceived;
} socketStat_t;

/*
 * NAME
 *	ipxSocket	- A IPX Socket of a virtual work station.
 *
 * DESCRIPTION
 *	The IPX Socket represents a address in the IPX stack.  The socket
 *	defines the actual IPX socket ID, and context of the socket.  It is
 *	one of three such sockets associated with a ipxSocketSet in a 
 *	ipxClient virtual work station object.
 *
 *	ipcChannel		- The GIPC Channel to the IPX stack socket
 *				  endpoint.
 *	stat			- The current statistics of the socket.  See
 *				  socketStat structure for a description.
 *	socketID		- The 16 bit value of the IPX socket address
 *				  bound to this socket.  (In the range
 *				  of 0x4000-0x7FFF).
 */
typedef struct ipxSocket {
	void_t		*ipcChannel;
	socketStat_t	stat;
	uint8		socketID[IPX_SOCK_SIZE];
} ipxSocket_t;


/*
 * NAME
 *	ipxSocketSet -	A virtual workstation set of ordinal IPX sockets.
 *
 * DESCRIPTION
 *	The IPX Socket Set of a virtual workstation client context.  Part
 *	of a ipxClient structure.  A socket set consists of three sockets, 
 *	which are ordinally related, and have an ephemeral base assigned.
 *	
 *	serviceSocket	- (Ephemeral Base ox4000-0x7FFD)
 *			  Used to send and receive all NCP Requests and 
 *			  Responses for NOS service.
 *	watchdogSocket	- (serviceSocket + 1)
 *			  The keep alive heartbeet of a IPX connection.  The
 *			  watch dog packet is sent the NetWare Server, and 
 *			  it is up to the NetWare Client to respond for the
 *			  connection to be kept alive on idle sessions.
 *	outOfBandSocket - (serviceSocket + 2)
 *			  The out of band indication of a IPX connection.  The
 *			  NetWare Server sends a event such as message pending,
 *			  SFT III redirection, or Cache Consistency on this
 *			  socket.  
 *	packetBurstSocket - (serviceSocket + 3)
 *				Used to send and receive all NCP Packet Burst
 *				requests and responses.
 *
 */

typedef struct ipxSockSet {
	ipxSocket_t	serviceSocket;
	ipxSocket_t	watchdogSocket;
	ipxSocket_t	outOfBandSocket;
	ipxSocket_t	packetBurstSocket;
} ipxSocketSet_t;
	
/*
 * NAME
 *	ipcTask	- The IPX Endpoint of a IPX connection
 *
 * DESCRIPTION
 *	The focal object of a IPX connection (Endpoint)to a NetWare Server.  The
 *	connection is multiplexed into a common socket set for a UNIX UID
 *	(ipxClient).  Each connection uses the same client side address, but
 *	has a seperate Server Address, which when concatenated with the
 *	client address forms a unique IPX connection.  This multiplexing was
 *	done for 2 reasons.  First, it emulates the behavior of DOS, Windows,
 *	and OS/2 clients.  Second, it reduces IPC resources, and IPX stack
 *	resources.
 *
 *	gtsEndPoint		- The generic GTS End Point component.
 * 	state			- The current state (inclusive OR) of a IPX
 *				  connection.  Valid states are:
 *					IPX_TASK_FREE
 *						Availabe for a connection.
 *					IPX_TASK_INUSE	
 *						Connecton is valid.
 *					IPX_TASK_TRANSMIT
 *						Connection has an outstanding
 *						NCP Request.
 *					IPX_TASK_TIMEDOUT
 *						Connection has timeout out.
 *	syncSemaphore		- The P/V semaphore used between
 *				  IPXEngGetPacket(3K) and
 *				  IPXEngInterruptHandler(3K) to synchronize
 *				  on a received NCP Response.  The traditional
 *				  sleep/wakeup will deadlock if the response
 *				  arrive before the NCP is ready to process
 *				  it, thus the P/V is employed making event
 *				  timing immaterial.
 *	burstSyncSemaphore - simliar to syncSemaphore for Packet Burst.
 *	callOutID		- The id of the IPXEngConnectionTimeout(3K) 
 *				  retransmission handler for this connection
 *				  on the call out list.
 *	burstCallOutID - similar to callOutID for Packet Burst.
 *	timerTicks		- HZ ticks remaining in current timer until
 *				  a retransmission is to take place.
 *	waitTime		- Accumulating HZ ticks of an outstanding
 *				  request, waiting on a response.  Incremented
 *				  every 'timerTicks' quantum.
 *	smoothedRoundTrip	- Current smoothed round trip time in 
 *				  HZ ticks.  See IPXEngGetPacket(3K) for a
 *				  description.
 *	roundTripStart		- System time in microseconds when the request
 *				  was sent.  Used as the base of delta
 *				  calculation of a round trip time when the
 *				  response is received.  See IPXEngGetPacket(3K)
 *				  for a description.
 *	smoothedVariance	- Smoothed delta betwen request/response 
 *				  round trip times in HZ ticks. See
 *				  IPXEngGetPacket(3K) for a description.
 *	reTransBeta		- Current base timer value in HZ ticks
 *				  before retransmitting NCP request. Set to 
 *				  (smoothedRoundTrip + (2 * smoothedVariance)).
 *				  See IPXEngGetPacket(3K) for a description.
 *	curBackOffDelta		- Current exponential backoff delta in bit
 *				  shift applied to 'reTransBeta' for timer
 *				  expiration. See IPXEngGetPacket(3K) for
 *				  a description. 
 *	clientPtr		- Pointer to the ipxClient virtual workstation
 *				  object of the UNIX UID.
 *	ipcMsgHandle		- Pointer to the GIPC duplicate message of
 *				  the outstanding NCP Request.  Used to
 *				  retransmit the Request in retransmitting
 *				  the request.
 *	asyncHandler		- The call back handler for asynchronous 
 *				  events.
 *	asyncConetextHandle	- The handle to pass back to the call back
 *				  handler.
 *	address			- The NET:NODE:SOCKET IPX address of the
 *				  NetWare Server connected to this endpoint.
 *	sequenceNumber		- The NCP sequence number of the outstanding
 *				  NCP Request.
 *	channel			- Pointer back to the ncp_channel_t allocated
 *					by NCP for this connection, containing packet burst 
 *					information used by IPXENG.
 *	taskLock		- Lock guarding the ipxTask
 *	taskHold		- Hold count indicating as to wheter the syncSemaphore
 *					  has been set or not
 */
typedef struct ipxTask {
	GTS_ENDPOINT_T		gtsEndPoint;
	int32				state;
	psuedoSema_t		*syncSemaphore;
	int32				callOutID;
	int32				timerTicks;
	int32				waitTime;
	int32				smoothedRoundTrip;
	timestruc_t			roundTripStart;
	timestruc_t			roundTripTime;
	int32				smoothedVariance;
	int32				reTransBeta;
	int32				curBackOffDelta;
	struct ipxClient	*clientPtr;
	void_t				*ipcMsgHandle;
	void_t				(*asyncHandler)();
	void_t				*asyncContextHandle;
	ipxAddress_t		address;
	int32				sequenceNumber;
	void_t				*channel;
	lock_t				*taskLock;
} ipxTask_t;

/*	We can safely manipulate the ncp channel data in IPXEng because
 *	the sleep_lock guarding ncp_channel_t is currently held by the
 *	calling lwp
 */
#define IPXTASK_HOLD(taskPtr) {					\
	((ncp_channel_t *)(taskPtr->channel))->channelHold++;			\
}

#define IPXTASK_IS_HELD(taskPtr)				\
	(((ncp_channel_t *)(taskPtr->channel))->channelHold > 0)

#define IPXTASK_RELE(taskPtr) 					\
	((ncp_channel_t *)(taskPtr->channel))->channelHold--;			\
	if (((ncp_channel_t *)(taskPtr->channel))->channelHold < 0)		\
		((ncp_channel_t *)(taskPtr->channel))->channelHold = 0;

/*
 * NAME
 *	ipxClient	- Virtual workstation of UNIX UID.
 *
 * DESCRIPTION
 *	The focal object of a client, which is represented as a virtual
 *	workstation.  All IPX connections (ipxTask) to NetWare Servers for
 *	a UNIX UID are multiplexed into the same socket set.
 *
 * 	state			- The current state (inclusive OR) of a 
 *				  virtual workstation.  Valid states are:
 *					IPX_CLIENT_FREE
 *						Availabe for a new virtual 
 *						workstation.
 *					IPX_CLIENT_INUSE
 *						Virtual workstation is
 *						allocated to a UNIX UID.
 *	syncSemaphore		- The P/V semaphore used between
 *				  IPXengEngAllocateIPCSocket(3K) and
 *				  IPXEngStreamsTPIBind(3K) to synchronize
 *				  on a received bind ack.  The traditional
 *				  sleep/wakeup will deadlock if the ack
 *				  arrives before the process is ready to receive
 *				  it, thus the P/V is employed making event
 *				  timing immaterial.
 *	credentailsPtr		- A pointer to the generic credentials of the
 *				  UNIX UID associated with this virtual
 *				  work station.
 *	taskList		- A pointer to the vector of ipxTask
 *				  connections using this virtual work station.
 *	numTasks		- The number of connections in place with
 *				  NetWare Servers at this time on this virtual
 *				  work station.
 *	stats			- NUC Diagnostic Socket Statistics
 */
typedef struct ipxClient {
	int32			state;
	int32			syncSemaphore;
	void_t			*credentialsPtr;
	ipxTask_t		*taskList;	
	int32			numTasks;
	ipxSocketSet_t	socketSet;
	NUC_IPXENG_STATS_T stats;
	rwlock_t		*clientRWLock;
} ipxClient_t;

/*
 * Macros from the former ipxstat.c module
 */
#define	IPXEngResetSocketStats( socket )		\
	do {						\
       		(socket)->stat.packetsReceived = 0;	\
        	(socket)->stat.badPacketsReceived = 0;	\
        	(socket)->stat.packetsSent = 0;		\
	} while (0)

#define	IPXEngIncPacketReceiptCount( socket )	\
        (socket)->stat.packetsReceived++

#define	IPXEngIncBadPacketReceiptCount( socket )	\
        (socket)->stat.badPacketsReceived++

#define	IPXEngIncPacketTransmitCount( socket )	\
        (socket)->stat.packetsSent++

/*
 * Macros formerly functions in ipxestruct.c
 */
#define	IPXEngIsTaskValid( endPointPtr )	\
        ((endPointPtr)->state & IPX_TASK_TIMEDOUT) ? TRUE : FALSE 
/*
    NCP_REPLY_HEADER_T is used to map the NCP response header and the
	#defines of the connectionStatus bitfield.
*/
typedef struct  {
    uint16  replyType;
    uint8   sequenceNumber;
    uint8   connectionNumberLowOrder;
    uint8   taskNumber;
    uint8   connectionNumberHighOrder;
    uint8   completionCode;
    uint8   connectionStatus;
}NCP_REPLY_HEADER_T;

#define CONN_STATUS_BAD_CONNECTION		0x01
#define CONN_STATUS_NO_SLOTS_AVAIL		0x04
#define CONN_STATUS_SERVER_DOWN			0x10
#define CONN_STATUS_MESSSAGE_WAITING	0x40

#define NCP_SERVER_BUSY 0x9999
#define NCP_LOST_CONNECTION 0x01
#define NCP_SERVER_GOING_DOWN 0x10

#ifdef NUC_DEBUG

#ifdef _KERNEL_HEADERS
#include <util/cmn_err.h>
#elif defined(_KERNEL)
#include <sys/cmn_err.h>
#else
#include <sys/cmn_err.h>
#endif	/* _KERNEL_HEADERS */

#define IPXENG_CMN_ERR cmn_err
#else
#define IPXENG_CMN_ERR 
#endif	/* NUC_DEBUG */

#endif /* _NET_NUC_IPXENG_IPXENGINE_H */
