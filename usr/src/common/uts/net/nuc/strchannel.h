/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/strchannel.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/strchannel.h,v 2.51.2.1 1994/12/12 01:34:51 stevbam Exp $"

#ifndef _NET_NUC_STREAMS_STRCHANNEL_H
#define _NET_NUC_STREAMS_STRCHANNEL_H

/*
 *  Netware Unix Client 
 *
 *	MODULE:
 *		strchannel.h -	The NUC STREAMS IPC Mechanism package strChannel
 *				object definitions.  Component of the NUC Core
 *				Services Provider Device.
 *
 *	ABSTRACT:
 *		The strchannel.h is included with STREAMS IPC Mechanism package 
 *		functions to define the strChannel focal object.
 *
 */

#ifdef _KERNEL_HEADERS
#include <fs/vnode.h>
#include <net/nuc/gipcchannel.h>
#else  _KERNEL_HEADERS
#include <sys/vnode.h>
#include <sys/gipcchannel.h>
#endif _KERNEL_HEADERS

/*
 * STR Channel State
 */
#define	STR_CHANNEL_BOGUS	0x00	/* ASSERT(PANIC) if present	*/
#define	STR_CHANNEL_IDLE	0x01	/* No Message(s) on read queue	*/
#define	STR_CHANNEL_SLEEPING	0x02	/* A sleep waiting on message	*/
#define	STR_CHANNEL_MESSAGE	0x03	/* Message(s) on read queue	*/
#define	STR_CHANNEL_BLOCKED	0x04	/* Peer is flow controlling	*/
#define	STR_CHANNEL_HANGUP	0x05	/* Peer is closing on us	*/
#define	STR_CHANNEL_WANT_IOCTL	0x06	/* Channel sleep for IOCTL	*/

/*
 * STREAMS Channel Object Type
 */
#define	STREAMS_CHANNEL	0xA1C	/* Object Tag A C(hannel)	*/

/*
 * NAME
 *	strChannel - The NW STREAMS Head Channel Object
 *
 * DESCRIPTION
 *	This data structure defines the NW STREAM Head Inter Process Communication
 *	Channel object, which is the focal object of the NUC STREAMS Head Package
 *	itself.  This object defines the context of a Streams  Channel which is
 *	active.  It is paired with a Generic IPC Channel object managed by the
 *	GIPC layer to form the basis of a GIPC Channel to communicate with a
 *	peer STREAMS process in the UNIX kernel.
 *
 */
typedef	struct	{
	uint32		objectType;		/* STREAMS_CHANNEL	*/
	GIPC_CHANNEL_T	gipcChannel;
	uint32		channelState;		/* See above definitions*/
	uint32		eventMode;		/* See gipccommon.h	*/
	int32		advisoryLock;		/* Sleep Lock		*/
	queue_t		*readQueue;		/* Read side of STREAM	*/
	queue_t		*writeQueue;		/* Write side of STREAM	*/
	int32		syncSemaphore;		/* Synch event semaphore*/
	opaque_t	(*callBackFunction)();	/* Async Call Back Func	*/
	opaque_t	*callBackHandle;	/* Call back handle aync*/
	mblk_t		*currentMessage;	/* Current Message	*/
	mblk_t		*curControlBlock;	/* Control Block pointer*/
	uchar		*curControlMark;	/* Control Byte pointer	*/
	mblk_t		*curDataBlock;		/* Data Block pointer	*/
	uchar		*curDataMark;		/* Data Byte pointer	*/
	mblk_t		*curPreCtlBlock;	/* Preview Ctl Block ptr*/
	mblk_t		*curPreDataBlock;	/* Preview Data Blk ptr	*/
	vnode_t		*vnode;			/* Vnode to use during NWstrCloseStream */
	rwlock_t	*channelRWLock;		/* RW lock for this channel */
	sv_t		*strChannelSV;		/* SV for this channel */
}STR_CHANNEL_T;

#endif /* _NET_NUC_STREAMS_STRCHANNEL_H */
