/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:sap_lib.h	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sap_lib.h,v 1.11 1994/08/16 18:13:25 vtag Exp $"

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

#include <sys/sap_app.h>

/*
**	Message types for private local machine messages
*/
#define SAP_ADVERTISE_SERVICE	10	/* Advertise my service */
#define SAP_UNADVERTISE_SERVICE	11	/* Advertise my service no longer */
#define SAP_NOTIFY_ME			12	/* Notify me if change */
#define SAP_UNNOTIFY_ME			13	/* Notify me no longer if change */
#define SAP_ACK					14	/* Ack an advertise or notify */
#define SAP_NACK				15	/* Nack an advertise or notify */
#define SAP_ADVERTISE_PERMANENT	16	/* Advertise my service until sap exits */

/*
**	Packet used for SAP_ADVERTISE_SERVICE request.
**	On ACK, the same packet is returned
**	with operation set to SAP_ADVERTISE_ACK
*/
typedef struct SapAdvertise {
	ipxHdr_t	IpxHeader;			/* Ipx header */
    uint16		SapOperation;		/* (Net Order) */
	pid_t		Pid;				/* (Mach Order) Process pid */
									/* On NACK, Pid Type contains status */
	uid_t		Uid;				/* Advertiser's user id */
	uint16		ServerType;			/* (Net Order) Server Type */
    uint8		ServerSocket[IPX_SOCK_SIZE];	/* (Net Order) Socket */
    uint8		ServerName[NWMAX_SERVER_NAME_LENGTH];/* (Net Order) SvrName */
} SapAdvertise_t;

/*
**	Structure used to store a list of a processes advertised servers.
**	This list is maintained so  that we may Unadvertise his services
**	when he issues a ShutdownSAP API
*/
typedef struct AdvertList {
	struct	AdvertList	*next;
	uint8	ServerName[NWMAX_SERVER_NAME_LENGTH];
	uint16	ServerSocket;
	uint16	ServerType;
} AdvertList_t;

/*
**	Structure of the sapouts file which identifies services advertised
**	with the PERMANENT option.  All numerical values are in machine order.
*/
extern const char persistFile[];

#define STARTVAL	0x2A536150
#define ENDVAL		0x7041732A

typedef struct PersistRec {
	uint32			StartRec;
	PersistList_t	ListRec;
	uint32			StopRec;
} PersistRec_t;
	
/*
**	Packet used for NOTIFY_ME request.
**	On ACK, the same packet is returned with operation set to SAP_NOTIFY_ACK
*/
typedef struct SapNotify {
	ipxHdr_t	IpxHeader;		/* Ipx header */
    uint16		SapOperation;	/* (Net Order) Operation set to SAP_NOTIFY_ME */
	pid_t		Pid;			/* (Mach Order) Process pid */
								/* On NACK, Pid Type contains status */
	int			Signal;			/* (Mach Order) Notification signal */
    uint16		ServerType;		/* (Net Order) Server Type */
} SapNotify_t;

/*
**	Minimum sap packet, just contains operation
*/
typedef struct SapOp {
	ipxHdr_t	IpxHeader;		/* Ipx header */
    uint16		SapOperation;	/* (Net Order) Operation set to SAP_NOTIFY_ME */
	pid_t		Status;			/* Same field as pid, returns status */
} SapOp_t;

/*
**	Packet used for GET_SHM_ID request
**	On ACK, the same packet format is returned with id set 
*/
typedef struct SapShmId {
	ipxHdr_t	IpxHeader;		/* Ipx header */
    uint16		SapOperation;	/* (Net Order) Operation set to SAP_NOTIFY_ME */
	pid_t		Status;			/* Same field as pid, returns status */
	int			ShmId;			/* (Mach Order) Shared memory id */
} SapShmId_t;

/*
**	SAP Shared Memory control structure
*/
typedef struct SapShmHdr {
	SAPD		D;				/* Statistics */
	int32	 	LanInfo; 	  	/* Byte offset to Lan Information */
	int32	 	ServerPool;   	/* Byte offset unused server entry pool */
	int32		NameHashSize;	/* Size of Name Hash Table */
	int32		NameHash;		/* Byte Offset to Name Hash Start */
} SapShmHdr_t;

/*
**	Server Entry structure
*/
typedef struct ServerEntry
{
    int32		NameLink;		/* Link to next server name with same hash */
    uint32 		ChangedLink;	/* Link to next changed server */
								/*  Link only valid if in current change list */
	uint32		RevisionStamp;	/* Current time stamp value */
    uint16		ServerType;		/* Server Type (Net order) */
    uint16		HopsToServer;	/* Hops to Server (machine order) */
								/* ServerAddress must be 4 byte aligned */
    uint8		ServerAddress[IPX_ADDR_SIZE];
								/* ServerName is length-preceded in byte 0 !! */
    uint8		ServerName[NWMAX_SERVER_NAME_LENGTH];
	pid_t		LocalPid;		/* Local server pid */
	netInfo_t	N;				/* NetInfo information */
								/* Addresses visible only to sapd processes */
    struct InfoSourceEntry  *SourceListLink;	/* Link to list of sources */
} ServerEntry_t;

typedef struct SapRespHeader
{
	ipxHdr_t				ipxHdr;	
	uint16					Operation;
} SapRespHeader_t;

/* Prototypes for functions in sapreq.c called by sap_lib.c */
extern int SAPRequestServers( uint16, uint16, int *, SAPI *, int );
