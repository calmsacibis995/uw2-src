/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/diagd/nwdiagd.h	1.4"
#ident	"$Id: nwdiagd.h,v 1.6 1994/06/13 15:01:01 vtag Exp $"

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

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <tiuser.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stropts.h>

#include "sys/nwportable.h"
#include "sys/nwtdr.h"
#include "sys/ipx_app.h"
#include "nps.h"
#include "sys/ripx_app.h"
/* #include "sys/ncp_app.h" */
#include "sys/ipxspx_diag.h"
#include "nwconfig.h"
#include "nwmsg.h"
#include "npsmsgtable.h"
#include "memmgr_types.h"
#include "sys/diag_app.h"

#define SPX_SOCKETS		"spx_max_sockets"
#define IPX_SOCKETS		"ipx_max_sockets"

#define IPX_DIAG_FD			0	/* IPX */
#define LIPMX_DIAG_FD		1	/* IPX */
#define RIPX_DIAG_FD		2	/* IPX */
#define NUM_IPX_FD			3

#define IPX_QUERY_SAPD_FD	3	/* Requires SPX */
#define IPX_COUNT_PKTS_FD	4	/* Requires SPX */
#define IPX_SND_PKTS_FD		5	/* Requires SPX */
#define SPX_DIAG_FD			6	/* SPX */
#define NUM_SPX_FD			4

#define NUM_DIAG_FD			7

/*********************************************************************/

#define SAPS_SOCK_HIGH 0x04
#define SAPS_SOCK_LOW 0x52
#define NETWARE_SERVER_TYPE GETINT16(4)
/*
#define GENERAL_SERVICE_REQUEST 1
#define ALL_SERVER_TYPES (WORD)0xFFFF
#define NEAREST_SERVER_REQUEST 3
#define GENERAL_SERVICE_REPLY 2
*/
#define NEAREST_SERVER_RESPONSE 4
#define SAPS_DEFAULT_NICE_VALUE 20
#define SAPS_MIN_PRIORITY 40
#define SAPS_MAX_PRIORITY 1

lanInfo_t *lanInfoTable;

/*********************************************************************/

#define	DOWN				16

#define	SLEEP				-1
#define	NOSLEEP				0
#define	CHECK				-1
#define	NOCHECK				0
#define	ALL					-1

#define	CHANGED				0x01

#define	TFILE_SERVER		GETINT16((uint16)0x0004)
#define	NETSERVERSOCKET		GETINT16((uint16)0x0452)
#define	ROUTERSOCKET		GETINT16((uint16)0x0453)

#define	NETLOCALBIT			0x01
#define	NETSTARBIT			0x02
#define	NETRELIABLEBIT		0x04
#define	NETWANBIT			0x08

#define	NETALIVEBIT			0x40
#define	NETDMABIT			0x80

/*********************************************************************/

static char titleStr[100];
int ipxFd;
int spxFd;
#define OK 0
#define OSShortTermAllocTag
#define SFree(ptr) free((ptr))
#define Free(ptr) free((ptr))
/* has to be for mips */
#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#define CMovB(from, to, len) memcpy((to), (from), (len))
#define CSetB(val, addr, len) memset((addr), (val), (len))

#define NLong(addr) (addr)

#define PutShort(val, addr) *(uint16 *)(addr) = GETINT16((uint16) \
	(val))

#define GetShort(addr) GETINT16(*(uint16 *)(addr))
#define GetLong(addr) GETINT32(*(uint32 *)(addr))
#define InvertLong(num) GETINT32(num)

#ifdef NW_TLI
extern int t_errno;
#endif


/*********************************************************************/

/* router down will be checked by IPX in the kernel */
BYTE					ALLHOSTS[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
BYTE					TrackFlag = 0;
BYTE					MyNetworkAddress[12];
LONG					RouterSemaphore = FALSE;
LONG					ageInterval  = 50;

/*********************************************************************/
/* Function prototypes for static procedures */

/*********************************************************************/

struct ServerEntry
{
	struct ServerEntry		*Link;
	WORD					ServerType;
	BYTE					ServerName[48];		/* Length-preceded!! */
	BYTE					ServerAddress[12];
	WORD					HopsToServer;
	BYTE					ServerEntryChanged;
	BYTE					ChangeBindery;
	BYTE					IsRecordedInBindery;
	struct InfoSourceEntry	*SourceListLink;
};	/* also replicated in WATCHDOG.386	*/

struct InfoSourceEntry
{
	struct InfoSourceEntry	*NextSource;
	BYTE					SourceAddress[6];
	WORD					HopsToSource;
	BYTE					ServTimer;
	BYTE					ServConnectedLAN;
};

struct InfoPtrPtr
{
	struct InfoSourceEntry	*InfoPtr;
};


struct ServPtrPtr
{
	struct ServerEntry		*ServPtr;

};

struct ServPacketInfo
{
	WORD					TargetType;
	BYTE					TargetServer[48];
	BYTE					TargetAddress[12];
	WORD					ServerHops;
};


struct ServerEntry			*ServerList = NULL;

struct ServerPacketStruct
{
	WORD					Checksum;  /* -1 when not used */
	WORD					PacketLength;
	BYTE					TransportControl;
	BYTE					PacketType;
	LONG					DestinationNet;
	BYTE					DestinationHost[6];
	WORD					DestinationSocket;
	LONG					SourceNet;
	BYTE					SourceHost[6];
	WORD					SourceSocket;
	WORD					Operation;
	struct ServPacketInfo	ServerTable[7];
};

/* for word alignment we need exact offsets into the packet data */
#define SIZE_OF_SERVER_TABLE_ENTRY 64
#define GETSAPOPERATION(x) (*(uint16 *)((char *)(x) + OPERATION_OFFSET)
#define PUTSAPOPERATION(x,y) (*(uint16 *)((char *)(x) +  \
	OPERATION_OFFSET) = GETINT16((uint16)(y)))

#define MAX_TOKEN_VALUE_LENGTH 100
#define MAX_FILE_NAME_LENGTH 100
#define MAX_ENUM_LENGTH 100
#define MAX_LAN_STR_LENGTH 100

#define MIN_NEAREST_REPLY_DELAY 0
#define MAX_NEAREST_REPLY_DELAY 300

uint16 pnwSapSocket = NETSERVERSOCKET;

char *standardOutTokenStr = "SAP_STANDARD_OUT";
char *errorOutTokenStr = "SAP_ERROR_OUT";
char *nearestResponseTokenStr = "SAP_REPLY_DELAY";

unsigned char standardOut[MAX_FILE_NAME_LENGTH];
unsigned char errorOut[MAX_FILE_NAME_LENGTH];
unsigned int nearestReplyDelay;
int GotAlarm = FALSE;

/*
	TimeOut declarations
*/
#define SAP_ALARM_INTERVAL 60
#define SAP_RESTART_FACTOR 2

time_t lastAlarmHit; 

/* the maximum number of lans that can be connected to ipx */
uint32 ipxMaxConnectedLans;
