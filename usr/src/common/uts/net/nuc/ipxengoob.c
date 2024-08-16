/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengoob.c	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengoob.c,v 2.51.2.3 1995/01/05 17:54:21 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxoob.c
 *
 *	ABSTRACT: Out-of-band socket receive handler
 *
 *	Function declared in this module:
 *		IPXEngOOBSocketInterruptHandler
 */

#ifdef _KERNEL_HEADERS
#include <io/stream.h>
#include <svc/time.h>
#include <net/tihdr.h>

#include <net/nuc/nwctypes.h>
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

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/time.h>
#include <sys/tihdr.h>
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

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ipxeng

extern rwlock_t *nucTSLock;


/*
 * BEGIN_MANUAL_ENTRY(IPXEngOOBSocketInterruptHandler(3K), \
 *			./man/kernel/ts/ipxeng/OOBAsyncHandler)
 * NAME
 *	IPXEngOOBSocketInterruptHandler - OutOfBand Socket Asynchronous Handler.
 *
 * SYNOPSIS
 *      static void_t
 *      (*ipcChannel->callBackFunction)(ipcChannel, clientHandle, msgType)
 *
 *      opaque_t        *ipcChannel;
 *      ipxClient_t     *clientHandle;
 *      uint32          msgType;
 *
 * INPUT
 *      ipcChannel      - A pointer to the GIPC Channel associated with the
 *                        Message Socket.
 *      clientHandle    - A pointer to the IPX Client Socket Set attached to
 *                        the GIPC Channel.
 *      msgType         - Set to GIPC_NORMAL_MSG, since the Novell IPX stack
 *			  does not provide OUT_OF_BAND messages on the service
 *			  socket, but on a seperate socket.  Thus a IN_BAND
 *			  message on the message socket is in reality an
 *			  OUT_OF_BAND message.
 *	
 * OUTPUT
 *	ipxEndpoint->address	- The endpoint address assoicated with
 *				  connected NetWare server who died is
 *				  changed to the new NetWare server taking
 *				  over.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'IPXEngOOBSocketInterruptHandler' is called when a asynchronous
 *	message arrives on a message socket.  This handler is registered with
 *	the GIPC layer via the NWpcRegisterAsyncEventHandler(3K) to handle all
 *	receive messages on the GIPC Channel associated with the message socket
 *	for a client socket set.  This handler runs out of process context, and
 *	must trap all SFT III redirects, but pass Message indications and 
 *	Cache Consistency indications to the NCP call back handler, if one is
 *	registered via IPXEngRegisterAsyncHnadler(3K).
 *
 * NOTES
 *	In 3.11.x we need to provide the iovec to a mapped (previewed) chain,
 *	so that the call back will inherit a previewd buffer chain.
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), IPXEngRegisterAsyncHandler(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
IPXEngOOBSocketInterruptHandler (opaque_t *ipcChannel,
								ipxClient_t *clientHandle,
								uint32 eventMsgType )
{

	ipxSocket_t	*oobs;
	ipxTask_t	*ipxConnection;
	NUC_IOBUF_T	control;
	NUC_IOBUF_T	frag1;
	int32		diagnostic, residualBlocks;
	uint32		tempInt;
	typedef	struct	{
		struct	T_unitdata_ind	tpiHeader;
		ipxAddress_t		ipxAddress;
		uint8			ipxPacketType;
	}UNIT_DATA_IND_T;
	UNIT_DATA_IND_T	*tpiUnitDataInd;

	typedef	struct	{
		uint8	connectionNumber;
		uint8	signitureChar;
	}MESSAGE_NOTICE_T;
	MESSAGE_NOTICE_T	*messageNotice;
	pl_t				pl;

	NTR_ENTER(3, ipcChannel, clientHandle, eventMsgType, 0, 0);
	
	oobs = &(clientHandle->socketSet.outOfBandSocket);

	/*
	 * Map the TPI T_UNITDATA_IND and NCP header into our visibility
	 */
	GIPC_PREVIEW (ipcChannel, &control, &frag1, &tempInt, TRUE, &diagnostic,
					&residualBlocks);
	if (diagnostic) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXE:OOBAsyncHandler called without message! \n");
#endif	/* NUC_DEBUG	*/
		/*
		 * Not sure how we got called, simply return 
		 */
		return ( NTR_LEAVE(0) );
	}

	tpiUnitDataInd = (UNIT_DATA_IND_T *) control.buffer;
	if ( tpiUnitDataInd->tpiHeader.PRIM_type != T_UNITDATA_IND ) {
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXE:OOBAsyncHandler called without T_UNITDATA_IND!\n");
#endif	/* NUC_DEBUG	*/
		/*
		 * We are only interested in valid data messages
		 */
		GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD,
			&diagnostic);

		return ( NTR_LEAVE(0) );
	}

	pl = RW_WRLOCK (nucTSLock, plstr);

	/*
	 * Find the connection associated with this NetWare Server
	 */
	if (IPXEngFindTask_l ( clientHandle->taskList,
			&(tpiUnitDataInd->ipxAddress), &ipxConnection)) {

		RW_UNLOCK (nucTSLock, pl);

		/*
		 * We don't have a connection anymore, flush the packet
		 */
#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXE:OOBAsyncHandler called on Stale Connection!\n");
#endif	/* NUC_DEBUG	*/
		GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD,
			&diagnostic);
		return ( NTR_LEAVE(0) );
	}

	RW_UNLOCK (nucTSLock, pl);

	/*
	 * Decode the event inidication
	 * 
	 * Is it a Message Notice, to pick messages on the NetWare Server?
	 */
	messageNotice = (MESSAGE_NOTICE_T *) frag1.buffer;
	if ( messageNotice->signitureChar == '!' ) {
		/*
		 * Its a message notice, call back to NCP handler if it
		 * is regstered.
		 */
		if ( ipxConnection->asyncHandler ) {
			(*ipxConnection->asyncHandler)(ipxConnection,
				ipxConnection->asyncContextHandle,
				eventMsgType );
			GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &diagnostic);
		} else {
			/*
			 * No way to notify NCP of the event, flush the packet,
			 * and let NCP discover the message on a future reply.
			 */
			GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &diagnostic);
		}
		return ( NTR_LEAVE ( 0 ) );
	}

	return (NTR_LEAVE ( 0 ));

}
