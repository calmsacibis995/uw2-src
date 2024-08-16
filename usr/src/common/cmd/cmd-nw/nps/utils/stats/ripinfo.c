/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/ripinfo.c	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: ripinfo.c,v 1.6 1994/09/01 21:40:01 vtag Exp $"
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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/ripx_app.h>
#include "nwconfig.h"
#include "nwmsg.h"
#include "npsmsgtable.h"

char ErrorStr[80];								/* global error string */
static int GetInfo( void);
#define RIPX "/dev/ripx"

/*ARGSUSED*/
int
main( int argc, char *argv[] )
{
	int ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_RIPI, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Error exit.\n",
			argv[0], ccode);
		exit(1);
	}
 
	return( GetInfo());
} /* end main() */

static int							/* Return: status */
GetInfo( void)
{
	int	ripFd;						/* rip clone device file descriptor */
	struct strioctl strioc;			/* ioctl structure */
	RouterInfo_t infobuf;
	time_t	hr, min, secs;

	if( (ripFd = open(RIPX, O_RDWR )) == -1 ) { /* open ripx device */
		sprintf( ErrorStr, MsgGetStr(R_RIPX_OPEN_FAIL), RIPX );
		perror( ErrorStr );			
		return ( -1 ); 
    }

	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd =  RIPX_STATS;
	strioc.ic_timout = 3;
	strioc.ic_len = sizeof(infobuf);
	strioc.ic_dp = (char *)&infobuf;
	if ( ioctl( ripFd, I_STR, &strioc ) == -1 ) {
		sprintf( ErrorStr, MsgGetStr(R_RIPX_STATS_FAIL), RIPX );
		perror( ErrorStr );		
        return ( -1 ); 
	}

    time( &secs);
    secs -= infobuf.StartTime;

    hr = secs / 3600;
    secs -= hr * 3600;
    min = secs / 60;
    secs -= min * 60;

	printf(MsgGetStr(R_RIPX_VERSION),
			infobuf.RipxMajorVersion, infobuf.RipxMinorVersion,
			infobuf.RipxRevision[0], infobuf.RipxRevision[1]);
    printf(MsgGetStr(R_NEWLINE));
    printf(MsgGetStr(R_RIP_ACTIVE), hr, min, secs);
    printf(MsgGetStr(R_RECEIVED_PKTS), infobuf.ReceivedPackets);
    printf(MsgGetStr(R_NO_LAN_KEY), infobuf.ReceivedNoLanKey);
    printf(MsgGetStr(R_RECV_BAD_LEN), infobuf.ReceivedBadLength);
    printf(MsgGetStr(R_RECV_COALESCE), infobuf.ReceivedCoalesced);
    printf(MsgGetStr(R_COALESCE_FAIL), infobuf.ReceivedNoCoalesce);
    printf(MsgGetStr(R_ROUTER_REQ), infobuf.ReceivedRequestPackets);
    printf(MsgGetStr(R_ROUTER_RESP), infobuf.ReceivedResponsePackets);
    printf(MsgGetStr(R_UNKNOWN_REQ), infobuf.ReceivedUnknownRequest);
    printf(MsgGetStr(R_NEWLINE));
    printf(MsgGetStr(R_TOT_SENT),
		infobuf.SentAllocFailed +
		infobuf.SentBadDestination +
		infobuf.SentRequestPackets +
		infobuf.SentResponsePackets);
    printf(MsgGetStr(R_SENT_ALLOC_FAIL), infobuf.SentAllocFailed);
    printf(MsgGetStr(R_SENT_BAD_DEST), infobuf.SentBadDestination);
    printf(MsgGetStr(R_SENT_REQ_PKTS), infobuf.SentRequestPackets);
    printf(MsgGetStr(R_SENT_RESP_PKTS), infobuf.SentResponsePackets);
    printf(MsgGetStr(R_NEWLINE));
    printf(MsgGetStr(R_SENT_LAN0_DROPPED), infobuf.SentLan0Dropped);
    printf(MsgGetStr(R_SENT_LAN0_ROUTED), infobuf.SentLan0Routed);
    printf(MsgGetStr(R_NEWLINE));
    printf(MsgGetStr(R_IOCTLS),
		infobuf.RipxIoctlInitialize +
		infobuf.RipxIoctlGetHashSize +
		infobuf.RipxIoctlGetHashStats +
		infobuf.RipxIoctlDumpHashTable +
		infobuf.RipxIoctlGetRouterTable +
		infobuf.RipxIoctlGetNetInfo +
		infobuf.RipxIoctlCheckSapSource +
		infobuf.RipxIoctlResetRouter +
		infobuf.RipxIoctlDownRouter +
		infobuf.RipxIoctlStats +
		infobuf.RipxIoctlUnknown);
    printf(MsgGetStr(R_IOC_INITIALIZE), infobuf.RipxIoctlInitialize);
    printf(MsgGetStr(R_IOC_GET_HASH_SIZE), infobuf.RipxIoctlGetHashSize);
    printf(MsgGetStr(R_IOC_GET_HASH_STATS), infobuf.RipxIoctlGetHashStats);
    printf(MsgGetStr(R_IOC_DUMP_HASH), infobuf.RipxIoctlDumpHashTable);
    printf(MsgGetStr(R_IOC_GET_ROUTER), infobuf.RipxIoctlGetRouterTable);
    printf(MsgGetStr(R_IOC_GET_NET_INFO), infobuf.RipxIoctlGetNetInfo);
    printf(MsgGetStr(R_IOC_CHK_SAP_SRC), infobuf.RipxIoctlCheckSapSource);
    printf(MsgGetStr(R_IOC_RESET_ROUTER), infobuf.RipxIoctlResetRouter);
    printf(MsgGetStr(R_IOC_DOWN_ROUTER), infobuf.RipxIoctlDownRouter);
    printf(MsgGetStr(R_IOC_STATS), infobuf.RipxIoctlStats);
    printf(MsgGetStr(R_IOC_UNKNOWN), infobuf.RipxIoctlUnknown);
	return(0);
}
