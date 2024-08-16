/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx_app.h	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_SPX_APP_H  /* wrapper symbol for kernel use */
#define _NET_NW_SPX_APP_H  /* subject to change without notice */

#ident	"$Id: spx_app.h,v 1.6 1994/09/07 04:48:44 meb Exp $"

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
 */

/*
 *   The parts of SPX exposed to the application programmer.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/ipx_app.h>
#else
#include <sys/ipx_app.h>
#endif /* _KERNEL_HEADERS */

#define TLI_SPX_CONNECTION_FAILED ((uint8) 0xED)
#define TLI_SPX_CONNECTION_TERMINATED ((uint8) 0xEC)
#define TLI_SPX_PACKET_OVERFLOW ((uint8) 0xFD)
#define TLI_SPX_MALFORMED_PACKET ((uint8) 0xFE)
#define SPX_GET_CONN_INFO  ( ( 's' << 8 ) | 1 ) 
#define SPX_GS_MAX_PACKET_SIZE  ( ( 's' << 8 ) | 2 ) 
#define SPX_GS_DATASTREAM_TYPE  ( ( 's' << 8 ) | 3 ) 
#define SPX_T_SYNCDATA_IOCTL  ( ( 's' << 8 ) | 4 )
#define SPX_CHECK_QUEUE  ( ( 's' << 8 ) | 5 )
#define SPX_GET_STATS  ( ( 's' << 8 ) | 6 )
#define SPX_SET_PARAM  ( ( 's' << 8 ) | 7 )
#define SPX_SPX2_OPTIONS  ( ( 's' << 8 ) | 8 )
#define SPX_GET_CON_STATS  ( ( 's' << 8 ) | 9 )

/* cant conflict with TPI primitive numbers */
#define SPX_T_SYNCDATA_REQ 0x0000FFFF 

typedef struct spx_optmgmt {
	uint8 spxo_retry_count;
	uint8 spxo_watchdog_flag;
	uint16 spxo_min_retry_delay;
} SPX_OPTMGMT;

typedef struct spxopt_s {
	unsigned char spx_connectionID[2];
	unsigned char spx_allocationNumber[2];
} SPX_OPTS;


/* New SPXII structure is used for calls t_listen, t_accept, t_connect and
 * t_optmgmt.  This structure is expanable in future versions of SPXII
 * and therefore a variable should never be declared directly (ie.
 * "struct spx2_options localoptions;" should not be done) and the size of
 * the structure should never be taken (ie. "sizeof(SPX2_OPTIONS);" ).  The
 * function t_alloc should always be used to allocate the structure and
 * the options field of the "struct t_info" should be used to determine the
 * size of the structure (t_getinfo). If the tli calls are not used to 
 * allocate and determine the size, a TBUFOVFLW error could be generated when
 * an application is run with a newer version of SPXII.		
 */
#define OPTIONS_VERSION 1 
#define VERSION1SPX2OPTSIZE (13 * sizeof(uint32))
#define SPX2_NO_NEGOTIATE_OPTIONS 0xAAAA 
#define SPX2_NEGOTIATE_OPTIONS 0x5555 

/* spxII Sessions Flags */
#define SPX_SF_NONE			0x00
#define SPX_SF_IPX_CHECKSUM	0x01
#define SPX_SF_SPX2_SESSION	0x02



typedef struct spx2_options {
	uint32	versionNumber;			/* Must be set to OPTION_VERSION */

	uint32	spxIIOptionNegotiate;	/* Exchange options and negotiate
									    packet size*/

	uint32	spxIIRetryCount;		/*  Number of Transmit retries on 
									   data packets*/

	uint32	spxIIMinimumRetryDelay;	/* Minimum retry timeout, in millisec */

	uint32	spxIIMaximumRetryDelta;	/* Maximum retry delta, in milliseconds */

	uint32	spxIIWatchdogTimeout;	/* This is a SYSTEM Parameter for NWU. 
									   This value can only be changed in 
									   spx_tune.h    */

	uint32	spxIIConnectionTimeout;	/* Number of milliseconds to wait for full
									   connection setup */

	uint32	spxIILocalWindowSize;	/* Number of data packets in recieve 
									   window */

	uint32	spxIIRemoteWindowSize;	/* Remote endpoints initial receive window 
									   size */

	uint32	spxIIConnectionID;		/* Valid only after connection is 
									   established */

	uint32	spxIIInboundPacketSize;	/* Maximum recieve packet size */

	uint32	spxIIOutboundPacketSize;/* Maximum transmit packet size */

	uint32	spxIISessionFlags;		/* Session characteristic options */
									/* END of VERSION 1 */
} SPX2_OPTIONS;


typedef struct spxConStatisics {
	time_t	con_startTime;				/* Time of connection open() */
	uint16	con_connection_id;			/* connection id number */
	long	con_state;					/* current connection state */
	uint16	con_retry_count;			/* # of retries befor disconnect */
	uint16	con_retry_time;				/* min  milliseconds between retries */
	ipxAddr_t con_addr;					/* IPX address of this endpoint */

	/* following statics are only valid if in DATAXFER state or higher */
	ipxAddr_t o_addr;					/* IPX address of other endpoint */
	uint16	o_connection_id;			/* conn id number of other endpoint */
	uint8	con_type;					/* connection info byte
										   bits 0-1  other endpoint type 
										     0 = unknown/not connected
										     1 = SPX, 2 = SPXII
										   bits 2-6 unused
										   bit 7 
											1- multiple open on device
											  NVT uses this mode
										 */
	uint8	con_window_size;			/* current receive window size */
	uint8	con_remote_window_size;		/* current transmit window size */
	uint8	con_ipxChecksum;			/* if non-zero ipxchecksums are used */
	uint16	con_send_packet_size;		/* current max transmit packet size */
	uint16	con_rcv_packet_size;		/* current max receive packet size */
	uint32	con_round_trip_time;		/* last round trip time in msec */

	/* the reset are counters */
	uint32	con_window_choke;			/* # of times the connection was in flow
										** control due to other endpoint window
										** closure */
	uint32	con_canput_fail;			/* # of times canput failed to IPX */
	uint32	con_send_mesg_count;		/* # of messages sent to SPX by app */
	uint16	con_unknown_mesg_count;		/* # unknown msgs sent to SPX by app */
	uint16	con_send_bad_mesg;			/* # of bad packets SPX asked to send */
	uint32	con_send_packet_count;		/* # of packets sent to IPX by SPX */
	uint16	con_send_packet_timeout;	/* # of packets resent due to timeout */
	uint16	con_send_packet_nak;		/* # of packets resent due to NAK */
	uint32	con_send_ack;				/* # of ACK packets sent */
	uint16	con_send_nak;				/* # of NACK packets sent */
	uint16	con_send_watchdog;			/* # of watch dog packets sent */

	uint32	con_rcv_packet_count;		/* # of SPX packets received */
	uint16	con_rcv_bad_packet;			/* # of bad packets SPX received */
	uint16	con_rcv_bad_data_packet;	/* # of bad data packets SPX received */
	uint16	con_rcv_dup_packet;			/* # of duplicate packets received */
	uint16	con_rcv_packet_qued;		/* # packets qued due to app too slow */
	uint16	con_rcv_packet_outseq;		/* # packets rcved out of sequence */
	uint16	con_rcv_watchdog;			/* # of watch dog packets received */
	uint32	con_rcv_packet_sentup;		/* # packets rcved sent upstream */
	uint32	con_rcv_ack;				/* # of ACK packets received */
	uint32	con_rcv_nak;				/* # of NACK packets received */
} spxConStats_t;

typedef struct spxStatisics {
	uint16	spx_major_version;			/* Major Version */ 
	uint16	spx_minor_version;			/* Minor Version */ 
	char	spx_revision[2];			/* Revision */ 
	time_t	spx_startTime;				/* Time driver loaded */
	uint16	spx_max_connections;		/* Max configured SPX connections */ 
	uint16	spx_current_connections;	/* current SPX connections (opens) */ 
	uint16	spx_max_used_connections;	/* Max SPX simutaneously connections */
	uint16	spx_alloc_failures;			/* # of mem alloc (allocb) failures */
	uint16	spx_open_failures;			/* # of open failures */
	uint16	spx_connect_req_count;		/* # Connect Requests Rcved from apps */
	uint16	spx_connect_req_fails;		/* # of Connect Requests Failures */
	uint16	spx_listen_req;				/* # of Listens posted by application */
	uint16	spx_listen_req_fails;		/* # of Listens Failures */
	uint16	spx_ioctls;					/* # of IOCTL sent to SPX by app */
	uint32	spx_send_mesg_count;		/* # of messages sent to SPX by app */
	uint16	spx_unknown_mesg_count;		/* # unknown msgs sent to SPX by app */
	uint16	spx_send_bad_mesg;			/* # of bad msgs SPX asked to send */
	uint32	spx_send_packet_count;		/* # of packets sent to IPX by SPX */
	uint16	spx_send_packet_timeout;	/* # of packets resent due to timeout */
	uint16	spx_send_packet_nak;		/* # of packets resent due to NAK */

	uint32	spx_rcv_packet_count;		/* # of SPX packets received */
	uint16	spx_rcv_bad_packet;			/* # of bad packets SPX received */
	uint16	spx_rcv_bad_data_packet;	/* # of bad data packets SPX received */
	uint16	spx_rcv_dup_packet;			/* # of duplicate packets received */
	uint32	spx_rcv_packet_sentup;		/* # packets rcved sent upstream */
	uint16	spx_rcv_conn_req;			/* # Connect Requests Rcved from wire*/

	uint16	spx_abort_connection;		/* # of SPX connection aborted */
	uint16	spx_max_retries_abort;		/* # of SPX conn. aborted due to 
										** max retries exceeded */
	uint16	spx_no_listeners;			/* # of Conn. Req with no Listeners */
} spxStats_t;

typedef struct spxParameters {
	uint16	spx_max_connections;	/* max # of SPX connections simultaneously
									   active */
	uint16	spx_max_sockets;		/* max # of sockets open simultaneously
									   should be larger than max connections */
} spxParam_t;

typedef struct SpxIIHdr {
    ipxHdr_t ipxHdr;
    uint8   connectionControl;
    uint8   dataStreamType;
    uint16  sourceConnectionId;
    uint16  destinationConnectionId;
    uint16  sequenceNumber;
    uint16  acknowledgeNumber;
    uint16  allocationNumber;
    uint16  negotiateSize;
    } spxIIHdr_t;

typedef struct SpxHdr {
	ipxHdr_t ipxHdr;
	uint8 	connectionControl;
	uint8 	dataStreamType;
	uint16 	sourceConnectionId;
	uint16 	destinationConnectionId;
	uint16 	sequenceNumber;
	uint16 	acknowledgeNumber;
	uint16 	allocationNumber;
	} spxHdr_t;

#endif /* _NET_NW_SPX_APP_H */
