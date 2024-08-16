/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengcommon.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengcommon.h,v 2.51.2.1 1994/12/12 01:23:12 stevbam Exp $"

#ifndef _NET_NUC_IPXENG_IPXENGCOMMON_H
#define _NET_NUC_IPXENG_IPXENGCOMMON_H

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *		ipxengcommon.h - Common definitions for the IPX ENGINE.
 *
 *  ABSTRACT:
 *		ipxengcommon.h contains structures defined by the IPX Engine and
 *		exported for use by the NUCDIAGD daemon that implements the
 *		diagnostic socket.
 *
 */

/*
	IPXENGSTATS is a structure containing various statistics maintained 
	by IPXENG and made available to user-space programs via an ioctl to
	NWMP.

		requestCount - count of requests made since the NUC was started.
		operatorAbortCount - count of connections closed (always zero
			since the operator is not consulted for error recovery).
		operatorRetryCount - count of number of operator retries (always
			zero since the operator is not consulted for error recovery).
		timeoutCount - count of connections closed due to timeout.
		writeErrorCount - count of errors trying to send the packet 
			(always equals timeoutCount).
		invalidReplyHeaderCount - count of server response packets
			not labelled 0x3333.
		invalidSlotCount -  count of server responses containing an
			incorrect connection number.
		invalidSequenceNumber - count of server response packets 
			received with an invalid sequence number.
		errorReceivingCount - count of overruns (always zero).
		noRouterFoundCount - count of NO ROUTE AVAILABLE errors.
		beingProcessedCount - count of 0x9999 packets received.
		unknownErrorCount - count of packets received with an undefined
			error value in the communications error byte.
		invalidServerSlotCount - count of packets received indicating
			the server slot was invalid, usually occuring when the
			server is going down or if the station's connection was
			cleared at the system console.
		allocateCannotFindRouteCount - count of times a CREATE CONNECTION
			request was failed because a route could not be found.
		allocateNoSlotsAvailCount - count of times a CREATE CONNECTION
			request was failed because the server was out of
			connections.
		allocateServerIsDownCount - count of times a CREATE CONNECTION
			request was made but the file server was down.

*/
typedef struct ipxengstats {
	uint32	requestCount;
	uint32	operatorAbortCount;
	uint32	operatorRetryCount;
	uint32	timeoutCount;
	uint32	writeErrorCount;
	uint32	invalidReplyHeaderCount;
	uint32	invalidSlotCount;
	uint32	invalidSequenceNumberCount;
	uint32	errorReceivingCount;
	uint32	noRouterFoundCount;
	uint32	beingProcessedCount;
	uint32	unknownErrorCount;
	uint32	invalidServerSlotCount;
	uint32	allocateCannotFindRouteCount;
	uint32	allocateNoSlotsAvailCount;
	uint32	allocateServerIsDownCount;
} IPXENG_STATS_T; 

extern IPXENG_STATS_T ipxengStats;

#endif /* _NET_NUC_IPXENG_IPXENGCOMMON_H */
