/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/sap.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_SAP_H  /* wrapper symbol for kernel use */
#define _NET_NW_SAP_H  /* subject to change without notice */

#ident	"$Id: sap.h,v 1.3 1994/08/16 18:14:09 vtag Exp $"

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

#ifdef _KERNEL_HEADERS
#include <net/nw/sap_app.h>
#else
#include <sys/sap_app.h>
#endif /* _KERNEL_HEADERS */

/*
**	Boundry values for arrays and other misc
**	magic numbers
*/
#define TRACK_ON		/* if we want tracking on or not */

#define	SAP_BUFFER_SIZE	1024
#define SAP_MAX_UPDATE	SAP_MAX_SAPS_PER_PACKET	/* 7 servers per update */	
#define SAP_ID_LENGTH	SAP_INFO_LENGTH			/* length of SAP ID packet */

#define	SAP_SOCKET	((uint16)SAP_SAS)
#define NCP_SOCKET	((uint16)0x0451)

/*	Server Types */
#define	FILE_SERVER			((uint16)FILE_SERVER_TYPE)
#define ALL_SERVER_TYPES	((uint16)ALL_SERVER_TYPE)

/* SAP request types */
#define	NEAREST_SERVER_REQUEST		((uint16)SAP_NSQ)
#define GENERAL_SERVICE_REQUEST		((uint16)SAP_GSQ)

/* SAP reply types */
#define GENERAL_SERVICE_REPLY		((uint16)SAP_GSR)
#define NEAREST_SERVER_REPLY		((uint16)SAP_NSR)

/* Hop count variables */
#define	SHUTDOWN_BROADCAST			((uint16)SAP_SHUTDOWN)

typedef struct {
		uint16	requestType;
		uint16	serverType;
}	SAP_REQUEST_PACKET;

typedef struct {
		uint16	serverType;
		char	serverName[NWMAX_SERVER_NAME_LENGTH]; 
		uint8	network[IPX_NET_SIZE];
		uint8	node[IPX_NODE_SIZE];
		uint16	socket;
		uint16	hops;
}	SAP_ID_PACKET;

typedef struct {
		uint16			replyType; 
		SAP_ID_PACKET	sid;
}	SAP_REPLY_PACKET;

union SAPReadUnion {
		char				SAPBuffer[SAP_BUFFER_SIZE];

		struct 
		{
			uint16	requestType;
			SAP_ID_PACKET	sid;
		} srp;
};
#endif /* _NET_NW_SAP_H */
