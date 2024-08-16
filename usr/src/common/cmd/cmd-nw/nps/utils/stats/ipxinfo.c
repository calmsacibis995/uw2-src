/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/ipxinfo.c	1.11"
#ident	"$Id: ipxinfo.c,v 1.9.2.1 1994/11/08 20:57:19 vtag Exp $"

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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/ipx_app.h>
#include "nwmsg.h"
#include "nwconfig.h"
#include "npsmsgtable.h"

char ErrorStr[80];								/* global error string */
static int GetInfo( void);
#define IPX "/dev/ipx"

/*ARGSUSED*/
int
main( int argc, char *argv[] )
{
	int ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_IPXI, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Error exit.\n",
			argv[0], ccode);
		exit(1);
	}
	
	return( GetInfo());
} /* end main() */

static int							/* Return: status */
GetInfo( void)
{
	int	ipxFd;						/* ipx clone device file descriptor */
	struct strioctl strioc;			/* ioctl structure */
	struct {
		IpxLanStats_t	 l;
		IpxSocketStats_t s;
	} infobuf;
	time_t	  hr, min, secs;

	if( (ipxFd = open(IPX, O_RDWR )) == -1 ) { /* open ipx device */
		sprintf( ErrorStr, MsgGetStr(I_IPX_OPEN_FAIL), IPX );
		perror( ErrorStr );			
		return ( -1 ); 
    }

	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd =  IPX_STATS;
	strioc.ic_timout = 3;
	strioc.ic_len = sizeof(infobuf);
	strioc.ic_dp = (char *)&infobuf;
	if ( ioctl( ipxFd, I_STR, &strioc ) == -1 ) {
		sprintf( ErrorStr, MsgGetStr(I_IPX_STATS_FAIL), IPX );
		perror( ErrorStr );		
        return ( -1 ); 
	}

    time( &secs);
    secs -= infobuf.l.StartTime;

    hr = secs / 3600;
    secs -= hr * 3600;
    min = secs / 60;
    secs -= min * 60;

	if( infobuf.l.IpxSocketMux) {
		printf(MsgGetStr(I_IPX_VERSION),
				infobuf.s.IpxMajorVersion, infobuf.s.IpxMinorVersion,
				infobuf.s.IpxRevision[0], infobuf.s.IpxRevision[1]);
	}
	printf(MsgGetStr(I_LIPMX_VERSION),
			infobuf.l.LipmxMajorVersion, infobuf.l.LipmxMinorVersion,
			infobuf.l.LipmxRevision[0], infobuf.l.LipmxRevision[1]);
	printf(MsgGetStr(I_NEWLINE));
    printf(MsgGetStr(I_LAN_ACTIVE), hr, min, secs);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_LAN_STATS));
	printf(MsgGetStr(I_IINFO));
	printf(MsgGetStr(I_DLPI_SIZE), infobuf.l.InProtoSize);
	printf(MsgGetStr(I_DLPI_TYPE), infobuf.l.InBadDLPItype);
	printf(MsgGetStr(I_COMB), infobuf.l.InCoalesced);
	printf(MsgGetStr(I_TRIM_PKT), infobuf.l.TrimPacket);
	printf(MsgGetStr(I_UP_PROP), infobuf.l.InPropagation);
	printf(MsgGetStr(I_UP_NOT_PROP), infobuf.l.InNoPropagate);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_RECD_LAN), infobuf.l.InTotal);
	printf(MsgGetStr(I_SMALL), infobuf.l.InBadLength);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_ECHO), infobuf.l.InDriverEcho);
	printf(MsgGetStr(I_RIP), infobuf.l.InRip);
	printf(MsgGetStr(I_RIP_DROP), infobuf.l.InRipDropped);
	printf(MsgGetStr(I_RIP_ROUTE), infobuf.l.InRipRouted);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_SAP), infobuf.l.InSap);
	printf(MsgGetStr(I_SAP_INVAL), infobuf.l.InSapBad);
	printf(MsgGetStr(I_SAP_ROUTE_IPX), infobuf.l.InSapIpx);
	printf(MsgGetStr(I_SAP_ROUTE_SAPS), infobuf.l.InSapNoIpxToSapd);
	printf(MsgGetStr(I_SAP_DROP), infobuf.l.InSapNoIpxDrop);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_DIAG_MY_NET), infobuf.l.InDiag);
	printf(MsgGetStr(I_DIAG_INT_NET), infobuf.l.InDiagInternal);
	printf(MsgGetStr(I_DIAG_NIC), infobuf.l.InDiagNIC);
	printf(MsgGetStr(I_DIAG_IPX), infobuf.l.InDiagIpx);
	printf(MsgGetStr(I_DIAG_NO_IPX), infobuf.l.InDiagNoIpx);
	printf(MsgGetStr(I_NIC_DROPPED), infobuf.l.InNICDropped);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_IN_BROADCAST), infobuf.l.InBroadcast);
	printf(MsgGetStr(I_BROADCAST_INT), infobuf.l.InBroadcastInternal);
	printf(MsgGetStr(I_BROADCAST_NIC), infobuf.l.InBroadcastNIC);
	printf(MsgGetStr(I_BROAD_DIAG_NIC), infobuf.l.InBroadcastDiag);
	printf(MsgGetStr(I_BROAD_DIAG_FWD), infobuf.l.InBroadcastDiagFwd);
	printf(MsgGetStr(I_BROAD_DIAG_RTE), infobuf.l.InBroadcastDiagRoute);
	printf(MsgGetStr(I_BROAD_DIAG_RESP), infobuf.l.InBroadcastDiagResp);
	printf(MsgGetStr(I_BROAD_ROUTE), infobuf.l.InBroadcastRoute);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_PKT_FWD), infobuf.l.InForward);
	printf(MsgGetStr(I_PKT_ROUTE), infobuf.l.InRoute);
	printf(MsgGetStr(I_PKT_ROUTE_INT), infobuf.l.InInternalNet);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_OINFO));
	printf(MsgGetStr(I_OUT_PROP), infobuf.l.OutPropagation);
	printf(MsgGetStr(I_OUT_TOT_STREAM), infobuf.l.OutTotalStream);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_OUT_TOTAL), infobuf.l.OutTotal);
	printf(MsgGetStr(I_FILL_IN_DEST), infobuf.l.OutFillInDest);
	printf(MsgGetStr(I_OUT_SAME_SOCKET), infobuf.l.OutSameSocket);
	printf(MsgGetStr(I_BAD_LAN), infobuf.l.OutBadLan);
	printf(MsgGetStr(I_OUT_MAX_SDU), infobuf.l.OutBadSize);
	printf(MsgGetStr(I_NO_LAN), infobuf.l.OutNoLan);
	printf(MsgGetStr(I_ROUTE_INT), infobuf.l.OutInternal);
	printf(MsgGetStr(I_OUT_SENT), infobuf.l.OutSent);
	printf(MsgGetStr(I_OUT_QUEUED), infobuf.l.OutQueued);
	printf(MsgGetStr(I_OUT_PACED), infobuf.l.OutPaced);
	printf(MsgGetStr(I_NEWLINE));
	printf(MsgGetStr(I_IOCTL_TOT), infobuf.l.Ioctl);
	printf(MsgGetStr(I_IOCTL_SET_LANS), infobuf.l.IoctlSetLans);
	printf(MsgGetStr(I_IOCTL_GET_LANS), infobuf.l.IoctlGetLans);
	printf(MsgGetStr(I_IOCTL_SAP_Q), infobuf.l.IoctlSetSapQ);
	printf(MsgGetStr(I_IOCTL_SET_LINFO), infobuf.l.IoctlSetLanInfo);
	printf(MsgGetStr(I_IOCTL_GET_LINFO), infobuf.l.IoctlGetLanInfo);
	printf(MsgGetStr(I_IOCTL_GET_NODE), infobuf.l.IoctlGetNodeAddr);
	printf(MsgGetStr(I_IOCTL_GET_NET), infobuf.l.IoctlGetNetAddr);
	printf(MsgGetStr(I_IOCTL_GET_STATS), infobuf.l.IoctlGetStats);
	printf(MsgGetStr(I_IOCTL_LINK), infobuf.l.IoctlLink);
	printf(MsgGetStr(I_IOCTL_UNLINK), infobuf.l.IoctlUnlink);
	printf(MsgGetStr(I_IOCTL_UNKNOWN), infobuf.l.IoctlUnknown);
	
	if( infobuf.l.IpxSocketMux) {
		printf(MsgGetStr(I_NEWLINE));
		printf(MsgGetStr(I_SOCKET_STATS));
		printf(MsgGetStr(I_NEWLINE));
		printf(MsgGetStr(I_SOCKETS_BOUND), infobuf.s.IpxBoundSockets);
		printf(MsgGetStr(I_NON_TLI_BIND), infobuf.s.IpxBind);
		printf(MsgGetStr(I_TLI_BIND), infobuf.s.IpxTLIBind);
		printf(MsgGetStr(I_TLI_OPTMGT), infobuf.s.IpxTLIOptMgt);
		printf(MsgGetStr(I_TLI_UNKNOWN), infobuf.s.IpxTLIUnknown);
		printf(MsgGetStr(I_NEWLINE));
		printf(MsgGetStr(I_TOTAL_IPX_PKTS),
						infobuf.s.IpxOutData + infobuf.s.IpxTLIOutData);
		printf(MsgGetStr(I_SWITCH_SUM), infobuf.s.IpxSwitchSum);
		printf(MsgGetStr(I_SWITCH_SUM_FAIL), infobuf.s.IpxSwitchSumFail);
		printf(MsgGetStr(I_SWITCH_EVEN), infobuf.s.IpxSwitchEven);
		printf(MsgGetStr(I_SWITCH_EVEN_ALLOC), infobuf.s.IpxSwitchEvenAlloc);
		printf(MsgGetStr(I_SWITCH_ALLOC_FAIL), infobuf.s.IpxSwitchAllocFail);
		printf(MsgGetStr(I_SWITCH_INVAL_SOCK), infobuf.s.IpxSwitchInvalSocket);
		printf(MsgGetStr(I_NON_TLI_PKTS), infobuf.s.IpxOutData);
		printf(MsgGetStr(I_OUT_BAD_SIZE), infobuf.s.IpxOutBadSize);
		printf(MsgGetStr(I_OUT_TO_SWITCH), infobuf.s.IpxOutToSwitch);
		printf(MsgGetStr(I_TLI_OUT_PKTS), infobuf.s.IpxTLIOutData);
		printf(MsgGetStr(I_TLI_BAD_STATE), infobuf.s.IpxTLIOutBadState);
		printf(MsgGetStr(I_TLI_BAD_SIZE), infobuf.s.IpxTLIOutBadSize);
		printf(MsgGetStr(I_TLI_BAD_OPT), infobuf.s.IpxTLIOutBadOpt);
		printf(MsgGetStr(I_TLI_FAIL_ALLOC), infobuf.s.IpxTLIOutHdrAlloc);
		printf(MsgGetStr(I_TLI_TO_SWITCH), infobuf.s.IpxTLIOutToSwitch);
		printf(MsgGetStr(I_NEWLINE));
		printf(MsgGetStr(I_IPX_IN), infobuf.s.IpxInData);
		printf(MsgGetStr(I_SUM_FAIL), infobuf.s.IpxSumFail);
		printf(MsgGetStr(I_BUSY_SOCK), infobuf.s.IpxBusySocket);
		printf(MsgGetStr(I_IPX_ROUTED_TLI_ALLOC), infobuf.s.IpxRoutedTLIAlloc);
		printf(MsgGetStr(I_SOCK_NOT_BOUND), infobuf.s.IpxSocketNotBound);
		printf(MsgGetStr(I_DATA_TO_SOCK), infobuf.s.IpxDataToSocket);
		printf(MsgGetStr(I_IPX_ROUTED), infobuf.s.IpxRouted);
		printf(MsgGetStr(I_IPX_SENT_TLI), infobuf.s.IpxRoutedTLI);
		printf(MsgGetStr(I_NEWLINE));
		printf(MsgGetStr(I_TOTAL_IOCTLS), 
			infobuf.s.IpxIoctlBindSocket + 
			infobuf.s.IpxIoctlUnbindSocket + 
			infobuf.s.IpxIoctlSetWater + 
			infobuf.s.IpxIoctlStats + 
			infobuf.s.IpxIoctlUnknown);
		printf(MsgGetStr(I_IOC_SET_WATER), infobuf.s.IpxIoctlSetWater);
		printf(MsgGetStr(I_IOC_SET_BIND), infobuf.s.IpxIoctlBindSocket);
		printf(MsgGetStr(I_IOC_UNBIND), infobuf.s.IpxIoctlUnbindSocket);
		printf(MsgGetStr(I_IOC_STATS), infobuf.s.IpxIoctlStats);
		printf(MsgGetStr(I_IOC_UNKNOWN), infobuf.s.IpxIoctlUnknown);
	}
	return(0);
}
