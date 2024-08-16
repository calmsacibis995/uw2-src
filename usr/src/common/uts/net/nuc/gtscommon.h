/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gtscommon.h	1.10"
#ifndef _NET_NUC_GTS_GTSCOMMON_H
#define _NET_NUC_GTS_GTSCOMMON_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gtscommon.h,v 2.51.2.2 1994/12/16 18:58:20 ram Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    gtscommon.h - The NUC Generic Transport Service layer common
 *                  definitions.  Component of the NUC Core Services
 *                  Provider Device.
 *
 *  ABSTRACT:
 *    The gtscommon.h is included with distributed service protocol engine
 *    functions, the generic transport service layer, and the specific
 *    transport service packages.  This provides a consistent semantic 
 *    representation of interface information used between these layers.
 *
 */

/*
 * Generic Transport Service common constants. 
 */

/*
 * Real Transport Stacks (Used with NWgtsOps.OpenTransportEndPoint)
 */
#ifdef GARY_WE_CANNOT_USE_THESE_YET

#define	ARPA_UDP	0x01	/* ARPA UDP Stack	*/
#define	ARPA_TCP	0x02	/* ARPA TCP Stack	*/
#define	DEC_DDP		0x04	/* DECNET DDP Stack	*/
#define	DEC_DCP		0x08	/* DECNET DCP Stack	*/
#define	NOVELL_IPX	0x10	/* NOVELL IPX Stack	*/
#define	NOVELL_SPX	0x20	/* NOVELL SPX Stack	*/
#define	OSI_CLTP	0x40	/* OSI ISO-8602	Stack	*/	
#define	OSI_COTP	0x80	/* OSI ISO-8073 Stack	*/

#endif GARY_WE_CANNOT_USE_THESE_YET


#define	ARPA_UDP	0	/* ARPA UDP Stack	*/
#define	ARPA_TCP	1	/* ARPA TCP Stack	*/
#define	DEC_DDP		2	/* DECNET DDP Stack	*/
#define	DEC_DCP		3	/* DECNET DCP Stack	*/
#define	NOVELL_IPX	4	/* NOVELL IPX Stack	*/
#define	NOVELL_SPX	5	/* NOVELL SPX Stack	*/
#define	OSI_CLTP	6	/* OSI ISO-8602	Stack	*/	
#define	OSI_COTP	7	/* OSI ISO-8073 Stack	*/
/*
 * Indexes of real transport stack packages into NWgipcOpsSw[].
 */
#define	UDP_PACKAGE	0	/* ARPA UDP Stack	*/
#define	TCP_PACKAGE	1	/* ARPA TCP Stack	*/
#define	DDP_PACKAGE	2	/* DECNET DDP Stack	*/
#define	DCP_PACKAGE	3	/* DECNET DCP Stack	*/
#define	IPX_PACKAGE	4	/* NOVELL IPX Stack	*/
#define	SPX_PACKAGE	5	/* NOVELL SPX Stack	*/
#define	CLTP_PACKAGE	6	/* OSI ISO-8602 Stack	*/
#define	COTP_PACKAGE	7	/* OSI ISO-8073 Stack	*/

/*
 * GTS Service Modes (Used with NWgtsOps.OpenTransportEndPoint)
 */
#define	GTS_REQUESTOR	0x00	/* A Requestor		*/
#define	GTS_RESPONDER	0x01	/* A Responder		*/
#define	GTS_ANONYMOUS	0x02	/* A Anonymous Listener	*/

/*
 * GTS Event Modes (Used with NWtsListenForPeerConnection(3K) and 
 * NWtsReceiveMessageFromPeer(3K) operations.
 */
#define	GTS_SYNC	0x00	/* Synchronous (Blocked) mode		*/
#define	GTS_ASYNC	0x01	/* Asyncrhonous (Non-Blocked) mode	*/

/*
 * GTS Message Types
 */
#define	GTS_IN_BAND	0x01	/* In-Band Message	*/
#define	GTS_OUT_OF_BAND	0x02	/* Out-of-Band Message	*/

/*
 * GTS Initialization Requests
 */
#define	GTS_START	0x00	/* Startup the GTS	*/
#define	GTS_STOP	0x01	/* Shutdown the GTS	*/

/*
 * NAME
 *    NWgtsOps - The Generic Transport Service operations list.
 * 
 * DESCRIPTION
 *    This data structure lists the Generic Transport Service object operation 
 *    handlers which service requests on Generic Transport Services.  This
 *    is the object oriented public interface to a Generic Transport EndPoint
 *    object.
 */
typedef struct {
	void_t	*AcceptPeerConnectionAbadoned;

	ccode_t	(*CloseTransportEndPoint)(opaque_t *endPoint,int32 *diagnostic);

	ccode_t	(*ConnectToPeer)(opaque_t *endPoint, opaque_t *remoteAddress,
			int32 *diagnostic);

	ccode_t	(*DisConnectFromPeer)(opaque_t *endPoint, int32 *diagnostic);

	void_t	*GetConfigurationInfoAbadoned;

	void_t	*GetEndPointStateAbandoned;

	ccode_t	(*InitializeTransportService)(uint32 mode);

	void_t	*ListenForPeerConnectionAbandoned;

	ccode_t	(*OpenTransportEndPoint)(uint32 serviceMode, opaque_t *laddress,
			uint32 eventMode, opaque_t *credendital,
			opaque_t **endPoint, int32 *diagnositc);

	ccode_t	(*PreviewMessageFromPeer)(opaque_t *endPoint,NUC_IOBUF_T *frag1,
			NUC_IOBUF_T *frag2, uint32 *msgType, int32 *diagnostic);

	ccode_t	(*ReceiveMessageFromPeer)(opaque_t *endPoint,NUC_IOBUF_T *frag1,
			NUC_IOBUF_T *frag2, uint32 *msgType, boolean_t is_it_pb,
			int32 *diagnostic);

	ccode_t	(*RegisterAsyncEventHandler)(opaque_t *endPoint, 
			opaque_t (*callBackFunc)(), opaque_t *callBackHandle,
			int32 *diagnostic);

	void_t	*RejectPeerConnectionAbandoned;

	ccode_t	(*SendMessageToPeer)(opaque_t *endPoint, NUC_IOBUF_T *frag1,
			NUC_IOBUF_T *frag2, uint32 cksum, uint32 msgType, int32 *diagnostic,
			uint8 *signature);
} GTS_OPS_T;

/*
 * GTS Service Interface Structures
 */

/*
 * GTS Configuration Interface Structure, used on NWtsGetConfigurationInfo(3K)
 * operations.
 */
typedef	struct	{
	int32	numRealStacks;	/* Number gen'ed into NWgtsOpsSw[]	*/
	uint32	realStackMask;	/* Inclusive OR of gen'ed TS Packages	*/
}GTS_CONFIG_T;

/*
 * GTS End Point State Interface Structure, used on NWtsGetEndPointState(3K)
 * operations.
 */
typedef	struct	{
	uint32	realStack;	/* Real Transport Stack	*/
	uint32	serviceMode;	/* GTS Service Mode	*/
	uint32	eventMode;	/* GTS Event Mode	*/
	uint32	eventState;	/* Current State	*/
	opaque_t *localAddress;	/* TS Package Specific	*/
	opaque_t *peerAddress;	/* TS Package Specific	*/
}GTS_STATE_T;

#endif /* _NET_NUC_GTS_GTSCOMMON_H */
