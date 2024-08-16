/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gfileina.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwafp.h"
#include "nwcaldef.h"

/*manpage*NWAFPGetEntryIDFromName*******************************************
SYNTAX:  NWCCODE N_API NWAFPGetEntryIDFromName
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        AFPEntryID,
            pnstr8         AFPPathString,
            pnuint32       newAFPEntryID
         );

REMARKS:

ARGS: >  conn
      >  volNum
      >  AFPEntryID
      >  AFPPathString
      <  newAFPEntryID

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 04  AFP Get Entry ID From Name

CHANGES: 19 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPGetEntryIDFromName
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   nuint32        AFPEntryID,
   pnstr8         AFPPathString,
   pnuint32       newAFPEntryID
)
{
   nuint32 luTmpID;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE) NWNCP35s4AFPGetEntryIDFromName(&access,
         (nuint8) volNum, NSwap32(AFPEntryID), AFPPathString[0],
         AFPPathString + 1, &luTmpID)) == 0)
   {
      if (newAFPEntryID)
         *newAFPEntryID = NSwap32(luTmpID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gfileina.c,v 1.7 1994/09/26 17:46:28 rebekah Exp $
*/
