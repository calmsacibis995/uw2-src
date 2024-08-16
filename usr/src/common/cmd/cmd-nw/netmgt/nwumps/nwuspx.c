/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwuspx.c	1.14"
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
static char rcsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwuspx.c,v 1.13.2.2 1994/10/21 22:52:35 rbell Exp $";
#endif

/****************************************************************************
** Source file:   nwuspx.c
**
** Description:   
**
** Contained functions:
**                      nwuSPXInit()
**                      nwuSPXGetSysObj()
**                      nwuSPXGetCircObj()
**
** Author:   Rick Bell
**
** Date Created:  August 3, 1993
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/* including system include files */
#include <stdio.h>
#include <sys/nwportable.h>

#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stropts.h>
#include <signal.h>
#include <tiuser.h>  /* for diags */
#include <unistd.h>

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

#include "sys/spx_app.h"
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

time_t  CurrentTime = 0;

/* External Variables */
extern   char  errorStr[];
extern   char  nwumpsTitleStr[];
extern   int   mode;
extern   int   spxFlag;

extern	int 	spxFd;
extern	char  	*spxDevice;

/* SPX Structures */
spxStats_t     spxSysStats = {0};
spxConStats_t  spxCircStats = {0};


int      NumSPXConnections = 0;

/* Forward References */
static   int nwuSPXGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuSPXGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuSPXGetSysStats(void);
static   int nwuSPXGetCircStats(int connID);

void nwuSPXInit(void) 
   {
   register OT ot;

   if(spxFlag == TRUE)
      NumSPXConnections = nwuSPXGetNumConn();

   if(mode == NWUMPS_DEBUG)
      printf("The number of SPX Connections %d.\n", NumSPXConnections);

/* NetWare SPX Groups         */
/* System Group               */
/* SPX System Table           */

   if(ot = text2obj("nwuSPXSysInstance"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysInstance;
      }

   if(ot = text2obj("nwuSPXSysState"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysState;
      }

   if(ot = text2obj("nwuSPXSysMajorVer"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysMajorVer;
      }

   if(ot = text2obj("nwuSPXSysMinorVer"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysMinorVer;
      }

   if(ot = text2obj("nwuSPXSysUpTime"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysUpTime;
      }

   if(ot = text2obj("nwuSPXSysMaxOpenSessions"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysMaxOpenSessions;
      }

   if(ot = text2obj("nwuSPXSysUsedOpenSessions"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysUsedOpenSessions;
      }

   if(ot = text2obj("nwuSPXSysMaxUsedOpenSessions"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysMaxUsedOpenSessions;
      }

   if(ot = text2obj("nwuSPXSysConnectReqCounts"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysConnectReqCounts;
      }

   if(ot = text2obj("nwuSPXSysConnectErrors"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysConnectErrors;
      }

   if(ot = text2obj("nwuSPXSysListenReqs"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysListenReqs;
      }

   if(ot = text2obj("nwuSPXSysListenErrors"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysListenErrors;
      }

   if(ot = text2obj("nwuSPXSysOutPackets"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysOutPackets;
      }

   if(ot = text2obj("nwuSPXSysOutErrors"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysOutErrors;
      }

   if(ot = text2obj("nwuSPXSysInPackets"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysInPackets;
      }

   if(ot = text2obj("nwuSPXSysInErrors"))
      {
      ot->ot_getfnx = nwuSPXGetSysObj;
      ot->ot_info = (caddr_t) nwuSPXSysInErrors;
      }

/* Circuit Group              */
/* SPX Circuit Table          */

   if(ot = text2obj("nwuSPXCircSysInstance"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSysInstance;
      }

   if(ot = text2obj("nwuSPXCircIndex"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircIndex;
      }

   if(ot = text2obj("nwuSPXCircState"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircState;
      }

   if(ot = text2obj("nwuSPXCircStartTime"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircStartTime;
      }

   if(ot = text2obj("nwuSPXCircRetryCounts"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRetryCounts;
      }

   if(ot = text2obj("nwuSPXCircRetryTime"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRetryTime;
      }

   if(ot = text2obj("nwuSPXCircLocNetNumber"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircLocNetNumber;
      }

   if(ot = text2obj("nwuSPXCircLocNode"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircLocNode;
      }

   if(ot = text2obj("nwuSPXCircEndNetNumber"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircEndNetNumber;
      }

   if(ot = text2obj("nwuSPXCircEndNode"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircLocNode;
      }

   if(ot = text2obj("nwuSPXCircType"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircType;
      }

   if(ot = text2obj("nwuSPXCircIPXChkSum"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircIPXChkSum;
      }

   if(ot = text2obj("nwuSPXCircSndWinSize"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndWinSize;
      }

   if(ot = text2obj("nwuSPXCircSndPktSize"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndPktSize;
      }

   if(ot = text2obj("nwuSPXCircSndMsgCounts"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndMsgCounts;
      }

   if(ot = text2obj("nwuSPXCircSndPktCounts"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndPktCounts;
      }

   if(ot = text2obj("nwuSPXCircSndNAKs"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndNAKs;
      }

   if(ot = text2obj("nwuSPXCircSndErrorCtrs"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircSndErrorCtrs;
      }

   if(ot = text2obj("nwuSPXCircRcvWinSize"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvWinSize;
      }

   if(ot = text2obj("nwuSPXCircRcvPktSize"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvPktSize;
      }

   if(ot = text2obj("nwuSPXCircRcvPktCounts"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvPktCounts;
      }

   if(ot = text2obj("nwuSPXCircRcvNAKs"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvNAKs;
      }

   if(ot = text2obj("nwuSPXCircRcvPktQues"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvPktQues;
      }

   if(ot = text2obj("nwuSPXCircRcvPktSentUps"))
      {
      ot->ot_getfnx = nwuSPXGetCircObj;
      ot->ot_info = (caddr_t) nwuSPXCircRcvPktSentUps;
      }

   }

static int  nwuSPXGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

   int      status = FAILURE;

   uint32   NWUSPXSysInErrors = 0;
   uint32   NWUSPXSysOutErrors = 0;

   time_t  SPXUpTime = 0;

   ifvar = (int) ot->ot_info;

   if(spxFlag == TRUE)
      status = nwuSPXGetSysStats();

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
/* NetWare SPX Groups */
/* System Group               */
/* SPX System Table    */

      case nwuSPXSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case nwuSPXSysState:
         if(spxFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case nwuSPXSysMajorVer:
      return o_integer(oi, v, spxSysStats.spx_major_version);

      case nwuSPXSysMinorVer:
      return o_integer(oi, v, spxSysStats.spx_minor_version);

      case nwuSPXSysUpTime:
         if(spxFlag == TRUE)
            {
            if(status == SUCCESS) 
               {
               time(&CurrentTime);
               SPXUpTime = CurrentTime - spxSysStats.spx_startTime;

               if(mode == NWUMPS_DEBUG)
                   {
                   printf("%s: The SPX start time is %s.\n", nwumpsTitleStr, 
                          ctime(&spxSysStats.spx_startTime));
                   printf("%s: The Current time is %s.\n", nwumpsTitleStr, 
                          ctime(&CurrentTime));
                   }
               }
             }
      return o_integer(oi, v, SPXUpTime);

      case nwuSPXSysMaxOpenSessions:
      return o_integer(oi, v, spxSysStats.spx_max_connections);

      case nwuSPXSysUsedOpenSessions:
      return o_integer(oi, v, spxSysStats.spx_current_connections);

      case nwuSPXSysMaxUsedOpenSessions:
      return o_integer(oi, v, spxSysStats.spx_max_used_connections);

      case nwuSPXSysConnectReqCounts:
      return o_integer(oi, v, spxSysStats.spx_connect_req_count);

      case nwuSPXSysConnectErrors:
      return o_integer(oi, v, spxSysStats.spx_connect_req_fails);

      case nwuSPXSysListenReqs:
      return o_integer(oi, v, spxSysStats.spx_listen_req);

      case nwuSPXSysListenErrors:
      return o_integer(oi, v, spxSysStats.spx_listen_req_fails);

      case nwuSPXSysOutPackets:
      return o_integer(oi, v, spxSysStats.spx_send_packet_count);

      case nwuSPXSysOutErrors:
         if(spxFlag == TRUE)
            {
            if(status == SUCCESS) 
               {
               NWUSPXSysOutErrors = spxSysStats.spx_alloc_failures +
                                spxSysStats.spx_open_failures + 
                                spxSysStats.spx_connect_req_fails + 
				spxSysStats.spx_listen_req_fails + 
				spxSysStats.spx_unknown_mesg_count + 
				spxSysStats.spx_send_bad_mesg + 
				spxSysStats.spx_send_packet_timeout + 
				spxSysStats.spx_send_packet_nak;

               }
             }
      return o_integer(oi, v, NWUSPXSysOutErrors);

      case nwuSPXSysInPackets:
      return o_integer(oi, v, spxSysStats.spx_rcv_packet_count);

      case nwuSPXSysInErrors:
         if(spxFlag == TRUE)
            {
            if(status == SUCCESS) 
               {
               NWUSPXSysInErrors = spxSysStats.spx_rcv_bad_packet + 
	      			spxSysStats.spx_rcv_bad_data_packet + 
				spxSysStats.spx_rcv_dup_packet + 
				spxSysStats.spx_abort_connection +
				spxSysStats.spx_max_retries_abort + 
				spxSysStats.spx_no_listeners;

               }
            }
      return o_integer(oi, v, NWUSPXSysInErrors);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuSPXGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            int   index = 0;
            int   i = 0;

   int      status;

            uint32   SPXCircType;
            uint32   SPXCircSndErrorCtrs;
            uint32   SPXCircRcvErrorCtrs;

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
            {
            return int_SNMP_error__status_noSuchName;
            }

      index = oid->oid_elements[oid->oid_nelem - 1];

      if(index >= NumSPXConnections)
            return int_SNMP_error__status_noSuchName;

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] > NumSPXConnections)
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
      fprintf(stderr, "Index: %d.\n", index);

   if(spxFlag == TRUE)
      nwuSPXGetCircStats(index);

   switch(ifvar)
      {
/* Circuit Group              */
/* SPX Circuit Table   */

      case nwuSPXCircSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case nwuSPXCircIndex:
      return o_integer(oi, v, index);

      case nwuSPXCircState:
         if(spxFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case nwuSPXCircStartTime:
      return o_integer(oi, v, spxCircStats.con_startTime);

      case nwuSPXCircRetryCounts:
      return o_integer(oi, v, spxCircStats.con_retry_count);

      case nwuSPXCircRetryTime:
      return o_integer(oi, v, spxCircStats.con_retry_time);

      case nwuSPXCircLocNetNumber:
      return o_string(oi, v, (char *) &spxCircStats.con_addr.net, IPX_NET_SIZE);

      case nwuSPXCircLocNode:
      return o_string(oi, v, (char *) &spxCircStats.con_addr.node, IPX_NODE_SIZE);
      case nwuSPXCircEndNetNumber:
      return o_string(oi, v, (char *) &spxCircStats.o_addr.net, IPX_NET_SIZE);

      case nwuSPXCircEndNode:
      return o_string(oi, v, (char *) &spxCircStats.o_addr.node, IPX_NODE_SIZE);

      case nwuSPXCircType:
	if(spxCircStats.con_type == NWUMPS_SPX_TYPE)
		SPXCircType = NWUMPS_SPX;
	else if(spxCircStats.con_type == NWUMPS_SPXII_TYPE)
		SPXCircType = NWUMPS_SPXII;
	else
		SPXCircType = NWUMPS_UNKNOWN;
      return o_integer(oi, v, SPXCircType);

      case nwuSPXCircIPXChkSum:
      return o_integer(oi, v, spxCircStats.con_ipxChecksum);

      case nwuSPXCircSndWinSize:
      return o_integer(oi, v, spxCircStats.con_remote_window_size);

      case nwuSPXCircSndPktSize:
      return o_integer(oi, v, spxCircStats.con_send_packet_size);

      case nwuSPXCircSndMsgCounts:
      return o_integer(oi, v, spxCircStats.con_send_mesg_count);

      case nwuSPXCircSndPktCounts:
      return o_integer(oi, v, spxCircStats.con_send_packet_count);

      case nwuSPXCircSndNAKs:
      return o_integer(oi, v, spxCircStats.con_send_nak);

      case nwuSPXCircSndErrorCtrs:
	SPXCircSndErrorCtrs = spxCircStats.con_canput_fail +
                              spxCircStats.con_unknown_mesg_count +
                              spxCircStats.con_send_bad_mesg +
                              spxCircStats.con_send_packet_timeout +
                              spxCircStats.con_send_packet_nak;
      return o_integer(oi, v, SPXCircSndErrorCtrs);

      case nwuSPXCircRcvWinSize:
      return o_integer(oi, v, spxCircStats.con_window_size);

      case nwuSPXCircRcvPktSize:
      return o_integer(oi, v, spxCircStats.con_rcv_packet_size);

      case nwuSPXCircRcvPktCounts:
      return o_integer(oi, v, spxCircStats.con_rcv_packet_count);

      case nwuSPXCircRcvNAKs:
      return o_integer(oi, v, spxCircStats.con_rcv_nak);

      case nwuSPXCircRcvErrorCtrs:
	SPXCircRcvErrorCtrs = spxCircStats.con_rcv_bad_packet +
                              spxCircStats.con_rcv_bad_data_packet;
      return o_integer(oi, v, SPXCircRcvErrorCtrs);

      case nwuSPXCircRcvPktQues:
      return o_integer(oi, v, spxCircStats.con_rcv_packet_qued);

      case nwuSPXCircRcvPktSentUps:
      return o_integer(oi, v, spxCircStats.con_rcv_packet_sentup);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

int nwuSPXGetNumConn(void)      /* Return: max SPX connection open */
   {
   struct   strioctl ioc;
   spxStats_t  infobuf = {0};

   if(mode == NWUMPS_DEBUG)
      printf("%s: Opening up a spx device.\n", nwumpsTitleStr);

   ioc.ic_cmd = SPX_GET_STATS;
   ioc.ic_timout = 3;
   ioc.ic_len = sizeof(infobuf);
   ioc.ic_dp = (char *)&infobuf;

   if(mode == NWUMPS_DEBUG)
      printf("%s: Doing ioctl SPX_GET_STATS.\n", nwumpsTitleStr);

   if(ioctl(spxFd, I_STR, &ioc) < 0) 
      {
      sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), "SPX_GET_STATS",
 		spxDevice, spxSysStats);
      perror(errorStr);
      return (FAILURE);
      }

   return(infobuf.spx_max_used_connections);
   }

static int nwuSPXGetSysStats(void)
   {
   struct   strioctl ioc;
   static   time_t   LastTimeRefreshed;
            time_t   CurrentTime;

   int      maxConn;

   time(&CurrentTime);

   if(mode == NWUMPS_DEBUG)
      printf("The Current time is %s.\n", ctime(&CurrentTime));

   if(CurrentTime - LastTimeRefreshed > DATA_FRESHNESS)
      {
      if(mode == NWUMPS_DEBUG)
         printf("%s: Opening up a router device.\n", nwumpsTitleStr);

      if(mode == NWUMPS_DEBUG)
         printf("%s: Setting up ioc structure for SPX_GET_STATS.\n", nwumpsTitleStr);

      ioc.ic_cmd = SPX_GET_STATS;
      ioc.ic_timout = 3;
      ioc.ic_len = sizeof(spxSysStats);
      ioc.ic_dp = (char *)&spxSysStats;

      if(mode == NWUMPS_DEBUG)
         printf("%s: Doing ioctl SPX_GET_STATS.\n", nwumpsTitleStr);

      if(ioctl(spxFd, I_STR, &ioc) < 0) 
         {
         sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), "SPX_GET_STATS", spxDevice, spxSysStats);
         perror(errorStr);
         return (FAILURE);
         }

      time(&LastTimeRefreshed);

      if(mode == NWUMPS_DEBUG)
         printf("The last time the data was refreshed %s.\n", ctime(&CurrentTime));
      }
   else if(mode == NWUMPS_DEBUG)
      printf("The Data does not need to be refreshed.\n");

   return(SUCCESS);
   }

static int nwuSPXGetCircStats(int connID)    /* Return: status open */
   {
   struct strioctl strioc;       /* ioctl structure */
   static   time_t   LastTimeRefreshed;
            time_t   CurrentTime;

   time(&CurrentTime);

   if(mode == NWUMPS_DEBUG)
      printf("The Current time is %s.\n", ctime(&CurrentTime));

   if(CurrentTime - LastTimeRefreshed > DATA_FRESHNESS)
      {
      if(mode == NWUMPS_DEBUG)
         printf("%s: Opening up a router device.\n", nwumpsTitleStr);

      if(mode == NWUMPS_DEBUG)
         printf("%s: Setting up ioc structure for SPX_GET_CON_STATS.\n", nwumpsTitleStr);
   
      strioc.ic_cmd = SPX_GET_CON_STATS;
      strioc.ic_timout = 5;
      strioc.ic_len = sizeof(spxCircStats);
      strioc.ic_dp = (char *)&spxCircStats;

      spxCircStats.con_connection_id = GETINT16(connID);

      if(ioctl(spxFd, I_STR, &strioc) == -1) 
         {
         sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED), "SPX_GET_CON_STATS", spxDevice, spxCircStats);
         perror(errorStr);
         return (FAILURE);
         }

      time(&LastTimeRefreshed);

      if(mode == NWUMPS_DEBUG)
         printf("The last time the data was refreshed %s.\n", ctime(&CurrentTime));
      }
   else if(mode == NWUMPS_DEBUG)
      printf("The Data does not need to be refreshed.\n");

   return(SUCCESS);
   }


