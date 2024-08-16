/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/gipcconf.h	1.9"
#ifndef _NET_NUC_GIPC_GIPCCONF_H
#define _NET_NUC_GIPC_GIPCCONF_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/gipcconf.h,v 2.51.2.1 1994/12/12 01:21:55 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *	MODULE:
 *		gipcconf.h -	The NUC Generic Inter Process Communicaton layer
 *				configuration definitions.  Component of the
 *				NUC Core Services Provider Device.
 *
 *	ABSTRACT:
 *		The gipcconf.h is included with Generic Inter Process
 *		Communication layer functions to configure private data
 *		structures and objects within this layer.  
 *
 */

/*
 * The Release level of the NUC GIPC Layer
 */
#define	NWPC_MAJOR_VERSION	3
#define	NWPC_MINOR_VERSION	11

/*
 * Generic Inter Process Communication private manifest constants
 */
#define	MAX_IPC_MECHANISMS	2	/* Sockets & STREAMS	*/

/*
 * GIPC Layer States
 *
 * The GIPC States start in GIPC_DOWN -> GIPC_UP -> GIPC_GOING_DOWN cycle,
 * where the GIPC_GOING_DOWN is when the layer has been requested to stop,
 * and is waiting for all open channels to close.
 */
#define	GIPC_DOWN	0	/* Completely down		*/
#define	GIPC_UP		1	/* Completely up		*/
#define	GIPC_GOING_DOWN	2	/* Closing out before down	*/


#ifdef ALLOCATE_GIPC_CONF     /* Declared in NWgipcSpace.c */

/*
 * Configure the Specific IPC Head Layer with packages
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/streamsconf.h>
#elif defined(_KERNEL)
#include <sys/streamsconf.h>
#else
#include <sys/streamsconf.h>
#endif	/* _KERNEL_HEADERS */

/* 
 * Allocate and initialize NWgipcOpsSw[] Generic Inter Process Communication
 * Operations Switch structure.
 */
GIPC_OPS_T *NWgipcOpsSw[MAX_IPC_MECHANISMS] = { 

#ifdef	SOCKETSIPC
	&NWsocketsOps,
#else
	(GIPC_OPS_T *) NULL,
#endif	/* SOCKETSIPC */

#ifdef	STREAMSIPC
	&NWstreamsOps
#else
	(GIPC_OPS_T *) NULL
#endif	/* STREAMSIPC */
};

#else ALLOCATE_GIPC_CONF

/*
 * Reference NWgipcOpsSw[] Generic Inter Process Communication Operations
 * Switch structure.
 *
 * This array of structures is loaded with the configured Dependent Inter Process
 * Communication package operations.  Thus to use one, it must be called
 * indirectly through NWgipcOpsSw[], which provides an object orented interface.
 *
 * Example:
 *   (*NWgipcOpsSw[RITCHIE_STREAMS].OpenIpcChannel)()
 *
 *   which calls the RITCHIE STREAMS IPC Service Open Operation.
 */

extern GIPC_OPS_T *NWgipcOpsSw[];

#endif                       /* ALLOCATE_GIPC_CONF                  */

#endif /* _NET_NUC_GIPC_GIPCCONF_H */
