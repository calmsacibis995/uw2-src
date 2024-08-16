/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/include/spxapi.h	1.3"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/include/spxapi.h,v 1.3 1994/08/18 16:26:37 vtag Exp $
 */
#ifndef __SPXAPI_H__
#define __SPXAPI_H__

#include <sys/nwportable.h>

/*
 * The following are for general purpose SPX use
 */

#define SPX_DRIVER_NAME				"/dev/nspx2"
#define SPX_ADDRESS_LENGTH			12

#define SPXMAX_DATA_BYTES			534

/* "errnoType" values */
#define SPXET_ERRNO					0
#define SPXET_T_ERRNO				1
#define SPXET_TLOOK					2
#define SPXET_T_DISCONNECT			3
#define SPXET_USER					4

/* Disconnect reason codes */
#define SPX_CONNECTION_FAILED		0xED
#define SPX_CONNECTION_TERMINATED	0xEC
#define SPX_PACKET_MALFORMED		0xFD

/* User error codes (only RPRINTER mode) */
#define SPX_BAD_HEADER				1
#define SPX_PACKET_LENGTH_TOO_SHORT	2


typedef uint8	SPXAddress_ta[SPX_ADDRESS_LENGTH];


typedef struct {
	uint16		  checksum;
	uint16		  length;
	uint8		  transportControl;
	uint8		  packetType;
	SPXAddress_ta destinationAddress;
	SPXAddress_ta sourceAddress;
	uint8		  connectionControl;
	uint8		  dataStreamType;
	uint16		  sourceConnID;
	uint16		  destinationConnID;
	uint16		  sequenceNumber;
	uint16		  acknowledgeNumber;
	uint16		  allocationNumber;
	uint16		  negotiationSize;
} SPXIIHeader_t;


typedef struct {
	int		  	  fd;
	int		  	  errnoType;
	int		  	  errno;
	SPXAddress_ta spxAddress;
	int		  	  rprinterMode;
	uint8	  	  dataStreamType;
	uint16		  destinationConnID;
	int			  isSpxII;	
	int			  packetCount;
	int			  headerSize;
} SPXHandle_t;


/*
 * The following are for use with the SPXConnect call
 */
typedef struct {
	uint16 connectionID;
	uint16 allocationNumber;
} SPXOptionsInfo_t;


/*
 * The following are for use with the SPXWaitForInput call
 */
typedef struct {
	int index;
	int fd;
	int hasInput;
} SPXWait_t;

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

#define SPX_SF_SPX2_SESSION 0x02	/* spxIISessionFlag for spx2 session */
#define SPX2_CC_FLAG		0x08	/* spxII Connection control flag	 */


int SPXOpenTransport(SPXHandle_t *spxHandle,int16 socket );
int SPXCloseTransport(SPXHandle_t *spxHandle );
int SPXSaveHeader(SPXHandle_t *spxHandle);
int SPXConnect(
	SPXHandle_t		 *spxHandle,
	SPXAddress_ta	  spxAddress,
	SPXOptionsInfo_t *spxOptions);
int SPXDisconnect( SPXHandle_t	*spxHandle);
int SPXSend(
	SPXHandle_t	*spxHandle,
	uint8 packet[],
	int  packetLength,
	int  *rSentLength,
	int  moreFlag);
int SPXReceive(SPXHandle_t *spxHandle, uint8 packet[], int *packetLength);
void SPXGetSocketFromAddress(
	SPXAddress_ta *spxAddress,
	uint16		  *socket);
void SPXSetSocketInAddress(
	SPXAddress_ta *spxAddress,
	uint16		   socket);
char * SPXDisplayAddress( SPXAddress_ta spxAddress);
char * SPXDisplayErrno( SPXHandle_t	*spxHandle);


#endif /* __SPXAPI_H__ */
