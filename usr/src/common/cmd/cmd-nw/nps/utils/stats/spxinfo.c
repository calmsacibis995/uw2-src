/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/spxinfo.c	1.9"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: spxinfo.c,v 1.11 1994/09/01 21:40:13 vtag Exp $"
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
typedef int * queue_t; /* Make sys/tihdr.h happy */
#include <sys/tihdr.h>
#include <sys/spx_app.h>
#include "nwconfig.h"
#include "nwmsg.h"
#include "npsmsgtable.h"

#define SPX_DEVICE "/dev/nspx"
char ErrorStr[80];								/* global error string */
static int GetInfo(int);
static int GetConInfo(int, int, int);

/*ARGSUSED*/
int
main(int argc, char *argv[])
{
	int		spxFd;
	int		maxConn;
	int		conId;
	int		i;
	int		ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_SPXI, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Error exit.\n",
			argv[0], ccode);
		exit(1);
	}

	if ((spxFd = open(SPX_DEVICE, O_RDWR)) == -1) {			/* open spx device */
		sprintf(ErrorStr, MsgGetStr(S_SPX_OPEN_FAIL), SPX_DEVICE);
		perror(ErrorStr);			
		return(-1); 
    }
	if ((maxConn = GetInfo(spxFd)) == -1) {
        return(-1); 
	}

	if (argc == 2) {		/* check for 1 connId pass in as argument */
		conId = atoi(argv[1]);
		if (GetConInfo(spxFd, conId, 1) == -1) {
			return(-1); 
		}
	} else {
		for(i=0; i < maxConn; i++) {
			if (GetConInfo(spxFd, i, 0) == -1) {
				return(-1); 
			}
		}
	}
	return(0);
}				 /* end main() */

static int							/* Return: max SPX connection open */
GetInfo(int spxFd)
{
	struct strioctl strioc;			/* ioctl structure */
	spxStats_t	infobuf;
	time_t		hr, min, secs;


	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd = SPX_GET_STATS;
	strioc.ic_timout = 5;
	strioc.ic_len = sizeof(infobuf);
	strioc.ic_dp = (char *)&infobuf;
	if (ioctl(spxFd, I_STR, &strioc) == -1) {
		sprintf(ErrorStr, MsgGetStr(S_SPX_STATS_FAIL), SPX_DEVICE);
		perror(ErrorStr);		
        return(-1); 
	}

	time(&secs);
	secs -= infobuf.spx_startTime;

	hr = secs/3600;
	secs -= hr * 3600;
	min = secs/60;
	secs -= min * 60;

	printf(MsgGetStr(S_SPX_GENERAL));
	printf(MsgGetStr(S_SPX_VERSION),
			infobuf.spx_major_version, infobuf.spx_minor_version,
			infobuf.spx_revision[0], infobuf.spx_revision[1]);
	printf(MsgGetStr(S_NEWLINE));
	printf(MsgGetStr(S_TIME_ACTIVE) , hr, min, secs);
	printf(MsgGetStr(S_MAX_CONNS), infobuf.spx_max_connections);
	printf(MsgGetStr(S_CURRENT_CONNS), infobuf.spx_current_connections);
	printf(MsgGetStr(S_SIMUL_CONNS), infobuf.spx_max_used_connections);

	/* display the general counters now */
	printf(MsgGetStr(S_ALLOC_FAIL), infobuf.spx_alloc_failures);
	printf(MsgGetStr(S_OPEN_FAIL), infobuf.spx_open_failures);
	printf(MsgGetStr(S_IOCTLS), infobuf.spx_ioctls);
	printf(MsgGetStr(S_NEWLINE));

	/* display the send counters now */
	printf(MsgGetStr(S_SEND_COUNT), infobuf.spx_send_mesg_count);
	printf(MsgGetStr(S_UNKNOWN_COUNT), infobuf.spx_unknown_mesg_count);
	printf(MsgGetStr(S_BAD_SENDS), infobuf.spx_send_bad_mesg);

	printf(MsgGetStr(S_CONN_REQS), infobuf.spx_connect_req_count);
	printf(MsgGetStr(S_FAIL_CONNS), infobuf.spx_connect_req_fails);
	printf(MsgGetStr(S_LISTENS), infobuf.spx_listen_req);
	printf(MsgGetStr(S_LISTENS_FAIL), infobuf.spx_listen_req_fails);

	printf(MsgGetStr(S_IPX_SENDS), infobuf.spx_send_packet_count);
	printf(MsgGetStr(S_TIME_RETRANS), infobuf.spx_send_packet_timeout);
	printf(MsgGetStr(S_NAK_RETRANS), infobuf.spx_send_packet_nak);
	printf(MsgGetStr(S_NEWLINE));

	/* display the receive counters now */
	printf(MsgGetStr(S_IPX_RECV), infobuf.spx_rcv_packet_count);
	printf(MsgGetStr(S_BAD_IPX_RECV), infobuf.spx_rcv_bad_packet);
	printf(MsgGetStr(S_BAD_DATA), infobuf.spx_rcv_bad_data_packet);
	printf(MsgGetStr(S_DUP_DATA), infobuf.spx_rcv_dup_packet);
	printf(MsgGetStr(S_SENT_UP), infobuf.spx_rcv_packet_sentup);
	printf(MsgGetStr(S_IPX_CONNS), infobuf.spx_rcv_conn_req);
	printf(MsgGetStr(S_NO_LISTENS), infobuf.spx_no_listeners);
	printf(MsgGetStr(S_ABORT_CONN), infobuf.spx_abort_connection);
	printf(MsgGetStr(S_RETRY_ABORT), infobuf.spx_max_retries_abort);


	return(infobuf.spx_max_used_connections);
}

static int							/* Return: status open */
GetConInfo(int spxFd, int conId, int showinactive)
{
	struct strioctl strioc;			/* ioctl structure */
	spxConStats_t	infobuf;
	time_t			hr, min, secs;


	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd = SPX_GET_CON_STATS;
	strioc.ic_timout = 5;
	strioc.ic_len = sizeof(infobuf);
	strioc.ic_dp = (char *)&infobuf;

	infobuf.con_connection_id = GETINT16(conId);

	if (ioctl(spxFd, I_STR, &strioc) == -1) {
		sprintf(ErrorStr, MsgGetStr(S_SPX_CON_STATS_FAIL), SPX_DEVICE);
		perror(ErrorStr);		
        return(-1); 
	}

	time(&secs);
	secs -= infobuf.con_startTime;

	hr = secs/3600;
	secs -= hr * 3600;
	min = secs/60;
	secs -= min * 60;

	if (infobuf.con_startTime == 0) {
		if (showinactive) {
			printf(MsgGetStr(S_NEWLINE));
			printf(MsgGetStr(S_CONN_INACTIVE), conId);
		}
	} else {
		printf(MsgGetStr(S_NEWLINE));
		printf(MsgGetStr(S_NEWLINE));
		printf(MsgGetStr(S_CONN_STATS), conId);
		printf(MsgGetStr(S_CONN_ACTIVE), hr, min, secs);
		printf(MsgGetStr(S_CONN_ADDR));
		printf(MsgGetStr(S_NET_ADDR),
           	infobuf.con_addr.net[0], infobuf.con_addr.net[1],
			infobuf.con_addr.net[2],infobuf.con_addr.net[3]);
		printf(MsgGetStr(S_NODE_ADDR),
           	infobuf.con_addr.node[0], infobuf.con_addr.node[1],
           	infobuf.con_addr.node[2], infobuf.con_addr.node[3],
			infobuf.con_addr.node[4],infobuf.con_addr.node[5]);
		printf(MsgGetStr(S_SOCKET_ADDR),
           	infobuf.con_addr.sock[0], infobuf.con_addr.sock[1]);

		printf(MsgGetStr(S_CONN_NUMBER), infobuf.con_connection_id);
		printf(MsgGetStr(S_NEWLINE));

		if(infobuf.con_state >= TS_DATA_XFER) {
			printf(MsgGetStr(S_OTHER_ADDR));
			printf(MsgGetStr(S_NET_ADDR),
   	        	infobuf.o_addr.net[0], infobuf.o_addr.net[1],
				infobuf.o_addr.net[2],infobuf.o_addr.net[3]);
			printf(MsgGetStr(S_NODE_ADDR),
   	        	infobuf.o_addr.node[0], infobuf.o_addr.node[1],
   	        	infobuf.o_addr.node[2], infobuf.o_addr.node[3],
				infobuf.o_addr.node[4],infobuf.o_addr.node[5]);
			printf(MsgGetStr(S_SOCKET_ADDR),
   	        	infobuf.o_addr.sock[0], infobuf.o_addr.sock[1]);
	
			printf(MsgGetStr(S_CONN_NUMBER), infobuf.o_connection_id);
			printf(MsgGetStr(S_NEWLINE));
		}

		printf(MsgGetStr(S_CONN_STATE), infobuf.con_state);
		printf(MsgGetStr(S_MAX_RETRIES), infobuf.con_retry_count);
		printf(MsgGetStr(S_MIN_TIMEOUT), infobuf.con_retry_time);

		if(infobuf.con_state >= TS_DATA_XFER) {
			printf(MsgGetStr(S_NEWLINE));
			printf(MsgGetStr(S_DATA_XFER));
			if(infobuf.con_type == 2) {
				printf(MsgGetStr(S_SPXII_END));
			} else if(infobuf.con_type == 1) {
				printf(MsgGetStr(S_SPX_END));
			} else {
				printf(MsgGetStr(S_UNKNOWN_END));
			}
			if(infobuf.con_ipxChecksum)
				printf(MsgGetStr(S_USE_CHKSUM));
			else
				printf(MsgGetStr(S_NO_CHKSUM));


			printf(MsgGetStr(S_RCV_WIN_SIZE), infobuf.con_window_size);
			printf(MsgGetStr(S_TRAN_WIN_SIZE), infobuf.con_remote_window_size);
			printf(MsgGetStr(S_TRAN_PKT_SIZE), infobuf.con_send_packet_size);
			printf(MsgGetStr(S_RCV_PKT_SIZE), infobuf.con_rcv_packet_size);
			printf(MsgGetStr(S_ROUND_TRIP), infobuf.con_round_trip_time);
			printf(MsgGetStr(S_WIN_CLOSED), infobuf.con_window_choke);
			printf(MsgGetStr(S_FLOW_CNTL), infobuf.con_canput_fail);
		}
		printf(MsgGetStr(S_NEWLINE));
		printf(MsgGetStr(S_APP_TO_SPX), infobuf.con_send_mesg_count);
		printf(MsgGetStr(S_UNK_FROM_APP), infobuf.con_unknown_mesg_count);
		printf(MsgGetStr(S_BAD_FROM_APP), infobuf.con_send_bad_mesg);
		printf(MsgGetStr(S_NEWLINE));

		printf(MsgGetStr(S_SENT_TO_IPX), infobuf.con_send_packet_count);
		printf(MsgGetStr(S_IPX_RESENDS), infobuf.con_send_packet_timeout);
		printf(MsgGetStr(S_IPX_NAK_RESEND), infobuf.con_send_packet_nak);
		printf(MsgGetStr(S_IPX_ACKS), infobuf.con_send_ack);
		printf(MsgGetStr(S_IPX_NACKS), infobuf.con_send_nak);
		printf(MsgGetStr(S_IPX_WATCH), infobuf.con_send_watchdog);
		printf(MsgGetStr(S_NEWLINE));
		printf(MsgGetStr(S_NEWLINE));

		printf(MsgGetStr(S_CON_RCV_PACKET), infobuf.con_rcv_packet_count);
		printf(MsgGetStr(S_CON_RCV_WATCH), infobuf.con_rcv_watchdog);
		printf(MsgGetStr(S_CON_RCV_ACK), infobuf.con_rcv_ack);
		printf(MsgGetStr(S_CON_RCV_NACK), infobuf.con_rcv_nak);
		printf(MsgGetStr(S_CON_RCV_BAD_PACKET), infobuf.con_rcv_bad_packet);
		printf(MsgGetStr(S_CON_RCV_BAD_DATA), infobuf.con_rcv_bad_data_packet);
		printf(MsgGetStr(S_CON_RCV_DUP), infobuf.con_rcv_dup_packet);
		printf(MsgGetStr(S_CON_RCV_OUTSEQ), infobuf.con_rcv_packet_outseq);
		printf(MsgGetStr(S_CON_RCV_SENTUP), infobuf.con_rcv_packet_sentup);
		printf(MsgGetStr(S_CON_RCV_QUEUED), infobuf.con_rcv_packet_qued);
	}	

	return(0);
}
