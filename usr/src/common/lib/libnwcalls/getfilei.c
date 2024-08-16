/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getfilei.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwafp.h"
#include "ncpafp.h"

/*manpage*NWAFPGetFileInformation*******************************************
SYNTAX:  NWCCODE N_API NWAFPGetFileInformation
         (
            NWCONN_HANDLE          conn,
            nuint16                volNum,
            nuint32                AFPEntryID,
            nuint16                reqMask,
            pnstr8                 path,
            nuint16                structSize,
            NW_AFP_FILE_INFO NWPTR AFPFileInfo
         )

REMARKS:

ARGS: >  conn
      >  volNum
      >  AFPEntryID
      >  reqMask
         Information requested bit mask

         AFP_GET_ATTRIBUTES       0x0001
         AFP_GET_PARENT_ID        0x0002
         AFP_GET_CREATE_DATE      0x0004
         AFP_GET_ACCESS_DATE      0x0008
         AFP_GET_MODIFY_DATETIME  0x0010
         AFP_GET_BACKUP_DATETIME  0x0020
         AFP_GET_FINDER_INFO      0x0040
         AFP_GET_LONG_NAME        0x0080
         AFP_GET_ENTRY_ID         0x0100
         AFP_GET_DATA_LEN         0x0200
         AFP_GET_RESOURCE_LEN     0x0400
         AFP_GET_NUM_OFFSPRING    0x0800
         AFP_GET_OWNER_ID         0x1000
         AFP_GET_SHORT_NAME       0x2000
         AFP_GET_ACCESS_RIGHTS    0x4000
         AFP_GET_PRO_DOS_INFO     0x8000

         Bits may be or'd together.

      >  path
         AFP style directory path, relative to AFPbaseID

      >  structSize
         Size of AFPFileInfo buffer

      <  AFPFileInfo
         - Structure for returning AFP file information
         typedef struct
         {
            nuint32 entryID;
            nuint32 parentID;
            nuint16 attributes;
            nuint32 dataForkLength;
            nuint32 resourceForkLength;
            nuint16 numOffspring;
            nuint16 creationDate;
            nuint16 accessDate;
            nuint16 modifyDate;
            nuint16 modifyTime;
            nuint16 backupDate;
            nuint16 backupTime;
            nuint8 finderInfo[32];
            nstr8 longName[34];
            nuint32 ownerID;
            nstr8 shortName[14];
            nuint16 accessPrivileges;
            nuint8 proDOSInfo[6];
         } AFPFILEINFO, NW_AFP_FILE_INFO;

         * Attributes are defined as follows:

            0x0100 = Read Only
            0x0200 = Hidden
            0x0400 = System
            0x0800 = Execute Only
            0x1000 = Subdirectory
            0x2000 = Archive
            0x4000 = Undefined
            0x8000 = Sharable File
            0x0001 = Search Mode
            0x0002 = Search Mode
            0x0004 = Search Mode
            0x0008 = Undefined
            0x0010 = Transaction
            0x0020 = Index
            0x0040 = Read Audit
            0x0080 = Write Audit

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 15  AFP Get File Information

CHANGES: 19 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPGetFileInformation
(
   NWCONN_HANDLE          conn,
   nuint16                volNum,
   nuint32                AFPEntryID,
   nuint16                reqMask,
   pnstr8                 path,
   nuint16                structSize,
   NW_AFP_FILE_INFO NWPTR AFPFileInfo
)
{
   NWNCPAFPFileInfo info;
   NW_AFP_FILE_INFO oldInfo;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE) NWNCP35s15AFPGetDirEntryInfo(&access,
         (nuint8) volNum, NSwap32(AFPEntryID), NSwap16(reqMask),
         path[0], path+1, &info)) == 0)

   {
      oldInfo.entryID            = NSwap32(info.luEntryID);
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

      NWCMemMove(AFPFileInfo, &oldInfo, structSize);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getfilei.c,v 1.7 1994/09/26 17:46:00 rebekah Exp $
*/
