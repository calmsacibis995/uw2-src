/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwumpsd.h	1.8"
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
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwumpsd.h,v 1.8.2.1 1994/10/13 21:47:08 rbell Exp $
 */

/****************************************************************************
** Source file:   nwumpsd.h
**
** Description:   
**
** Author:   Rick Bell
**
** Date Created:  March 16, 1993
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

#ifndef _NWUMPS_NWUMPSD_H_
#define _NWUMPS_NWUMPSD_H_

/*
** Strings for NWUMPS Configuration Tokens
*/
#define NWUMPS_SERVER_NAME "server_name"
#define NWUMPS_FLAG        "nwumps"
#define NWUMPS_PROGRAM     "nwumps_daemon"
#define NWUMPS_BIN_DIR     "binary_directory"
#define NWUMPS_ETC_DIR     "nm_etc_directory"
#define NWUMPS_DIAG_FLAG   "diagnostics"
#define NWUMPS_LAN         "lan_"
#define NWUMPS_NETWORK     "_network"
#define NWUMPS_ADAPTER     "_adapter"
#define NWUMPS_ADAPTER_TYPE "_adapter_type"
#define NWUMPS_FRAME_TYPE  "_frame_type"



#define NWUMPS_DEBUG       3

#define DATA_FRESHNESS     5    /* The data had to been acquired in this
                                   last 5 seconds or it is refreshed. 
                                */

#define NWUMPS_REFRESH     300	/* This is in seconds. This is for large
                                   reallocation of tables.
                                */

#define SMUX_NWUMPS_FD     0
#define IPX_NWUMPS_FD      1
#define NUM_NWUMPS_FD      2

#define NWUMPS_NO          1
#define NWUMPS_YES         2

#define  NWUMPS_OFF        1
#define  NWUMPS_ON         2
#define  NWUMPS_AUTO       3

#define  NWUMPS_DOWN       1
#define  NWUMPS_UP         2
#define  NWUMPS_SLEEPING   3

#define  NWUMPS_UNKNOWN    1
#define  NWUMPS_SPX        2
#define  NWUMPS_SPXII      3

#define  NWUMPS_PROTO_OTHER    1
#define  NWUMPS_PROTO_LOCAL    2
#define  NWUMPS_PROTO_RIP      3
#define  NWUMPS_PROTO_NLSP     4
#define  NWUMPS_PROTO_STATIC   5
#define  NWUMPS_PROTO_SAP      6

#define  NWUMPS_IPX_DELAY  200

#define  NWUMPS_BROADCAST_TYPE   2

#define  NWUMPS_TYPE_SIZE  2
#define  NWUMPS_NAME_SIZE  48

#define  NWUMPS_NOT_SUPPORTED 0

#define  NWUMPS_SPX_TYPE        1
#define  NWUMPS_SPXII_TYPE      2

#define  NWUMPS_POLL_ERROR_COUNT 20

#define  NWUMPS_IPX_TRAP_CIRCUIT_DOWN 1
#define  NWUMPS_IPX_TRAP_CIRCUIT_UP   2

/* External Variables */

/* NetWare Diagnostics Groups */

/* System Group               */
/* Diagnostic System Table    */

#define  nwuSPXSysInstance                61111
#define  nwuSPXSysState                   61112
#define  nwuSPXSysMajorVer                61113
#define  nwuSPXSysMinorVer                61114
#define  nwuSPXSysUpTime                  61115
#define  nwuSPXSysMaxOpenSessions         61116
#define  nwuSPXSysUsedOpenSessions        61117
#define  nwuSPXSysMaxUsedOpenSessions     61118
#define  nwuSPXSysConnectReqCounts        61119
#define  nwuSPXSysConnectErrors           611110
#define  nwuSPXSysListenReqs              611111
#define  nwuSPXSysListenErrors            611112
#define  nwuSPXSysOutPackets              611113
#define  nwuSPXSysOutErrors               611114
#define  nwuSPXSysInPackets               611115
#define  nwuSPXSysInErrors                611116

/* Circuit Group              */
/* SPX Circuit Table          */

#define  nwuSPXCircSysInstance            62111
#define  nwuSPXCircIndex                  62112
#define  nwuSPXCircState                  62113
#define  nwuSPXCircStartTime              62114
#define  nwuSPXCircRetryCounts            62115
#define  nwuSPXCircRetryTime              62116
#define  nwuSPXCircLocNetNumber           62117
#define  nwuSPXCircLocNode                62118
#define  nwuSPXCircEndNetNumber           62119
#define  nwuSPXCircEndNode                621110
#define  nwuSPXCircType                   621111
#define  nwuSPXCircIPXChkSum              621112
#define  nwuSPXCircSndWinSize             621113
#define  nwuSPXCircSndPktSize             621114
#define  nwuSPXCircSndMsgCounts           621115
#define  nwuSPXCircSndPktCounts           621116
#define  nwuSPXCircSndNAKs                621117
#define  nwuSPXCircSndErrorCtrs           621118
#define  nwuSPXCircRcvWinSize             621119
#define  nwuSPXCircRcvPktSize             621120
#define  nwuSPXCircRcvPktCounts           621121
#define  nwuSPXCircRcvNAKs                621122
#define  nwuSPXCircRcvErrorCtrs           621123
#define  nwuSPXCircRcvPktQues             621124
#define  nwuSPXCircRcvPktSentUps          621125

/* NetWare Diagnostics Groups */
/* System Group               */
/* Diagnostic System Table    */

#define  nwuDiagSysInstance               71111
#define  nwuDiagSysState                  71112
#define  nwuDiagSysMajorVer               71113
#define  nwuDiagSysMinorVer               71114
#define  nwuDiagSysUpTime                 71115

/* Circuit Group              */
/* Diagnostic Circuit Table   */

#define  nwuDiagCircSysInstance           72111
#define  nwuDiagCircIndex                 72112
#define  nwuDiagCircState                 72113
#define  nwuDiagCircIPXSPXReqs            72114
#define  nwuDiagCircLanDvrReqs            75115
#define  nwuDiagCircFileSrvReqs           75116
#define  nwuDiagCircUnknownReqs           75117
#define  nwuDiagCircSPXDiagSocket         75118
#define  nwuDiagCircTimeOfLastReq         75119

/* IPX MIB              */
/* System Group         */
/* Basic System Table   */

#define  ipxBasicSysInstance              111111
#define  ipxBasicSysExistState            111112
#define  ipxBasicSysNetNumber             111113
#define  ipxBasicSysNode                  111114
#define  ipxBasicSysName                  111115
#define  ipxBasicSysInReceives            111116
#define  ipxBasicSysInHdrErrors           111117
#define  ipxBasicSysInUnknownSockets      111118
#define  ipxBasicSysInDiscards            111119
#define  ipxBasicSysInBadChecksums        1111110
#define  ipxBasicSysInDelivers            1111111
#define  ipxBasicSysNoRoutes              1111112
#define  ipxBasicSysOutRequests           1111113
#define  ipxBasicSysOutMalformedRequests  1111114
#define  ipxBasicSysOutDiscards           1111115
#define  ipxBasicSysOutPackets            1111116
#define  ipxBasicSysConfigSockets         1111117
#define  ipxBasicSysOpenSocketFails       1111118

/* Advanced System Table   */
#define  ipxAdvSysInstance                111211
#define  ipxAdvSysMaxPathSplits           111212
#define  ipxAdvSysMaxHops                 111213
#define  ipxAdvSysInTooManyHops           111214
#define  ipxAdvSysInFiltered              111215
#define  ipxAdvSysInCompressDiscards      111216
#define  ipxAdvSysNETBIOSPackets          111217
#define  ipxAdvSysForwPackets             111218
#define  ipxAdvSysOutFiltered             111219
#define  ipxAdvSysOutCompressDiscards     1112110
#define  ipxAdvSysCircCount               1112111
#define  ipxAdvSysDestCount               1112112
#define  ipxAdvSysServCount               1112113

/* Circuit Group  */
/* Circuit Table  */
#define  ipxCircSysInstance               112111
#define  ipxCircIndex                     112112
#define  ipxCircExistState                112113
#define  ipxCircOperState                 112114
#define  ipxCircIfIndex                   112115
#define  ipxCircName                      112116
#define  ipxCircType                      112117
#define  ipxCircDialName                  112118
#define  ipxCircLocalMaxPacketSize        112119
#define  ipxCircCompressState             1121110
#define  ipxCircCompressSlots             1121111
#define  ipxCircStaticStatus              1121112
#define  ipxCircCompressedSent            1121113
#define  ipxCircCompressedInitSent        1121114
#define  ipxCircCompressedRejectsSent     1121115
#define  ipxCircUncompressedSent          1121116
#define  ipxCircCompressedReceived        1121117
#define  ipxCircCompressedInitReceived    1121118
#define  ipxCircCompressedRejectsReceived 1121119
#define  ipxCircUncompressedReceived      1121120
#define  ipxCircMediaType                 1121121
#define  ipxCircNetNumber                 1121122
#define  ipxCircStateChanges              1121123
#define  ipxCircInitFails                 1121124
#define  ipxCircDelay                     1121125
#define  ipxCircThroughput                1121126
#define  ipxCircNeighRouterName           1121127
#define  ipxCircNeighInternalNetNum       1121128

/* Forwarding Group  */
/* Destination Table */
#define  ipxDestSysInstance               113111
#define  ipxDestNetNum                    113112
#define  ipxDestProtocol                  113113
#define  ipxDestTicks                     113114
#define  ipxDestHopCount                  113115
#define  ipxDestNextHopCircIndex          113116
#define  ipxDestNextHopNICAddress         113117
#define  ipxDestNextHopNetNum             113118

/* Static Routes Table  */
#define  ipxStaticRouteSysInstance        113211
#define  ipxStaticRouteCircIndex          113212
#define  ipxStaticRouteNetNum             113213
#define  ipxStaticRouteExistState         113214
#define  ipxStaticRouteTicks              113215
#define  ipxStaticRouteHopCount           113216

/* Services Group */
/* Services Table */
#define  ipxServSysInstance               114111
#define  ipxServType                      114112
#define  ipxServName                      114113
#define  ipxServProtocol                  114114
#define  ipxServNetNum                    114115
#define  ipxServNode                      114116
#define  ipxServSocket                    114117
#define  ipxServHopCount                  114118

/* Destination Services Table */
#define  ipxDestServSysInstance           114211
#define  ipxDestServNetNum                114212
#define  ipxDestServNode                  114213
#define  ipxDestServSocket                114214
#define  ipxDestServName                  114215
#define  ipxDestServType                  114216
#define  ipxDestServProtocol              114217
#define  ipxDestServHopCount              114218

/* Static Services Table   */
#define  ipxStaticServSysInstance         114311
#define  ipxStaticServCircIndex           114312
#define  ipxStaticServName                114313
#define  ipxStaticServType                114314
#define  ipxStaticServExistState          114315
#define  ipxStaticServNetNum              114316
#define  ipxStaticServNode                114317
#define  ipxStaticServSocket              114318
#define  ipxStaticServHopCount            114319

/* Traps    */
#define  ipxTrapCircuitDown               115111
#define  ipxTrapCircuitUp                 115112

/* RIPSAP MIB           */
/* System Group         */
/* RIP System Table     */

#define  ripSysInstance                   101111
#define  ripSysState                      101112
#define  ripSysIncorrectPackets           101113

/* SAP System Table     */

#define  sapSysInstance                   101211
#define  sapSysState                      101212
#define  sapSysIncorrectPackets           101213

/* Circuit Group        */
/* RIP Circuit Table    */

#define  ripCircSysInstance               102111
#define  ripCircIndex                     102112
#define  ripCircState                     102113
#define  ripCircPace                      102114
#define  ripCircUpdate                    102115
#define  ripCircAgeMultiplier             102116
#define  ripCircPacketSize                102117
#define  ripCircOutPackets                102118
#define  ripCircInPackets                 102119

/* SAP Circuit Table    */

#define  sapCircSysInstance               102211
#define  sapCircIndex                     102212
#define  sapCircState                     102213
#define  sapCircPace                      102214
#define  sapCircUpdate                    102215
#define  sapCircAgeMultiplier             102216
#define  sapCircPacketSize                102217
#define  sapCircGetNearestServerReply     102218
#define  sapCircOutPackets                102219
#define  sapCircInPackets                 1022110

typedef struct 
   {
   char              AdapterDevice[NWCM_MAX_STRING_SIZE];
   char              AdapterType[NWCM_MAX_STRING_SIZE];
   char              FrameType[NWCM_MAX_STRING_SIZE];
   uint32            Network;
   } lanConfInfo_t;

/* Forward References */

void  nwumpsSendTrap(int, ...);

void  nwuIPXInit(void);
void  nwuSPXInit(void);
void  nwuDiagInit(void);
void  nwuRIPSAPInit(void);

int   nwuIPXGetNumLans(void);
int   nwuIPXGetLanInfo(int numLans);
int   nwuIPXGetBasicSys(void);
int   nwuIPXGetRouterTable(void);
int   nwuIPXGetServerTable(uint32 *numServers);
int   nwuIPXGetDestServTable(uint32 *numDestServers);

int   nwuSPXGetNumConn(void);

#endif /* _NWUMPS_NWUMPSD_H_ */
