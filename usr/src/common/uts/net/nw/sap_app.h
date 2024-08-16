/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/sap_app.h	1.12"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_SAP_APP_H  /* wrapper symbol for kernel use */
#define _NET_NW_SAP_APP_H  /* subject to change without notice */

#ident	"$Id: sap_app.h,v 1.18 1994/09/24 12:57:48 vtag Exp $"

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
#include <util/types.h>
#include <net/nw/ipx_app.h>
#include <net/nw/ripx_app.h>
#else
#include <sys/types.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#endif /* _KERNEL_HEADERS */

/*
**	The following structure is used by processes accessing the
**	Sap Server on the local machine.  This data does not access
**	the network but instead uses information available locally on
**	the machine.
**
**	Note: All values are returned in machine order,
**	including all values in the netInfo_t structure.
**		The only fields that will be in network order are:
**			serverName		(Strings are always the same order anyway)
**			serverAddress	(12 byte IPX address)
*/	
typedef struct sap_info {
        uint16		serverType;		/* Assigned type of server */
        uint8		serverName[NWMAX_SERVER_NAME_LENGTH]; /* Server Name */
        ipxAddr_t	serverAddress;	/* Server address, net order */
        uint16		serverHops;		/* Number of intermediate nets mach order */
		netInfo_t	netInfo;		/* Info about the net to access server */
									/* All values returned in machine order */
} SAPI, *SAPIP;

/*
**	The sap statistics data structure, all data in machine order
**	Returned as response to the SAP API SAPGetStatistics.
*/
typedef struct sap_data {

	/* Control Information */
	time_t		StartTime;		/* Time started in seconds since epoch */
	pid_t		SapPid;			/* Pid of Sap Process */
	uint16		Lans;			/* Number of Connected Lans, including lan 0 */
	uint8		MyNetworkAddress[IPX_ADDR_SIZE];/* My network address */
	int32		ConfigServers;	/* Total configured server entries */
	clock_t		RevisionStamp;	/* Revision of last update */
	int32	 	ServerPoolIdx; 	/* Index to next unused server entry */
	uint32		ProcessesToNotify;/* Num of processes to notify of change */

	/* Misc Information */
	uint32		NotificationsSent;/* Notifications sent to processes */

	/* Packets Received */
	uint32		TotalInSaps;	/* Total sap pkts received */
	uint32		GSQReceived;	/* General Server Queries received */
	uint32		GSRReceived;	/* General Server Replies received */
	uint32		NSQReceived;	/* Nearest Server Queries received */
	uint32		SASReceived;	/* Local Sap Advertise a Server received */
	uint32		SNCReceived;	/* Local Sap Notify of Change received */
	uint32		GSIReceived;	/* Local Sap Get Shared Memory Id */
	uint32		NotNeighbor;	/* Packet Receieved, Source Not on LAN */
	uint32		EchoMyOutput;	/* Packet Receieved, Echo of packet we sent */
	uint32		BadSizeInSaps;	/* Sap pkts received, bak pkt size */
	uint32		BadSapSource;	/* Sap pkts received, sap source bad */

	/* Rip-Sap Interaction */
	uint32		TotalInRipSaps;	/* Total (rip network down) pkts received */
	uint32		BadRipSaps;		/* Bad (rip network down) pkts received */
	uint32		RipServerDown;	/* Server set to down from rip interaction */

	/* Packets Sent */
	uint32		TotalOutSaps;	/* Total sap pkts sent */
	uint32		NSRSent;		/* Nearest Server Responses sent */
	uint32		GSRSent;		/* General Server Replies sent */
	uint32		GSQSent;		/* General Server Queries sent */
	uint32		SASAckSent;		/* Local Sap Advertise Server Request Acks */
	uint32		SASNackSent;	/* Local Sap Advertise Server Request Nacks */
	uint32		SNCAckSent;		/* Local Sap Notify of Change Acks */
	uint32		SNCNackSent;	/* Local Sap Notify of Change Nacks */
	uint32		GSIAckSent;		/* Local Sap Get Shared Memory Id Acks */
	uint32		BadDestOutSaps;	/* Sap pkts sent bad destination net */

	/* Memory Allocation Errors */
	uint32		SrvAllocFailed;	/* Number server alloc request failures */
	uint32		MallocFailed;	/* Number Malloc request failures */

} SAPD, *SAPDP;

/*
**	Sap Lan data structure.  Returned in response to the
**	SAP API SapGetLanData.
*/
typedef struct SapLanData {
	uint16		LanNumber;		/* Lan Number */
	uint16		UpdateInterval;	/* Periodic Update Interval in seconds */
	uint16		AgeFactor;		/* Num Periodic Intervals missed to down srvr */
	uint16		PacketGap;		/* Time in Milliseconds between packets */
								/* Time is zero for a WAN, nonzero for a LAN */
	int32		Network;		/* NetWork number for htis lan */
	int32		LineSpeed;		/* Linespeed in MBS.  If sign bit set KBS */
								/* Currently always zero */
	uint32		PacketSize;		/* Packet size that will be sent on this lan */
	uint32		PacketsSent;	/* Packets sent */
	uint32		PacketsReceived;/* Packets received */
	uint32		BadPktsReceived;/* Bad Packets received */
} SAPL, *SAPLP;

/*
**	Structure used by APIs to retrieve records stored in sapouts file.
**	which identifies services advertised with the PERMANENT option
**	Type and Socket values are in machine order.
*/
typedef struct PersistList {
	uint8	ServerName[NWMAX_SERVER_NAME_LENGTH];
	uint16	ServerType;
	uint16	ServerSocket;
} PersistList_t;

/*
**	The following are SAP API functions are used by processes accessing
**	the Sap Server on the local machine.
*/

#if defined( __STDC__) || defined(__cplusplus)

#ifdef __cplusplus
extern "C" {
#endif
extern int SAPMapMemory( void);
extern void SAPUnmapMemory( void);
extern int SAPStatistics( SAPDP );
extern int SAPGetAllServers( uint16, int *, SAPIP, int);
extern int SAPGetChangedServers( uint16, int *, SAPIP, int, uint32, uint32 *);
extern int SAPGetNearestServer( uint16, SAPIP);
extern int SAPAdvertiseMyServer( uint16, uint8 *, uint16, int);
extern int SAPGetServerByName( uint8 *, uint16, int *, SAPIP, int);
extern int SAPGetServerByAddr(ipxAddr_t *, uint16, int *, SAPIP, int);
extern int SAPListPermanentServers(int *, PersistList_t *, int);
extern int SAPNotifyOfChange(int, void (*func)(int), uint16);
extern int SAPGetLanData(int, SAPLP);
extern int SAPPerror(int, char *, ...);
extern int EnableIpxServerMode( void);
extern int DisableIpxServerMode( void);
#ifdef __cplusplus
}
#endif

#else
extern int SAPMapMemory();
extern void SAPUnmapMemory();
extern int SAPStatistics();
extern int SAPGetAllServers();
extern int SAPGetChangedServers();
extern int SAPGetNearestServer();
extern int SAPAdvertiseMyServer();
extern int SAPGetServerByName();
extern int SAPGetServerByAddr();
extern int SAPListPermanentServers();
extern int SAPNotifyOfChange();
extern int SAPGetLanData();
extern int SAPPerror();
extern int EnableIpxServerMode();
extern int DisableIpxServerMode();
#endif

/*
**	Constants used for SAPNotifyOfChange() and SAPAdvertiseMyServer()
*/
#define SAP_ADVERTISE				1	/* Advertise my service */
#define SAP_STOP_ADVERTISING		2	/* End Advertising my service */
#define SAP_ADVERTISE_FOREVER		3	/* Advertise beyond life of process */
#define SAP_STOP_NOTIFICATION		-1	/* End notification of changes */

/*
**	Common server types
*/
#define FILE_SERVER_TYPE 		0x0004		/* File server SAP type. */
#define PRINT_SERVER_TYPE 		0x0047		/* Print Server */
#define BETRIEVE_SERVER_TYPE	0x004B		/* BTrieve Server */
#define ACCESS_SERVER_TYPE 		0x0098		/* NetWare Access Server */
#define OLD_NVT_SERVER_TYPE		0x009E		/* NVT over NVT protocol SAP type */
#define I386_SERVER_TYPE 		0x0107		/* 386 NetWare */
#define SPX_NVT_SERVER_TYPE		0x0247		/* NVT over SPX protocol SAP type */
#define TIME_SYNC_SERVER_TYPE	0x026B		/* Time Syncronization */
#define DIRECTORY_SERVER_TYPE	0x0278		/* Directory Server */
#define UNIXWARE_REMOTE_APP_TYPE 0x03E1		/* UnixWare Remote App Server */
#define UNIXWARE_INSTALL_TYPE	0x03EE		/* UnixWare 2.0 Install Server */
#define UNIXWARE_TYPE 			0x03E4		/* UnixWare Platform */
#define ALL_SERVER_TYPE			0xFFFF		/* Return all server types. */

/*
**	The following errors are returned by the SAP API functions
**	All errors are returned as negative numbers.
*/
#define SAPL_SERVTYPE	1	/* Server type cannot be zero */
#define SAPL_SERVNAME	2	/* Server name too long or too short */
#define SAPL_INVALFUNC	3	/* Invalid advertise function */ 
#define SAPL_NORESP		4	/* Sap daemon not responding */
#define SAPL_DUPCALLBACK 5	/* Callback function already identified */
#define SAPL_SIGNAL 	6	/* Error trying to setup signal, see errno */
#define SAPL_NWCM		7	/* function NWCMGetConfigFIlePath failed */
#define SAPL_INVALSOCK	8	/* Socket number may not be 0 */
#define SAPL_ENOMEM		9	/* Count not do local NWALLOC */
#define SAPL_NOT_SUPPORTED	10	/* Function not supported, SAPD not running */
#define SAPL_TRYAGAIN	11	/* Sapouts file in use, try again */
#define SAPL_UNUSED_12	12	/* Unused */
#define SAPL_UNUSED_13	13	/* Unused */
#define SAPL_UNUSED_14	14	/* Unused */
#define SAPL_UNUSED_15	15	/* Unused */
#define SAPL_UNUSED_16	16	/* Unused */
#define SAPL_UNUSED_17	17	/* Unused */
#define SAPL_UNUSED_18	18	/* Unused */
#define SAPL_UNUSED_19	19	/* Unused */
#define SAPL_UNUSED_20	20	/* Unused */

/*
**	SAP API function errors
**	The following require interpretation of errno for complete error message
*/
#define SAPL_ERRNO_START 21	/* Start of errors that require errno */
#define SAPL_SOCKET		21	/* ioctl IPX_SET_SOCKET failed */
#define SAPL_FTOK		22	/* functions FTOK failed */
#define SAPL_SHMGET		23	/* function shmget failed */
#define SAPL_SHMAT		24	/* function shmat failed */
#define SAPL_IPXOPEN	25	/* Open of /dev/ipx failed */
#define SAPL_GETMSG	 	26	/* getmsg on /dev/ipx failed */
#define SAPL_PUTMSG 	27	/* Error trying send message to sap daemon */
#define SAPL_OSAPOUTS	28	/* Error opening sapouts file */
#define SAPL_RWSAPOUT	29	/* Sapouts file read/write error */
#define SAPL_BADSAPOUT	30	/* Sapouts file read/write error, file truncated */
#define SAPL_UNUSED_31	31	/* Unused */
#define SAPL_UNUSED_32	32	/* Unused */
#define SAPL_UNUSED_33	33	/* Unused */
#define SAPL_UNUSED_34	34	/* Unused */
#define SAPL_UNUSED_35	35	/* Unused */
#define SAPL_UNUSED_36	36	/* Unused */
#define SAPL_UNUSED_37	37	/* Unused */
#define SAPL_UNUSED_38	38	/* Unused */
#define SAPL_UNUSED_39	39	/* Unused */

/*
**	Sap Agent Error Numbers, passed through by the Sap API functions
*/
#define SAPD_START		40	/* Start of Sap Daemon Error numbers */
#define SAPD_ENOMEM		40	/* Not enough space to allocate service */
#define SAPD_BUSY		41	/* Server already advertised by another process */
#define SAPD_NOFIND		42	/* Server to unadvertise not found */
#define SAPD_NOPERM		43	/* can't unadvertise service you didn't advertise */
#define SAPD_PID_INVAL	44	/* Pid is invalid, library internal error */
#define SAPD_NAME_ZERO	45	/* Name specified has length zero */
#define SAPD_INSUF_PERM	46	/* Insufficient permission to execute this function. */
#define SAPD_UNUSED_47	47	/* Unused */
#define SAPD_UNUSED_48	48	/* Unused */
#define SAPD_UNUSED_49	49	/* Unused */

/*
**	The following errors are returned by the SAP API ServerMode functions
**	All errors are returned as negative numbers.
*/
#define SER_INVRTYPE	50	/* Invalid ROUTER type */
#define SER_ROPEN		51	/* open of RIPX failed */
#define SER_RIOCTL		52	/* RIPX_SET_ROUTER_TYPE ioctl to RIPX failed */
#define SER_NWCMGD		53	/* NWCMGetConfigDirPath call failed */
#define SER_NWCMGP		54	/* NWCMGetConfigParam call failed */
#define SER_FSAP		55	/* fork of sapd failed */
#define SER_ESAP		56	/* execl of sapd failed */
#define SER_NPSD		57	/* IPX Stack Daemon not active */
#define SER_SERV		58	/* Wrong server_type for function requested */
#define SER_INTADDR		59	/* IPX internal_network set cannot change modes */
#define SER_FNUCSAP		60	/* fork of nucsapd failed */
#define SER_UNUSED_61	61	/* Unused */
#define SER_UNUSED_62	62	/* Unused */
#define SAP_ERRNO_LAST    63  /* Last valid SAPPerror value


/**************************************************************************/
/*
**	The following are definitions of SAP data is it is seen on the wire.
**
**	Packets are seen as an IPX header (IPX_HDR_SIZE)
**	Followed by SAP data as described by the structures below.
**
**	Sap Packet Operations
*/
#define SAP_GSQ         1           /* General Server Query */
#define SAP_GSR         2           /* General Server Response */
#define SAP_PIB         2           /* Periodic Identification Broadcast */
#define SAP_NSQ         3           /* Nearest Server Query */
#define SAP_NSR         4           /* Nearest Server Response */

/*
**	Magic "hops" value for shutdown advertising.
*/
#define SAP_DOWN    	16              

/*
**	Packet type and socket for SAP
*/
#define SAP_SAS         0x0452		/* SAP Server Advertising socket. */
#define SAP_PACKET_TYPE 0x04

/* Some sockets numbers */
#define NVT_SOCKET				((uint16)0x08063)

/*
**	BROADCAST interval determines how often to send our view of the
**	world, as well as the interval to "age" servers.  When we have
**	not received a GSR packet about a server in MAX_FAIL_COUNT
**	age intervals, it is marked SAP_SHUTDOWN hops, and we inform the
**	world it is down.
**	
*/
#define SAP_TIMER_MULTIPLIER  	30	/* Multiplier for periodic brdcsts in sec */
#define SAP_MAX_SAPS_PER_PACKET		7	/* Max SAPS struct per packet. */
#define SAP_INFO_LENGTH				sizeof(struct saps_s)

/*
**
**	SAP server information structure gives server information
**	and is found in GSR, and NSR packets
*/
typedef struct saps_s {
        uint16		serverType;		/* Assigned type of server. */
        uint8		serverName[NWMAX_SERVER_NAME_LENGTH];         
        ipxAddr_t	serverAddress;	/* Server address. */
        uint16		serverHops;		/* Number of intermediate nets hi-lo. */
} SAPS, *SAPSP;

/*
**	SAP Periodic Broadcast Structure (PIB packet) used to inform sap that my
**	service is still alive.  Must be sent every minute or so.
**	You could actually use the GSR packet and advertise several services
**	at once if you need to.  Actually, though, it is better to use the
**	API SAPAdvertiseMyServer and dispense with building packets, and sending
**	them periodically.  Just do the SAPAdvertiseMyServer once and be done
**	with it.
*/
typedef struct sapb_s {
        uint16  sapOperation;           /* Type of SAP packet. */
        SAPS    serverInfo;             /* SAP Server information array. */
} SAPB, *SAPBP;

/*
**	SAP query packet (GSQ or NSQ) - Don't use this, 
**	use instead the API SAPGetKnownServers or SAPGetChangedServers for
**	all or changed information, and use SAPGetNearestServer for a NSQ.
*/
typedef struct sapq_q {
        uint16 sapOperation;	/* query operation number */
        uint16 serverType;		/* server type wanted hi-lo */
} SAPQ, *SAPQP;

/*
**	Query response max packet.  (NSR or GSR packet)
**	with up to MAX_SAPS_PER_PACKET SAPS structures allowed in the packet.
*/
typedef struct sapqr_q {
        uint16 sapOperation;
		SAPS    sap[SAP_MAX_SAPS_PER_PACKET];
} SAPQR, *SAPQRP;
#endif /* _NET_NW_SAP_APP_H */
