/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gipcmacros.h	1.9"
#ifndef _NET_NUC_GIPC_GIPCMACROS_H
#define _NET_NUC_GIPC_GIPCMACROS_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gipcmacros.h,v 2.51.2.1 1994/12/12 01:22:02 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *	  MODULE:
 *		gipcmacros.h -	The NUC Macros for the GIPC abstraction. 
 *				Component of the NUC Kernel Requestor.
 *
 * 	 ABSTRACT:
 *		The gipcmacros.h provides the macro routine definitions used
 *		to call GIPC Switch NWgipcOpsSw[] functions.  Refer to the old
 *		NWgipcOps.info for a description of these facilities.  The
 *		complete integrated man pages were not back ported from UW2.1
 *		semantically optimizaed version.
 *		
 *	The following NWgipcOpsSw[] operations are accessed via macros
 *	contained in this module.
 *		GIPC_CLOSE()
 *		GIPC_FLUSH()
 *		GIPC_FREE()
 *		GIPC_INIT()
 *		GIPC_OPEN()
 *		GIPC_PREVIEW()
 *		GIPC_IOCTL()
 *		GIPC_RECEIVE()
 *		GIPC_REGISTER_CALLBACK()
 *		GIPC_RESEND()
 *		GIPC_SEND()
 *		GIPC_MSIZE()
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/gipcconf.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/gipccommon.h>
#include <sys/gipcchannel.h>
#include <sys/gipcconf.h>
#endif /* ndef _KERNEL_HEADERS */

/*
 * Close a IPC Channel
 */
#define	GIPC_CLOSE(CHANNEL, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->CloseIpcChannel) \
		(CHANNEL, DIAGNOSTIC)

/*
 * Flush messages on a IPC Channel
 */
#define	GIPC_FLUSH(CHANNEL, TYPE, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->FlushIpcMessage) \
		(CHANNEL, TYPE, DIAGNOSTIC)

/*
 * Free duplicated message 
 */
#define	GIPC_FREE(CHANNEL, DUPMSG, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->FreeIpcMessage) \
		(CHANNEL, DUPMSG, DIAGNOSTIC)

/*
 * Initialize IPC Head 
 */
#define	GIPC_INIT(REQUEST, DIAGNOSTIC) \
{ \
	int	i; \
	for ( i=0; i < MAX_IPC_MECHANISMS; i++) { \
		if ( NWgipcOpsSw[i] ) { \
			(*NWgipcOpsSw[i]->InitializeIpcService)(REQUEST, DIAGNOSTIC); \
		} \
	} \
}

/*
 * Open a IPC Channel
 */
#define	GIPC_OPEN(IPC_NAME, PEER_NAME, MODE, CHANNEL, DIAGNOSTIC) \
	if ( (IPC_NAME < MAX_IPC_MECHANISMS) && NWgipcOpsSw[IPC_NAME] ) { \
		(*NWgipcOpsSw[IPC_NAME]->OpenIpcChannel) (IPC_NAME, PEER_NAME, \
			 MODE, CHANNEL, DIAGNOSTIC); \
	} else { \
		*DIAGNOSTIC = NWD_GIPC_NO_IPC; \
	}

/*
 * Preview a message on a IPC Channel
 */
#define	GIPC_PREVIEW(CHANNEL, CONTROL, DATA, TYPE, REWIND, DIAGNOSTIC, RBLK) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->PreviewIpcMessage) \
		(CHANNEL, CONTROL, DATA, TYPE, REWIND, DIAGNOSTIC, RBLK)

#ifdef NOT_USED
/*
 * Issue a private IOCTL message
 */
#define	GIPC_IOCTL(CHANNEL, CMD, CSIZE, MSG, MSIZE, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->PrivateIoctlMessage) \
		(CHANNEL, CMD, CSIZE, MSG, MSIZE, DIAGNOSTIC)
#endif /* NOT_USED */

/*
 * Receive a message on a IPC Channel
 */
#define	GIPC_RECEIVE(CHANNEL, CONTROL, FRAG1, FRAG2, TYPE, DIAGNOSTIC, BYTES) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->ReceiveIpcMessage) \
		(CHANNEL, CONTROL, FRAG1, FRAG2, TYPE, DIAGNOSTIC, BYTES)

/*
 * Register a Asyncrhonous call back event handler for IPC Channel
 */
#define	GIPC_REGISTER_CALLBACK(CHANNEL, FUNCTION, HANDLE, DIAGNOSTIC) \
     (*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->RegisterAsyncEventHandler) \
		(CHANNEL, FUNCTION, HANDLE, DIAGNOSTIC)

/*
 * ReSend a message on a IPC Channel
 */
#define	GIPC_RESEND(CHANNEL, DUPMSG, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->ReSendIpcMessage) \
		(CHANNEL, DUPMSG, DIAGNOSTIC)

/*
 * Send a message on a IPC Channel
 */
#define	GIPC_SEND(CHANNEL, CONTROL, FRAG1, FRAG2, TYPE, DUPMSG, DIAGNOSTIC, \
		 SIGNATURE) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->SendIpcMessage) \
		(CHANNEL, CONTROL, FRAG1, FRAG2, TYPE, DUPMSG, DIAGNOSTIC, \
			 SIGNATURE)

#ifdef NOT_USED
/*
 * Return the size of a message on a IPC Channel
 */
#define	GIPC_MSIZE(CHANNEL, CBLOCKS, CBYTES, DBLOCKS, DBYTES, DIAGNOSTIC) \
	(*(((GIPC_CHANNEL_T *)(CHANNEL))->realIpcOps)->SizeOfIpcMessage) \
		(CHANNEL, CBLOCKS, CBYTES, DBLOCKS, DBYTES, DIAGNOSTIC)
#endif /* NOT_USED */

#endif /* _NET_NUC_GIPC_GIPCMACROS_H */
