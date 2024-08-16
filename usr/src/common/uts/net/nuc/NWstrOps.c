/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/NWstrOps.c	1.24"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/NWstrOps.c,v 2.57.2.4 1995/02/13 07:53:59 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    NWstrOps.c -	The NUC STREAMS IPC Mechanism package Operations.
 *			Component of the NUC Core Services Device.
 *
 *  ABSTRACT:
 *    The NWstrOps.c contains the version STREAMS HEAD functions.  See
 *    NWstrIntroduction(3K) for a complete description of the STREAMS HEAD
 *    package in the Generic Inter Process Communications Layer.
 *
 *   The following NWgipcOpsSw[] Operations are contained in this module.
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].CloseIpcChannel)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].FlushIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].FreeIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].InitializeIpcService)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].OpenIpcChannel)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].PreviewIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].PrivateIoctlMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].ReceiveIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].RegisterAsyncEventHandler)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].ReSendIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].SendIpcMessage)()
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].SizeOfIpcMessage)()
 *
 */

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/gipcconf.h>
#include <net/nuc/strchannel.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/headstrconf.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nuc_prototypes.h>

#include <mem/kmem.h>
#include <util/ksynch.h>
#include <net/nuc/nuc_hier.h>

#include <io/ddi.h>

#else /* ndef _KERNEL_HEADERS */

#include <sys/strsubr.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/nucmachine.h>
#include <sys/gipccommon.h>
#include <sys/gipcchannel.h>
#include <sys/gipcconf.h>
#include <sys/strchannel.h>
#include <sys/ipx_app.h>
#include <sys/headstrconf.h>
#include <sys/streamsconf.h>
#include <sys/nuctool.h>

#include "nuc_hier.h"

#endif /* ndef _KERNEL_HEADERS */

LKINFO_DECL(channelRWLockInfo, "NETNUC:NUC:channelRWLock", 0);
LKINFO_DECL(NWstrChannelListSleepLockInfo, "NETNUC:NUC:strChannelListSleepLockInfo", 0);

#define NTR_ModMask	NTRM_gipc

extern lock_t	*criticalSectLock;
extern rwlock_t	*nucIPCLock;
extern sv_t		*nucHandleIpxHangupSV;

sleep_t			*NWstrChannelListSleepLock;

void_t	NWstrReadHeadService (queue_t *);
ccode_t	NWstrCloseStream ( queue_t *, vnode_t *);
ccode_t NWstrOpenStream ( char *, queue_t **, vnode_t **, int32 *);

int	copyin(), copyout(), bufcall();
extern int strmsgsz, strctlsz;

/*
 * Forward Reference Functions
 */

void_t strIoctlTimeOut(); 
ccode_t drainStreamMsg();
mblk_t *buildStreamMsg();


/*
 * BEGIN_MANUAL_ENTRY(NWstrIntroduction(3K), \
 *			./man/kernel/kipc/streams/Introduction)
 * NAME
 *	NWstrIntroduction	- Introduction to the Novell NetWare UNIX
 *				  Client STREAMS Head Inter Process
 *				  Communication Package.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 * DESCRIPTION
 *	The installed NUC STREAMS IPC mechanism package.
 *	This package is implemented as a private STREAMS Head, which
 *	operates in parallel with the regular STREAMS Head, but services NUC
 *	Core Service Requests directly in a request/response event driven
 *	manner.  It is plugged into the Generic Inter Process Communications
 *	Layer Operations Switch under the name RITCHIE_STREAMS (ie. 
 *	NWgipcOpsSw[RITCHIE_STREAMS]).  Thus all RITCHIE_STREAMS services are
 *	provided to the GIPC via this private STREAMS Head.  See
 *	NWpcIntroduction(3K) for information on the GIPC.  This package is 
 *	tightly coupled with the GIPC layer, and does not expect to be directly
 *	called, rather it is designed to be indirectly called through the GIPC
 *	layer.
 *
 *	The Streams Inter Process Communication Operations are externalized
 *	indirectly through the Generic Inter Process Communication Operation
 *	Switch structure NWgipcOpsSw[].  This provides an object oriented
 *	interface to its services.  The Generic Inter Process Communication
 *	(GIPC) layer of the Core Services makes STREAMS operation calls via the
 *	NWgipcOpsSw[RITCHIE_STREAMS] for service.  The	STREAMS provides the
 *	inter process communications mechanims necessary for Cor Services to 
 *	to communicate with the transport stacks in a consistent transparent
 *	fashion.
 *
 * STREAMS INTER PROCESS COMMUNICATIONS OPERATIONS
 *	close STREAMS channel		- NWstrCloseIpcChannel(3K)
 *	flush current STREAMS message	- NWstrFlushIpcMessage(3K)
 *	free a re-send IPC message	- NWstrFreeIpcMessage(3K)
 *	start or stop NUC STREAMS	- NWstrInitializeIpcService(3K)
 *	open a STREAMS channel		- NWstrOpenIpcChannel(3K)
 *	preview a STREAMS message	- NWstrPreviewIpcMessage(3K)
 *	send a private control message	- NWstrPrivateIoctlMessage(3K)
 *	receive a STREAMS message	- NWstrReceiveIpcMessage(3K)
 *	register async event handler	- NWstrRegisterAsyncEventHandler(3K)
 *	re send a IPC message		- NWstrReSendIpcMessage(3K)
 *	send a STREAMS message		- NWstrSendIpcMessage(3K)
 *	get size of a STREAMS message	- NWstrSizeOfIpcMessage(3K)
 *
 * OPERATION NOTES
 *
 *	The 'NWstrInitializeIpcService' is used to start or stop the NUC STREAMS
 *	Head Package.  It by default is in a STOP state
 *	until specifically started, which brings it ready for service.  A
 *	switch from the START state to the STOP state will cause all Channels
 *	to be closed, and will not allow any new service until started again.
 *
 *	The 'NWstrOpenIpcChannel' is called to establish a STREAMS Channel.  This
 *	Channel is used for all communication with the peer process it is 
 *	opened with.
 *
 *	The 'NWstrRegisterAsyncEventHandler' is issued for Channels which operate
 *	asyncrhonously (ie. opened with GIPC_ASYNC mode).  The registered call
 *	back function will be called by the NUC STREAMS Head to process messages
 *	on the read queue of the STREAM.  It must be stressed that the
 *	asyncrhonous handler function registered runs in a asynchronously
 *	scheduled context within the UNIX kernel, and therefore has no Process
 *	Context, which precludes it from sleeping.  Only one Asyncrhonous Event
 *	may be scheduled at a time on Channel, therefore the the handler need not 
 *	protect itself from a race with another event on the same Channel.  See
 *	Event Syncrhonization Paragraph for event scheduling.	
 *
 *	The 'NWstrPrivateIoctlMessage' is used to send private control messages
 *	to the peer process.  This is analogous to the UNIX IOCTL(2) system 
 *	call.  The message must be format according to the STREAMS PROGRAMMER's
 *	Guide V3.2/386 Page B-4.  This operation always	operates synchronously,
 *	thus it will block until a response is received	from the peer process.
 *
 *	The 'NWstrSendIpcMessage' is used to send a message to the peer process
 *	of the STREAM.  It honors flow control back pressure on the Channel, and
 *	will block on Channels opened for syncrhonous operation, or return a
 *	diagnostic indicating blocking state to Channels opened for asyncrhonous
 *	operation.
 *
 *	The 'NWstrFreeIpcMessage' is used to free a duplicated message which has
 *	been sent via NWstrSendIpcMessage(3K) for use in NWstrReSendIpcMessage(3K)
 *	when it is no longer needed for a re-transmission.
 *
 *	The 'NWstrReSendIpcMessage' is used to re-transmit a IPC message to the
 *	peer process.  This allows a duplicated IPC message to be re-sent to the
 *	peer process in the case of a lost message (ie. a Transport time out).
 *
 *	The 'NWstrSizeOfIpcMessage' is used to determine the size of a received
 *	STREAMS message.  It is normally used in conjuction with
 *	NWstrPreviewIpcMessage(3K) to determine the total number of blocks in the
 *	message, and with NWstrReceiveIpcMessage(3K) to determine the total number
 *	of bytes in the message.
 *
 *	The 'NWstrPreviewIpcMessage' is used to map the STREAMS message into the
 *	callers visibility one block per portion at a time.  This allows peeking,
 *	or specific data extraction without having to copy the message.
 *
 *	The 'NWstrReceiveIpcMessage' is used to copy the STREAMS message into the
 *	callers buffers.  It will fill the callers buffers and notify the caller
 *	if there is more left to be copied.  It is the responsibility of the 
 *	caller to completely drain the message by subsequent calls to receive or
 *	by flushing the residual unconsumed portions of the message.
 *
 *	The 'NWstrFlushIpcMessage' is used to flush the residual portions of a 
 *	a message being received, or to flush one or all messages on the read
 *	and/or write queues of the STREAM.
 *
 *	The 'NWstrCloseIpcChannel' is called to dismantle a STREAMS Channel.  Any
 *	queued messages will be flushed prior to close, therefore close should
 *	not be called until the messages have been drained, or loss of data can
 *	occur.
 *
 *	NUC STREAMS HEAD PROCEDURES
 *
 *	The 'NWstrReadHeadPut' is called by the peer process to receive a new
 *	STREAMS message.  It will process Ioctl and non consumer messages
 *	directly.  All other GIPC consumer messages are queued for service
 *	by NWstrReadHeadService(3K).  This is done to stop upstream messages
 *	from consuming any more or the stack.  Since it is not known whether
 *	we are called in any interrupt context, the messages is trapped out to
 *	STREAMS service sheduling to avoid stack consumtion and interrupt time
 *	consumption.   Also it is not known the depth of STREAMS MUXes on the
 *	stack, thus this protects stack consumption in this case as well.
 *
 *	The 'NWstrReadHeadService' is called by the STREAMS scheduler to process
 *	messages which have been queued by NWstrReadHeadPut(3K).  This occurs
 *	when a message received from a peer process is destined for the GIPC
 *	consumer.  This service procedure will schedule the message if the
 *	Channel is avaialbe (no message being consumed by a process), otherwise
 *	the service procedure will disable the queue from scheduling to 
 *	guarantee event synchronization on the Channel. 
 *
 *	The 'NWstrWriteHeadService' is called by the STREAMS scheduler to
 *	process all down stream messages which are sent to the peer process. 
 *	In order to insure context synchronicity for the request/response
 *	model, all messages sent to peer processes are queued for delayed
 *	service in a process contextless state, which allows the NUC STREAMS
 *	Head to prepare itself for the	response prior to sending the message.
 *
 *	EVENT SYNCHRONIZATION
 *
 *	The receipt of messages is event driven.  The NUC STREAMS provides a
 *	comprehensive event synchronization mechanism.  The NUC STREAMS may be
 *	interrupted to receive a message, which will be queued.  If the
 *	Channel is operating syncrhonously, it will wakeup the receive handler
 *	if the process is waiting in receive, or when the next receive is issued
 *	the message will be processed.  Thus it is fair to say that Channels
 *	operating synchronously will never be interrupted, thus event
 *	synchronizaton is not an issue.  For Channels operating asyncrhonously,
 *	the asyncrhonous event handler will be called when there is no current
 *	message being processed.  Once an event is scheduled via asyncrhonous
 *	event handler, the Channel may not be rescheduled until the current 
 *	message is processed (drained, via NWstrReceiveIpcMsg(3K) or
 *	NWstrFlushIpcMessage(3K)).  Thus the only way a asyncrhonous event
 *	handler can be interrupted is when it processes the message.  Thus it
 *	is recommended that the asyncrhonous event handler simply preview the 
 *	message and wakeup the context the message belongs to, where upon the
 *	process context will drain the message, and allow further scheduling.
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY(NWstrCloseIpcChannel(3K), \
 *			 ./man/kernel/kipc/streams/CloseIpcChannel)
 * NAME
 *	NWstrCloseIpcChannel -
 *					Close a STREAMS Inter Process
 *					Communication Channel.  Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					Swtich structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].CloseIpcChannel(ipcChannel, diagnostic)
 *	
 *	opaque_t	*ipcChannel;	/* Opaque to caller	*\
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object to close.
 *
 * OUTPUT
 *	diagnostic	- Unused at this time.
 *
 * RETURN VALUES
 *	0	- Successful Completion.
 *
 * DESCRIPTION
 *	The 'NWstrCloseIpcChannel' closes the specified Channel with the peer
 *	process STREAMS device.  The channel (including STREAMS) will be
 *	dismantled and is no longer usable upon return.
 *
 * NOTES
 *	Any data queued on the channel will be flushed prior to close, which
 *	results in permanent loss of this unconsumed data.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrOpenIpcChannel(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrCloseIpcChannel (GIPC_CHANNEL_T  *ipcChannel, int32 *diagnostic)
{

	STR_CHANNEL_T		*strChannel;
	STR_CHANNEL_T		*aStrChannel;
	register	int		found;
	pl_t				pl;

	NTR_ENTER(2, ipcChannel, diagnostic, 0, 0, 0);

	*diagnostic = SUCCESS;

	if( NWstrInitialized == FALSE ) {
		*diagnostic = NWD_GIPC_NO_RESOURCE;
		return( NTR_LEAVE( FAILURE));
	}

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;

	NTR_ASSERT(strChannel->objectType == STREAMS_CHANNEL);
	NTR_ASSERT(strChannel->channelState != STR_CHANNEL_BOGUS);

	/*
	 * Request our private Stream Close function to close the Stream with
	 * the peer process if it still exists.
	 */
	if( strChannel->readQueue ) {
		NWstrCloseStream(strChannel->readQueue, strChannel->vnode);
		strChannel->readQueue = NULL;
	}

	/*
	 * Find the STREAM Channel Object in the active list, delete it from
	 * the list, and free the memory of the object itself.
	 */
	found = FALSE;

	SLEEP_LOCK (NWstrChannelListSleepLock, NUCSTRSLEEPPRI);

	NWtlRewindSLList(NWstrChannelList);
	while ( NWtlGetContentsSLList(NWstrChannelList, (void_t *)&aStrChannel)
				== SUCCESS) {
		if ( aStrChannel == strChannel ) {
			NWtlDeleteNodeSLList(NWstrChannelList);
			pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
			strChannel->channelState = STR_CHANNEL_BOGUS;
			strChannel->objectType = STR_CHANNEL_BOGUS;
			RW_UNLOCK (strChannel->channelRWLock, pl);
			RW_DEALLOC (strChannel->channelRWLock);
			NWtlDestroySemaphore(strChannel->syncSemaphore);
			kmem_free (strChannel, sizeof (STR_CHANNEL_T));
#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM_FREE: NWstrCloseIpcChannel: free STR_CHANNEL_T * at 0x%x, size = 0x%x",
				strChannel, sizeof(STR_CHANNEL_T), 0 );
#endif
			found = TRUE;
			break;
		}
		NWtlNextNodeSLList(NWstrChannelList);
	}

	SLEEP_UNLOCK (NWstrChannelListSleepLock);

	if ( !found ) {
		/*
		 * Couldn't find it in the active list, time to panic!!
		 */
		cmn_err(CE_PANIC, "NUC STREAM List Object Missing!!");
	}

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrFlushIpcMessage(3K), \
 *			 ./man/kernel/kipc/streams/FlushIpcMessage)
 * NAME
 *	NWstrFlushIpcMessage -
 *					Flush message(s) on a STREAMS Inter
 *					Process Communication Channel.  Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					swtich structure NWgipcOpsSw[].
 *					
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpssw[RITCHIE_STREAMS].FlushIpcMessage(ipcChannel, flushType,
 *					diagnostic)
 *	
 *	opaque_t	*ipcChannel;	/* Opaque to caller *\
 *	uint32		flushType;
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 *	flushType	- Set to one of the following requests:
 *				GIPC_FLUSH_RHEAD	- Head of read queue.
 *				GIPC_FLUSH_RALL		- All on read queue.
 *				GIPC_FLUSH_WHEAD	- Head of write queue.
 *				GIPC_FLUSH_WALL		- All on write queue.
 *
 * OUTPUT
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAM Channel.
 *
 *			[NWD_GIPC_NO_MESSAGE]	- There is no message(s) on the
 *						  Channel of 'flushType'.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrFlushIpcMessage' flushes all queued data on the STREAM that
 *	matches a requested type.  The STREAM is full duplex, with both read
 *	and write queues, thus all or the head message of each queue may be
 *	specified.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrReceiveIpcMessage(3K), NWstrSendIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrFlushIpcMessage (GIPC_CHANNEL_T *ipcChannel, uint32 flushType,
						int32 *diagnostic)
{
	register	int32			flushCount;
	register	int32			readFlushed;
	register	STR_CHANNEL_T	*strChannel;
	pl_t						pl;

	NTR_ENTER(3, ipcChannel, flushType, diagnostic, 0, 0);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	NTR_ASSERT(strChannel);

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		RW_UNLOCK (strChannel->channelRWLock, pl);
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	flushCount = 0;
	readFlushed = 0;


	if ( flushType & GIPC_FLUSH_RALL) {
		if ( strChannel->currentMessage) {
			/*
			 * Flush the partially completed current message
			 */
			freemsg(strChannel->currentMessage);
			flushCount++;
			readFlushed++;
		}
		if ( qsize(strChannel->readQueue) ) {
			/*
			 * Flush all of the read queue 
			 */
			flushq(strChannel->readQueue, FLUSHALL);
			flushCount++;
			readFlushed++;
		}
	}

	if ( flushType & GIPC_FLUSH_RHEAD ) {
		if (strChannel->currentMessage) {
			/*
			 * Flush the partially completed current message
			 */
			freemsg(strChannel->currentMessage);
			flushCount++;
			readFlushed++;
		}
	}

	if ( flushType & GIPC_FLUSH_WALL ) {
		if ( qsize(strChannel->writeQueue) ) {
			/*
			 * Flush all of the write queue 
			 */
			flushq(strChannel->writeQueue, FLUSHALL);
			flushCount++;
		}
	}

	if ( flushType & GIPC_FLUSH_WHEAD ) {
		if ( qsize(strChannel->writeQueue) ) {
			/*
			 * Flush the write queue head
			 */
			freemsg(getq(strChannel->writeQueue));
			flushCount++;
		}
	}

	if ( readFlushed ) {
		strChannel->currentMessage = NULL;
		/*
		 * Enable our Read Queue for Servicing, and attempt to schedule
		 * it now.
		 */
		strChannel->channelState = STR_CHANNEL_IDLE;
		enableok(strChannel->readQueue);
    		if (strChannel->readQueue->q_first ) {
			qenable(strChannel->readQueue);
		}
	}

	RW_UNLOCK (strChannel->channelRWLock, pl);

	if ( flushCount ) {
		return( NTR_LEAVE( SUCCESS));
	} else {
		/*
		 * There were no message of type requested to be flushed
		 */
		*diagnostic = NWD_GIPC_NO_MESSAGE;
		return( NTR_LEAVE( FAILURE));
	}
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrFreeIpcMessage(3K), \
 *			./man/kernel/kipc/streams/FreeIpcMsg)
 * NAME
 *	NWstrFreeIpcMessage -	Free a duplicated STREAMS message. Member
 *				of the Generic Inter Process
 *				Communication Layer Operations Switch
 *				structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].FreeIpcMessage(ipcChannel, dupMessage,
 *				diagnostic);
 *	
 *	opaque_t	*ipcChannel;	/* Opaque to caller *\
 *	opaque_t	*dupMessage;	/* Opaque to caller *\
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 *	dupMessage	- A pointer to the STREAMS message buffer
 *			  chain duplicated by a previous
 *			  NWstrSendIpcMessage(3K) which is to be
 *			  free'd.
 *
 * OUTPUT
 *	diagnostic	- CURRENTLY NOT USED.
 *
 * RETURN VALUES
 *	0	- Successful completion.  An unsuccessful completion at this
 *		  time causes a PANIC!!
 *
 * DESCRIPTION
 *	The 'NWstrFreeIpcMessage' frees and releases a duplicated STREAMS message 
 *	back to the STREAMS I/O Sub-System.  This is done when a previously sent
 *	message is no longer needed for a re-send (ie. Transport re-transmit).
 *
 * NOTES
 *	The supplied 'dupMessage' is a trusted argument, erroneous arguments
 *	will yield unpredictable results, and likely PANIC!! the system.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrReSendIpcMessage(3K), NWstrSendIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrFreeIpcMessage (GIPC_CHANNEL_T *ipcChannel, mblk_t *dupMessage,
						int32 *diagnostic)
{
	register	STR_CHANNEL_T	*strChannel;

	NTR_ENTER(3, ipcChannel, dupMessage, diagnostic, 0, 0);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	/*
	 * Attempt to free the STREAMS message back to STREAMS I/O Sub-System
	 */
	freemsg(dupMessage);

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrInitializeIpcService(3K), \
 *			./man/kernel/kipc/streams/InitializeIpc)
 * NAME
 *	NWstrInitializeIpcService -
 *						Enables or Disables the NUC
 *						STREAMS Head Package.
 *						Member of the Generic
 *						Inter Process Communication
 *						Layer Operations Switch structure
 *						NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].InitializeIpcService(requestType)
 *	
 *	uint32	requestType;
 *
 * INPUT
 *	requestType	- A GIPC_START specifies the NUC STREAMS Head  is to be 
 *			  initialized and made ready f or use.  A GIPC_STOP
 *			  specifies the NUC STREAMS Head  is to be stopped and no
 *			  longer useable until a GIPC_START is issued.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, no diagnostic, just simply
 *		  catastrophic.
 *
 * DESCRIPTION
 *	The 'NWstrInitializeIpcService' is called to either startup the 
 *	NUC STREAMS Head  or to shutdown the NUC STREAMS Head .
 *
 * NOTES
 *	This operation depends upon its GIPC parent layer to serialize RACE
 *	conditions during this phase.  It is important to note, that any request
 *	other than a GIPC_START to this operation when the NUC STREAMS Head is
 *	stopped, will cause the kernel to PANIC!!, as this violates the rule
 *	that the GIPC layer manage the RACE's.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucLock
 *	LOCKS HELD WHEN RETURNED:
 *		nucLock
 *
 *
 * SEE ALSO
 *	NWstrIntroduction(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrInitializeIpcService(uint32 requestType, int32 *diagnostic)
{
	STR_CHANNEL_T	*strChannel;

	NTR_ENTER(2, requestType, diagnostic, 0, 0, 0);

	*diagnostic = SUCCESS;

	switch (requestType) {

		case GIPC_START: {
			if (NWstrInitialized == FALSE) {

				/*
				 * Create the NWstrChannelList generic linked list for the
				 * strChannel objects to be linked and unlinked into during
				 * this session.
				 */

				if ((NWstrChannelListSleepLock = SLEEP_ALLOC (NUCCHANSLOCK_HIER,
					&NWstrChannelListSleepLockInfo, KM_NOSLEEP)) == NULL) {
					*diagnostic = NWD_GIPC_NO_RESOURCE;
					return( NTR_LEAVE( FAILURE));
				}
	
				/*	NWstrChannelList is not being guarded by
				 *	NWstrChannelListSleepLock at this point since this is called
				 *	during initialization. At this point there should be no
				 *	other lwp's trying to paly with NWstrChannelList
				 */
				if (NWtlInitSLList((SLList **)(&NWstrChannelList)) != SUCCESS) {
					*diagnostic = NWD_GIPC_NO_RESOURCE;
					return( NTR_LEAVE( FAILURE));
				}
	
				NWstrInitialized = TRUE;


			}

			return( NTR_LEAVE( SUCCESS));
		}

		case GIPC_STOP: {
			if( NWstrInitialized == TRUE ) {
	
	
				SLEEP_LOCK (NWstrChannelListSleepLock, NUCSTRSLEEPPRI);

				while( (NWtlGetContentsSLList(NWstrChannelList,
							(void_t *)&strChannel) == SUCCESS ) ) {
					SLEEP_UNLOCK (NWstrChannelListSleepLock);
					NWstrCloseIpcChannel (&(strChannel->gipcChannel),
						diagnostic);
					SLEEP_LOCK (NWstrChannelListSleepLock, NUCSTRSLEEPPRI);
				}
	
				/*
				 * Notify the NUC STREAMS Head that it is no longer ready 
				 * for use.  This is an advisory lock in effect, thus
				 * the NWstrReadHeadPut(3K) must honor the lock and drop
				 * any new messages received.
				 */
				
				NWtlDestroySLList(NWstrChannelList);

				NWstrChannelList = NULL;
				SLEEP_UNLOCK (NWstrChannelListSleepLock);
				SLEEP_DEALLOC (NWstrChannelListSleepLock);

				NWstrInitialized = FALSE;
			}

			return( NTR_LEAVE( SUCCESS));
		}

		default:
			/*
			 * Unknown Request, let caller know
			 */
			*diagnostic = NWD_GIPC_NO_RESOURCE;
			return( NTR_LEAVE( FAILURE));
	}
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrOpenIpcChannel(3K), \
 *			 ./man/kernel/kipc/streams/OpenIpcChannel)
 * NAME
 *	NWstrOpenIpcChannel -
 *					Open a STREAMS Inter Process
 *					Communication Channel.  Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					Swtich structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].OpenIpcChannel(myIpcName, peerProcessName,
 *				eventMode, ipcChannel, diagnostic)
 *
 *	uint32		myIpcName;
 *	opaque_t	*peerProcessName;	/* Opaque to caller	*\
 *	uint32		eventMode;
 *	opaque_t	**ipcChannel;		/* Opaque to caller	*\
 *	int32		*diagnostic;
 *
 * INPUT
 *	myIpcName	- Ignored, assumed to be RITCHIE_STREAMS.
 *
 *	peerProcessName	- A pointer to the pathname of the peer process
 *			  device to open.  This may either be a explicit
 *			  device minor or the clone device.
 *
 *	eventMode	- Selects either synchronous or asyncrhonous (non-blocked)
 *			  operations with NWstrReceiveIpcMessage(3K) and
 *			  NWstrSendIpcMessage(3K).  GIPC_SYNC specifies
 *			  synchronous operation, while GIPC_ASYNC specifies
 *			  asynchronous operation.
 *
 * OUTPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object opened.
 *
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_BAD_PEER]	- The 'peerProcessName' peer
 *						  process is unknown to UNIX.
 *
 *			[NWD_GIPC_NO_RESOURCE]	- A Channel could not be opened
 *						  due to a resource shortage.
 *
 *			[NWD_GIPC_PEER_REJECT]	- The 'peerProcessName' peer
 *						  process rejected the open.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrOpenIpcChannel' opens a Channel with the specified peer process
 *	using STREAMS IPC mechanism.  Upon successful completion, messages may
 *	be passed with the peer process in a full duplex manner.
 *
 * NOTES
 *	The Channel is a full duplex communication path, which is linked directly
 *	to the STREAMS Queue Head.  The underlying STREAMS mechanism is used to
 *	queue messages.
 *
 *	The Generic Inter Process Communication Layer is our parent, and where
 *	possible GIPC definitions are inherited and used for semantic convergence
 *	with our parent.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrCloseIpcChannel(3K)
 *
 * END_MANUAL_ENTRY
 */

/*ARGSUSED*/
ccode_t
NWstrOpenIpcChannel(
	uint32		myIpcName,
	char		*peerProcessName,
	uint32		eventMode,
	GIPC_CHANNEL_T	**ipcChannel,
	int32		*diagnostic)
{

	queue_t			*headReadQueue;
	STR_CHANNEL_T	*strChannel;
	pl_t				pl;

	NTR_ENTER(5, myIpcName, peerProcessName, eventMode, eventMode, diagnostic);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent,
	 * GIPC to initialize us.
	 */
	NTR_ASSERT(NWstrInitialized);

	/*
	 * NOTE: It is the responsiblity of GIPC to serialze Opens, thus this
	 *       RACE handling is implicitly done for us.
	 */

	/*
	 * Create a strChannel object, and link into the active channel list
	 */

	strChannel = (STR_CHANNEL_T *) kmem_zalloc(sizeof(STR_CHANNEL_T), KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NWstrOpenIpcChannel: alloc STR_CHANNEL_T * at 0x%x, size = 0x%x",
                strChannel, sizeof(STR_CHANNEL_T), 0 );
#endif NUCMEM_DEBUG
	strChannel->channelRWLock = RW_ALLOC (NUCCHANLOCK_HIER, plstr,
				&channelRWLockInfo, KM_SLEEP);

	SLEEP_LOCK (NWstrChannelListSleepLock, NUCSTRSLEEPPRI);

	if ( NWtlAddToSLList(NWstrChannelList, strChannel) != SUCCESS ) {

		SLEEP_UNLOCK (NWstrChannelListSleepLock);
		RW_DEALLOC (strChannel->channelRWLock);

		/*
		 * No more resources, let caller know this
		 */
		kmem_free (strChannel, sizeof(STR_CHANNEL_T));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWstrOpenIpcChannel: free STR_CHANNEL_T * at 0x%x, size = 0x%x",
                strChannel, sizeof(STR_CHANNEL_T), 0 );
#endif

		*diagnostic = NWD_GIPC_NO_RESOURCE;
		return( NTR_LEAVE( FAILURE));
	}



	/*
	 * Request our private Stream Open function open a Stream with the 
	 * peer process.
	 */
	if ( (NWstrOpenStream(peerProcessName, &headReadQueue, &(strChannel->vnode),
			diagnostic)) != SUCCESS ) {
		/*
		 * Couldn't open with peer, propagate the diagnostic back up
		 */
		RW_DEALLOC (strChannel->channelRWLock);
		kmem_free (strChannel, sizeof(STR_CHANNEL_T));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWstrOpenIpcChannel: free STR_CHANNEL_T * at 0x%x, size = 0x%x",
                strChannel, sizeof(STR_CHANNEL_T), 0 );
#endif
		NWtlDeleteNodeSLList(NWstrChannelList);

		SLEEP_UNLOCK (NWstrChannelListSleepLock);

		return( NTR_LEAVE( FAILURE));
	}


	if ( NWtlCreateAndSetSemaphore((int *)&(strChannel->syncSemaphore), 0)
			!= SUCCESS ) {
		/*
		 * Couldn't create a sync semaphore for this channel
		 */
		RW_DEALLOC (strChannel->channelRWLock);
		kmem_free (strChannel, sizeof(STR_CHANNEL_T));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWstrOpenIpcChannel: free STR_CHANNEL_T * at 0x%x, size = 0x%x",
                strChannel, sizeof(STR_CHANNEL_T), 0 );
#endif
		NWtlDeleteNodeSLList(NWstrChannelList);

		SLEEP_UNLOCK (NWstrChannelListSleepLock);

		*diagnostic = NWD_GIPC_NO_RESOURCE;
		return( NTR_LEAVE( FAILURE));
	}

	SLEEP_UNLOCK (NWstrChannelListSleepLock);


	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	/*
	 * The 'peerProcessName' is attached, time to make the 'strChannel'
	 * context ready for use, and return.
	 */

	headReadQueue->q_ptr = (caddr_t) strChannel;	/* Link STREAMS to Chan	*/
	WR(headReadQueue)->q_ptr = (caddr_t) strChannel;
	(strChannel)->readQueue = headReadQueue;	/* Link Chan to STREAMS	*/
	(strChannel)->writeQueue = WR(headReadQueue);
	(strChannel)->objectType = STREAMS_CHANNEL;
	(strChannel)->channelState = STR_CHANNEL_IDLE;
	(strChannel)->eventMode = eventMode;
	(strChannel)->advisoryLock = FALSE;
	(strChannel)->callBackFunction = NULL;
	(strChannel)->currentMessage = NULL;

	/*
	 * Return the GIPC Channel component
	 */
	*ipcChannel = &(strChannel->gipcChannel);
	(*ipcChannel)->realIpcOps = NWgipcOpsSw[RITCHIE_STREAMS];
	(*ipcChannel)->realIpcChannel = strChannel;



	RW_UNLOCK (strChannel->channelRWLock, pl);

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrPreviewIpcMessage(3K), \
 *			./man/kernel/kipc/streams/PreviewIpcMsg)
 * NAME
 *	NWstrPreviewIpcMessage -	Preview the head message on the read
 *					queue of a Channel.  Member 
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					Switch structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *	
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].PreviewIpcMessage(ipcChannel, controlMsg,
 *				dataMsg, msgType, rewind, diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	NUC_IOBUF_T	*controlMsg;
 *	NUC_IOBUF_T	*dataMsg;
 *	uint32		*msgType;
 *	uint32		rewind;
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 * OUTPUT
 *	controlMsg->buffer	- A pointer to next control message contiguous
 *				  block.
 *
 *	controlMsg->bufferLength - Size in bytes of 'controlMsg' block.
 *	
 *	dataMsg->buffer		- A pointer to the next data message contiguous
 *				  block.
 *
 *	dataMsg->bufferLength	- Size in bytes of 'dataMsg->buffer'.
 *
 *	dataMsg->memoryType	- Set to IOMEM_KERNEL indicating buffer in the kernel.
 *
 *	msgType			- Set to GIPC_NORMAL_MSG for regular messages,
 *				  and GIPC_HIPRI_MSG for priority messages.
 *
 *	rewind			- Set to TRUE if the caller wishes to rewind to the
 *				the beginning of the control and data portion of 
 *				the message.
 *
 *	diagnostic		- set to one of the following when return value
 *				  <0.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  GIPC Channel.
 *
 *			[NWD_GIPC_NO_MESSAGE]	- There is no message on the
 *						  Channel read queue.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	>0	- Partial successful completion, the value indication the 
 *		  total number of blocks (control and data) which have not
 *		  been previewed yet.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrPreviewIpcMessage' maps the next contiguous blocks of the
 *	STREAMS buffers into the callers arguments.  The concept next, is
 *	relative to the those not already previewed.  This permits a recursive
 *	walk through of a message without data copy. The NWstrSizeOfIpcMessage(3K)
 *	can be used to determine the total number of blocks and bytes in both the
 *	control and data portions.  If rewind is TRUE, previewing is restarted
 *	at the beginning of both the control and data portion of the message.
 *
 * NOTES
 *	Previews are available only on Channels which were opened for asynchronous
 *	use (ie the GIPC_ASYNC mode selected).  This is not a blocked operation,
 *	it will return immediately.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrReceiveIpcMessage(3K),
 *	NWstrRegisterAsyncEventHandler(3K),  NWstrSendIpcMessage(3K),
 *	NWstrSizeOfIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrPreviewIpcMessage (GIPC_CHANNEL_T *ipcChannel, NUC_IOBUF_T *controlMsg,
						NUC_IOBUF_T *dataMsg, uint32 *msgType,
						uint32 rewind, int32 *diagnostic,
						int32 *residualBlocks)
{

	register	mblk_t			*tmpBlock;
	register	STR_CHANNEL_T	*strChannel;
	pl_t							pl;

	NTR_ENTER(5, ipcChannel, controlMsg, dataMsg, msgType, rewind);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);


	if ( !strChannel->currentMessage ) {
		RW_UNLOCK (strChannel->channelRWLock, pl);
		/*
		 * No message is available, and channel is operating
		 * asynchronously
		 */
		*diagnostic = NWD_GIPC_NO_MESSAGE;
		return( NTR_LEAVE( FAILURE));
	}

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		freemsg(strChannel->currentMessage);
		RW_UNLOCK (strChannel->channelRWLock, pl);
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 * Set the message type
	 */
	switch (strChannel->currentMessage->b_datap->db_type) {

	case M_PCPROTO:
		*msgType = GIPC_HIPRI_MSG;
		break;

	default:
		*msgType = GIPC_NORMAL_MSG;
	}

	/*
		If the caller wants to restart at the beginning, rewind the
		preview.
	*/
	if (rewind) {
		strChannel->curPreCtlBlock = strChannel->currentMessage;
		if (strChannel->currentMessage->b_cont)
			strChannel->curPreDataBlock = strChannel->currentMessage->b_cont;
	}

	/*
	 * Map in the next control block
	 */
	if ( strChannel->curPreCtlBlock ) {
		controlMsg->buffer =
			(opaque_t *) strChannel->curPreCtlBlock->b_rptr;
		controlMsg->bufferLength =
			(int)(strChannel->curPreCtlBlock->b_wptr -
				strChannel->curPreCtlBlock->b_rptr);
		controlMsg->memoryType = IOMEM_KERNEL;
		if ( strChannel->curPreCtlBlock->b_cont &&
			strChannel->curPreCtlBlock->b_cont->b_datap->db_type
				!= M_DATA) {
			/*
			 * Another control block?, seems strange to have 2, oh
			 * well, pass it on through on the next preview iterate
			 * cycle
			 */
			strChannel->curPreCtlBlock = strChannel->curPreCtlBlock->b_cont;
		} else {
			/*
			 * No more control blocks
			 */
			strChannel->curPreCtlBlock = NULL;
		}
	} else {
		controlMsg->buffer = NULL;
		controlMsg->bufferLength = 0;
	}

	/*
	 * Map in the next data block
	 */
	if ( strChannel->curPreDataBlock ) {
		dataMsg->buffer =
			(opaque_t *) strChannel->curPreDataBlock->b_rptr;
		dataMsg->bufferLength =
			(int)(strChannel->curPreDataBlock->b_wptr -
				strChannel->curPreDataBlock->b_rptr);
		dataMsg->memoryType = IOMEM_KERNEL;
		strChannel->curPreDataBlock = strChannel->curPreDataBlock->b_cont;
	} else {
		dataMsg->buffer = NULL;
		dataMsg->bufferLength = 0;
	}

	if ( strChannel->curPreCtlBlock || strChannel->curPreDataBlock ) {
		/*
		 * There is more message to be previewed.
		 */
		*residualBlocks = 0;
		for ( tmpBlock = strChannel->curPreCtlBlock; tmpBlock; 
				tmpBlock = tmpBlock->b_cont ) {
			if ( tmpBlock->b_datap->db_type == M_DATA ) {
				break;
			}
			*residualBlocks++;	/* Add Control Block	*/
		}
		for ( tmpBlock = strChannel->curPreDataBlock; tmpBlock; 
			tmpBlock = tmpBlock->b_cont ) {
			*residualBlocks++;	/* Add Data Block	*/
		}
	} else {
		/*
		 * Message has been completely previewed
		 */
		*residualBlocks = 0;
	}

	RW_UNLOCK (strChannel->channelRWLock, pl);

	return( NTR_LEAVE( *residualBlocks));

}


/*
 * BEGIN_MANUAL_ENTRY(NWstrPrivateIoctlMessage(3K), \
 *			 ./man/kernel/kipc/streams/PrivateIoctlMsg)
 * NAME
 *	NWstrPrivateIoctlMessage -
 *					Send and receive a private command
 *					message to the peer process.  Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					Switch structure NWgipcOps[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].PrivateIoctlMessage(ipcChannel, privateCmd,
 *					privateCmdSize,	privateMsg,
 *					privateMsgSize, diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	struct	iocblk	*privateCmd;
 *	int32		*privateCmdSize;
 *	opaque_t	*privateMsg;		/* Opaque to provider	*\
 *	int32		*privateMsgSize;
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 *	privateCmd	- A pointer to a (struct) iocblk, fully populated
 *			  according to the rules of the STREAMS PROGRAMMER'S
 *			  GUIDE V3.2/386 Page B-4.
 *
 *	privateCmdSize	- Size in bytes of 'privateCmd'.  Must be equal to
 *			  sizeof(struct iocblk).
 *
 *	privateMsg	- A pointer to the opaque data buffer to send to
 *			  the peer process.
 *
 *	privateMsgSize	- Size in bytes of the 'privateMsg'.  Must be >= to
 *			  privateCmd->ioc_count;
 *
 * OUTPUT
 *	privateCmdSize	- Size in bytes of the 'privateCmd' returned from the
 *			  peer process and copied into 'privateCmd'.
 *
 *	privateMsgSize	- Size in bytes of the 'privateMsg' returned from the
 *			  peer process and copied into 'privateMsg'.
 *
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_IOCTL_FAIL]	- The peer process refused to
 *						  complete the request.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAMS Channel.
 *
 *			[NWD_GIPC_NO_RESOURCE]	- STREAMS buffers could not be
 *						  allocated to send the message.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrPrivateIoctlMessage' provides a opaque command message facility
 *	for the caller to use for sending and receiving commands to the peer
 *	process via the Channel.  This mechnism provides the UNIX IOCTL(2) 
 *	facility to callers. 
 *
 * NOTES
 *	The message sent and received must be formatted according to STREAMS
 *	rules which are as follows:
 *
 * SEE ALSO
 *	Intro(2), Ioctl(2), NWstrIntroduction(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrPrivateIoctlMessage(
	GIPC_CHANNEL_T	*ipcChannel,
	struct	iocblk	*privateCmd,
	int32		*privateCmdSize,
	opaque_t	*privateMsg,
	int32		*privateMsgSize,
	int32		*diagnostic)
{

	register	mblk_t		*ioctlMessage;
	register	int		timeOutId;
	register	int32		fillOffset;
	pl_t					pl;
	register	uchar		*fillAddress;
	register	NUC_IOBUF_T	*datap;
	STR_CHANNEL_T			*strChannel;
	ccode_t					returnCode;
	struct {
		NUC_IOBUF_T			controlMsg;
		NUC_IOBUF_T			dataMsg;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NTR_ENTER(5,ipcChannel,privateCmd,privateCmdSize,privateMsg,privateMsgSize);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	/*
	 * Verify the privateCmdSize and privateMsgSize are valid!!
	 */
	if ( *privateCmdSize != sizeof(struct iocblk) ||
			(*privateMsgSize < privateCmd->ioc_count) ||
			( (strmsgsz > 0) && (*privateMsgSize > strmsgsz) )) {
		/*
		 * The privateCmdSize or privateMsgSize is invalid
		 */
		*diagnostic = NWD_GIPC_IOCTL_FAIL;
		returnCode = FAILURE;
		goto done;
	}

	/*
	 * Make a STREAMS message out of the two buffers
	 */
	KAlloc->controlMsg.buffer = (opaque_t *) privateCmd;
	KAlloc->controlMsg.bufferLength = *privateCmdSize;
	KAlloc->controlMsg.memoryType = IOMEM_KERNEL;
	if ( privateCmd->ioc_count ) {
		/*
		 * Data to send on Ioctl
		 */
		KAlloc->dataMsg.buffer = privateMsg;
		KAlloc->dataMsg.bufferLength = *privateMsgSize;
		KAlloc->dataMsg.memoryType = IOMEM_KERNEL;
		datap = &KAlloc->dataMsg;
	} else {
		datap = NULL;
	}

	ioctlMessage = buildStreamMsg(&KAlloc->controlMsg, datap, NULL,
			GIPC_HIPRI_MSG, diagnostic);

	if ( !ioctlMessage ) {
		/*
		 * STREAMS Buffer could'nt be build, either a resource shortage,
		 * or memory fault in a user space buffer, diagnostic is set
		 */
		returnCode = FAILURE;
		goto done;
	}

	/*
	 * Change the message type from M_PCPROTO to M_IOCTL
	 */
	ioctlMessage->b_datap->db_type = M_IOCTL;

	/*
	 * Is the downstream channel blocking?
	 */
	while ( !canput(strChannel->writeQueue->q_next) ) {
		/*
		 * Downstream is blocking?
		 */
		if ( strChannel->eventMode == GIPC_ASYNC ) {
			/*
			 * Can't block, let caller know we would block
			 */
			*diagnostic = NWD_GIPC_BLOCKING;
			returnCode = FAILURE;
			goto done;
		}
#ifdef NEVER_EXECUTED
		else {
			/*
			 * Sleep til awakened by NWstrWriteHeadService(3K)
			 * and then RACE with any other processes blocked
			 * to send.
			 */
			strChannel->channelState = STR_CHANNEL_SLEEPING;
			pl = LOCK (criticalSectLock, NUCPLHI);
			SV_WAIT (&strChannel->strChannelSV, primed, criticalSectLock);
		}
#endif NEVER_EXECUTED
	} 
	
	/*
	 * Note that this channel wants a IOCTL response, which will be a key
	 * to timeouts, and the underlying message serializing by our read put
	 * and read service procedures.
	 */

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	strChannel->channelState = STR_CHANNEL_WANT_IOCTL;

	RW_UNLOCK (strChannel->channelRWLock, pl);

	putnext(strChannel->writeQueue, ioctlMessage);	/* Send the IOCTL	*/

	/*
	 * Arm a timeout to trap not responding IOCTL's.
	 *
	 * We will go ahead and place our call back on the UNIX callout[]
	 * list, as we are not expecting very many IOCTL's, and certainly
	 * not simultaneously.  If this becomes a problem, then we can
	 * have a seperate SKULKER which wires itself onto the callout[]
	 * list, and scans the strChannels for timeouts.
	 */
	timeOutId = itimeout(strIoctlTimeOut,(caddr_t)strChannel, IOCTL_TIMEOUT,
						 plstr);

	/*
	 * Now wait for the response to our request
	 */
	NWtlPSemaphore(strChannel->syncSemaphore);

	/*
	 * Did we timeout or receive the reply?
	 */
	pl = RW_RDLOCK (strChannel->channelRWLock, plstr);
	if ( strChannel->channelState != STR_CHANNEL_WANT_IOCTL ) {

		RW_UNLOCK (strChannel->channelRWLock, pl);

		/* 
		 * We timed out, return with bad news
		 */
		*privateCmdSize = 0;
		*privateMsgSize = 0;
		*diagnostic = NWD_GIPC_IOCTL_FAIL;
		returnCode = FAILURE;
		goto done;
	} else {
		RW_UNLOCK (strChannel->channelRWLock, pl);
		untimeout(timeOutId);
		timeOutId = 0;
	}

	/*
	 * Copy the results of the IOCTL
	 */
	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
	*privateCmdSize = min(*privateCmdSize,
			( (int) strChannel->currentMessage->b_wptr -
			(int) strChannel->currentMessage->b_rptr) );

	bcopy((caddr_t)strChannel->currentMessage->b_rptr, privateMsg,
		*privateCmdSize);

	fillAddress = (uchar *) privateMsg;
	strChannel->curDataBlock = strChannel->currentMessage->b_cont;
	for ( fillOffset=0;
		fillOffset < *privateMsgSize && strChannel->curDataBlock;
		fillOffset++) {

		*fillAddress++ = *strChannel->curDataBlock->b_rptr++;
		if ( strChannel->curDataBlock->b_rptr == 
				strChannel->curDataBlock->b_wptr ) {
			strChannel->curDataBlock =
				strChannel->curDataBlock->b_cont;
		}
	}
	*privateMsgSize = fillOffset;

	if ( strChannel->currentMessage->b_datap->db_type == M_IOCNAK ) {
		/*
		 * Our Ioctl was rejected by peer
		 */

		*diagnostic = NWD_GIPC_IOCTL_FAIL;
	}

	/*
	 * Resultant message has been completely processed, destroy it.
	 */
	freemsg(strChannel->currentMessage);
	strChannel->currentMessage = NULL;

	/*
	 * Enable our Read Queue for Servicing, which will schedule
	 */
	strChannel->channelState = STR_CHANNEL_IDLE;

	RW_UNLOCK (strChannel->channelRWLock, pl);

	enableok(strChannel->readQueue);
	if (strChannel->readQueue->q_first ) {
		qenable(strChannel->readQueue);
	}

	if ( *diagnostic ) {
		returnCode = FAILURE;
	} else {
		returnCode = SUCCESS;
	}

done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return NTR_LEAVE(returnCode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrReceiveIpcMessage(3K), \
 *			 ./man/kernel/kipc/streams/ReceiveIpcMsg)
 * NAME
 *	NWstrReceiveIpcMessage -
 *					Receive the head message on the read
 *					queue of a Channel.  Member 
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					Switch structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *	
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].ReceiveIpcMessage(ipcChannel, controlMsg,
 *				dataFragment1, dataFragment2, msgType, diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	NUC_IOBUF_T	*controlMsg;
 *	NUC_IOBUF_T	*dataFragment1;
 *	NUC_IOBUF_T	*dataFragment2;
 *	uint32		*msgType;
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel			- A pointer to the GIPC Channel
 *					  descriptor object.
 *
 *	controlMsg->buffer		- A pointer to a contiguous block to
 *					  receive control message.
 *
 *	controlMsg->bufferLength	- Size in bytes of 'controlMsg->buffer'.
 *
 *	controlMsg->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment1->buffer		- A pointer to a contiguous block to
 *					  receive data message.
 *
 *	dataFragment1->bufferLength	- Size of 'dataFragment1->buffer'.
 *
 *	dataFragment1->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment2->buffer		- A pointer to a contiguous block to
 *					  receive data message into once
 *					  'dataFragment1->buffer' is filled.
 *
 *	dataFragment2->bufferLength	- Size of 'dataFragment2->buffer'.  Not
 *					  Used if 'dataFragment2' is NULL.
 *
 *	dataFragment2->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space. Not Used if 'dataFragment2'
 *					  is NULL.
 *
 * OUTPUT
 *	controlMsg->bufferLength	- Size in bytes copied into
 *					  'controlMsg->buffer'.
 *
 *	dataFragment1->bufferLength	- Size in bytes copied into
 *					  'dataFragment1->buffer'.
 *
 *	dataFragment2->bufferLength	- Size in bytes copied into
 *					  'dataFragment2->buffer'.
 *	
 *	msgType				- Set to GIPC_NORMAL_MSG for regular
 *					  messages, and  GIPC_HIPRI_MSG for
 *					  priority messages.
 *
 *	diagnostic			- set to one of the following when
 *					  return value <0.
 *
 *			[NWD_GIPC_BUF_FAULT]	- The 'controlMsg',
 *						  'dataFragment1', or 
 *						  'dataFragment2' located in the
 *						  USER Memory Space generated a
 *						  EFAULT.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAMS Channel.
 *
 *			[NWD_GIPC_NO_MESSAGE]	- There is no message on the
 *						  Channel read queue.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	>0	- Partial successful completion, the value indication the 
 *		  total number of bytes (control and data) which have not
 *		  been received yet.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrReceiveIpcMessage' copies STREAMS buffers into the callers
 *	buffers.  The message is logically divided into two portions, a control
 *	portion	and a data portion.  Each of these portions may span several
 *	contiguous blocks in the STREAMS buffers (ie is a linked list of blocks).
 *	Recursive calls can be made to walk through and copy the message if
 *	insufficeint control and data buffers are supplied to complete the
 *	message copy.  The NWstrSizeOfIpcMessage(3K) can be used to determine
 *	the total number of blocks and bytes in both the control and data
 *	portions.
 *
 * NOTES
 *	This function copies as much of the message as possible into the callers
 *	buffers.  Un-copied information must be copied via additional calls, or
 *	flushed (NWstrFlushIpcMessage(3K)) before another message can be 
 *	received.  If the Channel was opened with GIPC_SYNC, the call will block
 *	until a message is received.  Direct copy to User Space buffers from the
 *	STREAMS buffers is supported.
 *
 *	Upon completion of a message either by complete receipt or flush of the 
 *	message, the head message is discarded by the GIPC, and the next queued
 *	message if any will be moved to the head of the read queue.  For Channels
 *	operating asyncrhonously, the 'asyncFunction' registered via
 *	NWstrRegisterAsyncEventHandler(3K) will be called immediately to process
 *	the new head message.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrRegisterAsyncEventHandler(3K),  NWstrSendIpcMessage(3K),
 *	NWstrSizeOfIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrReceiveIpcMessage(
	GIPC_CHANNEL_T	*ipcChannel,
	NUC_IOBUF_T	*controlMsg,
	NUC_IOBUF_T	*dataFragment1,
	NUC_IOBUF_T	*dataFragment2,
	uint32		*msgType,
	int32		*diagnostic,
	int32		*residualBytes)
{

	register	mblk_t			*messageBlock;
	register	STR_CHANNEL_T	*strChannel;
	pl_t						pl;

	NTR_ENTER(5, ipcChannel, dataFragment1, dataFragment2, msgType, diagnostic);

	*diagnostic = SUCCESS;
	*residualBytes = 0;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	if ( !strChannel->currentMessage && strChannel->eventMode == GIPC_ASYNC ){
		RW_UNLOCK (strChannel->channelRWLock, pl);
		/*
		 * No message is availabe, and channel is operating
		 * asynchronously
		 */
		*diagnostic = NWD_GIPC_NO_MESSAGE;
		return( NTR_LEAVE( FAILURE));
	}

	if ( !strChannel->currentMessage ) {
		strChannel->channelState = STR_CHANNEL_SLEEPING;
		RW_UNLOCK (strChannel->channelRWLock, pl);
		NWtlPSemaphore(strChannel->syncSemaphore);
		pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
	}

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		freemsg(strChannel->currentMessage);
		RW_UNLOCK (strChannel->channelRWLock, pl);
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	strChannel->channelState = STR_CHANNEL_MESSAGE;

	/*
	 * Set the message type
	 */
	switch (strChannel->currentMessage->b_datap->db_type) {

	case M_PCPROTO:
		*msgType = GIPC_HIPRI_MSG;
		break;

	default:
		*msgType = GIPC_NORMAL_MSG;
	}

	/*
	 * Copy as much of the STREAMS message to the supplied buffers as 
	 * specified.
	 */

	drainStreamMsg(strChannel, controlMsg, dataFragment1,
		dataFragment2, diagnostic);

	if ( strChannel->curControlBlock || strChannel->curDataBlock ) {
		/*
		 * There is more message to be received.
		 */
		for ( messageBlock = strChannel->currentMessage; messageBlock;
				messageBlock = messageBlock->b_cont ) {
			*residualBytes += (int)(messageBlock->b_wptr
				- messageBlock->b_rptr);
		}
	} else {
		/*
		 * Message has been completely received, destroy it.
		 */
		freemsg(strChannel->currentMessage);
		strChannel->currentMessage = NULL;

		/*
		 * Enable our Read Queue for Servicing, and attempt to schedule
		 * it now.
		 */
		strChannel->channelState = STR_CHANNEL_IDLE;
	}

	RW_UNLOCK (strChannel->channelRWLock, pl);

	if ( *diagnostic) {
		return( NTR_LEAVE( FAILURE));
	} else {
		return( NTR_LEAVE( *residualBytes));
	}
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrRegisterAsyncEventHandler(3K), \
 *			 ./man/kernel/kipc/streams/RegisterAsync)
 * NAME
 *	NWstrRegisterAsyncEventHandler -
 *						Register an asyncrhonous events
 *						handler for a Channel.  Member
 *						of the Generic Inter Process
 *						Communication Layer Operations
 *						switch structure NWgipcOpssw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *	
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].RegisterAsyncEventHandler(ipcChannel,
 *					asyncFunction, callerHandle, diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	opaque_t	(*asyncFunction);	/* Opaque to provider	*\
 *	opaque_t	*callerHandle;		/* Opaque to provider	*\
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 *	asyncFunction	- A pointer to an opaque function which is to service
 *			  asynchronous events for the Channel.  This function
 *			  is expected to have the following declaration:
 *
 *				opaque_t
 *				asyncFunction(ipcChannel, callerHandle,
 *						eventMsgType)
 *
 *				opaque_t	*sipchannel;
 *				private_t	*callerHandle;
 *				uint32		eventMsgType;
 *
 *			  where the 'ipcChannel' and the supplied
 *			  'callerHandle' are set to the values specified on the
 *			  call to NWstrRegisterAsyncEventHandler(), and the
 *			  'eventMsgType' will be set to GIPC_NORMAL_MSG or
 *			  GIPC_HIPRI_MSG to indicate which 'msgType' has arrived
 *			  at the read queue head.
 *
 *			  A NULL 'asyncFunction' pointer disables asyncrhonous
 *			  call backs on a Channel.  Another registration must be
 *			  made to re arm the Channel for asycrhonous event
 *			  notification.
 *
 *	callerHandle	- A pointer to opaque information which is managed by
 *			  the 'asyncFunction' which is to be supplied on
 *			  asyncrhonous call backs.
 *
 * OUTPUT
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAMS Channel.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrRegisterAsyncEventHandler' arranges for a call back to a 
 *	registered function when an asyncrhonous event (message is ready for
 *	receipt).  The function is asyncrhonously called with the 'strChannel'
 *	message type, and the resource handle specified when a message is ready
 *	for processing.
 *
 * NOTES
 *	The STREAMS guarantees serialized processesing of read queue messages
 *	by queueing messages on the read side of the channel.  When the head 
 *	message is drained, either by NWstrReceiveIpcMessage(3K), or
 *	NWstrFlushIpcMessage(3K), the head message is discarded, and the next
 *	highest priority message is made the head, at which time the
 *	'asyncFunction' call be called again.  This guarantees the 'asyncFunction'
 *	not be interrupted until the head message is drained.
 *
 *	The 'asyncFunction' stays armed until reset via another registration.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrReceiveIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrRegisterAsyncEventHandler(
	GIPC_CHANNEL_T	*ipcChannel,
	opaque_t	(*asyncFunction)(),
	opaque_t	*callerHandle,
	int32		*diagnostic)
{

	STR_CHANNEL_T	*strChannel;
	pl_t	processorLevel;

	NTR_ENTER(4, ipcChannel, asyncFunction, callerHandle, diagnostic, 0);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	processorLevel = RW_RDLOCK (strChannel->channelRWLock, plstr);

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		RW_UNLOCK (strChannel->channelRWLock, processorLevel);
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	RW_UNLOCK (strChannel->channelRWLock, processorLevel);

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	/*
	 * Arm the channel for Call Backs
	 *
	 * Make this an atomic operation, thus preventing asynchronous
	 * scheduling while we are changing call back handler.
	 */
	processorLevel = LOCK (criticalSectLock, NUCPLHI);

	strChannel->callBackFunction	= asyncFunction;
	strChannel->callBackHandle	= callerHandle;

	UNLOCK (criticalSectLock, processorLevel);

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrReSendIpcMessage(3K), \
 *			./man/kernel/kipc/streams/Svr3/ReSendIpcMsg)
 * NAME
 *	NWstrReSendIpcMessage -	Re-Send a message to peer process. Member
 *				of the Generic Inter Process
 *				Communication Layer Operations
 *				structure NWgipcOps.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].ReSendIpcMessage(ipcChannel, dupMessage,
 *					diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	opaque_t	*dupMessage;		/* Opaque to caller	*\
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel			- A pointer to the GIPC Channel
 *					  descriptor object.
 *
 *	dupMessage			- A pointer to the STREAMS message buffer
 *					  chain duplicated by a previous
 *					  NWstrSendIpcMessage(3K) which is to be
 *					  duplicated and re-sent again.
 *
 * OUTPUT
 *	diagnostic			- set to one of the following when
 *					  return value <0.
 *
 *			[NWD_GIPC_BLOCKING]	- The 'ipcChannel' is blocking
 *						  due to flow control, try again
 *						  later.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'ipcChannel' argument is
 *						  not assoicated with an open
 *						  GIPC Channel.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrReSendIpcMessage' sends a duplicated STREAMS message to the peer
 *	process on the Channel.  The duplicated message was created by
 *	NWstrSendIpcMessage(3K).  This facility provides the necessary
 *	buffering and re-transmission of lost IPC messages (ie. Transport
 *	timeouts).
 *
 * NOTES
 *	If the channel was opened with GIPC_SYNC, the call will block if the
 *	IPC mechanism is blocking due to flow control.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrFreeIpcMessage(3K), NWstrSendIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrReSendIpcMessage(
	GIPC_CHANNEL_T	*ipcChannel,
	mblk_t		*dupMessage,
	int32		*diagnostic)
{
	register	int		processorLevel;
	register	mblk_t		*sendMessage;
	register	STR_CHANNEL_T	*strChannel;

	NTR_ENTER(3, ipcChannel, dupMessage, diagnostic, 0, 0);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	processorLevel = RW_RDLOCK (strChannel->channelRWLock, plstr);

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		RW_UNLOCK (strChannel->channelRWLock, processorLevel);
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	RW_UNLOCK (strChannel->channelRWLock, processorLevel);

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	if ( dupMessage->b_datap->db_type != M_PCPROTO ) {
		/*
	 	 * Is the downstream channel blocking?
	 	 */
		/*	processorLevel = LOCK (criticalSectLock, NUCPLHI);	*/
		while ( !canput(strChannel->writeQueue->q_next) ) {
			/*
			 * Downstream is blocking?
			 */
			if ( strChannel->eventMode == GIPC_ASYNC ) {
				/*	NWtlExitCriticalSection(processorLevel);	*/
				/*	UNLOCK (criticalSectLock, processorLevel);	*/
				/*
				 * Can't block, let caller know we would block
				 */
				*diagnostic = NWD_GIPC_BLOCKING;
				return( NTR_LEAVE( FAILURE));
			}
#ifdef NEVER_EXECUTED
			else {
				/*
				 * Sleep til awakened by NWstrWriteHeadService(3K)
				 * and then RACE with any other processes blocked
				 * to send.
				 */
				strChannel->channelState = STR_CHANNEL_SLEEPING;
				processorLevel = LOCK (criticalSectLock, NUCPLHI);
				SV_WAIT (&strChannel->strChannelSV, primed, criticalSectLock);
			}
#endif NEVER_EXECUTED
		} 
		/*	UNLOCK (criticalSectLock, processorLevel);	*/
	}

	/*
	 * Duplicate the STREAMS message
	 *
	 * Note: The First Block is copied into a new dblk_t pair, and the
	 *	 remaining blocks are duplicated and linked to the new 
	 *	 head block.  If the peer process alters the 2nd - Nth blocks
	 *	 this strategy will fail, and a complete copy will have to
	 *	 be done.
	 */
	if ( (sendMessage = copyb(dupMessage)) ) {
		if (dupMessage->b_cont) {
			/*
			 * Duplicate the remaining blocks and link them 
			 * to the new head.
			 */
			if ( !(sendMessage->b_cont
					= dupmsg(dupMessage->b_cont)) ) {
				/*
				 * Unable to duplicate the M_DATA chain
				 */
				freeb(sendMessage);
				sendMessage = NULL;
			}
		}
	}

	if ( !sendMessage ) {
		/*
		 * Duplicated Message couldn't be built due to resource
		 * shortage.
		 */

		*diagnostic = NWD_GIPC_NO_RESOURCE;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 *	Send the message downstream again.
	 */
	putnext(strChannel->writeQueue, sendMessage);

	processorLevel = RW_WRLOCK (strChannel->channelRWLock, plstr);
	strChannel->channelState = STR_CHANNEL_IDLE;
	RW_UNLOCK (strChannel->channelRWLock, processorLevel);

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrSendIpcMessage(3K), \
 *			 ./man/kernel/kipc/streams/SendIpcMsg)
 * NAME
 *	NWstrSendIpcMessage -		Send a message to peer process. Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					switch structure NWgipcOps[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].SendIpcMessage(ipcChannel, controlMsg,
 *				dataFragment1, dataFragment2, msgType,
 *				dupMessage, diagnostic)
 *	
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	NUC_IOBUF_T	*controlMsg;
 *	NUC_IOBUF_T	*dataFragment1;
 *	NUC_IOBUF_T	*dataFragment2;
 *	uint32		msgType;
 *	opaque_t	**dupMessage;
 *	int32		*diagnostic;
 *
 * INPUT
 *	ipcChannel			- A pointer to the GIPC Channel
 *					  descriptor object.
 *
 *	controlMsg->buffer		- A pointer to a contiguous block of
 *					  control portion to send.
 *
 *	controlMsg->bufferLength	- Size in bytes of 'controlMsg->buffer'.
 *
 *	controlMsg->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment1->buffer		- A pointer to a contiguous block of data
 *					  to send.
 *
 *	dataFragment1->bufferLength	- Size of 'dataFragment1->buffer'.
 *
 *	dataFragment1->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment2->buffer		- A pointer to a contiguous block of 
 *					  concatenated data to send.
 *
 *	dataFragment2->bufferLength	- Size of 'dataFragment2->buffer'.  Not
 *					  Used if 'dataFragment2' is NULL.
 *
 *	dataFragment2->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space. Not Used if 'dataFragment2'
 *					  is NULL.
 *
 *	msgType				- Set to GIPC_NORMAL_MSG for regular
 *					  messages, and GIPC_HIPRI_MSG for
 *					  priority messages.
 *
 * OUTPUT
 *	dupMessage			- A pointer to the duplicated STREAMS
 *					  message that was sent.  This is a
 *					  duplicated message, which is used with
 *					  NWstrReSendIpcMessage(3K).
 *
 *	diagnostic			- set to one of the following when
 *					  return value <0.
 *
 *			[NWD_GIPC_BUF_FAULT]	- The 'controlMsg',
 *						  'dataFragment1', or 
 *						  'dataFragment2' located in the
 *						  USER Memory Space generated a
 *						  EFAULT.
 *
 *			[NWD_GIPC_BIG_MSG]	- The ControlMsgSize exceeds
 *						  the largest single STREAMS
 *						  buffer.
 *
 *			[NWD_GIPC_BLOCKING]	- The 'strChannel' is blocking
 *						  due to flow control, try again
 *						  later.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAMS Channel.
 *
 *			[NWD_GIPC_NO_RESOURCE]	- STREAMS buffers could not be
 *						  allocated to send the message.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrSendIpcMessage' copies the callers buffers into STREAMS buffers,
 *	contructs an STREAMS message and queues the message for delivery to the
 *	peer process on the STREAM.
 *
 * NOTES
 *	If the channel was opened with GIPC_SYNC, the call will block if the
 *	STREAMS mechanism is blocking due to flow control.  Direct copy from User
 *	Space buffers to STREAMS buffers is supported.
 *
 *	It is the responsibility of the caller to free the 'dupMessage' returned
 *	when it is no longer needed via NWstrFreeIpcMessage(3K).  This must be
 *	done explicitly, as the NWstrCloseIpcChannel(3K) will not perform this
 *	operation, thus any un free'd duplicates STREAMS messages will be 
 *	left stranded.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrReceiveIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrSendIpcMessage(
	GIPC_CHANNEL_T	*ipcChannel,
	NUC_IOBUF_T	*controlMsg,
	NUC_IOBUF_T	*dataFragment1,
	NUC_IOBUF_T	*dataFragment2,
	uint32		msgType,
	mblk_t		**dupMessage,
	int32		*diagnostic,
	uint8		*signature)
{

	register	int		minPeerMsgSize;
	register	int		maxPeerMsgSize;
	pl_t					processorLevel;
	register	mblk_t		*sendMessage;
	register	int		sendMsgSize;
	register	STR_CHANNEL_T	*strChannel;

	NTR_ENTER(5, ipcChannel, controlMsg, dataFragment1, dataFragment2, msgType);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	processorLevel = RW_RDLOCK (strChannel->channelRWLock, plstr);

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		RW_UNLOCK (strChannel->channelRWLock, processorLevel);
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 * Validate the size of this message
	 */
	minPeerMsgSize = strChannel->writeQueue->q_next->q_minpsz;
	maxPeerMsgSize = strChannel->writeQueue->q_next->q_maxpsz;

	RW_UNLOCK (strChannel->channelRWLock, processorLevel);

	sendMsgSize = controlMsg->bufferLength + dataFragment1->bufferLength;
	if ( dataFragment2 )
		sendMsgSize += dataFragment2->bufferLength;
	if ( maxPeerMsgSize == INFPSZ ) {
		if ( strmsgsz > 0 ) {
			/*
			 * Bound by Kernel Constraint
			 */
			maxPeerMsgSize = strmsgsz;
		} else {
			/*
			 * Bound by our own limits
			 */
			maxPeerMsgSize = DEFAULT_MAX_MSG_SIZE;
		}
	} else {
		if ( strmsgsz > 0 ) {
			/*
			 * Bound by Module or Kernel Constraint
			 */
			maxPeerMsgSize = min(maxPeerMsgSize, strmsgsz);
		}
	}
	if ( ( sendMsgSize < minPeerMsgSize) || 
			( sendMsgSize > maxPeerMsgSize) ||
			( controlMsg->bufferLength > strctlsz ) ) {
		/*
		 * This message is to big, to small, or the control block
		 * portion is to big.
		 */
		*diagnostic = NWD_GIPC_BIG_MSG;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	if ( msgType != GIPC_HIPRI_MSG ) {
		/*
	 	 * Is the downstream channel blocking?
	 	 */
		while ( !canput(strChannel->writeQueue->q_next) ) {
			/*
			 * Downstream is blocking?
			 */
			if ( strChannel->eventMode == GIPC_ASYNC ) {
				/*
				 * Can't block, let caller know we would block
				 */
				*diagnostic = NWD_GIPC_BLOCKING;
				return( NTR_LEAVE( FAILURE));
			}
#ifdef NEVER_EXECUTED
			else {
				/*
				 * Sleep til awakened by NWstrWriteHeadService(3K)
				 * and then RACE with any other processes blocked
				 * to send.
				 */
				strChannel->channelState = STR_CHANNEL_SLEEPING;
				processorLevel = LOCK (criticalSectLock, NUCPLHI);
				SV_WAIT (strChannel->strChannelSV, primed, criticalSectLock);
			}
#endif NEVER_EXECUTED
		} 
	}

	/*
	 * Make a STREAMS message out of the three buffers
	 */
	sendMessage = buildStreamMsg(controlMsg, dataFragment1, dataFragment2,
			msgType, diagnostic, signature);

	if ( !sendMessage ) {
		/*
		 * STREAMS Buffer could'nt be built, either a resource shortage,
		 * or memory fault in a user space buffer, diagnostic is set
		 */
		return( NTR_LEAVE( FAILURE));
	}

	putnext(strChannel->writeQueue, sendMessage);	/* Send the message along */

	processorLevel = RW_WRLOCK (strChannel->channelRWLock, plstr);

	strChannel->channelState = STR_CHANNEL_IDLE;

	RW_UNLOCK (strChannel->channelRWLock, processorLevel);

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrSizeOfIpcMessage(3K), \
 *			 ./man/kernel/kipc/streams/SizeOfIpcMsg)
 * NAME
 *	NWstrSizeOfIpcMessage -
 *					Return the size in blocks and bytes of
 *					read queue head message.  Member
 *					of the Generic Inter Process
 *					Communication Layer Operations
 *					switch structure NWgipcOpsSw[].
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	ccode_t
 *	NWgipcOpsSw[RITCHIE_STREAMS].SizeOfIpcMessage(ipcChannel,
 *				controlMsgBlocks, controMsgBytes, dataMsgBlocks,
 *				dataMsgBytes, diagnostic)
 *
 *	opaque_t	*ipcChannel;		/* Opaque to caller	*\
 *	int32		*controlMsgBlocks;
 *	int32		*controlMsgBytes;
 *	int32		*dataMsgBlocks;
 *	int32		*dataMsgBytes;
 *
 * INPUT
 *	ipcChannel	- A pointer to the GIPC Channel descriptor
 *			  object.
 *
 * OUTPUT
 *	controlMsgBlocks - Set to total STREAMS blocks linked into the control
 *			   portion of the read queue head message.
 *
 *	controlMsgBytes	- Set to total bytes in the control portion of the read
 *			  queue head message.
 *
 *	dataMsgBlocks	- Set to total STREAMS blocks linked into the data portion
 *			  of the read queue head message.
 *
 *	dataMsgBytes	- Set to total bytes in the data portion of the read
 *			  queue head message.
 *
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_NO_CHANNEL]	- The 'strChannel' argument is
 *						  not assoicated with an open
 *						  STREAMS Channel.
 *
 *			[NWD_GIPC_NO_MESSAGE]	- There is no message on the
 *						  Channel read queue.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrSizeOfIpcMessage' is used to determine the total number of 
 *	STREAMS blocks the control and data portions of the message span.  It also
 *	returns the total number of bytes in the control and data portions of the
 *	message.  The total number of blocks is usefull for walk throughs with
 *	NWstrPreviewIpcMessage(3K), while the total number of bytes are usefull
 *	for walk throughs with NWstrReceiveIpcMessage(3K).
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrReceiveIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrSizeOfIpcMessage(GIPC_CHANNEL_T *ipcChannel, int32 *controlMsgBlocks,
						int32 *controlMsgBytes, int32 *dataMsgBlocks,
						int32 *dataMsgBytes, int32 *diagnostic)
{

	register	mblk_t		*message;
	register	STR_CHANNEL_T	*strChannel;
	pl_t					pl;

	NTR_ENTER (5, ipcChannel, controlMsgBlocks, controlMsgBytes, dataMsgBlocks, dataMsgBytes);

	*diagnostic = SUCCESS;

	/*
	 * Make some sanity checks, it is the responsibility of our parent
	 * GIPC to initialize us, and pass us valid channels.
	 */
	NTR_ASSERT(NWstrInitialized);

	strChannel = (STR_CHANNEL_T *)ipcChannel->realIpcChannel;
	NTR_ASSERT(strChannel);

	pl = RW_RDLOCK (strChannel->channelRWLock, plstr);

	if ( strChannel->channelState == STR_CHANNEL_HANGUP ) {
		RW_UNLOCK (strChannel->channelRWLock, pl);
		/*
		 * Peer closed on us, let caller know, so they can
		 * close out the channel.
		 */
		*diagnostic = NWD_GIPC_NO_CHANNEL;
		return( NTR_LEAVE( FAILURE));
	}

	/*
	 * Now sanity check the Channel Object.
	 */
	NTR_ASSERT(strChannel->readQueue);
	NTR_ASSERT(strChannel->writeQueue);

	if ( !strChannel->currentMessage ) {
		/*
		 * There are no read messages
		 */
		*diagnostic = NWD_GIPC_NO_MESSAGE;
		return( NTR_LEAVE( FAILURE));
	}

	message = strChannel->currentMessage;

	RW_UNLOCK (strChannel->channelRWLock, pl);

	*controlMsgBlocks = 0;
	*controlMsgBytes = 0;

	/*
	 * Compute the control blocks and bytes
	 */
	while ( message && (message->b_datap->db_type != M_DATA) ) {
		(*controlMsgBlocks)++;
		*controlMsgBytes += (int) (message->b_wptr - message->b_rptr);
		message = message->b_cont;
	}

	*dataMsgBlocks = 0;
	*dataMsgBytes = 0;

	/*
	 * Compute the the data blocks and bytes
	 */
	while ( message ) {
		(*dataMsgBlocks)++;
		*dataMsgBytes += (int) (message->b_wptr - message->b_rptr);
		message = message->b_cont;
	}

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrReadHeadPut(3K), \
 *			 ./man/kernel/kipc/streams/ReadHeadPut)
 * NAME
 *	NWreadHeadInit.qi_putp	- The NUC STREAMS Head Read Put Procedure,
 *				  which receives all upstream messages
 *				  being received from peer processes on
 *				  STREAMS.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	int
 *	NWstrReadHeadPut(upStreamQueue, newReadMessage )
 *
 *	queue_t	*upStreamQueue;
 *	mblk_t	*newReadMessage;
 *
 * INPUT
 *	upStreamQueue	- Pointer to the Read Head Stream Queue which is
 *			  interrupting, and putting a message to the NUC Read
 *			  Head.
 *
 *	newReadMessage	- Pointer to the head of a STREAMS message being received
 *			  (put from the peer to the Head).
 *
 * OUTPUT
 *	strChannel->currentMessage	- Set to new receive message
 *					  retreived from queue.
 *
 *	strChannel->curControlBlock	- Set to control portion block if any, or
 *					  NULL.
 *
 *	strChannel->curControlMark	- Set to the control first byte if any, or
 *					  NULL.
 *
 *	strChannel->curDataBlock	- Set to the data portion first block
 *					  if any, or NULL.
 *
 *	strChannel->curDataMark		- Set to the data first byte if any, or 
 *					  NULL.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'NWstrReadHeadPut' is the put (interrupt) procedure of the NUC STREAM
 *	Head.  It is the entry point for all messages arriving from peer
 *	processes conencted via STREAMS, and represents the termination of a 
 *	upstream.  It is loaded into the NWreadHeadInit.qi_putp, and called via
 *	putnext() STREAMS function by the peer process.
 *
 * NOTES
 *	This procedure is essentially analogous to a lower put procedure in a 
 *	Mutliplexing driver, or the regular STREAMS Head 'strrput()' procedure.
 *	Since the NUC Core Services has its own STREAM Head plugged into the
 *	Generic Inter Process Communications Operations Switch NWgipcOpsSw[],
 *	it must have its own put procedure for the private STREAMS Head.
 *	Messages which are destined for GIPC consuemrs are always queued to our
 *	NWstrReadHeadService(3K) for subsequent delivery.  This is always done,
 *	to prevent stack burden due to H/W interrupt drivers and MUXes 
 *	being on the stack.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrPrivateIoctlMessage(3K), NWstrReadHeadService(3K),
 *	NWstrReceiveIpcMessage(3K), NWstrRegisterAsyncEventHandler(3K)
 *
 * END_MANUAL_ENTRY
 */

void_t
NWstrReadHeadPut(queue_t *upStreamQueue, mblk_t *newReadMessage )
{

	register	STR_CHANNEL_T		*strChannel;
	register	struct	stroptions	*strOptions;
	register	uint32				msgType;
	pl_t							pl;
	extern		uint32				doDriverStop;
	extern		lock_t				*nucLock;

	NTR_ENTER (2, upStreamQueue, newReadMessage, 0, 0, 0);

	/*
	 * Trap out all ignored MESSAGES, these have no meaning in the
	 * NUC Core Service Subsystem.
	 */
	switch( newReadMessage->b_datap->db_type ) {

	case M_CTL:
	case M_BREAK:
	case M_DELAY:
	case M_PASSFP:
	case M_SIG:
	case M_PCSIG:
	case M_START:
	case M_STOP:
		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;
	}

	/*
	 * Handle STREAM Head Specific Messages, require no Core Service context
	 */
	switch( newReadMessage->b_datap->db_type ) {

	case M_FLUSH:
		/*
		 * Peer is requesting to flush either our Read Queue, and/or our
		 * Write Queue.  The first byte in the Block indicates which
		 * is to be flushed.
		 */
		if ( *newReadMessage->b_rptr & FLUSHR )
			/*
			 * Flush the Read Queue
			 */
			flushq(upStreamQueue, FLUSHALL);

		if ( *newReadMessage->b_rptr & FLUSHW ) {
			/*
			 * Flush the Write Queue
			 */
			flushq(WR(upStreamQueue), FLUSHALL);
			*newReadMessage->b_rptr &= ~FLUSHR;
			qreply(upStreamQueue, newReadMessage);
			NTR_LEAVE(0);
			return;
		}
		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;

	case M_IOCTL:
		/*
		 * This is very strange, NAK them
		 */
		newReadMessage->b_datap->db_type = M_IOCNAK;
		qreply(upStreamQueue, newReadMessage);
		NTR_LEAVE(0);
		return;

	case M_SETOPTS:
		/*
		 * Set the requested options in our STREAM Head.
		 */

		/*
		 * Note:
		 *	SO_READOPT	- Has no meaning in the NUC Core Services
		 *			  Request/Response Model.
		 *	SO_WROFF	- Currently not implemented.  If necessary
		 *			  this requires the 'strChannel' structure
		 *			  to add a write offset, which will be
		 *			  logically prepended to the beginning
		 *			  of M_DATA blocks by buildStreamMsg(3K).
		 */

		NTR_ASSERT((newReadMessage->b_wptr - newReadMessage->b_rptr) ==
			sizeof(struct stroptions));

		strOptions = (struct stroptions *) newReadMessage->b_rptr;

		if ( strOptions->so_flags & SO_MINPSZ ) {
			/* Reset the Mininum Packet Size Accepted by Read Queue */
			upStreamQueue->q_minpsz = strOptions->so_minpsz;
		}

		if ( strOptions->so_flags & SO_MAXPSZ ) {
			/* Reset the Maximum Packet Size Accepted by Read Queue */
			upStreamQueue->q_maxpsz = strOptions->so_maxpsz;
		}

		if ( strOptions->so_flags & SO_HIWAT ) {
			/* Reset the Flow Control High Water Mark of Read Queue */
			upStreamQueue->q_hiwat = strOptions->so_hiwat;
		}

		if ( strOptions->so_flags & SO_LOWAT ) {
			/* Reset the Flow Control High Water Mark of Read Queue */
			upStreamQueue->q_lowat = strOptions->so_lowat;
		}

		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;
	}

	/*
	 * Select our STREAMS Channel for this queue to handle Core Service 
	 * Context messages
	 */
	strChannel = (STR_CHANNEL_T *)upStreamQueue->q_ptr;
	if ( (strChannel == NULL) ||
			(strChannel->channelState == STR_CHANNEL_BOGUS) ) {
		/*
		 * Must have been interrupted while attempting to close,
		 * discard this message and return immediately!!
		 */
		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;
	}

	msgType = GIPC_NORMAL_MSG;


	/*
	 * Decode the message received
	 */
	switch( newReadMessage->b_datap->db_type ) {

	/*
	 * These messages have meaning and are propagated back up through
	 * to the GIPC callers.
	 */
	
	case M_PCPROTO:
		/*FALLTHRU*/
	case M_ERROR:
		msgType = GIPC_HIPRI_MSG;
		/*FALLTHRU*/
	case M_DATA:
		/*FALLTHRU*/
	case M_PROTO:
		/*
		 * These all can pass on up to consumers of GIPC, they are
		 * valid messages which are assumed to be responses to our
		 * requests, or a out of band message.
		 */
		break;

	case M_IOCACK:
		/*FALLTHRU*/
	case M_IOCNAK:
		/*
		 * Is this Channel waiting for a IOCTL response?
		 */
		pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
		if ( strChannel->channelState == STR_CHANNEL_WANT_IOCTL ) {
			/*
			 * We have our response, make it available and
			 * reschedule the process context waiting for it
			 */
			strChannel->currentMessage = newReadMessage;
			NWtlVSemaphore_l (strChannel->syncSemaphore);
			RW_UNLOCK (strChannel->channelRWLock, pl);
			NTR_LEAVE(0);
			return;
		} else {
			RW_UNLOCK (strChannel->channelRWLock, pl);
			/*
			 * This is a spurious IOCTL, drop it!!
			 */
			freemsg(newReadMessage);
			NTR_LEAVE(0);
			return;
		}
	
	case M_HANGUP:
		/*
		 * Peer is requesting that we close the STREAM
		 * Post this on channel, which wil cause GIPC caller to
		 * close.  The message itself will not be receieved,
		 * rather the the channelState will cause an error to
		 * be generated and passed back to the GIPC consumer, at
		 * which time the GIPC consumer will close the Channel.
		 */
		msgType = GIPC_HIPRI_MSG;
		pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
		strChannel->channelState = STR_CHANNEL_HANGUP;
		RW_UNLOCK (strChannel->channelRWLock, pl);

#ifdef NUC_DEBUG
		cmn_err (CE_WARN, "IPX sent us a hangup message\n");
#endif NUC_DEBUG

		pl = LOCK (nucLock, plstr);
		if( doDriverStop == FALSE ) {
			doDriverStop = TRUE;
			UNLOCK (nucLock, pl);


			NTR_PRINTF("NWstrReadHeadService: Waking up nucHandleIpxHangupSV\n", 0,0,0);
			SV_SIGNAL (nucHandleIpxHangupSV, 0);

		} else {
			UNLOCK (nucLock, pl);
		}
		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;

	default:
		/*
		 * Unknown message, discard it.
		 */
		freemsg(newReadMessage);
		NTR_LEAVE(0);
		return;
	}

	/*
	 * Valid Message destined to GIPC consumer.  Always queue to our
	 * service procedure, we don't know the depth of the stack or whether
	 * we are in a H/W interrupt context (ie. the message has been putnext()
	 * all the way from the driver to us).  Let our service procedure
	 * deliver it from STREAM context.
	 */

	
	if ( msgType == GIPC_HIPRI_MSG ) {
		/*
		 * Queue it to the head
		 */
		putbq(strChannel->readQueue, newReadMessage);
	} else {
		/*
		 * Queue it to the Tail
		 */
		putq(strChannel->readQueue, newReadMessage);
	}


	if ( (strChannel->currentMessage) ||
			(strChannel->channelState == STR_CHANNEL_WANT_IOCTL ) ){

		/*
		 * Message can not be scheduled until head releases hold on
		 * Channel.  Make sure our service doesn't run via putq().
		 */
		noenable(strChannel->readQueue);

	} else {
		/*
		 * Place this queue on the STREAMS Scheduler List
		 */

		NWstrReadHeadService( strChannel->readQueue);

	}


	NTR_LEAVE(0);
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrReadHeadService(3K), \
 *			 ./man/kernel/kipc/streams/ReadHeadService)
 * NAME
 *	NWreadHeadInit.qi_srvp	- The NUC STREAMS Head Read Service Procedure,
 *				  which scheduled to receive the head message
 *				  on the Head Read Queue, when messages were
 *				  unable to be received by our Put Procedure.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	int
 *	NWstrReadHeadService(upStreamQueue)
 *
 *	queue_t	*upStreamQueue;
 *
 * INPUT
 *	upStreamQueue	- Pointer to the Read Head Stream Queue which is
 *			  being scheduled for service by the NUC Read Head.
 *
 * OUTPUT
 *	strChannel->currentMessage	- Set to new receive message
 *					  retreived from queue.
 *
 *	strChannel->curControlBlock	- Set to control portion block if any, or
 *					  NULL.
 *
 *	strChannel->curControlMark	- Set to the control first byte if any, or
 *					  NULL.
 *
 *	strChannel->curDataBlock	- Set to the data portion first block
 *					  if any, or NULL.
 *
 *	strChannel->curDataMark		- Set to the data first byte if any, or 
 *					  NULL.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'NWstrReadHeadService' is called by the STREAMS scheduler to receive
 *	the head message on a Read Queue, which was unabled to be received 
 *	by our put procedure 'NWstrReadHeadPut(3K).  It is loaded into the 
 *	NWreadHeadInit.qi_svrp, and called via the STREAMS scheduler.
 *	
 * NOTES
 *	This procedure is analogous to a lower service procedure in a 
 *	Mutliplexing driver.  Since the NUC Core Services has its own STREAM
 *	Head plugged into the Generic Inter Process Communications Operations
 *	Switch NWgipcOpsSw[], it must have its own service procedure for the
 *	private STREAMS Head.  Messages which were not able to be scheduled by
 *	NWstrReadHeadPut(3K) are queued for service by this procedure to avoid
 *	overloading the stack (it is not known whether we are called in a
 *	H/W interrupt context, or what depth the stack is (how many STREAMS
 *	MUXes are between us and driver), thus we queue for deferred processing
 *	by STREAMS scheduler).  This service procedure serializes all GIPC
 *	consumer traffic on a Channel, thus guaranteeing the process 
 *	exclusivity to the Channel.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPreviewIpcMessage(3K),
 *	NWstrReadHeadPut(3K), NWstrReceiveIpcMessage(3K),
 *	NWstrRegisterAsyncEventHandler(3K)
 *
 * END_MANUAL_ENTRY
 */

void
NWstrReadHeadService(queue_t *upStreamQueue)
{

	register	STR_CHANNEL_T	*strChannel;
	register	uint32			msgType;
	pl_t						pl;

	NTR_ENTER (1, upStreamQueue, 0, 0, 0, 0);

	NTR_ASSERT(NWstrInitialized);

	/*
	 * Select our STREAMS Channel for this queue
	 */
	if ( ((strChannel = (STR_CHANNEL_T *)upStreamQueue->q_ptr) == NULL) ||
			(strChannel->channelState == STR_CHANNEL_BOGUS) ) {
		/*
		 * Must have been scheduled while attempting to close,
		 * discard this message and return immediately!!
		 */
		noenable(upStreamQueue);
		NTR_LEAVE(0);
		return;
	}

	msgType = GIPC_NORMAL_MSG;

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);

	if ( strChannel->currentMessage ||
			strChannel->channelState == STR_CHANNEL_WANT_IOCTL ||
			!qsize(strChannel->readQueue) ) {
		/*
		 * Can't schedule now, not sure how we got called, but we really
		 * should be NOENABLE'd right now on this queue, lets place our
		 * queue in this state, and let the current message request
		 * scheduling when the current message has drained!!!
		 */
		noenable(strChannel->readQueue);
		RW_UNLOCK (strChannel->channelRWLock, pl);
		NTR_LEAVE(0);
		return;
	}

	if ((strChannel->currentMessage = getq(strChannel->readQueue)) == NULL){
		/*
		 * We got scheduled with no message, make sure we are
	 	 * not enabled!
		 */
		noenable(strChannel->readQueue);
		RW_UNLOCK (strChannel->channelRWLock, pl);
		NTR_LEAVE(0);
		return;
	}

	/*
	 * Disable our queue, when the current message has drained we will again
	 * be enabled by our Head.
	 */
	noenable(strChannel->readQueue);

	/*
	 * Make this the current message for this channel
	 */

	if ( strChannel->currentMessage->b_datap->db_type != M_DATA ) {
		/*
		 * Message has a Control Block
		 */

		/*
		 * Is it Hi Priority
		 */
		if ( strChannel->currentMessage->b_datap->db_type == M_PCPROTO )
			msgType = GIPC_HIPRI_MSG;

		strChannel->curControlBlock = strChannel->curPreCtlBlock =
			strChannel->currentMessage;
		strChannel->curControlMark =
			strChannel->curControlBlock->b_rptr;
		if ( strChannel->currentMessage->b_cont ) {
			/*
			 * And Data Block(s)
			 */
			strChannel->curDataBlock = strChannel->curPreDataBlock
				= strChannel->currentMessage->b_cont;
			strChannel->curDataMark =
				strChannel->curDataBlock->b_rptr;
		} else {
			/*
			 * No Data Block(s)
			 */
			strChannel->curDataBlock = strChannel->curPreDataBlock
				= NULL;
			strChannel->curDataMark = NULL;
		}
	} else {
		/*
		 * No Control Block, only Data Block(s)
		 */
		strChannel->curControlBlock = strChannel->curPreCtlBlock
				= NULL;
		strChannel->curControlMark = NULL;
		strChannel->curDataBlock = strChannel->curPreDataBlock
			= strChannel->currentMessage;
		strChannel->curDataMark =
			strChannel->curDataBlock->b_rptr;
	}
	
	RW_UNLOCK (strChannel->channelRWLock, pl);

	if ( strChannel->eventMode == GIPC_ASYNC ) {

		if ( strChannel->callBackFunction ) {
			/*
			 * Call the Async Handler for this channel
			 */


			(*strChannel->callBackFunction)(
				&(strChannel)->gipcChannel,
				strChannel->callBackHandle, msgType);
		}
	}
#ifdef NEVER_EXECUTED
	else {
		/*
		 * Wakeup anybody sleeping on this channel
		 */
		NWtlVSemaphore_l(strChannel->syncSemaphore);
	}
#endif NEVER_EXECUTED


	NTR_LEAVE(0);
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(NWstrWriteHeadService(3K), \
 *			 ./man/kernel/kipc/streams/WriteHeadService)
 * NAME
 *	NWwriteHeadInit.qi_srvp	- The NUC STREAMS Head Write Service Procedure,
 *				  which is always scheduled to send messages
 *				  down stream to peer processes.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	int
 *	NWstrWriteHeadService(downStreamQueue)
 *
 *	queue_t	*downStreamQueue;
 *
 * INPUT
 *	downStreamQueue	- Pointer to the Write Head Stream Queue which is
 *			  being scheduled for service by the NUC Write Head.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'NWstrWriteHeadService' is called by the STREAMS scheduler to send
 *	messages from the NUC Write Head.  All down stream messages are queued
 *	for service to force a context switch prior to sending the message.  This
 *	is necessary as the NUC Stream Head is a Request/Response modeled, event
 *	driven architecture.  Therefore, the receive side must catch a response
 *	in its process context.  It is loaded into the NWwriteHeadInit.qi_svrp,
 *	and called via the STREAMS scheduler.
 *	
 * NOTES
 *	This procedure is essentially analogous to a lower service procedure in a 
 *	Mutliplexing driver.  Since the NUC Core Services has its own STREAM
 *	Head plugged into the Generic Inter Process Communications Operations
 *	Switch NWgipcOpsSw[], it must have its own service procedure for the
 *	private STREAMS Head.  By always delaying sending of messages from the
 *	NUC Stream Head down stream, it is assured that the process context
 *	sending the message can switch, so that messges received back up stream
 *	can be handled in the proper context.  Additionally, it is called when
 *	back flow control enabled, at which time it will wakeup a sleeping 
 *	process waiting to send.
 *
 *	Current thinking: We always putnext() in the write put routine, so
 *	the only reason we ever run this routine is when we're back-enabled.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPrivateIoctlMessage(3K),
 *	NWstrSendIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

void
NWstrWriteHeadService(downStreamQueue)

queue_t	*downStreamQueue;
{
	register	STR_CHANNEL_T	*strChannel;

	NTR_ENTER (1, downStreamQueue, 0, 0, 0, 0);

	NTR_ASSERT(NWstrInitialized);

	/*
	 * Select our STREAMS Channel for this queue
	 */
	if ( ((strChannel = (STR_CHANNEL_T *)downStreamQueue->q_ptr) == NULL) ||
			(strChannel->channelState == STR_CHANNEL_BOGUS) ) {
		/*
		 * Must have been scheduled while attempting to close,
		 * Return now, our queue isn't going to last much longer.
		 */
		noenable(downStreamQueue);
		NTR_LEAVE(0);
		return;
	}
	
	/*
	 *	We're running because we were back-enabled because
	 *	of flow control.  The logjam is now free, so wake up
	 *	anyone wishing to write on this stream.
	 */

	SV_BROADCAST (strChannel->strChannelSV, 0);

	NTR_LEAVE(0);
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(strIoctlTimeOut(3K), \
 *			 ./man/kernel/kipc/streams/IoctlTimeOut)
 * NAME
 *	(*callout[timeOutId].c_func)	- Private call back function for time outs
 *					  of NWstrPrivateIoctlMessage(3K).
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	private int
 *	strIoctlTimeOut(strChannel)
 *	
 *	opaque_t	*strChannel;	/* Opaque to caller timein()	*\
 *
 * INPUT
 *	strChannel			- Pointer to the strChannel object which
 *					  had an ioctl wait expire.
 *
 * OUTPUT
 *	strChannel->channelState	- Changed to STR_CHANNEL_IDLE;
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'strIoctlTimeOut' is called by UNIX callout handler timein()
 *	to service a timeout on a strChannel which is waiting for a 
 *	ioctl to complete.
 *
 * NOTES
 *	This function is expected to only be called in the case of a
 *	NWstrPrivateIoctlMessage(3K) timeout, therefore it should only be
 *	placed on the callout[] list by this procedure for this reason.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPrivateIoctlMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

void_t
strIoctlTimeOut(STR_CHANNEL_T *strChannel)
{
	pl_t		pl;

	NTR_ENTER (1, strChannel, 0, 0, 0, 0);

	pl = RW_WRLOCK (strChannel->channelRWLock, plstr);
	/*
	 * Sanity Check this channel is waiting for IOCTL
	 */
	NTR_ASSERT(strChannel->channelState == STR_CHANNEL_WANT_IOCTL);

	/*
	 * Mark the strChannel state as idle and wakeup the IOCTL who is
	 * waiting.
	 */
	strChannel->channelState = STR_CHANNEL_IDLE;
	NWtlVSemaphore_l(strChannel->syncSemaphore);
	RW_UNLOCK (strChannel->channelRWLock, pl);

	NTR_LEAVE(0);
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(drainStreamMsg(3K), \
 *			 ./man/kernel/kipc/streams/DrainStreamMsg)
 * NAME
 *	drainStreamMsg			- Private function to drain (copy) STREAMS
 *					  message into supplied buffers.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *	
 *	private ccode_t
 *	drainStreamMsg(strChannel, controlMsg, dataFragment1,
 *				dataFragment2, diagnostic)
 *	
 *	STR_CHANNEL_T	*strChannel;
 *	NUC_IOBUF_T	*controlMsg;
 *	NUC_IOBUF_T	*dataFragment1;
 *	NUC_IOBUF_T	*dataFragment2;
 *	int32		*diagnostic;
 *
 * INPUT
 *	strChannel			- A pointer to the STREAMS Channel
 *					  descriptor object.
 *
 *	controlMsg->buffer		- A pointer to a contiguous block to
 *					  receive control message.
 *
 *	controlMsg->bufferLength	- Size in bytes of 'controlMsg->buffer'.
 *
 *	controlMsg->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment1->buffer		- A pointer to a contiguous block to
 *					  receive data message.
 *
 *	dataFragment1->bufferLength	- Size of 'dataFragment1->buffer'.
 *
 *	dataFragment1->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment2->buffer		- A pointer to a contiguous block to
 *					  receive data message into once
 *					  'dataFragment1->buffer' is filled.
 *
 *	dataFragment2->bufferLength	- Size of 'dataFragment2->buffer'.  Not
 *					  Used if 'dataFragment2' is NULL.
 *
 *	dataFragment2->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space. Not Used if 'dataFragment2'
 *					  is NULL.
 *
 * OUTPUT
 *	controlMsg->bufferLength	- Size in bytes copied into
 *					  'controlMsg->buffer'.
 *
 *	dataFragment1->bufferLength	- Size in bytes copied into
 *					  'dataFragment1->buffer'.
 *
 *	dataFragment2->bufferLength	- Size in bytes copied into
 *					  'dataFragment2->buffer'.
 *	
 *
 *	diagnostic			- set to one of the following when
 *					  return value <0.
 *
 *			[NWD_GIPC_BUF_FAULT]	- The 'controlMsg',
 *						  'dataFragment1', or 
 *						  'dataFragment2' located in the
 *						  USER Memory Space generated a
 *						  EFAULT.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'buildStreamMsg' is called by NWstrReceiveIpcMessage(3K)
 *	to copy unconsumed STREAMS message into supplied buffers.
 *
 * NOTES
 *	This function copies as much of the message as possible into the callers
 *	buffers.   The message pointers are advanced to consume the copied
 *	portions.
 *
 * SEE ALSO
 *	NWstrReceiveIpcMessage(3K),
 *
 * END_MANUAL_ENTRY
 */

ccode_t
drainStreamMsg(strChannel, controlMsg, dataFragment1, dataFragment2, diagnostic)

STR_CHANNEL_T	*strChannel;
NUC_IOBUF_T	*controlMsg;
NUC_IOBUF_T	*dataFragment1;
NUC_IOBUF_T	*dataFragment2;
int32		*diagnostic;
{
	register	int32	copySize, fillOffset;
	register	uchar	*fillAddress;

	NTR_ENTER (5, strChannel, controlMsg, dataFragment1, dataFragment2, diagnostic);

	*diagnostic = SUCCESS;

#ifdef DEBUG_TRACE_STREAMS
	trace_iobuf( controlMsg);
	trace_iobuf( dataFragment1);
	trace_iobuf( dataFragment2);
#endif DEBUG_TRACE_STREAMS

	if ( controlMsg ) {
		/*
		 * Fill the control buffer
		 */
		fillAddress = (uchar *) controlMsg->buffer;
		for ( fillOffset=0;
			fillOffset < controlMsg->bufferLength &&
			strChannel->curControlBlock; ) {

			copySize = min((controlMsg->bufferLength - fillOffset),
				(strChannel->curControlBlock->b_wptr - 
					strChannel->curControlBlock->b_rptr));

			switch( controlMsg->memoryType) {
				case IOMEM_KERNEL: 				/* STREAMS to Kernel Buffer	*/
					bcopy((caddr_t)strChannel->curControlBlock->b_rptr,
						(caddr_t)fillAddress, copySize);
					break;

				case IOMEM_USER: 				/* STREAMS to User Buffer	*/
					NTR_EXT_CALL( copyout);
					if (copyout((caddr_t)strChannel->curControlBlock->b_rptr,
						(caddr_t)fillAddress, copySize)) {
						/*
						 * User Mode Buffer Fault, let caller
						 * know this
						 */
						NTR_EXT_RETURN( copyout);
						*diagnostic = NWD_GIPC_BUF_FAULT;
						return( NTR_LEAVE( FAILURE));
					}
					NTR_EXT_RETURN( copyout);
					break;

				case IOMEM_PHYSICAL:				/* STREAMS to Physical Memory	*/
#ifdef NOT_USED
					NWtlCopyToPhys( strChannel->curControlBlock->b_rptr,
					    fillAddress, copySize);
#else
					cmn_err(CE_PANIC,"IOMEM_PHYSICAL called, fill ctl buf");
#endif
					break;

			}

			fillAddress += copySize;
			fillOffset += copySize;
			strChannel->curControlBlock->b_rptr += copySize;
	
			if ( strChannel->curControlBlock->b_rptr == 
					strChannel->curControlBlock->b_wptr ) {
				/*
				 * Current Control Block drained, move to next
				 * block
				 */
				if ( strChannel->curControlBlock->b_cont &&
			   		strChannel->curControlBlock->b_cont->
						b_datap->db_type != M_DATA) {
					strChannel->curControlBlock =
					   strChannel->curControlBlock->b_cont;
				} else {
					/*
					 * No more Control Blocks in message
					 */
					strChannel->curControlBlock = NULL;
				}
			}
		}
		controlMsg->bufferLength = fillOffset;
	}

	if ( dataFragment1 ) {
		/*
		 * Fill the data buffer fragment1
		 */
		fillAddress = (uchar *) dataFragment1->buffer;
		for ( fillOffset=0;
			fillOffset < dataFragment1->bufferLength &&
			strChannel->curDataBlock; ) {

			copySize = min((dataFragment1->bufferLength - fillOffset),
				(strChannel->curDataBlock->b_wptr - 
					strChannel->curDataBlock->b_rptr));

			switch( dataFragment1->memoryType) {
				case IOMEM_KERNEL: 				/* STREAMS to Kernel Buffer	*/
					bcopy((caddr_t)strChannel->curDataBlock->b_rptr,
						(caddr_t)fillAddress, copySize);
					break;

				case IOMEM_USER: 				/* STREAMS to User Buffer	*/
					NTR_EXT_CALL( copyout);
					if (copyout((caddr_t)strChannel->curDataBlock->b_rptr,
						(caddr_t)fillAddress, copySize)) {
						/*
						 * User Mode Buffer Fault, let caller
						 * know this
						 */
						NTR_EXT_RETURN( copyout);
						*diagnostic = NWD_GIPC_BUF_FAULT;
						return( NTR_LEAVE( FAILURE));
					}
					NTR_EXT_RETURN( copyout);
					break;

				case IOMEM_PHYSICAL:			/* STREAMS to Physical Memory	*/
#ifdef NOT_USED
					NWtlCopyToPhys( strChannel->curDataBlock->b_rptr,
					    fillAddress, copySize);
#else
					cmn_err(CE_PANIC,"IOMEM_PHYSICAL called, fill data frag 1");
#endif
					break;

			}

			fillAddress += copySize;
			fillOffset += copySize;
			strChannel->curDataBlock->b_rptr += copySize;

			if ( strChannel->curDataBlock->b_rptr == 
					strChannel->curDataBlock->b_wptr ) {
				strChannel->curDataBlock =
					strChannel->curDataBlock->b_cont;
			}
		}
		dataFragment1->bufferLength = fillOffset;
		NTR_PRINTF("drainStreamMsg: dataFragment1->bufferLength=0x%x\n",
			dataFragment1->bufferLength, 0, 0);
	}
	
	if ( dataFragment2 ) {
		/*
		 * Fill the data buffer fragment2
		 */
		fillAddress = (uchar *) dataFragment2->buffer;
		for ( fillOffset=0;
			fillOffset < dataFragment2->bufferLength &&
			strChannel->curDataBlock; ) {

			copySize = min((dataFragment2->bufferLength - fillOffset),
				(strChannel->curDataBlock->b_wptr - 
					strChannel->curDataBlock->b_rptr));

			switch( dataFragment2->memoryType) {
				case IOMEM_KERNEL: 				/* STREAMS to Kernel Buffer	*/
					bcopy((caddr_t)strChannel->curDataBlock->b_rptr,
						(caddr_t)fillAddress, copySize);
					break;

				case IOMEM_USER: 				/* STREAMS to User Buffer	*/
					NTR_EXT_CALL( copyout);
					if (copyout((caddr_t)strChannel->curDataBlock->b_rptr,
						(caddr_t)fillAddress, copySize)) {
						/*
						 * User Mode Buffer Fault, let caller
						 * know this
						 */
						NTR_EXT_RETURN( copyout);
						*diagnostic = NWD_GIPC_BUF_FAULT;
						return( NTR_LEAVE( FAILURE));
					}
					NTR_EXT_RETURN( copyout);
					break;

				case IOMEM_PHYSICAL:			/* STREAMS to Physical Memory	*/
#ifdef NOT_USED
					NWtlCopyToPhys( strChannel->curDataBlock->b_rptr,
					    fillAddress, copySize);
#else
					cmn_err(CE_PANIC,"IOMEM_PHYSICAL called, fill data frag 2");
#endif
					break;

			}

			fillAddress += copySize;
			fillOffset += copySize;
			strChannel->curDataBlock->b_rptr += copySize;
	
			if ( strChannel->curDataBlock->b_rptr == 
					strChannel->curDataBlock->b_wptr ) {
				strChannel->curDataBlock =
					strChannel->curDataBlock->b_cont;
			}
		}
		dataFragment2->bufferLength = fillOffset;
		NTR_PRINTF("drainStreamMsg: dataFragment2->bufferLength=0x%x\n",
			dataFragment2->bufferLength, 0, 0);
	}

	return( NTR_LEAVE( SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(buildStreamMsg(3K), \
 *			 ./man/kernel/kipc/streams/BuildStreamMsg)
 * NAME
 *	buildStreamMsg			- Private function to construct a STREAMS
 *					  message from supplied buffers.
 *
 * SYNOPSIS
 *	#include "headstrconf.h"
 *
 *	private mblk_t
 *	*buildStreamMsg(controlMsg, dataFragment1, dataFragment2,
 *		msgType, diagnostic)
 *	
 *	NUC_IOBUF_T	*controlMsg;
 *	NUC_IOBUF_T	*dataFragment1;
 *	NUC_IOBUF_T	*dataFragment2;
 *	uint32		msgType;
 *	int32		*diagnostic;
 *
 * INPUT
 *	controlMsg->buffer		- A pointer to a contiguous block of
 *					  control portion to send.
 *
 *	controlMsg->bufferLength	- Size in bytes of 'controlMsg->buffer'.
 *
 *	controlMsg->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment1->buffer		- A pointer to a contiguous block of data
 *					  to send.
 *
 *	dataFragment1->bufferLength	- Size of 'dataFragment1->buffer'.
 *
 *	dataFragment1->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space, and IOMEM_PHYSICAL for physical memory buffers.
 *
 *	dataFragment2->buffer		- A pointer to a contiguous block of 
 *					  concatenated data to send.
 *
 *	dataFragment2->bufferLength	- Size of 'dataFragment2->buffer'.  Not
 *					  Used if 'dataFragment2' is NULL.
 *
 *	dataFragment2->memoryType	- Set to IOMEM_KERNEL for buffers which are
 *					  resident in kernel space, and IOMEM_USER for
 *					  buffers which are resident in USER
 *					  Space. Not Used if 'dataFragment2'
 *					  is NULL.
 *
 *	msgType				- Set to GIPC_NORMAL_MSG for regular
 *					  messages, and GIPC_HIPRI_MSG for
 *					  priority messages.
 *
 * OUTPUT
 *	diagnostic			- set to one of the following when
 *					  return value <0.
 *
 *			[NWD_GIPC_BUF_FAULT]	- The 'controlMsg',
 *						  'dataFragment1', or 
 *						  'dataFragment2' located in the
 *						  USER Memory Space generated a
 *						  EFAULT.
 *
 *			[NWD_GIPC_NO_RESOURCE]	- STREAMS buffers could not be
 *
 * RETURN VALUES
 *	!NULL	- Successful completion, message block pointer to the head
 *		  of the STREAMS message built.
 *
 *	NULL	- Failure, resource shortage at this time.
 *
 * DESCRIPTION
 *	The 'buildStreamMsg' is called by NWstrPrivateIoctlMessage(3K) and
 *	NWstrSendIpcMessage(3K) to build a STREAMS message out of supplied
 *	buffers.
 *
 * SEE ALSO
 *	NWstrIntroduction(3K), NWstrPrivateIoctlMessage(3K),
 *	NWstrSendIpcMessage(3K)
 *
 * END_MANUAL_ENTRY
 */

mblk_t *
buildStreamMsg (NUC_IOBUF_T *controlMsg, NUC_IOBUF_T *dataFragment1,
				   NUC_IOBUF_T *dataFragment2, uint32 msgType,
				   int32 *diagnostic, uint8 *signature)
{

	register	mblk_t	*streamMessage;
	register	int		priority;

	NTR_ENTER (5, controlMsg, dataFragment1, dataFragment2, msgType, diagnostic);

	*diagnostic = SUCCESS;

#ifdef DEBUG_TRACE_STREAMS
	trace_iobuf( controlMsg);
	trace_iobuf( dataFragment1);
	trace_iobuf( dataFragment2);
#endif DEBUG_TRACE_STREAMS

	if ( msgType == GIPC_HIPRI_MSG ) {
		priority = BPRI_MED;
	} else {
		priority = BPRI_LO;
	}

	/*
	 * Make a M_PROTO or M_PCPROTO block out of controlMsg
	 */
	if ( !(streamMessage = esballoc (controlMsg->buffer,
				controlMsg->bufferLength, priority,
				controlMsg->esr)) ) {
		/*
		 * STREAMS Resource Shortage, can't complete
		 */

		*diagnostic = NWD_GIPC_NO_RESOURCE;
		return( (mblk_t *)NTR_LEAVE( NULL));
	}

	streamMessage->b_wptr += controlMsg->bufferLength;

	if ( msgType == GIPC_HIPRI_MSG ) {
		streamMessage->b_datap->db_type = M_PCPROTO;
	} else {
		streamMessage->b_datap->db_type = M_PROTO;
	}

	if (dataFragment1->bufferLength) {
		if ( !(streamMessage->b_cont = esballoc (dataFragment1->buffer,
				dataFragment1->bufferLength, priority,
				dataFragment1->esr)) ) {
			*diagnostic = NWD_GIPC_NO_RESOURCE;
			return( (mblk_t *)NTR_LEAVE( NULL));
		}
		streamMessage->b_cont->b_wptr += dataFragment1->bufferLength;
	}

	if (dataFragment2 && (dataFragment2->bufferLength > 0)) {
		if ( !(streamMessage->b_cont->b_cont = esballoc (dataFragment2->buffer,
				dataFragment2->bufferLength, priority,
				dataFragment2->esr)) ) {
			*diagnostic = NWD_GIPC_NO_RESOURCE;
			return( (mblk_t *)NTR_LEAVE( NULL));
		}
		streamMessage->b_cont->b_cont->b_wptr += dataFragment2->bufferLength;
		if (signature) {
			if ( !(streamMessage->b_cont->b_cont->b_cont = esballoc (signature,
				8, priority, controlMsg->esr)) ) {
				*diagnostic = NWD_GIPC_NO_RESOURCE;
				return( (mblk_t *)NTR_LEAVE( NULL));
			}
			streamMessage->b_cont->b_cont->b_cont->b_wptr += 8;
		}
	} else {
		if (signature) {
			if ( !(streamMessage->b_cont->b_cont = esballoc (signature,
				8, priority, controlMsg->esr)) ) {
				*diagnostic = NWD_GIPC_NO_RESOURCE;
				return( (mblk_t *)NTR_LEAVE( NULL));
			}
			streamMessage->b_cont->b_cont->b_wptr += 8;
		}
	}

	return( (mblk_t *)NTR_LEAVE((uint_t) streamMessage));
}


#ifdef DEBUG_TRACE_STREAMS
void
trace_iobuf(iob)
NUC_IOBUF_T *iob;
{
	if( iob)
		NTR_TRACE( NVLTT_Gipcif_iobuf,
			iob->buffer, iob->bufferLength, iob->memoryType, 0);
	else							/* NULL iobuf, just show zips */
		NTR_TRACE( NVLTT_Gipcif_iobuf, 0, 0, 0, 0);
	return;
}
#endif /* DEBUG_TRACE_STREAMS */

ccode_t
NWstrValidateSignature (	GIPC_CHANNEL_T	*gipcChannel,
							md4_t			*md4,
							int				offset,
							int				len )
{
	mblk_t *m;
	int count;

	int b = 0;
	int r = 52;
	int a = offset;
	int i, j;

	int32 cblocks;
	int32 cbytes;
	int32 dblocks;
	int32 diag;
	uint32 length = 0;
	uint32 messageDigest[4];
	uint8 block[64];
	uint8 *signature;
	uint8 jbuf[8];
	uint8 *p;

	NTR_ENTER (4, gipcChannel, md4, offset, len, 0);

	j = NWstrSizeOfIpcMessage ( gipcChannel, &cblocks, &cbytes, &dblocks, 
		(int32 *) &length, &diag );


	/* adjust for signature */
	if( length >= 8 )
		length -=8;
	else {
		return(NTR_LEAVE(FALSE));
	}

	/* get to last data block */
	m = ((STR_CHANNEL_T *)gipcChannel->realIpcChannel)->curDataBlock;
	while (m->b_cont) {
		m = m->b_cont;
	}

	/* adjust strategy if signature spans blocks */
	if( (m->b_wptr - m->b_rptr) >= 8 ) {
		/* signature is in fully contained in last block */
		signature = m->b_wptr - 8;
		m->b_wptr -= 8;
	} else {
		/* signature spans blocks - yuck! */
		signature = jbuf;
		j = m->b_wptr - m->b_rptr;
		m->b_wptr -= j;
		bcopy( (caddr_t)m->b_wptr, (caddr_t)(signature + (8 - j)), j );
		/* get previous block */
		m = m->b_prev;
		m->b_wptr -= (8 - j);
		bcopy( (caddr_t)m->b_wptr, (caddr_t)signature, (8 - j) );
	}


	bzero((caddr_t)block, 64);
	bcopy((caddr_t)md4->sessionKey, (caddr_t)block, 8);
	if (len == 0) {
		bcopy((caddr_t)&length, (caddr_t)block + 8, 4);
	} else {
		bcopy((caddr_t)&len, (caddr_t)(block + 8), 4);
	}


	m = ((STR_CHANNEL_T *)gipcChannel->realIpcChannel)->curDataBlock;

	while (m && r) {
		if((m->b_wptr - m->b_rptr) < a ) {
			return(NTR_LEAVE(FALSE));
		}
		count = min(((m->b_wptr - m->b_rptr) - a), r);
		bcopy((caddr_t)(m->b_rptr + a), (caddr_t)(block + 12 + b), count);
		b += count;
		r -= count;
		a = 0;
		m = m->b_cont;
	}

	bcopy((caddr_t)md4->currentMessageDigest, (caddr_t)messageDigest, 16);
	BuildMessageDigest(messageDigest, block);

	p = (unsigned char *)&messageDigest[0];
	for (i = 0; i < 8; i++) {
		if (signature[i] != *p++)
			break;
	}
	if (i != 8)
		return(NTR_LEAVE(FALSE));

	return(NTR_LEAVE(TRUE));
}
