/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:conninfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpconn.h"

#include "nwclient.h"
#include "nwconnec.h"
#include "nwserver.h"

/*manpage*NWGetConnectionInformation****************************************
SYNTAX:  NWCCODE N_API NWGetConnectionInformation
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM     connNumber,
            pnstr8         objName,
            pnuint16       objType,
            pnuint32       objID,
            pnuint8        loginTime
         )

REMARKS:

ARGS: >  conn
      >  connNumber
      <  objName
      <  objType
      <  objID
      <  loginTime

INCLUDE: nwconnec.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Space
         0x89FC    No Such Object
         0x89FD    Bad Station Number
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 22  Get Station's Logged Info
         23 28  Get Station's Logged Info

CHANGES: 15 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetConnectionInformation
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNumber,
   pnstr8         objName,
   pnuint16       objType,
   pnuint32       objID,
   pnuint8        loginTime
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint32 tempObjID;
   nuint16 tempObjType;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if (serverVer >= 3110 || serverVer < 2000)
   {
      ccode = (NWCCODE) NWNCP23s28GetStationLoggedInfo(&access,
               (nuint32) connNumber, &tempObjID, &tempObjType,
               objName, loginTime, NULL);
   }
   else
   {
      ccode = (NWCCODE) NWNCP23s22GetStationLoggedInfo(&access,
               (nuint8) connNumber, &tempObjID, &tempObjType,
               objName, loginTime, NULL);
   }

   if (objID)
      *objID = NSwap32(tempObjID);

   if (objType)
      *objType = NSwap16(tempObjType);

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/conninfo.c,v 1.7 1994/09/26 17:44:41 rebekah Exp $
*/
