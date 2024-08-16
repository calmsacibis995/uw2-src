/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getvolst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"
#include "nwvol.h"

/*manpage*NWGetVolumeStats**************************************************
SYNTAX:  NWCCODE N_API NWGetVolumeStats
         (
            NWCONN_HANDLE  conn,
            nuint8         volNum,
            VOL_STATS NWPTR volInfo
         )

REMARKS: This call returns information about a specified volume.  A client must
         have console operator rights to make this call.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Volume Number identifies the volume in a volume table in the file
         server.  The volume table contains information about each volume on the
         file server.  A NetWare v2.1 file server can accommodate up to 32
         volumes (0..31).

         Logical Drive Number is the logical drive number of the drive on which
         the volume exists.

         Block Size is the number of 512-byte sectors contained in each block of
         the specified volume.

         Starting Block indicates the number of the first block of the volume.

         Total Blocks indicates the total number of blocks in the specified volume.

         Free Blocks indicates the total number of unused blocks in the specified
         volume.

         Total Directory Entries indicates the number of directory slots that are
         allocated for the specified volume.

         Free Directory Entries indicates the number of unused directory slots.

         Actual Max Used Directory Entries indicates the most directory slots
         ever used at one time on the volume.

         Volume Hashed Flag indicates whether the volume is hashed in file
         server memory (0 = not hashed).

         Volume Cached Flag indicates whether the volume is cached in file
         server memory (0 = volume not cached).

         Volume Removable Flag indicates whether a user can physically remove
         the volume from the file server (0 = disk cannot be removed).

         Volume Mounted Flag indicates whether the volume is physically
         mounted in the file server (0 = volume is not mounted).

         Volume Name is the name given to the volume.  Volume Name can be 1
         to 16 characters long.  It cannot contain spaces or the characters * ? : \
         or /.  If it is less than 16 characters, the remaining characters must all
         be the null character.

ARGS:  > conn
       > volNum
      <  volInfo

INCLUDE: nwvol.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x8998   Disk Map Error
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 233  Get Volume Information

CHANGES: 21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetVolumeStats
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  VOL_STATS NWPTR volInfo
)
{
   nuint32 luTemp;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP23s233GetVolumeInfo(&access, volNum, &luTemp,
      &volInfo->volumeNumber, &volInfo->logicalDriveNumber,
      &volInfo->sectorsPerBlock, &volInfo->startingBlock,
      &volInfo->totalBlocks, &volInfo->availableBlocks,
      &volInfo->totalDirectorySlots, &volInfo->availableDirectorySlots,
      &volInfo->maxDirectorySlotsUsed, &volInfo->isHashing,
      &volInfo->isCaching, &volInfo->isRemovable, &volInfo->isMounted,
      volInfo->volumeName);

   if (ccode == 0)
      volInfo->systemElapsedTime = (nint32) luTemp;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getvolst.c,v 1.7 1994/09/26 17:46:25 rebekah Exp $
*/
