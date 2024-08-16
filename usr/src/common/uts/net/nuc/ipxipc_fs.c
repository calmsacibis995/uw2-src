/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxipc_fs.c	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxipc_fs.c,v 2.51.2.2 1994/12/21 02:47:10 ram Exp $"

/*
 *  Netware Unix Client
 */

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
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/requester.h>
#include <net/nuc/nuc_prototypes.h>

#include <util/debug.h>

#define NVLT_ModMask    NVLTM_ipxeng

/*
 * BEGIN_MANUAL_ENTRY(IPXEngGetIPCPacketSourceAddr(3K), \
 *			./man/kernel/ts/ipxeng/GetIPCPacketSourceAddr)
 * NAME
 *	IPXEngGetIPCPacketSourceAddr
 *
 * SYNOPSIS
 *
 *	ccode_t
 *	IPXEngGetIPCPacketSourceAddr( ipcChannel, ipxAddress, sequenceNumber )
 *
 *	opaque_t	*ipcChannel;
 *	uint8		*ipxAddress;
 *	int32		*sequenceNumber;
 *
 * INPUT
 *	ipcChannel	- Ipc channel associated with an IPX socket
 *
 * OUTPUT
 *	ipxAddress	- NET:NODE:SOCKET of the server sending NCP response.
 *	sequenceNumber	- NCP Sequence number of the response.
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
IPXEngGetIPCPacketSourceAddr (	opaque_t		*ipcChannel,
								ipxAddress_t	**ipxAddr,
								int32			*sequenceNumber )
{
	ccode_t		ccode;
	NUC_IOBUF_T	control, frag1;
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

	NVLT_ENTER (3);

	GIPC_PREVIEW (ipcChannel, &control, &frag1, &msgType, TRUE, &diagnostic,
					&residualBlocks);

	if (diagnostic) {

#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"IPXEng; GetIPCPackSourceAddr, Preview FAILED! \n");
#endif	NUC_DEBUG

		GIPC_FLUSH( ipcChannel, GIPC_FLUSH_RHEAD, &filler );
		*ipxAddr = (ipxAddress_t *)NULL;
		IPXEngMapIPCDiagnostic (&diagnostic);
		return (NVLT_LEAVE (diagnostic));
	}

	gbuf = (struct getBuf *)control.buffer;

	/*
	 *	Now that the buffers have been mapped, make sure the reply is
	 *	a T_UNITDATA_IND
	 */
	switch (gbuf->stBuf.PRIM_type) {
	case T_UNITDATA_IND:

		*ipxAddr = &(gbuf->ipxAddress);
		*sequenceNumber = ((NCP_REPLY_HEADER_T *)
			frag1.buffer)->sequenceNumber;

		/*
		 * Trap out NCP_SERVER_BUSY here, so that we can hide this
		 * behaviour, and let our retransmission strategy account
		 * for this problem.
		 */

		if (((NCP_REPLY_HEADER_T *)frag1.buffer)->replyType ==
				NCP_SERVER_BUSY) {

#ifdef NUC_DEBUG
			IPXENG_CMN_ERR(CE_CONT, "IPXEng: Server busy received! \n");
#endif NUC_DEBUG

			ccode = NWD_GTS_BLOCKING;
		} else if (((NCP_REPLY_HEADER_T *)
				frag1.buffer)->connectionStatus & (NCP_LOST_CONNECTION)) {
			/*
			 * Reply indicates that the connection is no longer
			 * valid.
			 */
#ifdef NUC_DEBUG
			IPXENG_CMN_ERR (CE_CONT,
					"IPXEng: Connection not valid received! \n");
#endif NUC_DEBUG

			ccode = NWD_GTS_TIMED_OUT;
		} else {
			/*
			 * Packet Looks Good
			 */
			ccode = SUCCESS;
		}
		break;

	case T_UDERROR_IND:

#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"ipxEng: T_UDERROR_IND tpi prim received in packet\n");
#endif	NUC_DEBUG

		*ipxAddr = &(((T_UD_ERROR_HDR_T *) control.buffer)->ipxAddress);
		ccode = NWD_GTS_BAD_ADDRESS;
		break;

	default:

#ifdef	NUC_DEBUG
		IPXENG_CMN_ERR(CE_CONT, 
			"ipxEng: Unknown tpi prim = %d(dec) received in packet\n");
#endif	NUC_DEBUG

		*ipxAddr = (ipxAddress_t *)NULL;
		ccode = NWD_GTS_NO_MESSAGE;
		break;
	}

	return (NVLT_LEAVE (ccode));
}

