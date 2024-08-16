/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:acctinst.c	1.4"
#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerExtendedInfo***************************************
SYNTAX:  NWCCODE N_API NWGetFileServerExtendedInfo
         (
            NWCONN_HANDLE  conn,
            pnuint8        accountingVer,
            pnuint8        VAPVer,
            pnuint8        queueingVer,
            pnuint8        printServerVer,
            pnuint8        virtualConsoleVer,
            pnuint8        securityVer,
            pnuint8        internetBridgeVer
         );

REMARKS:

ARGS: >  conn
      <  accountingVer (optional)
      <  VAPVer (optional)
      <  queueingVer (optional)
      <  printServerVer (optional)
      <  virtualConsoleVer (optional)
      <  securityVer (optional)
      <  internetBridgeVer (optional)

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWGetFileServerVersionInfo

NCP:     n/a

CHANGES: 9 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerExtendedInfo
(
   NWCONN_HANDLE  conn,
   pnuint8        accountingVer,
   pnuint8        VAPVer,
   pnuint8        queueingVer,
   pnuint8        printServerVer,
   pnuint8        virtualConsoleVer,
   pnuint8        securityVer,
   pnuint8        internetBridgeVer
)
{
   VERSION_INFO verInfo;
   NWCCODE ccode;

   if((ccode = NWGetFileServerVersionInfo(conn, &verInfo)) == 0)
   {
      if(accountingVer)
         *accountingVer = verInfo.accountVersion;
      if(VAPVer)
         *VAPVer = verInfo.VAPVersion;
      if(queueingVer)
         *queueingVer = verInfo.queueVersion;
      if(printServerVer)
         *printServerVer = verInfo.printVersion;
      if(virtualConsoleVer)
         *virtualConsoleVer = verInfo.virtualConsoleVersion;
      if(securityVer)
         *securityVer = verInfo.restrictionLevel;
      if(internetBridgeVer)
         *internetBridgeVer = verInfo.internetBridge;
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/acctinst.c,v 1.6 1994/06/08 23:07:22 rebekah Exp $
*/
