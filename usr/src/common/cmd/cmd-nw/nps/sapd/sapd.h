/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/sapd.h	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sapd.h,v 1.5 1994/08/17 18:12:04 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include "nwmsg.h"
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/nwportable.h>
#include <sys/nwtdr.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include <sys/sap_app.h>
#include "sap_lib.h"
#include "microsleep.h"
#include "nwconfig.h"
#include "npsmsgtable.h"
#include "memmgr_types.h"
#include "nps.h"
#include "util_proto.h"

#ifndef PATH_MAX
#define PATH_MAX 255
#endif
/*********************************************************************/

#define SAPS_DEFAULT_NICE_VALUE 20
#define SAPS_MIN_PRIORITY 40
#define SAPS_MAX_PRIORITY 1

/*
 *	TimeOut declarations
 */
#define SAP_RESTART_FACTOR 2

/*
**	Put a delay between packets
*/
#define	PACKET_DELAY		-1
#define	NO_PACKET_DELAY		0

/*
**	Check for split horizions
*/
#define	CHECK_SPLIT_HORIZ	-1
#define	NO_SPLIT_HORIZ		0

/*
**	Length of Name in bytes used for hash computation
**	If you change this you need to change the GetNameHash function
*/
#define NAME_HASH_LENGTH 12

/*********************************************************************/
/*
**	DEBUG STUFF
*/

#ifdef YES
#undef YES
#endif
#ifdef NO
#undef NO
#endif
#define YES 1
#define NO	0

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#define SRVALLOC(size) SrvAlloc(size)
#define SRVFREE(ptr) SrvFree(ptr)

/*********************************************************************/

#ifdef max
#undef max
#endif
#define max(a,b)       ((a)<(b) ? (b) : (a))

typedef struct InfoSourceEntry
{
	struct InfoSourceEntry	*NextSource;
	uint8					SourceAddress[6];	/* Node Address of source */
	uint16					HopsToSource;		/* Hops to Source from SAP */
	uint32					ServTimer;		/* Age Timer, one per Broadcast Interval */
	uint8					ServConnectedLAN;	/* Lan Pkt arrived on */
	pid_t					LocalPid;
	netInfo_t				N;					/* NetInfo information */
} InfoSourceEntry_t;

typedef struct InfoPtrPtr
{
	InfoSourceEntry_t		*InfoPtr;
} InfoPtrPtr_t;


typedef struct ServPacketInfo
{
	uint16					TargetType;
	uint8					TargetServer[NWMAX_SERVER_NAME_LENGTH];
	uint8					TargetAddress[12];
	uint16					ServerHops;
} ServPacketInfo_t;

typedef struct ServerPacketStruct
{
	ipxHdr_t				ipxHdr;	
	uint16					Operation;
	/* can have multiple ServPacketInfo_t structures, DONOT use sizeof */
	ServPacketInfo_t		ServerTable[1];	
} ServerPacketStruct_t;

/*
**	Processes on local machine to notify of changes
*/
typedef struct NotifyStruct
{
	struct NotifyStruct	   *Next;		/* Forward Link - must be first item */
	pid_t					Pid;		/* Process pid */
	uint16					ServerType;	/* Server Type */	
	int						Signal;		/* Signal to Send */
} NotifyStruct_t;

typedef struct lanSapInfo
{
	uint32	gap;		/* number of microsec to delay between SAP packets */
	/* 
	** the following are base upon "sapTimerInterval" which is a 
	** multiple of 30 (SAP_TIMER_MULTIPLIER) seconds.
	*/
	uint16	bcstTimer;	/* current # of intervals for periodic broadcasts */
	uint16	maxBcst;	/* # of intervals before a periodic broadcasts */
	uint32	maxAge;		/* # of intervals before Server is Aged */
} lanSapInfo_t;
