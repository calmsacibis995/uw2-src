/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwudiag.c	1.12"
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
static char rcsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwudiag.c,v 1.13.2.1 1994/10/21 22:52:24 rbell Exp $";
#endif

/****************************************************************************
** Source file:   nwudiag.c
**
** Description:   
**
** Contained functions:
**                      nwuDiagInit()
**                      nwuDiagGetSysObj()
**                      nwuDiagSetSysObj()
**                      nwuDiagGetCircObj()
**                      nwuDiagSetCircObj()
**                      nwuDiagSendForStats()
**                      nwuDiagReceiveStats()
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

#include "sys/ipx_app.h"
#include "sys/diag_app.h"

#ifdef IPXE
#include "ipxe_app.h"
#endif /* IPXE */

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
extern   int   diagFlag;
extern   int   ipxFd;
extern   char *ipxDevice;

extern   IpxNetAddr_t    MyIPXNetAddr;
extern   IpxNodeAddr_t   MyIPXNodeAddr;

/* Diagnostic Structure */
DIAGSTAT    	diagStatisics = {0};

/* Forward References */
static   int   nwuDiagGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuDiagSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuDiagGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int   nwuDiagSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
         int   nwuDiagSendForStats(void);
         int   nwuDiagReceiveStats(void);

void nwuDiagInit(void)
   {
   register OT ot;

/* NetWare Diagnostics Groups */
/* System Group               */
/* Diagnostic System Table    */

   if(ot = text2obj("nwuDiagSysInstance"))
      {
      ot->ot_getfnx = nwuDiagGetSysObj;
      ot->ot_setfnx = nwuDiagSetSysObj;
      ot->ot_info = (caddr_t) nwuDiagSysInstance;
      }

   if(ot = text2obj("nwuDiagSysState"))
      {
      ot->ot_getfnx = nwuDiagGetSysObj;
      ot->ot_setfnx = nwuDiagSetSysObj;
      ot->ot_info = (caddr_t) nwuDiagSysState;
      }

   if(ot = text2obj("nwuDiagSysMajorVer"))
      {
      ot->ot_getfnx = nwuDiagGetSysObj;
      ot->ot_info = (caddr_t) nwuDiagSysMajorVer;
      }

   if(ot = text2obj("nwuDiagSysMinorVer"))
      {
      ot->ot_getfnx = nwuDiagGetSysObj;
      ot->ot_info = (caddr_t) nwuDiagSysMinorVer;
      }

   if(ot = text2obj("nwuDiagSysUpTime"))
      {
      ot->ot_getfnx = nwuDiagGetSysObj;
      ot->ot_info = (caddr_t) nwuDiagSysUpTime;
      }

/* Circuit Group              */
/* Diagnostic Circuit Table   */

   if(ot = text2obj("nwuDiagCircSysInstance"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircSysInstance;
      }

   if(ot = text2obj("nwuDiagCircIndex"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircIndex;
      }

   if(ot = text2obj("nwuDiagCircState"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircState;
      }

   if(ot = text2obj("nwuDiagCircIPXSPXReqs"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircIPXSPXReqs;
      }

   if(ot = text2obj("nwuDiagCircLanDvrReqs"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircLanDvrReqs;
      }

   if(ot = text2obj("nwuDiagCircFileSrvReqs"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircFileSrvReqs;
      }

   if(ot = text2obj("nwuDiagCircUnknownReqs"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_setfnx = nwuDiagSetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircUnknownReqs;
      }

   if(ot = text2obj("nwuDiagCircSPXDiagSocket"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircSPXDiagSocket;
      }

   if(ot = text2obj("nwuDiagCircTimeOfLastReq"))
      {
      ot->ot_getfnx = nwuDiagGetCircObj;
      ot->ot_info = (caddr_t) nwuDiagCircTimeOfLastReq;
      }
   }

static int  nwuDiagGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

   int     status = FAILURE;

   time_t  CurrentTime = 0;
   time_t  DiagnosticUpTime = 0;

   ifvar = (int) ot->ot_info;

   if(diagFlag == TRUE)
      status = nwuDiagSendForStats();

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
         printf("%s: This is a General Error Failure. The offset is %d.\n", nwumpsTitleStr, offset);
      return int_SNMP_error__status_genErr;
      }

   switch(ifvar)
      {
/* NetWare Diagnostics Groups */
/* System Group               */
/* Diagnostic System Table    */

      case nwuDiagSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of Diagnostics.  This will need
                                          to be changed if multiple Diagnostics
                                          are to be supported */

      case nwuDiagSysState:
         if(diagFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case nwuDiagSysMajorVer:
      return o_integer(oi, v, diagStatisics.MajorVersion);

      case nwuDiagSysMinorVer:
      return o_integer(oi, v, diagStatisics.MinorVersion);

      case nwuDiagSysUpTime:
         if(diagFlag == TRUE)
            {
            if(status == SUCCESS) 
               {
               time(&CurrentTime);
               DiagnosticUpTime = CurrentTime - diagStatisics.StartTime;

               if(mode == NWUMPS_DEBUG)
                   {
                   printf("%s: The Diagnostic start time is %s.\n", nwumpsTitleStr, ctime(&diagStatisics.StartTime));
                   printf("%s: The Current time is %s.\n", nwumpsTitleStr, ctime(&CurrentTime));
                   }
               }
            }
      return o_integer(oi, v, DiagnosticUpTime);


/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuDiagSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* NetWare Diagnostics Groups */
/* System Group               */
/* Diagnostic System Table    */

      case nwuDiagSysInstance:
      return int_SNMP_error__status_noError;

      case nwuDiagSysState:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuDiagGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

   int     status = FAILURE;

   time_t  CurrentTime = 0;
   time_t  DiagnosticUpTime = 0;

   ifvar = (int) ot->ot_info;

   if(diagFlag == TRUE)
      status = nwuDiagSendForStats();

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
         printf("%s: This is a General Error Failure. The offset is %d.\n", nwumpsTitleStr, offset);
      return int_SNMP_error__status_genErr;
      }

   switch(ifvar)
      {
/* Circuit Group              */
/* Diagnostic Circuit Table   */

      case nwuDiagCircSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of Diagnostics.  This will need
                                          to be changed if multiple Diagnostics
                                          are to be supported */

      case nwuDiagCircIndex:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of Diagnostics.  */

      case nwuDiagCircState:
         if(diagFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case nwuDiagCircIPXSPXReqs:
      return o_integer(oi, v, diagStatisics.IPXSPXReqs);

      case nwuDiagCircLanDvrReqs:
      return o_integer(oi, v, diagStatisics.LanDvrReqs);

      case nwuDiagCircFileSrvReqs:
      return o_integer(oi, v, diagStatisics.FileSrvReqs);

      case nwuDiagCircUnknownReqs:
      return o_integer(oi, v, diagStatisics.UnknownReqs);

      case nwuDiagCircSPXDiagSocket:
      return o_integer(oi, v, diagStatisics.SPXDiagSocket);

      case nwuDiagCircTimeOfLastReq:
      return o_integer(oi, v, diagStatisics.TimeOfLastReq);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuDiagSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* Circuit Group              */
/* Diagnostic Circuit Table   */

      case nwuDiagCircSysInstance:
      return int_SNMP_error__status_noError;

      case nwuDiagCircIndex:
      return int_SNMP_error__status_noError;

      case nwuDiagCircState:
      return int_SNMP_error__status_noError;

      case nwuDiagCircIPXSPXReqs:
      return int_SNMP_error__status_noError;

      case nwuDiagCircLanDvrReqs:
      return int_SNMP_error__status_noError;

      case nwuDiagCircFileSrvReqs:
      return int_SNMP_error__status_noError;

      case nwuDiagCircUnknownReqs:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

int nwuDiagSendForStats(void)
   {
   static time_t  LastTimeRefreshed = 0;
          time_t  CurrentTime = 0;

          uint16  ipxDiagSock = GETINT16(IPX_DIAGNOSTIC_SOCKET);
   static uint8   ipxAddr[12] = {0};

          uint8   ipxPktType = DIAG_STATISTICS;

   static struct  t_unitdata   udgram;

   time(&CurrentTime);

   if(mode == NWUMPS_DEBUG)
      printf("%s: The Current time is %s.\n", nwumpsTitleStr, ctime(&CurrentTime));

   if(CurrentTime - LastTimeRefreshed > DATA_FRESHNESS)
      {
      if(mode == NWUMPS_DEBUG)
         printf("%s: Setting up IPX Packet.\n", nwumpsTitleStr);

      IPXCOPYNET(&MyIPXNetAddr.myNetAddress, &ipxAddr[0]);
      IPXCOPYNODE(&MyIPXNodeAddr.myNodeAddress, &ipxAddr[4]);
      IPXCOPYSOCK(&ipxDiagSock, &ipxAddr[10]);

      udgram.addr.len = sizeof(ipxAddr_t);
      udgram.addr.maxlen = sizeof(ipxAddr_t);
      udgram.addr.buf = (char *)&ipxAddr[0];

      udgram.opt.len = sizeof(ipxPktType);
      udgram.opt.maxlen = sizeof(ipxPktType);
      udgram.opt.buf = (char *)&ipxPktType;

      udgram.udata.maxlen = sizeof(diagStatisics);
      udgram.udata.len = sizeof(diagStatisics);

      if(mode == NWUMPS_DEBUG)
         {
         printf("%s: Sending IPX Packet to the Diagnostic Responder at adrress: 0x%X%X%X%X %X%X%X%X%X%X %X%X.\n", 
		nwumpsTitleStr,
		ipxAddr[0], ipxAddr[1], ipxAddr[2], ipxAddr[3],	/* Network Address */
                ipxAddr[4], ipxAddr[5], ipxAddr[6], 	/* Node Address */
		ipxAddr[7], ipxAddr[8], ipxAddr[9], 
		ipxAddr[10], ipxAddr[11]);		/* Socket Address */
         }

      if(t_sndudata(ipxFd, &udgram) < 0)
         {
         printf("%s: The send udata failed.\n", nwumpsTitleStr);
         return(FAILURE);
         }

      if(nwuDiagReceiveStats() != SUCCESS)
         return(FAILURE);

      time(&LastTimeRefreshed);

      if(mode == NWUMPS_DEBUG)
         printf("%s: The last time the data was refreshed %s.\n", nwumpsTitleStr, ctime(&CurrentTime));
      }

   else if(mode == NWUMPS_DEBUG)
      printf("%s: The Data does not need to be refreshed.\n", nwumpsTitleStr);

   return(SUCCESS);
   }

int  nwuDiagReceiveStats(void)
   {  
   static struct  t_unitdata   udgram; 

   static uint8   ipxAddr[12] = {0};

   uint8   ipxPktType = 0;
   int      flags = 0;

   if(mode == NWUMPS_DEBUG)
      {
      printf("%s: You made it back from the Diagnostic Responder.\n", nwumpsTitleStr);
      }

   udgram.addr.len = sizeof(ipxAddr_t);
   udgram.addr.maxlen = sizeof(ipxAddr_t);
   udgram.addr.buf = (char *)&ipxAddr[0];

   udgram.opt.len = sizeof(ipxPktType);
   udgram.opt.maxlen = sizeof(ipxPktType);
   udgram.opt.buf = (char *)&ipxPktType;

   udgram.udata.maxlen = sizeof(diagStatisics);
   udgram.udata.buf = (caddr_t)&diagStatisics;

   ipxPktType = 0;

   if(t_rcvudata(ipxFd, &udgram, &flags) < 0)
      {
      printf("%s: The receive udata failed.\n", nwumpsTitleStr);
      return(FAILURE);
      }
	
   if(ipxPktType != DIAG_STATISTICS)
      {
      printf("%s: The ipxPktType wrong 0x%X.\n", 
			nwumpsTitleStr, ipxPktType);
      return(FAILURE);
      }

   if(udgram.udata.len != sizeof(diagStatisics))
      {
      printf("%s: The receive udata wrong length %d.\n", 
			nwumpsTitleStr, udgram.udata.len);
      return(FAILURE);
      }

   return(SUCCESS);
   }
