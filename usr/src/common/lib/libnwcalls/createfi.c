/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:createfi.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPCreateFile***************************************************
SYNTAX:  NWCCODE N_API NWAFPCreateFile
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        AFPEntryID,
            nuint8         delExistingFile,
            pnuint8        finderInfo,
            pnstr8         path,
            pnuint32       newAFPEntryID
         )

REMARKS:

ARGS: >  conn
      >  volNum
      >  AFPEntryID
      >  delExistingFile
      >  finderInfo
      >  path
      <  newAFPEntryID (optional)

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 14  AFP Create File

CHANGES: 19 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPCreateFile
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   nuint32        AFPEntryID,
   nuint8         delExistingFile,
   pnuint8        finderInfo,
   pnstr8         path,
   pnuint32       newAFPEntryID
)
{
   nuint8   proDOSInfo[6];
   nuint32  newID;
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(proDOSInfo, 0, 6);

   if ((ccode = (NWCCODE) NWNCP35s14AFPCreateFile(&access, (nuint8) volNum,
         NSwap32(AFPEntryID), delExistingFile, finderInfo,
         proDOSInfo, (nuint8) path[0], path + 1,
         &newID)) == 0)
   {
      if (newAFPEntryID)
         *newAFPEntryID = NSwap32(newID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/createfi.c,v 1.7 1994/09/26 17:44:54 rebekah Exp $
*/
