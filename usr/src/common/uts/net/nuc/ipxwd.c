/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxwd.c	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxwd.c,v 2.52.2.2 1994/12/21 02:47:13 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxwd.c
 *
 *	ABSTRACT: IPXEnging watchdog handler
 *
 *	Functions declared in this module:
 *	Public functions:
 *		IPXEngWDSocketInterruptHandler
 */

#ifdef _KERNEL_HEADERS

#include <net/nuc/nwctypes.h>
#include <io/stream.h>
#include <svc/time.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcmacros.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuc_prototypes.h>

#else _KERNEL_HEADERS

#include <sys/time.h>
#include <sys/ipx_app.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nuctool.h>
#include <sys/ipxengine.h>
#include <sys/gipccommon.h>
#include <sys/gipcmacros.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/nucerror.h>

#endif _KERNEL_HEADERS

#define NTR_ModMask	NTRM_ipxeng

extern rwlock_t			*nucTSLock;
extern struct free_rtn	NCPesr;

struct watchdogStruct {
	uint8	packetSlot;
	uint8	signatureChar;
};


/*
 * BEGIN_MANUAL_ENTRY(IPXEngWDSocketInterruptHandler(3K), \
 *			./man/kernel/ts/ipxeng/WDSocketInterruptHandler)
 * NAME
 *	IPXEngWDSocketInterruptHandler - Handler for the Watchdog socket
 *
 * SYNOPSIS
 *      void_t
 *      (*ipcChannel->callBackFunction)(ipcChannel, clientHandle, msgType)
 *
 *	opaque_t	*ipcChannel;
 *	ipxClient_t	*clientHandle;
 *	uint32		msgType;
 *
 * INPUT
 *      ipcChannel      - A pointer to the GIPC Channel associated with the
 *                        Watch Dog Socket.
 *      clientHandle    - A pointer to the IPX Client Socket Set attached to
 *                        the GIPC Channel.
 *      msgType         - Set to GIPC_NORMAL_MSG, since the Novell IPX stack
 *                        does not provide OUT_OF_BAND messages on the service
 *                        socket, but on a seperate socket.  Thus a IN_BAND
 *                        message on the Watch Dog socket is in reality an
 *                        OUT_OF_BAND message.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *      The 'IPXEngWDSocketInterruptHandler' is called when a asynchronous
 *      message arrives on a watch dog socket.  This handler is registered with
 *      the GIPC layer via the NWpcRegisterAsyncEventHandler(3K) to handle all
 *      receive messages on the GIPC Channel associated with the watch dog
 *	socket for a client socket set.  This handler runs out of process
 *	context, and responds directly to the NetWare keep alive message on
 *	current context that was interrupted.
 *
 * NOTES
 *	Should only be invoked as a result of a reply to a 
 *	watchdog request initiated by the NetWare Server.
 *
 * SEE ALSO
 *      NWtsIntroduction(3K), IPXEngRegisterAsyncHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngWDSocketInterruptHandler (opaque_t *ipcChannel,
								ipxClient_t *clientHandle, uint32 msgType)
{
	ccode_t			ccode;
	struct watchdogStruct wdStruct;
	ipxAddress_t	address;
	ipxTask_t	*taskPtr;
	NUC_IOBUF_T	frag1, frag2;
	uint32		mType;
	opaque_t	*ipcMsgHandle;
	char		reply = 'Y';
	pl_t		pl;

	NTR_ENTER(3, ipcChannel, clientHandle, msgType, 0, 0);

	frag1.buffer = (opaque_t *)&wdStruct;
	frag1.bufferLength = sizeof( struct watchdogStruct );
	frag1.memoryType = IOMEM_KERNEL;
	frag2.buffer = (opaque_t *)NULL;
	frag2.bufferLength = 0;
	frag2.memoryType = IOMEM_KERNEL;

	if(IPXEngGetIPCPacket(ipcChannel,address.ipxAddr,&frag1,&frag2,&mType)) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, "IPXWdEvent; IPXEngGetIPCPack failed! \n");
#endif	/* NUC_DEBUG	*/
		IPXEngFlushCurrentIPCPacket( ipcChannel );
		return( NTR_LEAVE (0) );
	}

	pl = RW_WRLOCK (nucTSLock, plstr);

	/*
	 *	Get the task state of the connection.  Respond 'Y' if connection
	 *	still valid.
	 */
	if (IPXEngFindTask_l (clientHandle->taskList,
							&address, &taskPtr)) {
		RW_UNLOCK (nucTSLock, pl);
		/*
		 * This connection is no longer exists
		 */
		reply = 'N';
	} else {
		RW_UNLOCK (nucTSLock, pl);
		pl = LOCK (taskPtr->taskLock, plstr);
		if ( taskPtr->state & IPX_TASK_TIMEDOUT ) {
			/*
			 * Connection has been timed out by NUC, but has yet to
			 * be disconnected.  This also constitutes a connection
			 * that no longer exists.
			 */
			reply = 'N';
		}
		UNLOCK (taskPtr->taskLock, pl);
	}


	if (wdStruct.signatureChar == '?') {
		wdStruct.signatureChar = reply;	
		frag1.buffer = (opaque_t *)&wdStruct;
		frag1.bufferLength = sizeof( struct watchdogStruct );
		frag1.memoryType = IOMEM_KERNEL;
		frag1.esr = &NCPesr;
		frag2.buffer = (opaque_t *)NULL;
		frag2.bufferLength = 0;
		frag2.memoryType = IOMEM_KERNEL;
		frag2.esr = &NCPesr;

		if (IPXEngSendIPCPacket( ipcChannel, address.ipxAddr, &frag1, 
			&frag2, 0, 0, mType, &ipcMsgHandle, NULL ) 
			== SUCCESS) {
		}
	} else {
#ifdef	NUC_DEBUG
		cmn_err (CE_CONT, "Bogus watchdog \n");
#endif	/* NUC_DEBUG	*/
	}

	return( NTR_LEAVE( 0 ) );
}
