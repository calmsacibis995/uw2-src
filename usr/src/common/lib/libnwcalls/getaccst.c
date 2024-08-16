/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getaccst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpacct.h"

#include "nwacct.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

/*manpage*NWGetAccountStatus************************************************
SYNTAX:  NWCCODE N_API NWGetAccountStatus
         (
            NWCONN_HANDLE  conn,
            nuint16        objType,
            pnstr8         objName,
            pnint32        balance,
            pnint32        limit,
            HOLDS_STATUS N_FAR * Holds
         )

REMARKS:

ARGS: >  conn
      >  objType
      >  objName
      <  balance (optional)
         Receives the object's account balance

      <  limit (optional)
         Receives the lowest level the object's account balance can reach
         before the object can no longer buy services on the network

      <  pHolds (optional)
         Receives the hold information in HOLDS_STATUS

INCLUDE: nwacct.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Memory
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89FC    No Such Object
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 150  Get Current Account Status

CHANGES: 21 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetAccountStatus
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   pnint32        balance,
   pnint32        limit,
   HOLDS_STATUS   N_FAR * Holds
)
{
   nint i;
   NWCCODE ccode;
   nuint8  bucket[120];
   nuint8  tempObjNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   tempObjNameLen = (nuint8) NWLTruncateString(objName, 48);

   if((ccode = (NWCCODE) NWNCP23s150GetAccountStatus(&access,
         NSwap16(objType), tempObjNameLen, objName, balance,
         limit, bucket, (pNWNCPHoldsInfo) Holds->holds)) == 0)
   {
      if(Holds)
      {
         for(i = 0; i < 16; i++)
         {
            if (Holds->holds[i].amount == 0)
               break;
            Holds->holds[i].objectID = NSwap32(Holds->holds[i].objectID);
         }

         Holds->holdsCount = (nuint16) i;
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getaccst.c,v 1.7 1994/09/26 17:45:42 rebekah Exp $
*/
