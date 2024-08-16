/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxconf.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxconf.h,v 2.51.2.1 1994/12/12 01:23:05 stevbam Exp $"

#ifndef _NET_NUC_IPXENG_IPXCONF_H
#define _NET_NUC_IPXENG_IPXCONF_H

/*
 *  Netware Unix Client 
 *
 *	  MODULE:
 *	    ipxconf.h - The NUC NOVELL IPX Transport Service Engine Package
 *			configuration.  Component of the NUC Core Services
 *			Device.
 *
 *	ABSTRACT:
 *		The ipxconf.h is included with the Generic Transport Service
 *		layer to configure for NOVELL IPX Transport Stack support.
 *		It configures NOVELL IPX into the Generic Transport Service
 *		Operations Switch NWgtsOpsSw[].  The IPX Transport Service
 *		Engine interface is exactly specified as that of the Generic
 *		Transport Service Layer per the NWftsOpsSw[] specification.
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/gtscommon.h>
#else  _KERNEL_HEADERS
#include <sys/gtscommon.h>
#endif _KERNEL_HEADERS
/*
 * Request NOVELL IPX Service be configured into the Generic Transport
 * Service Operations Switch.
 */
#define	NOVELLIPX

#ifdef _SPACE_C	/* Declared in space.c */

/*
 * Declare IPX Transport Service object public operations.
 */
extern ccode_t	IPXEngCloseEndpoint();
extern ccode_t 	IPXEngConnect();
extern ccode_t	IPXEngDisconnect();
/* extern ccode_t	IPXEngGetEndpointState();	*/
extern ccode_t	IPXEngInitialize();
extern ccode_t	IPXEngOpenEndpoint();
extern ccode_t	IPXEngPreviewPacket();
extern ccode_t	IPXEngGetPacket();
extern ccode_t	IPXEngRegisterAsyncHandler();
extern ccode_t	IPXEngSendPacket();

/* 
 * Allocate and initialize NWipxOps structure.
 */
GTS_OPS_T NWipxOps = { 
	NULL,						/* AcceptPeerConnection */
	IPXEngCloseEndpoint,
	IPXEngConnect,
	IPXEngDisconnect,
	NULL,						/* Get Config Info	*/
	NULL,
	IPXEngInitialize,
	NULL,						/* Listen for connection */
	IPXEngOpenEndpoint,
	IPXEngPreviewPacket,
	IPXEngGetPacket,
	IPXEngRegisterAsyncHandler,
	NULL,						/* Reject peer connection (N/A) */
	IPXEngSendPacket
};

#else

/*
 * Reference NWipxOps structure.
 */
extern GTS_OPS_T NWipxOps;
#endif                       /* ALLOCATE_NWIPX_CONF	*/

#endif /* _NET_NUC_IPXENG_IPXCONF_H */
