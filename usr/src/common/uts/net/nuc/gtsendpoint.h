/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gtsendpoint.h	1.8"
#ifndef _NET_NUC_GTS_GTSENDPOINT_H
#define _NET_NUC_GTS_GTSENDPOINT_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gtsendpoint.h,v 2.51.2.1 1994/12/12 01:22:25 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    gtsendpoint.h -	The NUC Generic Transport Service layer
 *			gtsEndPoint object definitions.  Component of the NUC
 *			Core Services Provider Device.
 *
 *  ABSTRACT:
 *    The gtsendpoint.h is included with Generic Transport Service layer
 *    functions to define the gtsEndPoint focal object.
 *
 */

/*
 * GTS EndPoint State
 */
#define	GTS_ENDPOINT_BOGUS	0x00	/* ASSERT(PANIC) if present	*/
#define	GTS_ENDPOINT_IDLE	0x01	/* Open End Point		*/
#define	GTS_ENDPOINT_CONNECTED	0x02	/* Connected EndPoint		*/
#define	GTS_ENDPOINT_LISTENING	0x03	/* Listening for a Connection	*/

/*
 * GTS EndPoint Object Type
 */
#define	GENERIC_GTS_ENDPOINT	0xAE	/* Object Tag A E(ndPoint)	*/

/*
 * NAME
 *	gtsEndPoint - The Generic Transport Service EndPoint Object
 *
 * DESCRIPTION
 *	This data structure defines the Generic Transport Service EndPoint
 *	object, which is the focal object of the GTS layer itself.
 *	This object defines the context of a GTS EndPoint which is active.  It
 *	is paired with a dependent GTS EndPoint object managed by the real
 *	Transport Service the EndPoint is associated with  to form the basis
 *	of a GTS EndPoint to communicate with a peer process over the transport.
 *
 * MEMBERS:
 *	realTsOps	- List of dependent TS Operations
 *			  (i.e. NWgtsOpsSw[tsName]).
 *	realEndPoint	- The dependent TS endpoint component.
 * NOTE
 *	The most popular real Transport is NOVELL_IPX.
 */
typedef	struct	{
	GTS_OPS_T	*realTsOps;
	opaque_t	*realEndPoint;
}GTS_ENDPOINT_T;

#endif /* _NET_NUC_GTS_GTSENDPOINT_H */
