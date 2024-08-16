/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gipcchannel.h	1.8"
#ifndef _NET_NUC_GIPC_GIPCCHANNEL_H
#define _NET_NUC_GIPC_GIPCCHANNEL_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gipcchannel.h,v 2.51.2.1 1994/12/12 01:21:38 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *	MODULE:
 *		gipcChannel.h -	The NUC Generic Inter Process Communicaton
 *		layer gipcChannel object definitions.  Component of the NUC
 *		Core Services Provider Device.
 *
 *	ABSTRACT:
 *		The gipcChannel.h is included with Generic Inter Process
 *		Communication layer functions to define the gipcChannel
 *		focal object.
 *
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/gipccommon.h>
#else  _KERNEL_HEADERS
#include <sys/gipccommon.h>
#endif _KERNEL_HEADERS

/*
 * GIPC Channel State
 */
#define	GIPC_CHANNEL_BOGUS	0x00	/* ASSERT(PANIC) if present	*/
#define	GIPC_CHANNEL_IDLE	0x01	/* No Message(s) on read queue	*/
#define	GIPC_CHANNEL_SLEEPING	0x02	/* A sleep waiting on message	*/
#define	GIPC_CHANNEL_MESSAGE	0x03	/* Message(s) on read queue	*/
#define	GIPC_CHANNEL_BLOCKED	0x04	/* Peer is flow controlling	*/

/*
 * GIPC Channel Object Type
 */
#define	GENERIC_GIPC_CHANNEL	0xAC	/* Object Tag A C(hannel)	*/

/*
 * NAME
 *	gipcChannel - The Generic Inter Process Communication Channel Object
 *
 * DESCRIPTION
 *	This data structure defines the Generic Inter Process Communication
 *	Channel object, which is the focal object of the GIPC layer itself.
 *	This object defines the context of a GIPC Channel which is active.  It
 *	is paired with a dependent IPC Channel object managed by the real IPC head
 *	the Channel is associated with  to form the basis of a GIPC Channel to
 *	communicate with a peer process in the UNIX kernel.
 *
 * NOTE
 *	The two most popular real IPC heads are BSD_SOCKETS and RITCHIE_STREAMS.
 */
typedef	struct	{
	GIPC_OPS_T	*realIpcOps;		/* NWgipcOpsSw[ipcName]		*/
	opaque_t	*realIpcChannel;	/* Dependent IPC Channel object	*/
}GIPC_CHANNEL_T;

#endif /* _NET_NUC_GIPC_GIPCCHANNEL_H */
