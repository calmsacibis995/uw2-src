/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/streamsconf.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/streamsconf.h,v 2.51.2.1 1994/12/12 01:35:01 stevbam Exp $"

#ifndef _NET_NUC_STREAMS_STREAMSCONF_H
#define _NET_NUC_STREAMS_STREAMSCONF_H

/*
 *  Netware Unix Client 
 *
 *	  MODULE:
 *    streamsconf.h -	The NUC STREAMS Mechanism IPC Head Service Package
 *			configuration.  Component of the NUC Core Services
 *			Device.
 *
 *	ABSTRACT:
 *		The streamsconf.h is included with STREAMS IPC layer to 
 *		configure the NWstreamsOps operations list for registration
 *		in GIPC NWgipcOpsSw[RITCHIE_STREAMS].
 *
 */

/*
 * Request STREAMS IPC Service be configured into the Generic Inter Process
 * Communication Operations Switch.
 */
#define	STREAMSIPC

#ifdef ALLOCATE_STREAMS_CONF	/* Declared in NWstrSpace.c */

/*
 * Declare STREAMS IPC Head Service object public operations.
 */
extern	ccode_t	NWstrCloseIpcChannel();
extern	ccode_t	NWstrFlushIpcMessage();
extern	ccode_t	NWstrFreeIpcMessage();
extern	ccode_t	NWstrInitializeIpcService();
extern	ccode_t	NWstrOpenIpcChannel();
extern	ccode_t	NWstrPreviewIpcMessage();
extern	ccode_t	NWstrPrivateIoctlMessage();
extern	ccode_t	NWstrReceiveIpcMessage();
extern	ccode_t	NWstrRegisterAsyncEventHandler();
extern	ccode_t	NWstrReSendIpcMessage();
extern	ccode_t	NWstrSendIpcMessage();
extern	ccode_t	NWstrSizeOfIpcMessage();

/* 
 * Allocate and initialize NWstreamsOps structure.
 */
GIPC_OPS_T NWstreamsOps = { 
	NWstrCloseIpcChannel,
	NWstrFlushIpcMessage,
	NWstrFreeIpcMessage,
	NWstrInitializeIpcService,
	NWstrOpenIpcChannel,
	NWstrPreviewIpcMessage,
	NWstrPrivateIoctlMessage,
	NWstrReceiveIpcMessage,
	NWstrRegisterAsyncEventHandler,
	NWstrReSendIpcMessage,
	NWstrSendIpcMessage,
	NWstrSizeOfIpcMessage
};

#else

/*
 * Reference the NWstrOps[] NUC STREAMS Head Operations 
 *
 * This structure is loaded with the NUC STREAMS Head Inter Process Communication
 * Operations, which are in turn plugged in to the Generic Inter Process 
 * Communication Operations Switch NWgicpOpsSw[RITCHIE_STREAMS].  Thus to use
 * one. it must be called inderectly through NWgipcOpsSw[RITCHIE_STREAMS],
 * which provides an object oriented interface.
 *
 * Example:
 *	(*NWgipcOpsSw[RITCHIE_STREAMS].OpenIpcChannel)()
 *
 *	which calls the NUC STREAMS Head Open operation.
 */
extern	GIPC_OPS_T	NWstreamsOps;
#endif                       /* ALLOCATE_STREAMS_CONF	*/

#endif /* _NET_NUC_STREAMS_STREAMSCONF_H */
