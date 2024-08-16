/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gipccommon.h	1.8"
#ifndef _NET_NUC_GIPC_GIPCCOMMON_H
#define _NET_NUC_GIPC_GIPCCOMMON_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gipccommon.h,v 2.51.2.1 1994/12/12 01:21:48 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    gipccommon.h -	The NUC Generic Inter Process Communication layer common
 *			definitions.  Component of the NUC Core Services
 *			Provider Device.
 *
 *  ABSTRACT:
 *    The gipccommon.h is included with Generic Transport Service engine
 *    functions, the generic inter-process communication layer, and the 
 *    specific IPC service packages.  This provides a consistent semantic
 *    representation of interface information used between these layers.
 *
 */

/*
 * Generic Inter Process Communication common manifest constants. 
 */

/*
 * Real IPC Mechanisms (Used with NWpcOpenIpcChannel(3K) operation).
 */
#define	BSD_SOCKETS	0x00	/* BSD SOCKETS IPC		*/
#define	RITCHIE_STREAMS	0x01	/* Ritchie's STREAMS IPC	*/

/*
 * GIPC Initialization Requests (Used with NWpcInitializeIpcService(3K) 
 * operation).
 */
#define	GIPC_START	0x00	/* Make the GIPC ready for service	*/
#define	GIPC_STOP	0x01	/* Stop GIPC, and prevent service	*/

/*
 * GIPC Event Modes (Used with NWpcOpenIpcChannel(3K) operation, and affects,
 * NWpcReceiveIpcMessage(3K) and NWpcSendIpcMessage(3K) operations).
 */
#define	GIPC_SYNC	0x00	/* Synchronous (Blocked) mode		*/
#define	GIPC_ASYNC	0x01	/* Asyncrhonous (Non-Blocked) mode	*/

/*
 * GIPC Message Types (Used with NWpcPreviewIpcMessage(3K),
 * NWpcReceiveIpcMessage(3K), NWpcRegisterAsyncEventHandler(3K),
 * and NWpcSendIpcMessage(3K) operations).
 */
#define	GIPC_NORMAL_MSG	0x01	/* In-Band Message	*/
#define	GIPC_HIPRI_MSG	0x02	/* Out-of-Band Message	*/

/*
 * GIPC Message Flush Types (Used with NWpcFlushIpcMessage(3K) operation).
 */
#define	GIPC_FLUSH_RALL		0x01L	/* Flush all messages on read queue	*/
#define	GIPC_FLUSH_RHEAD	0x02L	/* Flush message at head of read queue	*/
#define	GIPC_FLUSH_WALL		0x04L	/* Flush all messages on write queue	*/
#define	GIPC_FLUSH_WHEAD	0x08L	/* Flush message at head of write queue	*/

/*
 * NAME
 *    NWgipcOps - The Generic Inter Process Communications operations list.
 * 
 * DESCRIPTION
 *    This data structure lists the Generic Inter Process Communication object
 *    operation handlers which service requests on Generic Inter Process
 *    Communication.  This is the object oriented public interface to a Generic
 *    Inter Process Communication Channel object.
 *
 */
typedef struct {
	ccode_t	(*CloseIpcChannel)(opaque_t *ipcChannel, int32 *diagnostic);

	ccode_t	(*FlushIpcMessage)(opaque_t *ipcChannel, int32 flushType,
			int32 *diagnostic);

	ccode_t	(*FreeIpcMessage)(opaque_t *ipcChannel, opaque_t *dupMessage,
			int32 *diagnostic);

	ccode_t	(*InitializeIpcService)(uint32 requestType, int32 *diagnostic);

	ccode_t	(*OpenIpcChannel)(uint32 myIpcName, char *peerProcessName, 
			uint32 eventMode, opaque_t **ipcChannel,
			int32 *diagnostic);

	ccode_t	(*PreviewIpcMessage)(opaque_t *ipcChannel,
			NUC_IOBUF_T *controlMsg, NUC_IOBUF_T *dataMsg,
			uint32 *msgType, uint32 rewind, int32 *diagnostic,
			int32 *residualBlocks);

	ccode_t	(*PrivateIoctlMessage)(opaque_t *ipcChannel,
			opaque_t *privateCmd, int32 *privateCmdSize,
			opaque_t *privateMsg, int32 *privateMsgSize,
			int32 *diagnostic);

	ccode_t	(*ReceiveIpcMessage)(opaque_t *ipcChannel,
			NUC_IOBUF_T *controlMsg, NUC_IOBUF_T *dataFragment1,
			NUC_IOBUF_T *dataFragment2, uint32 *msgType,
			int32 *diagnostic, int32 *residualBytes);

	ccode_t	(*RegisterAsyncEventHandler)(opaque_t *ipcChannel,
			opaque_t (*asyncFunction)(), opaque_t *callerHandle,
			int32 *diagnostic);

	ccode_t	(*ReSendIpcMessage)(opaque_t *ipcChannel, opaque_t *dupMessage,
			int32 *diagnostic);

	ccode_t	(*SendIpcMessage)(opaque_t *ipcChannel, NUC_IOBUF_T *controlMsg,
			NUC_IOBUF_T *dataFragment1, NUC_IOBUF_T *dataFragment2,
			uint32 msgType, opaque_t **dupMessage,int32 *diagnostic,
			uint8 *signature);

	ccode_t	(*SizeOfIpcMessage)(opaque_t *ipcChannel,
			int32 *controlMsgBlocks, int32 *controlMsgBytes, 
			int32 *dataMsgBlocks, int32 *dataMsgBytes,
			int32 *diagnostic);
} GIPC_OPS_T;

#endif /* _NET_NUC_GIPC_GIPCCOMMON_H */
