/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/nwumps/nwuripsa.c	1.13"
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
static char rcsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/nwumps/nwuripsa.c,v 1.13.2.2 1994/10/21 22:52:33 rbell Exp $";
#endif

/****************************************************************************
** Source file:   nwuripsa.c
**
** Description:   
**
** Contained functions:
**                      nwuRIPSAPInit()
**                      nwuRIPGetStats()
**
**                      nwuRIPGetSysObj()
**                      nwuRIPSetSysObj()
**                      nwuRIPGetCircObj()
**                      nwuRIPSetCircObj()
**
**                      nwuSAPGetSysObj()
**                      nwuSAPSetSysObj()
**                      nwuSAPGetCircObj()
**                      nwuSAPSetCircObj()
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

#include "sys/ripx_app.h"
#include "sys/sap_app.h"
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

/* External Variables */
extern   char  errorStr[];
extern   char  nwumpsTitleStr[];
extern   int   mode;
extern   int   sapFlag;
extern   SAPD  SAPStats;

extern   int  ripxFd;
extern   char *ripxDevice;

/* Rip Structures */
RouterInfo_t   ripRouterStat = {0};


/* Forward References */
         int nwuRIPGetStats(void);

static   int nwuRIPGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuRIPSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuRIPGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuRIPSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);

static   int nwuSAPGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuSAPSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuSAPGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);
static   int nwuSAPSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset);

void nwuRIPSAPInit(void) 
   {
   register OT ot;

/* System Group         */
/* RIP System Table     */

   if(ot = text2obj("ripSysInstance"))
      {
      ot->ot_getfnx = nwuRIPGetSysObj;
      ot->ot_setfnx = nwuRIPSetSysObj;
      ot->ot_info = (caddr_t) ripSysInstance;
      }

   if(ot = text2obj("ripSysState"))
      {
      ot->ot_getfnx = nwuRIPGetSysObj;
      ot->ot_setfnx = nwuRIPSetSysObj;
      ot->ot_info = (caddr_t) ripSysState;
      }

   if(ot = text2obj("ripSysIncorrectPackets"))
      {
      ot->ot_getfnx = nwuRIPGetSysObj;
      ot->ot_info = (caddr_t) ripSysIncorrectPackets;
      }

/* SAP System Table  */

   if(ot = text2obj("sapSysInstance"))
      {
      ot->ot_getfnx = nwuSAPGetSysObj;
      ot->ot_setfnx = nwuSAPSetSysObj;
      ot->ot_info = (caddr_t) sapSysInstance;
      }

   if(ot = text2obj("sapSysState"))
      {
      ot->ot_getfnx = nwuSAPGetSysObj;
      ot->ot_setfnx = nwuSAPSetSysObj;
      ot->ot_info = (caddr_t) sapSysState;
      }
   
   if(ot = text2obj("sapSysIncorrectPackets"))
      {
      ot->ot_getfnx = nwuSAPGetSysObj;
      ot->ot_info = (caddr_t) sapSysIncorrectPackets;
      }

/* Circuit Group     */
/* RIP Circuit Table */

   if(ot = text2obj("ripCircSysInstance"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircSysInstance;
      }

   if(ot = text2obj("ripCircIndex"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircIndex;
      }

   if(ot = text2obj("ripCircState"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircState;
      }

   if(ot = text2obj("ripCircPace"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircPace;
      }

   if(ot = text2obj("ripCircUpdate"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircUpdate;
      }

   if(ot = text2obj("ripCircAgeMultiplier"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircAgeMultiplier;
      }

   if(ot = text2obj("ripCircPacketSize"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_setfnx = nwuRIPSetCircObj;
      ot->ot_info = (caddr_t) ripCircPacketSize;
      }

   if(ot = text2obj("ripCircOutPackets"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_info = (caddr_t) ripCircOutPackets;
      }

   if(ot = text2obj("ripCircInPackets"))
      {
      ot->ot_getfnx = nwuRIPGetCircObj;
      ot->ot_info = (caddr_t) ripCircInPackets;
      }

/* SAP Circuit Table */

   if(ot = text2obj("sapCircSysInstance"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircSysInstance;
      }

   if(ot = text2obj("sapCircIndex"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircIndex;
      }

   if(ot = text2obj("sapCircState"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircState;
      }

   if(ot = text2obj("sapCircPace"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircPace;
      }

   if(ot = text2obj("sapCircUpdate"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircUpdate;
      }

   if(ot = text2obj("sapCircAgeMultiplier"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircAgeMultiplier;
      }

   if(ot = text2obj("sapCircPacketSize"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircPacketSize;
      }

   if(ot = text2obj("sapCircGetNearestServerReply"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_setfnx = nwuSAPSetCircObj;
      ot->ot_info = (caddr_t) sapCircGetNearestServerReply;
      }

   if(ot = text2obj("sapCircOutPackets"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_info = (caddr_t) sapCircOutPackets;
      }

   if(ot = text2obj("sapCircInPackets"))
      {
      ot->ot_getfnx = nwuSAPGetCircObj;
      ot->ot_info = (caddr_t) sapCircInPackets;
      }
   }

static int  nwuRIPGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            int      status;

            uint32   RIPSysIncorrectPackets = 0;

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
/* RIP System Table     */

      case ripSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ripSysState:
      return o_integer(oi, v, NWUMPS_ON);

      case ripSysIncorrectPackets:
         nwuRIPGetStats();
         RIPSysIncorrectPackets = ripRouterStat.ReceivedNoLanKey +
                                 ripRouterStat.ReceivedBadLength +
                                 ripRouterStat.ReceivedNoCoalesce +
                                 ripRouterStat.ReceivedUnknownRequest +
                                 ripRouterStat.SentAllocFailed +
                                 ripRouterStat.SentBadDestination;
      return o_integer(oi, v, RIPSysIncorrectPackets);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuRIPSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* RIP System Table     */

      case ripSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case ripSysState:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }


static int  nwuRIPGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            int      status;

            uint32   RIPCircOutPackets = 0;

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
/* Circuit Group     */
/* RIP Circuit Table */

      case ripCircSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case ripCircIndex:
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ripCircState:
      return o_integer(oi, v, NWUMPS_ON);

      case ripCircPace:
         nwuRIPGetStats();
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ripCircUpdate:
         nwuRIPGetStats();
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ripCircAgeMultiplier:
         nwuRIPGetStats();
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ripCircPacketSize:
         nwuRIPGetStats();
      return o_integer(oi, v, NWUMPS_NOT_SUPPORTED);

      case ripCircOutPackets:
         nwuRIPGetStats();
         RIPCircOutPackets = ripRouterStat.SentResponsePackets +
                              ripRouterStat.SentRequestPackets;
      return o_integer(oi, v, RIPCircOutPackets);

      case ripCircInPackets:
         nwuRIPGetStats();
      return o_integer(oi, v, ripRouterStat.ReceivedPackets);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuRIPSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* Circuit Group     */
/* RIP Circuit Table */

      case ripCircSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case ripCircIndex:
      return int_SNMP_error__status_noError;

      case ripCircState:
      return int_SNMP_error__status_noError;

      case ripCircPace:
      return int_SNMP_error__status_noError;

      case ripCircUpdate:
      return int_SNMP_error__status_noError;

      case ripCircAgeMultiplier:
      return int_SNMP_error__status_noError;

      case ripCircPacketSize:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuSAPGetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            uint32   SAPSysIncorrectPackets = 0;

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
/* SAP System Table  */

      case sapSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case sapSysState:
         if(sapFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case sapSysIncorrectPackets:
         if(sapFlag == TRUE)
            SAPSysIncorrectPackets = SAPStats.NotNeighbor + 
                                    SAPStats.BadSizeInSaps;
      return o_integer(oi, v, SAPSysIncorrectPackets);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }
static int  nwuSAPSetSysObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* SAP System Table  */

      case sapSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case sapSysState:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noError;
      }
   }


static int  nwuSAPGetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
   {
            int   ifvar;
   register OID   oid = oi->oi_name;
   register OT    ot = oi->oi_type;
            OID   new;

            int   index = 0;
            int   i = 0;

   static   int      NumSAPCircuits = 0;
            uint32   SAPCircPace = 0;

            SAPL  LanData = {0};

   int      status;

   if(sapFlag == TRUE)
      NumSAPCircuits = (int)SAPStats.Lans;

   ifvar = (int) ot->ot_info;

   switch(offset) 
      {
      case NWtype_SNMP_SMUX__PDUs_get__request:
         if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
            {
            return int_SNMP_error__status_noSuchName;
            }

         index = oid->oid_elements[oid->oid_nelem - 1];

         if(index >= NumSAPCircuits)
            return (int_SNMP_error__status_noSuchName);

      break;

      case NWtype_SNMP_SMUX__PDUs_get__next__request:
         if(oid->oid_nelem > ot->ot_name->oid_nelem)
            {
            i = ot->ot_name->oid_nelem;
            oid->oid_elements[i]++;

            if(oid->oid_elements[i] >= NumSAPCircuits)
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
      printf("\nSAPGetLanData Index: %d.\n", index);

   if(sapFlag == TRUE)
      SAPGetLanData(index, &LanData);

   switch(ifvar)
      {
/* Circuit Group     */
/* SAP Circuit Table */

      case sapCircSysInstance:
      return o_integer(oi, v, 0);      /* Currently there is only one
                                          instance of IPX.  This will need
                                          to be changed if multiple IPX
                                          are to be supported */

      case sapCircIndex:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Lan Index.\n", LanData.LanNumber);
      return o_integer(oi, v, LanData.LanNumber);

      case sapCircState:
         if(sapFlag == TRUE)
            return o_integer(oi, v, NWUMPS_ON);
         else
            return o_integer(oi, v, NWUMPS_OFF);

      case sapCircPace:
         if(mode == NWUMPS_DEBUG)
            {
            printf("%6d Packet Size in bytes.\n", LanData.PacketSize);
            printf("%6d Line Speed in bits/sec.\n", LanData.LineSpeed);
            printf("%6d Minimum time in ms between packets.\n",
                     LanData.PacketGap);
            }

       if(LanData.LineSpeed < 1)
            SAPCircPace = 0;
       else
            SAPCircPace = ((LanData.PacketSize / (LanData.LineSpeed / 8)) + 
                        (LanData.PacketGap / 1000));

         if(mode == NWUMPS_DEBUG)
            printf("%6d The maximum pace at which SAP packets may be sent in packets/sec.\n", SAPCircPace);
      return o_integer(oi, v, SAPCircPace);

      case sapCircUpdate:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Periodic update interval in seconds.\n", 
                     LanData.UpdateInterval);
      return o_integer(oi, v, LanData.UpdateInterval);

      case sapCircAgeMultiplier:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Periodic Intervals before timeout a server.\n",
                     LanData.AgeFactor);
      return o_integer(oi, v, LanData.AgeFactor);

      case sapCircPacketSize:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Packet Size.\n", LanData.PacketSize);
      return o_integer(oi, v, LanData.PacketSize);

      case sapCircGetNearestServerReply:
      return o_integer(oi, v, NWUMPS_YES);

      case sapCircOutPackets:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Packets Sent.\n", LanData.PacketsSent);
      return o_integer(oi, v, LanData.PacketsSent);

      case sapCircInPackets:
         if(mode == NWUMPS_DEBUG)
            printf("%6d Packets Received.\n", LanData.PacketsReceived);
      return o_integer(oi, v, LanData.PacketsReceived);

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

static int  nwuSAPSetCircObj(OI oi, register struct type_SNMP_VarBind *v, int offset)
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
/* Circuit Group     */
/* SAP Circuit Table */

      case sapCircSysInstance:
      return int_SNMP_error__status_noError; /* Currently there is only one
                                                   instance of IPX.  This will 
                                                   need to be changed if 
                                                   multiple IPX are to be 
                                                   supported */

      case sapCircIndex:
      return int_SNMP_error__status_noError;

      case sapCircState:
      return int_SNMP_error__status_noError;

      case sapCircPace:
      return int_SNMP_error__status_noError;

      case sapCircUpdate:
      return int_SNMP_error__status_noError;

      case sapCircAgeMultiplier:
      return int_SNMP_error__status_noError;

      case sapCircPacketSize:
      return int_SNMP_error__status_noError;

      case sapCircGetNearestServerReply:
      return int_SNMP_error__status_noError;

/* Default  */

      default:
      return int_SNMP_error__status_noSuchName;
      }
   }

int nwuRIPGetStats(void)
   {
   struct   strioctl ioc;
   static   time_t   LastTimeRefreshed;
            time_t   CurrentTime;

   time(&CurrentTime);

   if(mode == NWUMPS_DEBUG)
      printf("The Current time is %s.\n", ctime(&CurrentTime));

   if(CurrentTime - LastTimeRefreshed > DATA_FRESHNESS)
      {
      if(mode == NWUMPS_DEBUG)
         printf("%s: Setting up ioc structure for RIPX_STATS.\n", nwumpsTitleStr);

      ioc.ic_cmd = RIPX_STATS;
      ioc.ic_timout = 3;
      ioc.ic_len = sizeof(ripRouterStat);
      ioc.ic_dp = (char *)&ripRouterStat;

      if(mode == NWUMPS_DEBUG)
         printf("%s: Doing ioctl RIPX_STATS.\n", nwumpsTitleStr);

      if(ioctl(ripxFd, I_STR, &ioc) < 0) 
         {
         sprintf(errorStr, MsgGetStr(NWUMPSD_IOCTL_TO_FAILED),"RIPX_STATS", ripxDevice, ripRouterStat);
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
