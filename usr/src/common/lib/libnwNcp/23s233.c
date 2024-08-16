/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s233.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s233GetVolumeInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s233GetVolumeInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint32 pluSystemIntervalMarker,
            pnuint8  pbuVolNum,
            pnuint8  pbuLogicalDriveNum,
            pnuint16 psuBlockSize,
            pnuint16 psuStartingBlock,
            pnuint16 psuTotalBlocks,
            pnuint16 psuFreeBlocks,
            pnuint16 psuTotalDirEntries,
            pnuint16 psuFreeDirEntries,
            pnuint16 psuActualMaxUsedDirEntries,
            pnuint8  pbuVolHashedFlag,
            pnuint8  pbuVolCachedFlag,
            pnuint8  pbuVolRemovableFlag,
            pnuint8  pbuVolMountedFlag,
            pnstr8   pbstrVolName,
         );

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

ARGS: <> pAccess
      >  buVolNum
      <  pluSystemIntervalMarker
      <  pbuVolNum
      <  pbuLogicalDriveNum
      <  psuBlockSize
      <  psuStartingBlock
      <  psuTotalBlocks
      <  psuFreeBlocks
      <  psuTotalDirEntries
      <  psuFreeDirEntries
      <  psuActualMaxUsedDirEntries
      <  pbuVolHashedFlag
      <  pbuVolCachedFlag
      <  pbuVolRemovableFlag
      <  pbuVolMountedFlag
      <  pbstrVolName,

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x8998   Disk Map Error
         0x89C6   No Console Rights

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 233  Get Volume Information

CHANGES: 21 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s233GetVolumeInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint32 pluSystemIntervalMarker,
   pnuint8  pbuVolNum,
   pnuint8  pbuLogicalDriveNum,
   pnuint16 psuBlockSize,
   pnuint16 psuStartingBlock,
   pnuint16 psuTotalBlocks,
   pnuint16 psuFreeBlocks,
   pnuint16 psuTotalDirEntries,
   pnuint16 psuFreeDirEntries,
   pnuint16 psuActualMaxUsedDirEntries,
   pnuint8  pbuVolHashedFlag,
   pnuint8  pbuVolCachedFlag,
   pnuint8  pbuVolRemovableFlag,
   pnuint8  pbuVolMountedFlag,
   pnstr8   pbstrVolName
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        233)
   #define NCP_STRUCT_LEN     ((nuint16)         2)
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REP_LEN        ((nuint)          40)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REP_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuRep,
               NCP_REP_LEN, NULL);
   if(lCode == 0)
   {
      if(pluSystemIntervalMarker)
            NCopyHiLo32(pluSystemIntervalMarker,&abuRep[0]);

      if(pbuVolNum)
            *pbuVolNum = abuRep[4];

      if(pbuLogicalDriveNum)
            *pbuLogicalDriveNum = abuRep[5];

      if(psuBlockSize)
            NCopyHiLo16(psuBlockSize, &abuRep[6]);

      if(psuStartingBlock)
            NCopyHiLo16(psuStartingBlock, &abuRep[8]);

      if(psuTotalBlocks)
            NCopyHiLo16(psuTotalBlocks, &abuRep[10]);

      if(psuFreeBlocks)
            NCopyHiLo16(psuFreeBlocks, &abuRep[12]);

      if(psuTotalDirEntries)
            NCopyHiLo16(psuTotalDirEntries, &abuRep[14]);

      if(psuFreeDirEntries)
            NCopyHiLo16(psuFreeDirEntries, &abuRep[16]);

      if(psuActualMaxUsedDirEntries)
            NCopyHiLo16(psuActualMaxUsedDirEntries, &abuRep[18]);

      if(pbuVolHashedFlag)
            *pbuVolHashedFlag= abuRep[20];

      if(pbuVolCachedFlag)
            *pbuVolCachedFlag= abuRep[21];

      if(pbuVolRemovableFlag)
            *pbuVolRemovableFlag= abuRep[22];

      if(pbuVolMountedFlag)
            *pbuVolMountedFlag= abuRep[23];

      if(pbstrVolName)
            NWCMemMove(pbstrVolName,&abuRep[24],16);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s233.c,v 1.7 1994/09/26 17:36:42 rebekah Exp $
*/
