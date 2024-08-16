/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/diag_app.h	1.2"
#ident	"$Id: diag_app.h,v 1.2 1994/02/18 15:12:42 vtag Exp $"
/*
 * Copyright 1991, 1992 Novell, Inc.
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

#ifndef _NET_NW_DIAG_APP_H
#define _NET_NW_DIAG_APP_H

#define DIAG_STATISTICS 0x66
/*
** The diagnostic statistics structure
** Returned as response to the Diagnostic API DIAGGetStatistics.
*/
typedef struct diag_stat 
{
	/* System Information */
	uint32   MajorVersion;		/* This is the Major Version Number */
	uint32   MinorVersion;		/* This is the Minor Version Number */
	time_t   StartTime;			/* Time started in seconds since epoch */

	/* Circuit Information */
	uint32   TotalReqs;			/* Total number of IPX/SPX requests */
	uint32   IPXSPXReqs;		/* Number of IPX/SPX requests */
	uint32   LanDvrReqs;		/* Number of Lan Driver requests */
	uint32   FileSrvReqs;		/* Number of File Server requests */
	uint32   ExtBridgeReqs;		/* Number of External Bridge requests */
	uint32   UnknownReqs;		/* Number of Unknown requests */
	uint32   SPXDiagSocket;		/* SPX Diagnostic Socket Number  */
	time_t   TimeOfLastReq;		/* Time in seconds since the last request */

} DIAGSTAT, *DIAGDSTATP;

#endif /* _NET_NW_DIAG_APP_H */

