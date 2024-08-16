/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:createdi.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPCreateDirectory**********************************************
SYNTAX:  NWCCODE N_API NWAFPCreateDirectory
         (
            NWCONN_HANDLE   conn,
            nuint16         volNum,
            nuint32         APFEntryID,
            pnuint8         finderInfo,
            pnstr8          path,
            pnuint32        newAFPEntryID
         )

REMARKS:

ARGS: >  volNum
         Volume number of directory entry

      >  APFEntryID
         AFP base ID

      >  finderInfo
         Finder information for new directory

      >  path
         AFP style directory path, relative to AFPbaseID

      <  newAFPEntryID
         ID of the newly created directory.

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 13  AFP Create Directory

CHANGES: 18 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAFPCreateDirectory
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  nuint32         APFEntryID,
  pnuint8         finderInfo,
  pnstr8          path,
  pnuint32        newAFPEntryID
)
{
   nuint8   proDOSInfo[6];
   nuint32  tempID;
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(proDOSInfo, 0, (nuint) 6);

   if ((ccode = (NWCCODE) NWNCP35s13AFPCreateDir(&access, (nuint8) volNum,
         NSwap32(APFEntryID), (nuint8) 0, finderInfo, proDOSInfo,
         path[0], path + 1, &tempID)) == 0)
   {
      if (newAFPEntryID)
         *newAFPEntryID = NSwap32(tempID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/createdi.c,v 1.7 1994/09/26 17:44:53 rebekah Exp $
*/
