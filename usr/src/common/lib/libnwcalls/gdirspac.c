/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gdirspac.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwdirect.h"

/*manpage*NWGetDirSpaceInfo*************************************************
SYNTAX:  NWCCODE N_API NWGetDirSpaceInfo
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint16        volNum,
            DIR_SPACE_INFO NWPTR spaceInfo
         )

REMARKS: Returns information on space usage for a directory of a volume. If
         the dirHandle is a zero the volume information is returned.  This
         includes the purageable blocks information in the DIR_SPACE_INFO
         structure below, if a dirHandle is passed in a directory is assumed
         and the purageable blocks information is set to zero.

ARGS: <  spaceInfo
         This is the return information structure. The information
         returned is shown below:

         typedef struct
         {
            nuint32  totalBlocks;
            nuint32  availableBlocks;

            nuint32  purgeableBlocks;       This will be set to 0 if...
            nuint32  notYetPurgeableBlocks; ... dirHandle != 0;

            nuint32  totalDirEntries;
            nuint32  availableDirEntries;
            nuint32  reserved;
            nuint8   sectorsPerBlock;
            nuint8   volName[17];
         } DIR_SPACE_INFO;

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 44  Get Volume and Purge Information
         22 45  Get Directory Information

CHANGES: 9 Jul 1992 - rewrote - jwoodbur
         13 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDirSpaceInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint16        volNum,
   DIR_SPACE_INFO NWPTR spaceInfo
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(dirHandle)
   {
      spaceInfo->purgeableBlocks       = (nuint32) 0;
      spaceInfo->notYetPurgeableBlocks = (nuint32) 0;

      ccode = (NWCCODE) NWNCP22s45GetDirInfo(&access, (nuint8) dirHandle,
         &spaceInfo->totalBlocks, &spaceInfo->availableBlocks,
         &spaceInfo->totalDirEntries, &spaceInfo->availableDirEntries,
         (pnuint8)&spaceInfo->reserved, &spaceInfo->sectorsPerBlock,
         &spaceInfo->volLen, (pnstr8)&spaceInfo->volName[0]);
   }
   else
   {
      ccode = (NWCCODE) NWNCP22s44VolGetPurgeInfo(&access, (nuint8) volNum,
         &spaceInfo->totalBlocks, &spaceInfo->availableBlocks,
         &spaceInfo->purgeableBlocks, &spaceInfo->notYetPurgeableBlocks,
         (pnuint32)&spaceInfo->totalDirEntries, &spaceInfo->availableDirEntries,
         (pnuint8)&spaceInfo->reserved, &spaceInfo->sectorsPerBlock,
         &spaceInfo->volLen, (pnstr8)&spaceInfo->volName[0]);
   }

	if (ccode == 0)
	   spaceInfo->volName[spaceInfo->volLen] = 0x00;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gdirspac.c,v 1.7 1994/09/26 17:45:37 rebekah Exp $
*/
