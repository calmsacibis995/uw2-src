/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scanafpf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPScanFileInformation******************************************
SYNTAX:  NWCCODE N_API NWAFPScanFileInformation
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        AFPEntryID,
            pnuint32       AFPLastSeenID,
            nuint16        searchMask,
            nuint16        reqMask,
            pnstr8         AFPPathString,
            nuint16        structSize,
            NW_AFP_FILE_INFO NWPTR pAFPFileInfo
         );

REMARKS: NWAFPScanFileInformation returns information about an AFP entry
         (directory or file). Iterative scanning of a directory is supported.

         If AFPPathString specifies a directory, all entries within the
         directory that fit the criteria of the searchMask and requestMask
         will be returned.

ARGS: >  conn
         connection handle to the server

      >  volNum
         Volume number of directory entry

      >  AFPEntryID
         AFP base ID.

      <> AFPLastSeenID
         This should be set to -1 for first call, for each subsequent
         call, this will be set to the entry ID of the previous call.

      >  searchMask
         Specifies which types of files to search for (AFP_SA_ constants
         in nwafp.h)

      >  reqMask
         Information requested bit mask (AFP_ constants in nwafp.h)

      >  AFPPathString
         AFP style directory path, relative to AFPbaseID

      >  structSize
         Size of pAFPFileInfo buffer

      <  pAFPFileInfo
         Structure for returning AFP INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 17  AFP Scan File Information

CHANGES: 18 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAFPScanFileInformation
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   nuint32        AFPEntryID,
   pnuint32       AFPLastSeenID,
   nuint16        searchMask,
   nuint16        reqMask,
   pnstr8         AFPPathString,
   nuint16        structSize,
   NW_AFP_FILE_INFO NWPTR pAFPFileInfo
)
{
   NWNCPAFPFileInfo info;
   NW_AFP_FILE_INFO oldInfo;
   NWCCODE  ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE) NWNCP35s17AFPScanFileInfo(&access,
         (nuint8) volNum, NSwap32(AFPEntryID),
         NSwap32(*AFPLastSeenID), (nuint16) 1,
         searchMask, NSwap16(reqMask),
         AFPPathString[0], &AFPPathString[1],
         NULL, &info)) == 0)
   {
      *AFPLastSeenID = oldInfo.entryID = NSwap32(info.luEntryID);
      oldInfo.parentID           = NSwap32(info.luParentID);
      oldInfo.attributes         = info.suAttr;
      oldInfo.dataForkLength     = info.luDataForkLen;
      oldInfo.resourceForkLength = info.luResourceForkLen;
      oldInfo.numOffspring       = info.suTotalOffspring;
      oldInfo.creationDate       = info.suCreationDate;
      oldInfo.accessDate         = info.suAccessDate;
      oldInfo.modifyDate         = info.suModifyDate;
      oldInfo.modifyTime         = info.suModifyTime;
      oldInfo.backupDate         = info.suBackupDate;
      oldInfo.backupTime         = info.suBackupTime;
      NWCMemMove(oldInfo.finderInfo, info.abuFinderInfo, (nuint) 32);
      NWCMemMove(oldInfo.longName,   info.abuLongName, (nuint) 32);
      oldInfo.ownerID            = NSwap32(info.luOwnerID);
      NWCMemMove(oldInfo.shortName,  info.abuShortName, (nuint) 12);
      oldInfo.accessPrivileges   = info.suAccessPrivileges;
      NWCMemMove(oldInfo.proDOSInfo, info.abuProDOSInfo, (nuint) 6);

      oldInfo.longName[32]  =
      oldInfo.longName[33]  =
      oldInfo.shortName[12] =
      oldInfo.shortName[13] = (nuint8) 0;

      /** Copy the number of bytes requested to the return buffer **/

      if(sizeof(NW_AFP_FILE_INFO) < structSize)
         structSize = sizeof(NW_AFP_FILE_INFO);

      NWCMemMove(pAFPFileInfo, &oldInfo, structSize);
   }
   else
      *AFPLastSeenID = (nuint32) -1L;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scanafpf.c,v 1.7 1994/09/26 17:49:18 rebekah Exp $
*/
