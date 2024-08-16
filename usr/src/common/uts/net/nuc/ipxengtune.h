/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengtune.h	1.9"
#ifndef _NET_NUC_IPXENG_IPXENGTUNE_H
#define _NET_NUC_IPXENG_IPXENGTUNE_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengtune.h,v 2.51.2.1 1994/12/12 01:24:16 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *	  MODULE: ipxengtune.h
 *	ABSTRACT: Tunable parameters that are set in Space.c, and
 *		  externed in to everywhere else.
 */

/*
 * NAME
 *	ipxEngTune	- The tuneable parameters of the IPX Engine
 *
 * DESCRIPTION
 *	The following structure defines the tunable parameters of the IPX
 *	Engine sub system.  This structure is loaded by the Space.c to provide
 *	these parameters to all other sub system functions at run time.
 *	It is comprised of the following elements.
 *
 *	maxClients		- The number of virtual work stations (i.e.
 *				  UNIX UID's that can use NetWare IPX Servers
 *				  concurrently.
 *	maxClientTasks		- The number of NetWare IPX Servers a virtual
 *				  work station can be attached to concurrently.
 *	timeoutQuantumLimit	- The maximum time to persit in retransmitting
 *				  an unacknowledged NCP Request.  Granularity
 *				  in HZ ticks.
 *	minRoundTripTicks	- The minimum NCP Request/Response smoothed 
 *				  round trip to be factored into dynamic
 *				  retransmission strategy.  Granularity
 *				  in HZ ticks.
 *	minVariance		- The minimum smoothed variance between NCP 
 *				  Request/Response smoothed round trips to be
 *				  factored into dynamic retransmission strategy.
 *				  Granularity in HZ ticks.
 *	memRegionSize		- The memory region size in bytes of the IPX
 *				  dynamic memory region.  Used only in static
 *				  memory models (i.e. SVR3.2).
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengparam.h>
#else  _KERNEL_HEADERS
#include <sys/ipxengine.h>
#include <sys/ipxengparam.h>
#endif _KERNEL_HEADERS

struct ipxEngTuneStruct {
	int32	maxClients;
	int32	maxClientTasks;
	int32	timeoutQuantumLimit;	/* In HZ ticks	*/
	int32	minRoundTripTicks;	/* In HZ ticks	*/
	int32	minVariance;		/* In HZ ticks	*/
};

#ifdef _SPACE_C

/*
 *	Space.c will define the structure, and assign values to it.
 *
 *	ipxengparam.h is where the manifest constants are kept.
 *	space.c includes this file
 *
 *	The tune strucuture is initialized as follws:
 *
 *	maxClients	- Set to the manifest IPXENG_MAX_CLIENTS.
 *	maxClientTasks	- Set to the manifest IPXENG_MAX_CLIENT_TASKS.
 *	timeoutQuantumLimit	- Initialized to the manifest
 *				  IPXENG_MAX_TIMEOUT_QUANTUM_LIMIT, and then
 *				  normalized to ticks at run time.
 *	minRoundTripTicks	- Initialized to the manifest
 *				  IPXENG_MIN_ROUNDTRIP, and then normalized to
 *				  ticks at run time.
 *	minVariance		- Initialized to the manifest
 *				  IPXENG_MIN_VARIANCE, and then normalized to
 *				  ticks at run time.
 *	memRegionSize		- Set to the manifest IPXENG_MEM_REGION_SIZE.
 */
struct ipxEngTuneStruct ipxEngTune = {
	NUCUSERS,
	NUCLOGINS,
	IPXENG_MAX_TIMEOUT_QUANTUM_LIMIT,
	IPXENG_MIN_ROUNDTRIP,
	IPXENG_MIN_VARIANCE
};

/*
 *	Client and task data structures
 */

ipxClient_t	clientList[NUCUSERS] = {0};
ipxTask_t	taskTable[NUCUSERS*NUCLOGINS] = {0};

#else  _SPACE_C

/*
 *	Everybody else that includes this will see it this way
 */

extern struct ipxEngTuneStruct ipxEngTune;

extern ipxClient_t	clientList[];
extern ipxTask_t	taskTable[];

#endif _SPACE_C	 /* If space.c */

#endif /* _NET_NUC_IPXENG_IPXENGTUNE_H */
