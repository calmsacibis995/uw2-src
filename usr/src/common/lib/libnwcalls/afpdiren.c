/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:afpdiren.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwafp.h"
#include "nwdpath.h"
#include "nwvol.h"
#include "nwnamspc.h"
#include "nwserver.h"

/*manpage*NWAFPDirectoryEntry***********************************************
SYNTAX:  NWCCODE N_API NWAFPDirectoryEntry
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 path
         );

REMARKS: Test a directory entry to see if it is a MAC file.

         Determine if a file or directory is a MAC file.  The Directory Base
         and Path specifications are given in Netware "short name" format.

ARGS: >  conn
      >  dirHandle
      >  path

INCLUDE: nwafp.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     n/a

CHANGES: 27 Aug 1993 - changed to bible spec - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPDirectoryEntry
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8 path
)
{
   NWCCODE ccode;
   nuint32 entryID;
   AFPFILEINFO AFPFileInfo;
   nstr8 volName[300];
   nuint16 volNum, serverVer;
   nint longNameLen;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      NW_ENTRY_INFO entryInfo;

      ccode = NWGetNSEntryInfo(conn, dirHandle, path,
                  NW_NS_DOS, NW_NS_DOS, 0x8000,
                  IM_OWNING_NAMESPACE, &entryInfo);
      if(!ccode)
         return (entryInfo.NSCreator == NW_NS_MAC ? 1 : 0);
   }

   if((ccode = NWAFPGetEntryIDFromPathName(conn, dirHandle,
                  path, &entryID)) != 0)
      return (ccode);

   if(dirHandle)
      NWGetVolumeInfoWithHandle(conn, dirHandle, volName,
            NULL, NULL, NULL, NULL, NULL, NULL);
   else
      ccode = NWParsePathConnRef(path, NULL, NULL, volName, NULL);

   if((ccode = NWGetVolumeNumber(conn, volName, &volNum)) != 0)
      return (ccode);

   if((ccode = NWAFPGetFileInformation(conn, volNum, entryID,
                  (nuint16)(AFP_GET_RESOURCE_LEN | AFP_GET_LONG_NAME |
                            AFP_GET_SHORT_NAME   | AFP_GET_FINDER_INFO),
                  (pnstr8)"", (nuint16)sizeof(AFPFileInfo), &AFPFileInfo)) != 0)
      return (ccode);

   if(AFPFileInfo.resourceForkLength > 0)
      return (1);

   longNameLen = NWCStrLen(AFPFileInfo.longName);
   if(longNameLen != NWCStrLen(AFPFileInfo.shortName))
      return (1);

   if(NWCMemCmp(AFPFileInfo.longName, AFPFileInfo.shortName, (nuint8) longNameLen))
      return (1);

   if(!NWCMemCmp(&AFPFileInfo.finderInfo[4], "mdos", 4))
      return (0);

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/afpdiren.c,v 1.7 1994/09/26 17:43:55 rebekah Exp $
*/

