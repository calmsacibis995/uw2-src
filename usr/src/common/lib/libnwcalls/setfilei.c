/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setfilei.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPSetFileInformation*******************************************
SYNTAX:  NWCCODE N_API NWAFPSetFileInformation
         (
            NWCONN_HANDLE            conn,
            nuint16                  volNum,
            nuint32                  AFPBaseID,
            nuint16                  reqBitMap,
            pnstr8                   AFPPathString,
            nuint16                  structSize,
            NW_AFP_SET_INFO NWPTR    pAFPSetInfo
         );

REMARKS:

ARGS:  > conn
       > volNum
       > AFPBaseID
       > reqBitMap
       > AFPPathString
       > structSize
       > pAFPSetInfo

INCLUDE: nwafp.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 16  AFP Set File Information

CHANGES: 25 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAFPSetFileInformation
(
  NWCONN_HANDLE            conn,
  nuint16                  volNum,
  nuint32                  AFPBaseID,
  nuint16                  reqBitMap,
  pnstr8                   AFPPathString,
  nuint16                  structSize,
  NW_AFP_SET_INFO NWPTR    pAFPSetInfo
)
{
   NWNCPAFPFileInfo macFileInfo;
   NW_AFP_SET_INFO  temp, NWPTR pInfo = pAFPSetInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(structSize > sizeof(NW_AFP_SET_INFO))
      structSize = sizeof(NW_AFP_SET_INFO);

   if(structSize < sizeof(NW_AFP_SET_INFO))
   {
      NWCMemMove(&temp, pAFPSetInfo, structSize);
      NWCMemSet(&temp + structSize, 0, sizeof(temp) - structSize);
      pInfo = &temp;
   }

   macFileInfo.luEntryID      = NSwap32(AFPBaseID);
   macFileInfo.suAttr         = pInfo->attributes;
   macFileInfo.suCreationDate = pInfo->creationDate;
   macFileInfo.suAccessDate   = pInfo->accessDate;
   macFileInfo.suModifyDate   = pInfo->modifyDate;
   macFileInfo.suModifyTime   = pInfo->modifyTime;
   macFileInfo.suBackupDate   = pInfo->backupDate;
   macFileInfo.suBackupTime   = pInfo->backupTime;

   NWCMemMove(macFileInfo.abuFinderInfo, pInfo->finderInfo, 32);
   NWCMemMove(macFileInfo.abuProDOSInfo, pInfo->proDOSInfo, 6);

   return ((NWCCODE) NWNCP35s16AFPSetFileInfo(&access, (nuint8) volNum,
               NSwap16(reqBitMap), (nuint8) *AFPPathString,
               AFPPathString+1, &macFileInfo));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setfilei.c,v 1.7 1994/09/26 17:49:57 rebekah Exp $
*/
