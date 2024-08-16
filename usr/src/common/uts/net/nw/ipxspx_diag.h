/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipxspx_diag.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_IPXSPX_DIAG_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPXSPX_DIAG_H  /* subject to change without notice */

#ident	"$Id: ipxspx_diag.h,v 1.4 1994/08/16 18:14:06 vtag Exp $"

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
 *
 */

#ifdef _KERNEL_HEADERS
#include	<net/nw/sap.h>
#else
#include	<sys/sap.h>
#endif /* _KERNEL_HEADERS */

/****************************************************
 * IPX diagnostics structs and defines used in common
 * with SPX diagnostics, applications, and daemons.
 ****************************************************/

#define DIAGS_VER_MAJ 1
#define DIAGS_VER_MIN 0
#define MAX_EXCLUSIONS 80

/* IPX Configuration Request
 * and exclusion list structures.
 */
typedef struct StructExclusionListEntry
{	uint8	nodeAddress[6];
} exclusionEntry_t;

typedef struct StructExclusionPacket
{	uint8				exclusionCount;
	exclusionEntry_t	exclList[MAX_EXCLUSIONS];
} exclusionList_t;

typedef struct ipxConfigReq
{	ipxHdr_t	header;
	exclusionList_t		excludeList;
} ipxConfigRequest_t;


typedef struct structConfiguration
{	uint8	majorVersion;
	uint8	minorVersion;
	uint16	spxDiagSock;
	uint8	numberOfComponents;
	uint8	componentStructure[IPX_MAX_PACKET_DATA - 5];
} ipxConfiguration_t;

/* IPX Configuration Response
 */
typedef struct StructConfigurationResponse
{	ipxHdr_t	header;
	ipxConfiguration_t	config;
} ipxConfigResponse_t;

#define MAX_LOCAL_NETWORKS 4

#define LAN_BOARD 0
#define VIRTUAL_BOARD 1
#define REDIRECTED_REMOTE_LINE 2

#define IPX_SPX_COMPONENT 0			/* spx active */
#define BRIDGE_DRIVER_COMPONENT 1	/* vs shell */
#define SHELL_DRIVER_COMPONENT 2	/* don't use PNW/NWU */
#define SHELL_COMPONENT 3			/* don't use PNW/NWU */
#define VAP_SHELL_COMPONENT 4		/* don't use PNW/NWU */
#define EXTERNAL_BRIDGE 5			/* we route */
#define INTERNAL_BRIDGE 6			/* we consume */
#define BRIDGE_COMPONENT 5			/* we route */
#define FILE_SERVER_COMPONENT 6
#define NONDEDICATED_IPX_SPX_COMPONENT 7	/* don't use PNW/NWU */
#define IPX_ONLY 8					/* spx not active */

typedef struct structDriver
{	uint8	localNetworkType;
	uint8	network[4];
	uint8	node[6];
} lanDriver_t;

typedef struct structBridge
{	uint8		numberOfNets;
	lanDriver_t		drivers[MAX_LOCAL_NETWORKS];
} bridgeDrivers_t;

/****************************************************
 * SPX diagnostics structs and defines available for
 * applications and daemons.
 ****************************************************/
/* IPX/SIPX component defines
 */
#define	IPXSPX_VERSION	0
#define	IPXSPX_RTN_IPX_STATS	1
#define	IPXSPX_RTN_SPX_STATS	2
#define	IPXSPX_START_SEND	3
#define	IPXSPX_ABORT_SEND	4
#define	IPXSPX_START_PKT_CNT	5
#define	IPXSPX_RTN_RCVD_PKT_CNT	6

#define IPX_MAJ_VER		3
#define IPX_MIN_VER		11
#define SPX_MAJ_VER		3
#define SPX_MIN_VER		11

/* Bridge Driver component defines
 */
#define	BDRIVER_RTN_STATUS	0
#define	BDRIVER_RTN_CONFIG	1
#define	BDRIVER_RTN_DIAG_STATS	2

#define	BDRIVER_BOARD_RUNNING	0
#define	BDRIVER_BOARD_NOEXIST	1
#define	BDRIVER_BOARD_DEAD	2

/* Bridge component defines
 */
#define	BRIDGE_RTN_STATS	0
#define	BRIDGE_RTN_LOCAL_TABLES	1
#define	BRIDGE_RTN_ALL_KNOWN_NETS	2
#define	BRIDGE_RTN_SPECIFIC_NET_INFO	3
#define	BRIDGE_RTN_ALL_KNOWN_SERVERS	4
#define	BRIDGE_RTN_SPECIFIC_SERVER_INFO	5
#define	BRIDGE_RESET_LAN_BOARD	6
#define	BRIDGE_REINIT_ROUTING_TABLES	7

typedef struct spxDiagRequest {
	uint8	index;
	uint8	type;
	uint8	data[576 - 42 - 2];	/* union ? */
} spxDiagReq_t;

typedef struct spxDiagResponse {
	uint8	cCode;
	uint8	interval[4];
	uint8	data[576 - 42 - 5];	/* union ? */
} spxDiagResp_t;

typedef struct structIpxStats
{	uint8	sendPacketCount[4];
	uint8	malformedPacketCount[2];
	uint8	getECBRequestCount[4];
	uint8	getECBFailureCount[4];
	uint8	AESEventCount[4];
	uint8	postponedAESEventCount[2];
	uint8	maxConfiguredSocketsCount[2];
	uint8	maxOpenSocketsCount[2];
	uint8	openSocketFailureCount[2];
	uint8	listenECBCount[4];
	uint8	ECBCancelFailureCount[2];
	uint8	findRouteFailureCount[2];
} IpxStats_t;

typedef struct structSpxStats
{	uint8	maxConnectionsCount[2];
	uint8	maxUsedConnectionsCount[2];
	uint8	establishConnectionRequest[2];
	uint8	establishConnectionFailure[2];
	uint8	listenConnectionRequestCount[2];
	uint8	listenConnectionFailureCount[2];
	uint8	sendPacketCount[4];
	uint8	windowChokeCount[4];
	uint8	badSendPacketCount[2];
	uint8	sendFailureCount[2];
	uint8	abortConnectionCount[2];
	uint8	listenPacketCount[4];
	uint8	badListenPacketCount[2];
	uint8	incomingPacketCount[4];
	uint8	badIncomingPacketCount[2];
	uint8	suppressedPacketCount[2];
	uint8	noSessionListenECBCount[2];
	uint8	watchdogDestroySessionCount[2];
} SpxStats_t;

typedef struct SPReq
{
	uint8	target[12];
	uint8	immediateAddress[6];
	uint16	numberOfPkts;
	uint8	ticksPerInterval;
	uint8	pktsPerInterval;
	uint16	pktSize;
	uint16	changeSize;
} sendPktsRequest_t;

typedef struct StructDriverConf
{
	uint8	networkAddress[4];
	uint8	nodeAddress[6];
	uint8	LANMode;
	uint8	nodeAddressType;
	uint8	maxDataSize[2];
	uint8	reserved1[2];
	uint8	LANHardwareID;
	uint8	transportTime[2];
	uint8	reserved2[11];
	uint8	majorVersion;
	uint8	minorVersion;
	uint8	ethernetFlagBits;
	uint8	selectedConfiguration;
	uint8	LANDescription[80];
	uint8	IOAddress1[2];
	uint8	IODecodeRange1[2];
	uint8	IOAddress2[2];
	uint8	IODecodeRange2[2];
	uint8	memoryAddress1[3];
	uint8	memoryDecodeRange1[2];
	uint8	memoryAddress2[3];
	uint8	memoryDecodeRange2[2];
	uint8	interruptIsUsed1;
	uint8	interruptLine1;
	uint8	interruptIsUsed2;
	uint8	interruptLine2;
	uint8	DMAIsUsed1;
	uint8	DMALine1;
	uint8	DMAIsUsed2;
	uint8	DMALine2;
	uint8	microChannelFlagBits;
	uint8	reserved3;
	uint8	textDescription[80];
} bDriverConfig_t;

typedef struct StructDriverStat
{
	uint8	 driverVersion[2];
	uint8	 statisticsVersion[2];
	uint8	 totalTxPacketCount[4];
	uint8	 totalRxPacketCount[4];
	uint8	 noECBAvailableCount[2];
	uint8	 packetTxTooBigCount[2];
	uint8	 packetTxTooSmallCount[2];
	uint8	 packetRxOverflowCount[2];
	uint8	 packetRxTooBigCount[2];
	uint8	 packetRxTooSmallCount[2];
	uint8	 packetTxMiscErrorCount[2];
	uint8	 packetRxMiscErrorCount[2];
	uint8	 retryTxCount[2];
	uint8	 checksumErrorCount[2];
	uint8	 hardwareRxMismatchCount[2];
	uint8	 numberOfCustomVariables[2];
	uint8	 variableData[495];
/*	uint8	 variableData[1]; */
} bDriverStats_t;

typedef struct StructRoutingInfo
{	uint8	routerForwardingAddress[6];
	uint8	routerBoardNumber;
	uint8	reserved[2];
	uint8	routeHops;
	uint8	routeTime[2];
} RoutingInfo_t;

typedef struct StructSpecificNetInfo
{	uint8	networkAddress[4];
	uint8	hopsToNet;
	uint8	reservedA[7];
	uint8	routeTimeToNet[2];
	uint8	numberOfKnownRouters[2];
	RoutingInfo_t	routingInfo[1];
	/* was MAX_ROUTES, but we only know best in 3.11 ?? */
} SpecificNetworkInfo_t;

typedef struct strSrvrInfo
{	uint8	serverType[2];
	uint8	serverName[NWMAX_SERVER_NAME_LENGTH];
} serverInfo_t;

typedef struct structAllKnownServers
{	uint8	numberOfServers[2];
	serverInfo_t	serverInfo[10];
} allKnownServers_t;

typedef struct StructRouteSourceInfo
{	uint8	routeSourceAddress[6];
	uint8	routeHopsToSource[2];
	uint8	reserved[2];
} routeSourceInfo_t;

typedef struct {
		uint8	serverType[2];
		char	serverName[NWMAX_SERVER_NAME_LENGTH]; 
		uint8	network[4];
		uint8	node[6];
		uint8	socket[2];
		uint8	hops[2];
} sapServerInfo_t;

typedef struct StrSpecSrvrInfo
{/* serverInfo_t	serverInfo;
	uint32	serverNet;
	uint8	serverNode[IPX_NODE_SIZE];
	uint16	serverSock;
	uint16	hopsToServer;
	*/
	sapServerInfo_t	sapId;
	uint8	reserved1[2];
	uint8	numberOfRoutes[2];
	routeSourceInfo_t	routeSourceInfo[1];
	/* was MAX_ROUTES, but we only know best in 3.11 ?? */
} specificServerInfo_t;

/* This should have been in sap.h (sap_app.h)
 */
typedef struct {
	uint16	replyType;
	SAP_ID_PACKET	sapId[SAP_MAX_UPDATE];
} sapUpdatePacket_t;

#endif /* _NET_NW_IPXSPX_DIAG_H */
