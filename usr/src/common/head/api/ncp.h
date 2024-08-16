/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/ncp.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/ncp.h 1.5 (Novell) 7/30/91
 */

/*
**	NetWare/SRC Header File
**
**	Description:
**		This header file contains the NCP definition as well as a 
**		#defines and typedefs for all the packet engine routines
*/
#include <sys/nwctypes.h>
/* Has this file already been included */
#ifndef	NCP_HEADER

#define	NCP_HEADER		/* nothing */

#define NW_MAX_ADDRESS			12
#define NW_PROVIDER_STRING		"/dev/ipx"
#define NW_MAX_PACKET_SIZE		546
#define NETWARE_PACKET_TYPE		0x11

/* Possible Request Types */
#define	CREATE_A_SERVICE_CONNECTION			0x1111
#define	FILE_SERVICE_REQUEST				0x2222
#define	FILE_SERVICE_RESPONSE				0x3333
#define	DESTROY_A_SERVICE_CONNECTION		0x5555
#define	PRIVATE_NCP_FUNCTION				0x8888
#define	PREVIOUS_REQUEST_BEING_PROCESSED	0x9999

/* Possible connection status field value */
#define NCP_STATUS_BAD						0x01

#define FILE_SERVER_TYPE					4

#define NCP_BINDERY_REQUEST					23
#define NCP_READ_PROPERTY					61

/*	Length of NCP items */
#define NCP_REQUEST_HEADER_LENGTH			7
#define	NCP_RESPONSE_HEADER_LENGTH			8
#define NCP_WATCHDOG_LENGTH					2

/*	Max possible data is MAX buffer size plus some control fields.  On
 *	a ReadFile, for instance, there is an extra 2 bytes telling the 
 *	requestor how many bytes were actually read.  15 bytes buffer
 *	should be sufficient for all NCP responses.  */
#define MAX_NEGOTIATED_BUFFER_SIZE	512	
#define	MAX_POSSIBLE_NCP_DATA	MAX_NEGOTIATED_BUFFER_SIZE + 15 
#define NWSMALLEST_BUFFER_SIZE 512

#define MAX_SERVER_NAME			48
#define MAX_PROPERTY_VALUE		128

#define MAX_SERVERS						10
#define MAX_CLIENTS						20
#define MAX_CONNECTIONS					50
#define MAX_CONNECTIONS_PER_CLIENT		8
#define MAX_REQUESTS					100
#define MAX_TASKS_PER_CLIENT 			16

#define NO_CONNECTION					0xFFFF

/* connection status flags */
#define CONNECTING						0xF1
#define CONNECTING_PLUS					0xF2
#define CONNECTION_OK					0xF3
#define CONNECTION_BAD					0xF4

#define DEFAULT_TASK					1
#define DEFAULT_REQUEST_SIZE			512

/* NW_DEFAULT_RETRIES X NW_NCP_TIMEOUT = Total retry time in seconds. */
#define NW_NCP_RETRIES					5
#define NW_NCP_TIMEOUT					3

/* connect and disconnect are pseudo-ncps used internally */
#define NCP_CONNECT						0x70
#define NCP_DISCONNECT					0x71
#ifndef NCP_NEGOTIATE_BUFFER_SIZE
#define NCP_NEGOTIATE_BUFFER_SIZE		33
#endif
#define CONNECTION_ZOMBIE				(RESPONSE_NCP *)0xFFFFFFFF

#define SAP_TIMEOUT_FAILURE 			222
#define SAP_FAILURE 					223
#define SAP_QUERY_SIZE					4
#define SAP_QUERY_LENGTH				4
#define SAP_RESPONSE_LENGTH				66
#define SAP_QUERY_NEAREST 				3
#define SAP_NEAREST_RESPONSE			4
/* NW_SAP_RETRIES X NW_SAP_TIMEOUT = Total retry time in seconds. */
#define NW_SAP_RETRIES					3
#define NW_SAP_TIMEOUT					4

typedef struct {
	int				sapFD;
	int				timeoutID;
	uint16			retries;
/*	tranAddr_t		transportInfo; */
	ipxAddr_t		transportInfo;
	char			*reqBuf;
	char			*repBuf;
	uint16			repLen;
	int32			packets;
} NW_sap_packet_t;

typedef struct {
	flag_t		inUseFlag;		/* indicates connection in use */
	uint8		connectionStatus;	/* indicates connection health */
	char		serverName[ MAX_SERVER_NAME ];
	uint8		serverAddress[ NW_MAX_ADDRESS ];	/* server address */
	uint16		majorVersion;		
	uint16		minorVersion;
	uint32		maxRequest;		/* negotiated buffer size */
	uint16		connectionNumber;	/* connection number at server */
	uint8		sequence;		/* sequence number of current request */
	int			timeoutID;		/* unique id for cancelling timeout */
	uint16		retries;		/* retries allowed on connection */
	uint16		retriesLeft;	/* retries left for current request */
	uint8		*reqBufPtr;	
	uint16		reqLen;
	uint8		*repBufPtr;
	uint16		*repLenPtr;
	uint16		connectionID;
	uint16		forwardSortIndex;
	uint16		backwardSortIndex;
} connectionEntry_t;

typedef struct {
	uint8		*reqBufPtr;	
	uint16		reqLen;
	uint8		*repBufPtr;
	uint16		*repLenPtr;
} NCP_Req_Rep_t;

typedef struct ncpReplyHeader {
	uint16		replyType;
	uint8		sequenceNumber;
	uint16		connNumber;
	uint8		currentTask;
	uint8		completionCode;
	uint8		connectionStatus;
} ncpReplyHeader_t;

#define ClearConnectionInfo(connId) { \
memset( &connTable[(connId)], 0, sizeof( connectionEntry_t )); }

#endif		/* of #ifndef NCP_HEADER */
