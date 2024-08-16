/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwctypes.h	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwctypes.h,v 2.51.2.1 1994/12/12 01:26:26 stevbam Exp $"

#ifndef _NET_NUC_NWCTYPES_H
#define _NET_NUC_NWCTYPES_H

/*
 *  Netware Unix Client 
 */

/*
 *	MODULE: nwctypes.h
 */

#ifdef _KERNEL_HEADERS
#include <io/stream.h>
#include <net/nw/nwportable.h>
#ifndef _NET_NUC_NUCMACHINE_H
#include <net/nuc/nucmachine.h>
#endif _NET_NUC_NUCMACHINE_H
#elif defined(_KERNEL)
#include <sys/stream.h>
#include <sys/nwportable.h>
#include <sys/nucmachine.h>
#else
#include <sys/stream.h>
#include <sys/nwportable.h>
#include <sys/nucmachine.h>
#endif	/* _KERNEL_HEADERS */


typedef int32 	version_t;
typedef int32 	proto_t;
#ifdef mips
typedef char	void_t;
#else
typedef void	void_t;
#endif
typedef void_t	opaque_t;
typedef uint8	flag8_t;
typedef int32	td_t;
typedef int32	nwmregion_t;
typedef uint32	bmask_t;
typedef int32	ccode_t;
typedef	uint8	uchar;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef SUCCESS
#define	SUCCESS	0
#endif

#ifndef FAILURE
#define	FAILURE	-1
#endif

#define SPI_LAYER_INACTIVE	0
#define SPI_LAYER_ACTIVE	1

/*
 * xautoFlags
 */
#define NUC_SINGLE_LOGIN_ONLY	0x0001

/*
 * NUC Event service routine structure.  Used for general callbacks.
 *
 * NOTE:
 *	Must be synchronized with the STREAMS free_rtn_t structure for now,
 *	it is passed directly to STREAMS routines and must be a union, but
 *	we don't want to use a STREAMS definition in generic code.
 *
 * MEMBERS:
 *	esrFunction	- Event Service Routine (Call back function)
 *			  void_t esrFunction( opaque_t *esrArgument);
 *	esrArgument	- Call back argument to pass.
 */

typedef	struct	{
	void_t		(*esrFunction)();
	opaque_t	*esrArgument;
}NUC_ESR_T;

/*
 * NUC I/O Interface Structure, used on GTS & GIPC Preview, Receive, and Send
 * Operations.
 *
 * NOTE:
 *	Now used throughout all of the NUC kernel components.
 *
 * MEMBERS:
 *	buffer		- Base address of the buffer.
 *	bufferLength	- Size in bytes of the buffer.
 *	memoryType	- Address space the buffer is in.  Set to one of the
 *			  followint:
 *				IOMEM_USER	- User address space.
 *						  (Not accepted by most of NUC)
 *				IOMEM_KERNEL	- Kernel address space.
 *				IOMEM_PHYSICAL	- Physical memory page.
 *						  (Not accepted by most of NUC)
 *				IOMEM_BYPASS	- Used on receives to 
 *						  skip bytes.
 *	esrInfo.free_func - ESR routine to call when buffer has been serviced.
 *			    Currently only used on sends.
 *	esrInfo.free_arg  - Argument to pass to the ESR routine.
 *	spare1		- Locally managed, private interpretation for extra
 *			  context information.
 *	spare2		- Locally managed, private interpretation for extra
 *			  context information.
 *
 * NOTE:
 *	The ESR call backs contain the following semantics.  A transaction
 *	contains multiple NUC_IOBUF_T fragments combined in a contiguous array
 *	fragmentList.  Each component layer in the transaction (i.e. NUCFS,
 *	NCP, IPXnts, etc) owns and manages a segment of the fragList.  The first
 *	fragment of a segment must have a ESR function which will be called
 *	by the lower layer when the fragment segment buffers are released by
 *	that layer.  Optionally all fragments in a segment may have ESR 
 *	functions, but it is the repsonsibility of the layer to call the 
 *	additonal ESR functions in the segment.
 *
 *	For examble, a standard NUCFS transaction has a fragList[5], in which
 *	fragList[2-3] are the NUCFS segment, fragList[1] is the NCP header
 *	segment and fragList[4] is the NCP signature segment, and fragList[0]
 *	is the IPXnts segment. Thus STREAMS calls back the IPXnts segment ESR
 *	in fragList[0], IPXnts calls back the NCP segment ESR in fragList[1],
 *	and NCP calls back the NUCFS segment ESR in fragList[2].  The NUCFS ESR
 *	must manage both fragList[2] and fragList[3] as members of its segment.
 *	The NUCFS call back the NCP signature segment ESR.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
typedef struct NUC_IOBUF_S {
	struct NUC_IOBUF_S*		next;
	opaque_t				*buffer;
	int32					bufferLength;
	int32					memoryType;
	frtn_t					*esr;
	frtn_t					esrStruct;
}NUC_IOBUF_T;
#endif defined(_KERNEL) || defined(_KMEMUSER)

#define	IOMEM_USER	0
#define	IOMEM_KERNEL	1
#define	IOMEM_PHYSICAL	2
#define	IOMEM_BYPASS	3

/*
	NUCS_*_TATS are structures containing various statistics maintained 
	by the specified component and made available by each component by 
	ioctl.

		requestCount - count of requests made since the NUC was started.
		operatorAbortCount - count of connections closed (always zero
			since the operator is not consulted for error recovery).
		operatorRetryCount - count of number of operator retries (always
			zero since the operator is not consulted for error recovery).
		timeoutCount - count of connections closed due to timeout.
		writeErrorCount - count of errors trying to send the packet 
			(always equals timeoutCount).
		invalidReplyHeaderCount - count of server response packets
			not labelled 0x3333.
		invalidSlotCount -  count of server responses containing an
			incorrect connection number.
		invalidSequenceNumber - count of server response packets 
			received with an invalid sequence number.
		errorReceivingCount - count of overruns (always zero).
		noRouterFoundCount - count of NO ROUTE AVAILABLE errors.
		beingProcessedCount - count of 0x9999 packets received.
		unknownErrorCount - count of packets received with an undefined
			error value in the communications error byte.
		invalidServerSlotCount - count of packets received indicating
			the server slot was invalid, usually occuring when the
			server is going down or if the station's connection was
			cleared at the system console.
		allocateCannotFindRouteCount - count of times a CREATE CONNECTION
			request was failed because a route could not be found.
		allocateNoSlotsAvailCount - count of times a CREATE CONNECTION
			request was failed because the server was out of
			connections.
		allocateServerIsDownCount - count of times a CREATE CONNECTION
			request was made but the file server was down.

*/
typedef struct {
	/*
		The following are maintained by the IPX Engine and are found
		in each ipxTask_t;
	*/
	uint32	ipxengRequestCount;
	uint16	ipxengOperatorAbortCount;
	uint16	ipxengOperatorRetryCount;
	uint16	ipxengTimeoutCount;
	uint16	ipxengWriteErrorCount;
	uint16	ipxengInvalidReplyHeaderCount;
	uint16	ipxengInvalidSlotCount;
	uint16	ipxengInvalidSequenceNumberCount;
	uint16	ipxengErrorReceivingCount;
	uint16	ipxengNoRouterFoundCount;
	uint16	ipxengBeingProcessedCount;
	uint16	ipxengUnknownErrorCount;
	uint16	ipxengInvalidServerSlotCount;
	uint16	ipxengNetworkGoneCount;
	uint16	ipxengReserved1;
	uint16	ipxengAllocateCannotFindRouteCount;
	uint16	ipxengAllocateNoSlotsAvailCount;
	uint16	ipxengAllocateServerIsDownCount;
} NUC_IPXENG_STATS_T;

typedef struct {
	/*
		The following are maintained by the IPX driver
	*/
	uint32	ipxSendPacketCount;
	uint16	ipxMalformedPacketCount;
	uint32	ipxGetEcbRequestCount;
	uint32	ipxGetEcbFailureCount;
	uint32	ipxAesEventCount;
	uint16	ipxPostponedAesEventCount;
	uint16	ipxMaxConfiguredSocketsCount;
	uint16	ipxMaxOpenSocketsCount;
	uint16	ipxOpenSocketFailureCount;
	uint32	ipxListenEcbCount;
	uint16	ipxEcbCancelFailureCount;
	uint16	ipxFindRouteFailureCount;
} NUC_IPX_STATS_T;

typedef struct {
	/*
		The following are maintained by the SPX driver
	*/
	uint16	spxMaxConnectionsCount;
	uint16	spxMaxUsedConnectionsCount;
	uint16	spxEstablishConnectionRequestCount;
	uint16	spxEstablishConnectionFailureCount;
	uint16	spxListenConnectionRequestCount;
	uint16	spxListenConnectionRequestFailureCount;
	uint32	spxSendPacketCount;
	uint32	spxWindowChokeCount;
	uint16	spxBadSendPacketCount;
	uint16	spxSendFailureCount;
	uint16	spxAbortConnectionCount;
	uint32	spxListenPacketCount;
	uint16	spxBadListenPacketCount;
	uint32	spxIncomingPacketCount;
	uint16	spxBadIncomingPacketCount;
	uint16	spxSuppressedPacketCount;
	uint16	spxNoSessionListenEcbCount;
	uint16	spxWatchdogDestroySessionCount;
} NUC_SPX_STATS_T;

typedef struct {
	/*
		The following are obtained from the network driver
	*/
	uint32	driverVersion;
	uint32	driverStatisticsVersion;
	uint32	driverTotalTxPacketCount;
	uint32	driverTotalRxPacketCount;
	uint32	driverNoEcbAvailableCount;
	uint32	driverPacketTxTooBigCount;	
	uint32	driverPacketTxTooSmallCount;	
	uint32	driverPacketRxOverflowCount;
	uint32	driverPacketRxTooBigCount;
	uint32	driverPacketRxTooSmallCount;
	uint32	driverPacketTxMiscErrorCount;
	uint32	driverPacketRxMiscErrorCount;
	uint32	driverChecksumErrorCount;
	uint32	driverHardwareRxMismatchCount;
} NUC_DRIVER_STATS_T;

typedef struct {
	uint32  userID;
	uint32  groupID;
	uint32	pid;
	uint32	flags;
} nwcred_t;

#endif /* _NET_NUC_NWCTYPES_H */
