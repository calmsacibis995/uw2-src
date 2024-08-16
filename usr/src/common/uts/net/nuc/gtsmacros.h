/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gtsmacros.h	1.12"
#ifndef _NET_NUC_GTS_GTSMACROS_H
#define _NET_NUC_GTS_GTSMACROS_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gtsmacros.h,v 2.52.2.2 1994/12/16 18:58:32 ram Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    gtsmacros.h -	The NUC Macros for the GTS abstraction.  Component of
 *			the NUC Kernel Requestor.
 *
 *  ABSTRACT:
 *    The gtsmacros.h provides the macro routine definitions used to call GTS
 *    Switch NWgtsOpsSw[] functions.  Refer to the old NWgtsMgmt.info for a
 *    description of these facilities.  The complete integrated man pages were
 *    not back ported from UW2.1 semnatically optimized version.
 *
 * 	The following NWgtsOpsSw[] operations are accessed via macros
 *	contained in this module.
 *
 *	GTS_ACCEPT()		* Abandoned functionality	*
 *	GTS_CLOSE()
 *	GTS_DISCONNECT()
 *	GTS_CONFIG()		* Abandoned functionality	*
 *	GTS_STATE()		* Abandoned functionality	*
 *	GTS_INIT()
 *	GTS_LISTEN()		* Abandoned functionality	*
 *	GTS_OPEN()
 *	GTS_PREVIEW()
 *	GTS_RECEIVE()
 *	GTS_REGISTER()
 *	GTS_REJECT()		* Abandoned functionality	*
 *	GTS_SEND()
 *
 *	NOTE:
 *		The abandoned functionality was never consumed in NUC, thus
 *		it is being dropped.  In UW 2.0 the MACROs return not supported,
 *		and in UW 2.1 the GTS has been semantically optimized, where
 *		these functions and others have been completely eliminated.
 */

#ifdef NOT_USED
/*
 * Accept Connection on a TS EndPoint
 */
#define	GTS_ACCEPT(LENDPOINT, RENDPOINT, DIAGNOSTIC) \
	*DIAGNOSTIC = NWD_GTS_NO_RESOURCE
#endif /* NOT_USED */

/*
 * Close a TS EndPoint
 */
#define	GTS_CLOSE(ENDPOINT, DIAGNOSTIC) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->CloseTransportEndPoint) \
		(ENDPOINT, DIAGNOSTIC)

/*
 * Connect a TS EndPoint
 */
#define	GTS_CONNECT(ENDPOINT, PADDRESS, DIAGNOSTIC) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->ConnectToPeer) \
		(ENDPOINT, PADDRESS, DIAGNOSTIC)

/*
 * Disconnect a TS EndPoint
 */
#define	GTS_DISCONNECT(ENDPOINT, DIAGNOSTIC) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->DisConnectFromPeer) \
		(ENDPOINT, DIAGNOSTIC)

#ifdef NOT_USED
/*
 * Get TS Configuration Information
 */
#define	GTS_CONFIG(CONFIG) \
	*DIAGNOSTIC = NWD_GTS_NO_RESOURCE

/*
 * Get TS EndPoint State
 */
#define	GTS_STATE(ENDPOINT, STATE, DIAGNOSTIC) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->GetEndPointState) \
		(ENDPOINT, STATE, DIAGNOSTIC)
#endif /* NOT_USED */

/*
 * Initialize TS Engine
 */
#define	GTS_INIT(REQUEST) \
{ \
	int	i; \
	pl_t	pl; \
	pl = RW_WRLOCK (nucTSLock, plstr); \
	for ( i=0; i < MAX_TP_STACKS; i++) { \
		if ( NWgtsOpsSw[i] ) { \
			(*NWgtsOpsSw[i]->InitializeTransportService)(REQUEST); \
		} \
	} \
	RW_UNLOCK (nucTSLock, pl); \
}

#ifdef NOT_USED
/*
 * Listen on a TS EndPoint
 */
#define	GTS_LISTEN(ENDPOINT, DIAGNOSTIC) \
	*DIAGNOSTIC = NWD_GTS_NO_RESOURCE
#endif /* NOT_USED */

/*
 * Open a TS EndPoint
 */
#define	GTS_OPEN(TSNAME, SMODE, LADDRESS, EMODE, CLIENT, ENDPOINT, DIAGNOSTIC) \
	if ( (TSNAME < MAX_TP_STACKS) && NWgtsOpsSw[TSNAME] ) { \
		(*NWgtsOpsSw[TSNAME]->OpenTransportEndPoint) \
			(SMODE, LADDRESS, EMODE, CLIENT, ENDPOINT, DIAGNOSTIC); \
	} else { \
		*DIAGNOSTIC = NWD_GTS_NO_STACK; \
	}

/*
 * Preview Message on TS EndPoint
 */
#define	GTS_PREVIEW(ENDPOINT, FRAG1, FRAG2, MTYPE, DIAGNOSTIC) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->PreviewMessageFromPeer) \
		(ENDPOINT, FRAG1, FRAG2, MTYPE, DIAGNOSTIC)

/*
 * Receive Message on TS EndPoint
 */
#define	GTS_RECEIVE(ENDPOINT, FRAG1, FRAG2, MTYPE, IS_IT_PB, DIAG) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->ReceiveMessageFromPeer) \
		(ENDPOINT, FRAG1, FRAG2, MTYPE, IS_IT_PB, DIAG)

/*
 * Register a Asyncrhonous call back event handler for TS EndPoint
 */
#define	GTS_REGISTER(ENDPOINT, FUNCTION, HANDLE, DIAGNOSTIC) \
     (*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->RegisterAsyncEventHandler) \
		(ENDPOINT, FUNCTION, HANDLE, DIAGNOSTIC)

#ifdef NOT_USED
/*
 * Reject a Connection on a TS EndPoint
 */
#define	GTS_REJECT(ENDPOINT, DIAGNOSTIC) \
	*DIAGNOSTIC = NWD_GTS_NO_RESOURCE
#endif /* NOT_USED */

/*
 * Send Message on TS EndPoint
 */
#define	GTS_SEND(ENDPOINT, FRAG1, FRAG2, CKSUM, MTYPE, DIAG, SIGN) \
	(*(((GTS_ENDPOINT_T *)(ENDPOINT))->realTsOps)->SendMessageToPeer) \
		(ENDPOINT, FRAG1, FRAG2, CKSUM, MTYPE, DIAG, SIGN)

#endif /* _NET_NUC_GTS_GTSMACROS_H */
