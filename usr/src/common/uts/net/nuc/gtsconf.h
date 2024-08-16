/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gtsconf.h	1.9"
#ifndef _NET_NUC_GTS_GTSCONF_H
#define _NET_NUC_GTS_GTSCONF_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gtsconf.h,v 2.51.2.1 1994/12/12 01:22:17 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    gtsconf.h - The NUC Generic Transport Service layer configuration
 *                definitions.  Component of the NUC Core Services
 *                Provider Device.
 *
 *  ABSTRACT:
 *    The gtsconf.h is included with Generic Transport Service layer
 *    functions to configure private data structures and objects within 
 *    this layer.  
 *
 */

/*
 * The Release Level of the NUC GTS Layer
 */
#define	NWTS_MAJOR_VERSION	3
#define	NWTS_MINOR_VERSION	11

/*
 * Generic Transport Service private constants
 */
#define	MAX_TP_STACKS	8	/* Number of TS Engines in GTS Switch */

/*
 * GTS Layer States 
 *
 * The GTS States start in GTS_DOWN -> GTS_UP -> GTS_GOING_DOWN cycle,
 * where the GTS_GOING_DOWN is when the layer has been requested to stop,
 * and is waiting for all open end points to close.
 */
#define	GTS_DOWN	0	/* Completely down		*/
#define	GTS_UP		1	/* Completely up		*/
#define	GTS_GOING_DOWN	2	/* Closing out before down	*/

/*
 * NAME
 *	NWgtsTune -	The tuneable parameters of the Generic Transport
 *			Service Layer.
 *
 * DESCRIPTION
 *	This data structure defines the Tuneable Parameters of the Generic
 *	Transport Service Layer.  This structure is initialized with the
 *	tuneable parameters found in "gts_tune.h", which can be modified
 *	by the system administrator when a new kernel is built.  Once
 *	initialized, this structure conveys the tune of the GTS for the
 *	active UNIX kernel.
 */
typedef	struct	{
	uint32	servers;		/* Concurrent NetWare Servers	   */
	uint32	virtualClients;		/* Core Service Concurrent Clients */
	uint32	threadsPerClient;	/* Thread Per Client		   */
	uint32	memSize;		/* NUC GTS Layer Region Size	   */
}GTS_TUNE_T;

/*
 * Configure the Specific Transport Engine Layer with packages
 */
/*
	#include <nuc/sys/udpconf.h>
	#include <nuc/sys/tcpconf.h>
	#include <nuc/sys/ddpconf.h>
	#include <nuc/sys/dcpconf.h>
	#include <nuc/sys/ipxconf.h>
	#include <nuc/sys/spxconf.h>
	#include <nuc/sys/cltpconf.h>
	#include <nuc/sys/cotpconf.h>
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/ipxconf.h>
#include <net/nuc/gtscommon.h>
#else _KERNEL_HEADERS
#include <sys/ipxconf.h>
#endif _KERNEL_HEADERS

#ifdef ALLOCATE_GTS_CONF     /* Declared in NWtsSpace.c */



/* 
 * Allocate and initialize NWgtsOpsSw[] Generic Transport Service Operations
 * Switch structure.
 */
GTS_OPS_T *NWgtsOpsSw[MAX_TP_STACKS] = { 

#ifdef	ARPAUDP
	&NWudpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* ARPAUDP */

#ifdef	ARPATCP
	&NWtcpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* ARPATCP */

#ifdef	DECDDP
	&NWddpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* DECDDP */

#ifdef	DECDCP
	&NWdcpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* DECDCP */

#ifdef	NOVELLIPX
	&NWipxOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* NOVELLIPX */

#ifdef	NOVELLSPX
	&NWspxOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* NOVELLSPX */

#ifdef	OSICLTP
	&NWcltpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* OSICLTP */

#ifdef	OSICOTP
	&NWcotpOps,
#else
	(GTS_OPS_T *) NULL,
#endif	/* OSICOTP */
	
};

#else

/*
 * Reference NWgtsOpsSw[] Generic Transport Service Operations Switch structure.
 *
 * This array of structures is loaded with the configured Dependent Transport
 * Service package operations.  Thus to use one, it must be called indirectly
 * through NWgtsOpsSw[], which provides an object orented interface.
 *
 * Example:
 *   (*NWgtsOpsSw[IPX_PACKAGE].OpenTransportEndPoint)()
 *
 *   which calls the IPX Transport Service Open Operation.
 */

extern GTS_OPS_T *NWgtsOpsSw[];

#endif                       /* ALLOCATE_NWGTS_CONF                  */

#endif /* _NET_NUC_GTS_GTSCONF_H */
