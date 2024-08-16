/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwuipx.c	1.16"
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

#if !defined(NO_SCCS_ID) && !defined(lint) && !defined(SABER)
static char rcsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwuipx.c,v 1.17.2.2 1994/10/21 22:52:27 rbell Exp $";
#endif

/****************************************************************************
** Source file:   nwuipx.c
**
** Description:   
**
** Contained functions:
**                      nwuIPXGetSysObj()
**                      nwuIPXSetSysObj()
**                      nwuIPXGetCircObj()
**                      nwuIPXSetCircObj()
**                      nwuIPXGetServObj()
**                      nwuIPXGetDestObj()
**                      nwuIPXGetDestServObj()
**                      nwuIPXGetBasicSys()
**                      nwuIPXGetLanInfo()
**
** Author:   Rick Bell
**
** Date Created:  June 9, 1993
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/* including system include files */
#include <stdio.h>
#include <sys/nwportable.h>

#ifndef OS_AIX
#include <stdlib.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stropts.h>
#include <signal.h>
#include <tiuser.h>  /* for diags */
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef OS_SUN4
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/nit_if.h>
#endif

#include "nps.h"
#include "nwconfig.h"

#include "sys/lipmx_app.h"
#include "sys/ipx_app.h"

#ifdef IPXE
#include "ipxe_app.h"
#endif /* IPXE */

#include "sys/ripx_app.h"
#include "sys/spx_app.h"
#include "sys/sap_app.h"
#include "sap_lib.h"

#include "nwmsg.h"
#include "netmgtmsgtable.h"
#include "nwumpsd.h"

/* including isode and snmp header files */
#ifdef OS_UNIXWARE
#include "snmp.h"
#include "objects.h"
#else
#ifdef PEPYPATH
#include "smux.h"
#include "objects.h"
#else
#include <isode/snmp/smux.h>
#include <isode/snmp/objects.h>
#endif
#endif

/* Include NetWare for Unix headers */
#ifdef OS_UNIXWARE
#include "nwsmuxsvr4.h"
#else
#include "nwsmux.h"
#endif

#ifdef BITTEST
#undef BITTEST
#endif

/* External Variables */
extern   char  errorStr[];
extern   char  nwumpsTitleStr[];
extern   int   mode;
extern   int   sapFlag;

extern   int  ipxFd;
extern   int  ripxFd;

extern   char *ipxDevice;
extern   char *ripxDevice;

extern   IpxNetAddr_t    MyIPXNetAddr;
extern   IpxNodeAddr_t   MyIPXNodeAddr;
extern   char            MyServerName[NWUMPS_NAME_SIZE];

lanConfInfo_t           *ipxLanInfo = 0;

struct 
{
  IpxLanStats_t    l;
  IpxSocketStats_t s;
} ipxStats = {0};

routeInfo_t             *RouterTable = 0;
ServerEntry_t          	**ServerTable = 0;
ServerEntry_t          	**DestServerTable = 0;

uint32                  IpxAdvSysForwPackets = 0; /* The number of IPX 
                                                       packets forwarded.   */

uint32                  IpxAdvSysServCount = 0;   /* The number of servers 
                                                     known to this IPX.   */
uint32                  IpxAdvSysDestCount = 0;   /* The number of destination
                                                    servers known to this IPX */

char                    NotSupported[80];

int      NumLans = 0;
int      NumRouters = 0;
uint32   NumServers = 0;
uint32   NumDestServ = 0;


/*
**  shared memory information
*/
extern SAPD          SAPStats;		/* SAP Statistics */
extern ServerEntry_t *SrvBase;		/* Server entry address base */

/* Forward References */
static   int   nwuIPXGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetStaticRouteObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXSetStaticRouteObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetStaticServObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXSetStaticServObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetServObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetDestObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuIPXGetDestServObj(OI oi, register struct type_SNMP_VarBind *v, int offset);

void nwuIPXInit(void)
   {
   register OT ot;

   bzero(NotSupported, 80);

   NumLans = nwuIPXGetNumLans();
   NumRouters = nwuIPXGetRouterTable();

   if(sapFlag == TRUE)
      {
      nwuIPXGetLanInfo(NumLans);
      nwuIPXGetServerTable(&NumServers);
      nwuIPXGetDestServTable(&NumDestServ);
      }

   if(mode == NWUMPS_DEBUG)
      {
      printf("Number of LAN Cards: %d.\n", NumLans);
      printf("Number of Routers: %d.\n", NumRouters);
      printf("Number of Servers: %d.\n", NumServers);
      printf("Number of Destination Servers: %d.\n", NumDestServ);
      }

/* System Group         */
/* Basic System Table   */

   if(ot = text2obj("ipxBasicSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInstance;
      }

   if(ot = text2obj("ipxBasicSysExistState"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysExistState;
      }

   if(ot = text2obj("ipxBasicSysNetNumber"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysNetNumber;
      }

   if(ot = text2obj("ipxBasicSysNode"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysNode;
      }

   if(ot = text2obj("ipxBasicSysName"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysName;
      }
   
   if(ot = text2obj("ipxBasicSysInReceives"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInReceives;
      }

   if(ot = text2obj("ipxBasicSysInHdrErrors"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInHdrErrors;
      }

   if(ot = text2obj("ipxBasicSysInUnknownSockets"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInUnknownSockets;
      }

   if(ot = text2obj("ipxBasicSysInDiscards"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInDiscards;
      }

   if(ot = text2obj("ipxBasicSysInBadChecksums"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInBadChecksums;
      }

   if(ot = text2obj("ipxBasicSysInDelivers"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysInDelivers;
      }

   if(ot = text2obj("ipxBasicSysNoRoutes"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysNoRoutes;
      }

   if(ot = text2obj("ipxBasicSysOutRequests"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysOutRequests;
      }

   if(ot = text2obj("ipxBasicSysOutMalformedRequests"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysOutMalformedRequests;
      }

   if(ot = text2obj("ipxBasicSysOutDiscards"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysOutDiscards;
      }

   if(ot = text2obj("ipxBasicSysOutPackets"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysOutPackets;
      }

   if(ot = text2obj("ipxBasicSysConfigSockets"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysConfigSockets;
      }

   if(ot = text2obj("ipxBasicSysOpenSocketFails"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxBasicSysOpenSocketFails;
      }
/* Advanced System Table   */

   if(ot = text2obj("ipxAdvSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysInstance;
      }

   if(ot = text2obj("ipxAdvSysMaxPathSplits"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysMaxPathSplits;
      }

   if(ot = text2obj("ipxAdvSysMaxHops"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_setfnx = nwuIPXSetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysMaxHops;
      }

   if(ot = text2obj("ipxAdvSysInTooManyHops"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysInTooManyHops;
      }

   if(ot = text2obj("ipxAdvSysInFiltered"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysInFiltered;
      }

   if(ot = text2obj("ipxAdvSysInCompressDiscards"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysInCompressDiscards;
      }

   if(ot = text2obj("ipxAdvSysNETBIOSPackets"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysNETBIOSPackets;
      }

   if(ot = text2obj("ipxAdvSysForwPackets"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysForwPackets;
      }

   if(ot = text2obj("ipxAdvSysOutFiltered"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysOutFiltered;
      }

   if(ot = text2obj("ipxAdvSysOutCompressDiscards"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysOutCompressDiscards;
      }

   if(ot = text2obj("ipxAdvSysCircCount"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysCircCount;
      }

   if(ot = text2obj("ipxAdvSysDestCount"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysDestCount;
      }

   if(ot = text2obj("ipxAdvSysServCount"))
      {
      ot->ot_getfnx = nwuIPXGetSysObj;
      ot->ot_info = (caddr_t) ipxAdvSysServCount;
      }

/* Circuit Group  */
/* Circuit Table  */

   if(ot = text2obj("ipxCircSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircSysInstance;
      }

   if(ot = text2obj("ipxCircIndex"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircIndex;
      }

   if(ot = text2obj("ipxCircExistState"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircExistState;
      }

   if(ot = text2obj("ipxCircOperState"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircOperState;
      }

   if(ot = text2obj("ipxCircIfIndex"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircIfIndex;
      }

   if(ot = text2obj("ipxCircName"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircName;
      }

   if(ot = text2obj("ipxCircType"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircType;
      }

   if(ot = text2obj("ipxCircDialName"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircDialName;
      }

   if(ot = text2obj("ipxCircLocalMaxPacketSize"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircLocalMaxPacketSize;
      }

   if(ot = text2obj("ipxCircCompressState"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressState;
      }

   if(ot = text2obj("ipxCircCompressSlots"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressSlots;
      }

   if(ot = text2obj("ipxCircStaticStatus"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_setfnx = nwuIPXSetCircObj;
      ot->ot_info = (caddr_t) ipxCircStaticStatus;
      }

   if(ot = text2obj("ipxCircCompressedSent"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedSent;
      }

   if(ot = text2obj("ipxCircCompressedInitSent"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedInitSent;
      }

   if(ot = text2obj("ipxCircCompressedRejectsSent"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedRejectsSent;
      }

   if(ot = text2obj("ipxCircUncompressedSent"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircUncompressedSent;
      }

   if(ot = text2obj("ipxCircCompressedReceived"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedReceived;
      }

   if(ot = text2obj("ipxCircCompressedInitReceived"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedInitReceived;
      }

   if(ot = text2obj("ipxCircCompressedRejectsReceived"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircCompressedRejectsReceived;
      }

   if(ot = text2obj("ipxCircUncompressedReceived"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircUncompressedReceived;
      }

   if(ot = text2obj("ipxCircMediaType"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircMediaType;
      }

   if(ot = text2obj("ipxCircNetNumber"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircNetNumber;
      }

   if(ot = text2obj("ipxCircStateChanges"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircStateChanges;
      }

   if(ot = text2obj("ipxCircInitFails"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircInitFails;
      }

   if(ot = text2obj("ipxCircDelay"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircDelay;
      }

   if(ot = text2obj("ipxCircThroughput"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircThroughput;
      }

   if(ot = text2obj("ipxCircNeighRouterName"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircNeighRouterName;
      }

   if(ot = text2obj("ipxCircNeighInternalNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetCircObj;
      ot->ot_info = (caddr_t) ipxCircNeighInternalNetNum;
      }

/* Forwarding Group  */
/* Destination Table */
   if(ot = text2obj("ipxDestSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestSysInstance;
      }

   if(ot = text2obj("ipxDestNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestNetNum;
      }

   if(ot = text2obj("ipxDestProtocol"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestProtocol;
      }

   if(ot = text2obj("ipxDestTicks"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestTicks;
      }

   if(ot = text2obj("ipxDestHopCount"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestHopCount;
      }

   if(ot = text2obj("ipxDestNextHopCircIndex"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestNextHopCircIndex;
      }

   if(ot = text2obj("ipxDestNextHopNICAddress"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestNextHopNICAddress;
      }

   if(ot = text2obj("ipxDestNextHopNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetDestObj;
      ot->ot_info = (caddr_t) ipxDestNextHopNetNum;
      }

/* Static Routes Table  */
   if(ot = text2obj("ipxStaticRouteSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteSysInstance;
      }

   if(ot = text2obj("ipxStaticRouteCircIndex"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteCircIndex;
      }

   if(ot = text2obj("ipxStaticRouteNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteNetNum;
      }

   if(ot = text2obj("ipxStaticRouteExistState"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteExistState;
      }

   if(ot = text2obj("ipxStaticRouteTicks"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteTicks;
      }

   if(ot = text2obj("ipxStaticRouteHopCount"))
      {
      ot->ot_getfnx = nwuIPXGetStaticRouteObj;
      ot->ot_setfnx = nwuIPXSetStaticRouteObj;
      ot->ot_info = (caddr_t) ipxStaticRouteHopCount;
      }

/* Services Group */
/* Services Table */

   if(ot = text2obj("ipxServSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServSysInstance;
      }

   if(ot = text2obj("ipxServType"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServType;
      }

   if(ot = text2obj("ipxServName"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServName;
      }

   if(ot = text2obj("ipxServProtocol"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServProtocol;
      }

   if(ot = text2obj("ipxServNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServNetNum;
      }

   if(ot = text2obj("ipxServNode"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServNode;
      }

   if(ot = text2obj("ipxServSocket"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServSocket;
      }

   if(ot = text2obj("ipxServHopCount"))
      {
      ot->ot_getfnx = nwuIPXGetServObj;
      ot->ot_info = (caddr_t) ipxServHopCount;
      }

/* Destination Services Table */

   if(ot = text2obj("ipxDestServSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServSysInstance;
      }

   if(ot = text2obj("ipxDestServNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServNetNum;
      }

   if(ot = text2obj("ipxDestServNode"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServNode;
      }

   if(ot = text2obj("ipxDestServSocket"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServSocket;
      }

   if(ot = text2obj("ipxDestServName"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServName;
      }

   if(ot = text2obj("ipxDestServType"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServType;
      }

   if(ot = text2obj("ipxDestServProtocol"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServProtocol;
      }

   if(ot = text2obj("ipxDestServHopCount"))
      {
      ot->ot_getfnx = nwuIPXGetDestServObj;
      ot->ot_info = (caddr_t) ipxDestServHopCount;
      }

/* Static Services Table   */
   if(ot = text2obj("ipxStaticServSysInstance"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServSysInstance;
      }

   if(ot = text2obj("ipxStaticServCircIndex"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServCircIndex;
      }

   if(ot = text2obj("ipxStaticServName"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServName;
      }

   if(ot = text2obj("ipxStaticServType"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServType;
      }

   if(ot = text2obj("ipxStaticServExistState"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServExistState;
      }

   if(ot = text2obj("ipxStaticServNetNum"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServNetNum;
      }

   if(ot = text2obj("ipxStaticServNode"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServNode;
      }

   if(ot = text2obj("ipxStaticServSocket"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServSocket;
      }

   if(ot = text2obj("ipxStaticServHopCount"))
      {
      ot->ot_getfnx = nwuIPXGetStaticServObj;
      ot->ot_setfnx = nwuIPXSetStaticServObj;
      ot->ot_info = (caddr_t) ipxStaticServHopCount;
      }
   }

static int  nwuIPXGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            char  buf[NWUMPS_NAME_SIZE];

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
               || oid->oid_elements[oid->oid_nelem - 1] != 0) 
            {
            return int_SNMP_error__status_noSuchName;
            }
      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;
            new->oid_elements[new->oid_nelem - 1] = 0;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;
            }
         else
            return NOTOK;
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   switch(ifvar)
      {
/* System Group         */
/* Basic System Table   */

      case ipxBasicSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxBasicSysExistState:
      return o_integer(oi, v, NWUMPS_ON);

      case ipxBasicSysNetNumber: /* IPX_GET_NET */
      return o_string(oi, v, (char *)&MyIPXNetAddr.myNetAddress.net, IPX_NET_SIZE);

      case ipxBasicSysNode:      /* IPX_GET_NODE */
      return o_string(oi, v, (char *)&MyIPXNodeAddr.myNodeAddress.node, IPX_NODE_SIZE);

      case ipxBasicSysName:      /* Configuration Manager - server_name */
      return o_string(oi, v, (char *)&MyServerName, NWUMPS_NAME_SIZE);

      case ipxBasicSysInReceives:   /* IPX Stats - IpxInData             */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxInData);

      case ipxBasicSysInHdrErrors:  /* IPX Stats - InBadLength        */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.l.InBadLength);

      case ipxBasicSysInUnknownSockets:   /* IPX Stats - IpxSocketNotBound */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxSocketNotBound);

      case ipxBasicSysInDiscards:            /* IPX Stats - IpxBusySocket */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxBusySocket);

      case ipxBasicSysInBadChecksums:     /* IPX Stats - IpxSumFail     */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxSumFail);

      case ipxBasicSysInDelivers:     /* IPX Stats - IpxDataToSocket     */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxDataToSocket);

      case ipxBasicSysNoRoutes:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ipxBasicSysOutRequests:           /* IPX Stats - IpxOutData */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxOutData);

      case ipxBasicSysOutMalformedRequests:  /* IPX Stats - IpxOutBadSize */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.s.IpxOutBadSize);

      case ipxBasicSysOutDiscards:           /* IPX Stats - OutSameSocket */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.l.OutSameSocket);

      case ipxBasicSysOutPackets:            /* IPX  Stats - OutTotal */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.l.OutTotal);

      case ipxBasicSysConfigSockets:         /* IPX IOCTL IPX_GET_MAX_SOCKETS */
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxBasicSysOpenSocketFails:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

/* Advanced System Table   */

      case ipxAdvSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxAdvSysMaxPathSplits:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ipxAdvSysMaxHops:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ipxAdvSysInTooManyHops:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ipxAdvSysInFiltered:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxAdvSysInCompressDiscards:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxAdvSysNETBIOSPackets: /* IPX Stats - InPropagation        */
         nwuIPXGetBasicSys();
      return o_integer(oi, v, ipxStats.l.InPropagation);

      case ipxAdvSysForwPackets:    /* IPX Stats - OutSent + OutQueued */
         nwuIPXGetBasicSys();
         IpxAdvSysForwPackets = ipxStats.l.OutSent +
                            ipxStats.l.OutQueued;
      return o_integer(oi, v, IpxAdvSysForwPackets);

      case ipxAdvSysOutFiltered:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxAdvSysOutCompressDiscards:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxAdvSysCircCount:  /* IPX IOCTL IPX_GET_CONFIGURED_LANS */
      return o_integer(oi, v, NumLans);

      case ipxAdvSysDestCount:  /* SAP Info - SAPStats.ConfigServers or 
                                   SAPStats.ServerPoolIdx */
         if(sapFlag == TRUE)
            {
            if(SAPStats.ServerPoolIdx == 0)
               IpxAdvSysDestCount = SAPStats.ConfigServers;
            else
               IpxAdvSysDestCount = SAPStats.ServerPoolIdx - 1;
            }
      return o_integer(oi, v, IpxAdvSysDestCount);

      case ipxAdvSysServCount:  /* SAP Info - SAPStats.ConfigServers or 
                                   SAPStats.ServerPoolIdx */
         if(sapFlag == TRUE)
            {
            if(SAPStats.ServerPoolIdx == 0)
               IpxAdvSysServCount = SAPStats.ConfigServers;
            else
               IpxAdvSysServCount = SAPStats.ServerPoolIdx - 1;
            }
      return o_integer(oi, v, IpxAdvSysServCount);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OS    os = ot->ot_syntax;

   ifvar = (int) ot->ot_info;

   if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
      return int_SNMP_error__status_noSuchName;

   if(os == NULLOS)
      return int_SNMP_error__status_genErr;

   switch(ifvar)
      {
/* System Group         */
/* Basic System Table   */

      case ipxBasicSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case ipxBasicSysExistState:
      return int_SNMP_error__status_noError;

      case ipxBasicSysNetNumber:
      return int_SNMP_error__status_noError;

      case ipxBasicSysNode:
      return int_SNMP_error__status_noError;

      case ipxBasicSysName:
      return int_SNMP_error__status_noError;

/* Advanced System Table   */

      case ipxAdvSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case ipxAdvSysMaxPathSplits:
      return int_SNMP_error__status_noError;

      case ipxAdvSysMaxHops:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetStaticRouteObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            char  buf[NWUMPS_NAME_SIZE];

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
               || oid->oid_elements[oid->oid_nelem - 1] != 0) 
            {
            return int_SNMP_error__status_noSuchName;
            }
      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;
            new->oid_elements[new->oid_nelem - 1] = 0;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;
            }
         else
            return NOTOK;
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   switch(ifvar)
      {
/* Static Routes Table  */

      case ipxStaticRouteSysInstance:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxStaticRouteCircIndex:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxStaticRouteNetNum:
      return o_string(oi, v, NotSupported, IPX_NET_SIZE);       /* This attribute is not supported in NWU */

      case ipxStaticRouteExistState:
      return o_integer(oi, v, NWUMPS_OFF); /* This attribute is not supported in NWU */

      case ipxStaticRouteTicks:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxStaticRouteHopCount:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXSetStaticRouteObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OS    os = ot->ot_syntax;

   ifvar = (int) ot->ot_info;

   if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
      return int_SNMP_error__status_noSuchName;

   if(os == NULLOS)
      return int_SNMP_error__status_genErr;

   switch(ifvar)
      {
/* Static Routes Table  */

      case ipxStaticRouteSysInstance:
      return int_SNMP_error__status_noError;

      case ipxStaticRouteCircIndex:
      return int_SNMP_error__status_noError;

      case ipxStaticRouteNetNum:
      return int_SNMP_error__status_noError;

      case ipxStaticRouteExistState:
      return int_SNMP_error__status_noError;

      case ipxStaticRouteTicks:
      return int_SNMP_error__status_noError;

      case ipxStaticRouteHopCount:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetStaticServObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            char  buf[NWUMPS_NAME_SIZE];

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
               || oid->oid_elements[oid->oid_nelem - 1] != 0) 
            {
            return int_SNMP_error__status_noSuchName;
            }
      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;
            new->oid_elements[new->oid_nelem - 1] = 0;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;
            }
         else
            return NOTOK;
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   switch(ifvar)
      {
/* Static Services Table   */
/* This table is not supported in NWU */

      case ipxStaticServSysInstance:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxStaticServCircIndex:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxStaticServName:
      return o_string(oi, v, NotSupported, NWUMPS_NAME_SIZE);      /* This attribute is not supported in NWU */

      case ipxStaticServType:
      return o_string(oi, v, NotSupported, NWUMPS_TYPE_SIZE);  /* This attribute is not supported in NWU */

      case ipxStaticServExistState:
      return o_integer(oi, v, NWUMPS_OFF);           /* This attribute is not supported in NWU */

      case ipxStaticServNetNum:
      return o_string(oi, v, NotSupported, IPX_NET_SIZE);       /* This attribute is not supported in NWU */

      case ipxStaticServNode:
      return o_string(oi, v, NotSupported, IPX_NODE_SIZE);       /* This attribute is not supported in NWU */

      case ipxStaticServSocket:
      return o_string(oi, v, NotSupported, IPX_SOCK_SIZE);       /* This attribute is not supported in NWU */

      case ipxStaticServHopCount:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXSetStaticServObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OS    os = ot->ot_syntax;

   ifvar = (int) ot->ot_info;

   if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
      return int_SNMP_error__status_noSuchName;

   if(os == NULLOS)
      return int_SNMP_error__status_genErr;

   switch(ifvar)
      {
/* Static Services Table   */
/* This table is not supported in NWU */

      case ipxStaticServSysInstance:
      return int_SNMP_error__status_noError;

      case ipxStaticServCircIndex:
      return int_SNMP_error__status_noError;

      case ipxStaticServName:
      return int_SNMP_error__status_noError;

      case ipxStaticServType:
      return int_SNMP_error__status_noError;

      case ipxStaticServExistState:
      return int_SNMP_error__status_noError;

      case ipxStaticServNetNum:
      return int_SNMP_error__status_noError;

      case ipxStaticServNode:
      return int_SNMP_error__status_noError;

      case ipxStaticServSocket:
      return int_SNMP_error__status_noError;

      case ipxStaticServHopCount:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            char  buf[NWUMPS_NAME_SIZE];

            int   index = 0;
            int   i = 0;

            SAPL  LanData = {0};

            int   status = 0;

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
               || oid->oid_elements[oid->oid_nelem - 1] != 0) 
            {
            return int_SNMP_error__status_noSuchName;
            }

      index = oid->oid_elements[oid->oid_nelem - 1];

      if(index >= NumLans)
            return (int_SNMP_error__status_noSuchName);

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] >= NumLans)
               return NOTOK;
            }

         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            index = 0;

            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;

            new->oid_elements[new->oid_nelem - 1] = index;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;

            oid = new;  /* For try_again  */
            }
         else
            index = oid->oid_elements[ot->ot_name->oid_nelem];
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   if(mode == NWUMPS_DEBUG)
      printf("Index: %d.\n", index);

   if(sapFlag == TRUE)
      SAPGetLanData(index + 1, &LanData);

   switch(ifvar)
      {
/* Circuit Group  */
/* Circuit Table  */

      case ipxCircSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxCircIndex:
      return o_integer(oi, v, index);

      case ipxCircExistState:
         if(sapFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case ipxCircOperState:
         if(sapFlag == TRUE)
            return o_integer(oi, v, NWUMPS_UP);
         else
            return o_integer(oi, v, NWUMPS_DOWN);

      case ipxCircIfIndex:        /* SAP Lan Data - LanData.LanNumber */
      return o_integer(oi, v, LanData.LanNumber);

      case ipxCircName:
         if(sapFlag == TRUE)
            {
            sprintf(buf, MsgGetStr(NWUMPSD_LAN_INFO), index + 1, 
		ipxLanInfo[index].Network, 
		ipxLanInfo[index].AdapterDevice, 
		ipxLanInfo[index].FrameType);

             if(mode == NWUMPS_DEBUG)
                 printf("%s.\n", buf);

            }
         else
            bzero(buf, NWUMPS_NAME_SIZE);

      return o_string(oi, v, buf, NWUMPS_NAME_SIZE);

      case ipxCircType:
      return o_integer(oi, v, NWUMPS_BROADCAST_TYPE);

      case ipxCircDialName:
      return o_string(oi, v, NotSupported, NWUMPS_NAME_SIZE);  /* This attribute is not supported in NWU */

      case ipxCircLocalMaxPacketSize:  /* SAP Lan Data - LanData.PacketSize */
      return o_integer(oi, v, LanData.PacketSize);

      case ipxCircCompressState:
      return o_integer(oi, v, NWUMPS_OFF); /* This attribute is not supported in NWU */

      case ipxCircCompressSlots:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircStaticStatus:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedSent:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedInitSent:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedRejectsSent:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircUncompressedSent:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedReceived:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedInitReceived:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircCompressedRejectsReceived:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircUncompressedReceived:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircMediaType:
      return o_string(oi, v, NotSupported, NWUMPS_TYPE_SIZE);

      case ipxCircNetNumber:     /* SAP Lan Data - LanData.Network */
      return o_string(oi, v, (char *)&LanData.Network, IPX_NET_SIZE);

      case ipxCircStateChanges:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircInitFails:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED); /* This attribute is not supported in NWU */

      case ipxCircDelay:
      return o_integer(oi, v, NWUMPS_IPX_DELAY);

      case ipxCircThroughput:    /* SAP Lan Data - LanData.LineSpeed */
      return o_integer(oi, v, LanData.LineSpeed);

      case ipxCircNeighRouterName:
      return o_string(oi, v, NotSupported, NWUMPS_NAME_SIZE);  /* This attribute is not supported in NWU */

      case ipxCircNeighInternalNetNum:
      return o_string(oi, v, NotSupported, IPX_NET_SIZE);      /* This attribute is not supported in NWU */

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OS    os = ot->ot_syntax;

   ifvar = (int) ot->ot_info;

   if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
      return int_SNMP_error__status_noSuchName;

   if(os == NULLOS)
      return int_SNMP_error__status_genErr;

   switch(ifvar)
      {
/* Circuit Group  */
/* Circuit Table  */

      case ipxCircSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case ipxCircIndex:
      return int_SNMP_error__status_noError;

      case ipxCircExistState:
      return int_SNMP_error__status_noError;

      case ipxCircOperState:
      return int_SNMP_error__status_noError;

      case ipxCircIfIndex:
      return int_SNMP_error__status_noError;

      case ipxCircName:
      return int_SNMP_error__status_noError;

      case ipxCircType:
      return int_SNMP_error__status_noError;

      case ipxCircDialName:
      return int_SNMP_error__status_noError;

      case ipxCircLocalMaxPacketSize:
      return int_SNMP_error__status_noError;

      case ipxCircCompressState:
      return int_SNMP_error__status_noError;

      case ipxCircCompressSlots:
      return int_SNMP_error__status_noError;

      case ipxCircStaticStatus:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetDestObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            char  buf[NWUMPS_NAME_SIZE];

            int   index = 0;
            int   i;

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
            {
            return int_SNMP_error__status_noSuchName;
            }

      index = oid->oid_elements[oid->oid_nelem - 1];

      if(index >= NumRouters)
            return (int_SNMP_error__status_noSuchName);

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] >= NumRouters)
               return NOTOK;
            }

         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            index = 0;

            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;

            new->oid_elements[new->oid_nelem - 1] = index;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;

            oid = new;
            }
         else
            index = oid->oid_elements[ot->ot_name->oid_nelem];
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   if(mode == NWUMPS_DEBUG)
      printf("Index: %d.\n", index);

   switch(ifvar)
      {
/* Forwarding Group  */
/* Destination Table */

      case ipxDestSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxDestNetNum:              /* RIPX Stat - net */
      return o_string(oi, v, (char *)&RouterTable[index].net, IPX_NET_SIZE);

      case ipxDestProtocol:
      return o_integer(oi, v, NWUMPS_PROTO_RIP);

      case ipxDestTicks:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ipxDestHopCount:           /* RIPX Stat - hops */
      return o_integer(oi, v, &RouterTable[index].hops);

      case ipxDestNextHopCircIndex:   /* RIPX Stat - connectedLan */
      return o_integer(oi, v, &RouterTable[index].connectedLan);

      case ipxDestNextHopNICAddress:   /* RIPX Stat - node */
      return o_string(oi, v, (char *)&RouterTable[index].node, IPX_NODE_SIZE);

      case ipxDestNextHopNetNum:   /* RIPX Stat - node */
      return o_string(oi, v, NotSupported, NWUMPS_TYPE_SIZE);  /* This attribute is not supported in NWU */

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetServObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            ipxAddr_t *ipxAddress;
            char  buf[NWUMPS_NAME_SIZE];

            int   index = 0;
            int   i = 0;

   if(sapFlag == FALSE)
      return int_SNMP_error__status_noSuchName;

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
            {
            return int_SNMP_error__status_noSuchName;
            }

      index = oid->oid_elements[oid->oid_nelem - 1];

      if(index >= NumServers)
            return int_SNMP_error__status_noSuchName;

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] >= NumServers)
               return NOTOK;
            }

         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            index = 0;

            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;

            new->oid_elements[new->oid_nelem - 1] = index;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;

            oid = new;
            }
         else
            index = oid->oid_elements[ot->ot_name->oid_nelem];
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   if(mode == NWUMPS_DEBUG)
      printf("Index: %d.\n", index);

   switch(ifvar)
      {
/* Services Group */
/* Services Table */

      case ipxServSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxServType:
         sprintf(buf, "%d", GETINT16(ServerTable[index]->ServerType));
      return o_string(oi, v, buf, NWUMPS_TYPE_SIZE);

      case ipxServName:
      return o_string(oi, v,
		(char *) &(ServerTable[index]->ServerName[1]),
		ServerTable[index]->ServerName[0]);

      case ipxServProtocol:
      return o_integer(oi, v, NWUMPS_PROTO_SAP);

      case ipxServNetNum:
         ipxAddress = (ipxAddr_t *)(ServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->net, IPX_NET_SIZE);

      case ipxServNode:
         ipxAddress = (ipxAddr_t *)(ServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->node, IPX_NODE_SIZE);

      case ipxServSocket:
         ipxAddress = (ipxAddr_t *)(ServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->sock, IPX_SOCK_SIZE);

      case ipxServHopCount:
      return o_integer(oi, v, ServerTable[index]->HopsToServer);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuIPXGetDestServObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            ipxAddr_t *ipxAddress;
            char  buf[NWUMPS_NAME_SIZE];

            int   index = 0;
            int   i = 0;

   if(sapFlag == FALSE)
      return int_SNMP_error__status_noSuchName;

   bzero(buf, NWUMPS_NAME_SIZE);

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
            {
            return int_SNMP_error__status_noSuchName;
            }

      index = oid->oid_elements[oid->oid_nelem - 1];

      if(index >= NumDestServ)
            return (int_SNMP_error__status_noSuchName);

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] >= NumDestServ)
               return NOTOK;
            }

         if(oid->oid_nelem == ot->ot_name->oid_nelem)
            {
            index = 0;

            if((new = oid_extend(oid, 1)) == NULLOID)
               return NOTOK;

            new->oid_elements[new->oid_nelem - 1] = index;

            if(v->name)
               free_SNMP_ObjectName(v->name);

            v->name = new;

            oid = new;
            }
         else
            index = oid->oid_elements[ot->ot_name->oid_nelem];
      break;

      default:
      return int_SNMP_error__status_genErr;
      }

   if(mode == NWUMPS_DEBUG)
      printf("Index: %d.\n", index);

   switch(ifvar)
      {
/* Services Group */
/* Destination Services Table */

      case ipxDestServSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ipxDestServNetNum:
         ipxAddress = (ipxAddr_t *)(DestServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->net, IPX_NET_SIZE);

      case ipxDestServNode:
         ipxAddress = (ipxAddr_t *)(DestServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->node, IPX_NODE_SIZE);

      case ipxDestServSocket:
         ipxAddress = (ipxAddr_t *)(DestServerTable[index]->ServerAddress);
      return o_string(oi, v, (char *) &ipxAddress->sock, IPX_SOCK_SIZE);

      case ipxDestServName:
      return o_string(oi, v,
		(char *) &(DestServerTable[index]->ServerName[1]),
		DestServerTable[index]->ServerName[0]);

      case ipxDestServType:
         sprintf(buf, "%d", GETINT16(DestServerTable[index]->ServerType));
      return o_string(oi, v, buf, NWUMPS_TYPE_SIZE);

      case ipxDestServProtocol:
      return o_integer(oi, v, NWUMPS_PROTO_SAP);

      case ipxDestServHopCount:
      return o_integer(oi, v, DestServerTable[index]->HopsToServer);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

int nwuIPXFindNet(uint32 CircNetwork)
   {
   char     TokenName[NWCM_MAX_STRING_SIZE];

   int      index;
   int      Cret;    /* Status from NWCM calls */

   uint32   Network;

   for(index = 1; index < 50; index ++)
      {
      sprintf(TokenName, "%s%d%s", NWUMPS_LAN, index, NWUMPS_NETWORK);

      if((Cret = NWCMGetParam(TokenName, NWCP_INTEGER, 
         &Network)) != SUCCESS)
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_LAN_X_X), NWUMPS_LAN, index, NWUMPS_NETWORK);
         return(FAILURE);
         }

      if(CircNetwork == Network)
         return(index);
      }
   }

int nwuIPXGetLanInfo(int numLans)
   {
   SAPL     LanData = {0};

   int      status = 0;
   int      index = 1;
   int      i = 0;

   char     TokenName[NWCM_MAX_STRING_SIZE];
   char     stringTokenVal[NWCM_MAX_STRING_SIZE];
   int      Cret;    /* Status from NWCM calls */

   ipxLanInfo = (lanConfInfo_t *)malloc(sizeof(lanConfInfo_t) * numLans);

   for(i = 0; i < numLans; i++)
      {
      SAPGetLanData(i + 1, &LanData);

      if((index = nwuIPXFindNet(LanData.Network)) < 1)
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_ERROR), NWUMPS_SERVER_NAME);
         return(FAILURE);
         }

      ipxLanInfo[i].Network = LanData.Network;

      sprintf(TokenName, "%s%d%s", NWUMPS_LAN, index, NWUMPS_NETWORK);

      if((Cret = NWCMGetParam(TokenName, NWCP_INTEGER, 
         &ipxLanInfo[i].Network)) != SUCCESS) 
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_LAN_X_X), NWUMPS_LAN, index, NWUMPS_NETWORK);
         return(FAILURE);
         }

      sprintf(TokenName, "%s%d%s", NWUMPS_LAN, index, NWUMPS_ADAPTER);
      
      if((Cret = NWCMGetParam(TokenName, NWCP_STRING, 
         ipxLanInfo[i].AdapterDevice)) != SUCCESS) 
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_LAN_X_X), NWUMPS_LAN, index, NWUMPS_ADAPTER);
         return(FAILURE);
         }
      
      sprintf(TokenName, "%s%d%s", NWUMPS_LAN, index, NWUMPS_ADAPTER_TYPE);

      if((Cret = NWCMGetParam(TokenName, NWCP_STRING, 
         ipxLanInfo[i].AdapterType)) != SUCCESS)
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_LAN_X_X), NWUMPS_LAN, index, NWUMPS_ADAPTER_TYPE);
         return(FAILURE);
         }
      
      sprintf(TokenName, "%s%d%s", NWUMPS_LAN, index, NWUMPS_FRAME_TYPE);
      
      if((Cret = NWCMGetParam(TokenName, NWCP_STRING, 
         ipxLanInfo[i].FrameType)) != SUCCESS)
         {
         NWCMPerror(Cret, MsgGetStr(NWUMPSD_CFG_LAN_X_X), NWUMPS_LAN, index, NWUMPS_FRAME_TYPE);
         return(FAILURE);
         }
    }

   return(SUCCESS);
   }

int nwuIPXGetBasicSys(void)
   {
   struct   strioctl ioc;
   static   time_t   LastTimeRefreshed;
            time_t   CurrentTime = 0;

   int      status;

   time(&CurrentTime);

   if(mode == NWUMPS_DEBUG)
      printf("The Current time is %s.\n", ctime(&CurrentTime));

   if(CurrentTime - LastTimeRefreshed > DATA_FRESHNESS)
      {
      if(mode == NWUMPS_DEBUG)
         printf("%s: Setting up ioc structure for IPX_STATS.\n", nwumpsTitleStr);

      ioc.ic_cmd = IPX_STATS;
      ioc.ic_timout = 3;
      ioc.ic_len = sizeof(ipxStats);
      ioc.ic_dp = (char *)&ipxStats;

      if(mode == NWUMPS_DEBUG)
         printf("%s: Doing ioctl IPX_STATS.\n", nwumpsTitleStr);

      if(ioctl(ipxFd, I_STR, &ioc) < 0) 
         {
         sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), nwumpsTitleStr,
                     "IPX_STATS", ipxDevice, 
                     ipxStats);
         perror(errorStr);
         return FAILURE;
         }

      time(&LastTimeRefreshed);

      if(mode == NWUMPS_DEBUG)
         printf("The last time the data was refreshed %s.\n", ctime(&CurrentTime));
      }

   else if(mode == NWUMPS_DEBUG)
      printf("The Data does not need to be refreshed.\n");

   return(SUCCESS);
   }

int nwuIPXGetNumLans(void)
   {
   struct   strioctl ioc;

   IpxConfiguredLans_t    ipxConfigLans = {0};

   int      status;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Setting up ioc structure for IPX_GET_CONFIGURED_LANS.\n", nwumpsTitleStr);

   ioc.ic_cmd = IPX_GET_CONFIGURED_LANS;
   ioc.ic_timout = 3;
   ioc.ic_len = sizeof(ipxConfigLans);
   ioc.ic_dp = (char *)&ipxConfigLans;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Doing ioctl IPX_GET_CONFIGURED_LANS.\n", nwumpsTitleStr);

   if(ioctl(ipxFd, I_STR, &ioc) < 0) 
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), nwumpsTitleStr,
                  "IPX_GET_CONFIGURED_LANS", ipxDevice, 
                  ipxConfigLans);
      perror(errorStr);
      return FAILURE;
      }

   return(ipxConfigLans.lans - 1); /* We don't include the internal LAN */
   }

int   nwuIPXGetRouterTable(void)
   {
   struct      strioctl ioc;
   struct      strbuf   data;
   int         flags;
   char        routeBuffer[ROUTE_TABLE_SIZE], *rp;
   char        *routeBufferEnd;
   size_t      ti;
   int         tiLimit;
   unsigned    tableSize;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Setting up ioc structure for RIPX_GET_ROUTER_TABLE.\n", nwumpsTitleStr);

   ioc.ic_cmd = RIPX_GET_ROUTER_TABLE;
   ioc.ic_timout = 0;
   ioc.ic_len = 0;
   ioc.ic_dp = NULL;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Doing ioctl RIPX_GET_ROUTER_TABLE.\n", nwumpsTitleStr);

   if(ioctl(ripxFd, I_STR, &ioc) < 0) 
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), 
                  "RIPX_GET_ROUTER_TABLE", ripxDevice, 
                  0);
      perror(errorStr);
      return(FAILURE);
      }

   data.maxlen = sizeof(routeBuffer);
   data.len = 0;
   data.buf = routeBuffer;
   flags = 0;

   tableSize = ROUTE_TABLE_SIZE;

   if(RouterTable != NULL)
	free(RouterTable);

   if((RouterTable = (routeInfo_t *) malloc(tableSize)) == NULL) 
      {
      (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
      return(FAILURE);
      }

   ti = 0;
   tiLimit = tableSize / sizeof(routeInfo_t);

   for(;;) 
      {
      if(getmsg(ripxFd, (struct strbuf *) NULL, &data, &flags) < 0) 
         {
         if(errno == EINTR)
            continue;
/*
            (void)fprintf(stderr, MsgGetStr(DROUT_FUNC_FAIL), getmsg);
*/
         perror("");
         return(FAILURE);
         }

      routeBufferEnd = routeBuffer + data.len;

      for(rp = routeBuffer; rp < routeBufferEnd; rp += ROUTE_INFO_SIZE) 
         {
         if(ti == tiLimit) 
            {
            tableSize += ROUTE_TABLE_SIZE;

            if((RouterTable = (routeInfo_t *) realloc((char *) RouterTable, tableSize)) == NULL) 
               {
               (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
               return(FAILURE);
               }

            tiLimit = tableSize / sizeof(routeInfo_t);
            }

         (void) memcpy((char *) &RouterTable[ti], rp, ROUTE_INFO_SIZE);

         if(RouterTable[ti++].endOfTable)
            goto tableComplete;
         }
      }

tableComplete:

   return(ti);
   }

int CompareServer(const void *cmp1, const void *cmp2)
   {
   ServerEntry_t **serverEntry1 = (ServerEntry_t **)cmp1;
   ServerEntry_t **serverEntry2 = (ServerEntry_t **)cmp2;

   int status = 0;
   
   if((status = strcmp((char *)&((*serverEntry1)->ServerName[1]), 
                       (char *)&((*serverEntry2)->ServerName[1]))) != 0)
	return(status);

   if((*serverEntry1)->ServerType < (*serverEntry2)->ServerType)
	status = -1;
   else if((*serverEntry1)->ServerType > (*serverEntry2)->ServerType)
	status = 1;

   return(status);
   }

int CompareDestServ(const void *cmp1, const void *cmp2)
   {
   int i;
   int status = 0;
   
   ServerEntry_t **serverEntry1 = (ServerEntry_t **)cmp1;
   ServerEntry_t **serverEntry2 = (ServerEntry_t **)cmp2;

   for(i = 0; i < IPX_ADDR_SIZE; i++)
       {
       if((*serverEntry1)->ServerAddress[i] < (*serverEntry2)->ServerAddress[i])
	   return(-1);
       else if((*serverEntry1)->ServerAddress[i] > (*serverEntry2)->ServerAddress[i])
	   return(1);
       }

   if((status = strcmp((char *)&((*serverEntry1)->ServerName[1]), 
                       (char *)&((*serverEntry2)->ServerName[1]))) != 0)
	return(status);

   if((*serverEntry1)->ServerType < (*serverEntry2)->ServerType)
	status = -1;
   else if((*serverEntry1)->ServerType > (*serverEntry2)->ServerType)
	status = 1;

   return(status);
   }

int nwuIPXGetServerTable(uint32 *numServers)
   {
   uint32   ServCount = 0;
   int      index;

   if(SAPStats.ServerPoolIdx == 0)
      ServCount = SAPStats.ConfigServers;
   else
      ServCount = SAPStats.ServerPoolIdx - 1;

   if(*numServers != ServCount)
      {
      if(ServerTable == 0)
         {
         if((ServerTable = (ServerEntry_t **)malloc(ServCount * sizeof(ServerEntry_t *))) == NULL) 
            {
            (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
            return(FAILURE);
            }
         }
      else
         {
         if((ServerTable = (ServerEntry_t **) realloc((char *)ServerTable, sizeof(ServerEntry_t *) * ServCount)) == NULL) 
            {
            (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
            return(FAILURE);
            }
         }

      for(index = 0; index < ServCount; index++)
         ServerTable[index] = &SrvBase[index + 1];

      qsort(ServerTable, ServCount, sizeof(ServerEntry_t *), CompareServer);

      *numServers = ServCount;
      }

   return(SUCCESS);
   }

int nwuIPXGetDestServTable(uint32 *numDestServers)
   {
   uint32   DestServCount = 0;
   int      index;

   if(SAPStats.ServerPoolIdx == 0)
      DestServCount = SAPStats.ConfigServers;
   else
      DestServCount = SAPStats.ServerPoolIdx - 1;

   if(*numDestServers != DestServCount)
      {
      if(DestServerTable == 0)
         {
         if((DestServerTable = (ServerEntry_t **)malloc(DestServCount * sizeof(ServerEntry_t *))) == NULL) 
            {
            (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
            return(FAILURE);
            }
         }
      else
         {
         if((DestServerTable = (ServerEntry_t **) realloc((char *)DestServerTable, sizeof(ServerEntry_t *) * DestServCount)) == NULL) 
            {
            (void) fprintf(stderr, "%s: Out of Memory\n", nwumpsTitleStr);
            return(FAILURE);
            }
         }

      for(index = 0; index < DestServCount; index++)
	DestServerTable[index] = &SrvBase[index + 1];

      qsort(DestServerTable, DestServCount, sizeof(ServerEntry_t *), CompareDestServ);
      *numDestServers = DestServCount;
      }

   return(SUCCESS);
   }
