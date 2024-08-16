/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s216.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s216ReadPhyDiskStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s216ReadPhyDiskStats
         (
            pNWAccess pAccess,
            nuint8   buPhysicalDiskNum,
            pNWNCPDiskStats pDiskStats,
         );

REMARKS: This call returns statistics about a specified disk.  A client must have
         console operator rights to make this call.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Physical Disk Channel indicates the disk channel to which the disk unit
         is attached.

         Drive Removable Flag indicates whether a disk is removable (0 =
         nonremovable).

         Physical Drive Type indicates the type of drive.

            1 = XT,
            2 = AT,
            3 = SCSI,
            4 = disk coprocessor,
            5 = PS/2 with MFM Controller,
            6 = PS/2 with ESDI Controller,
            7 = Convergent Technology SBIC,
            50..255 = Value-Added Disk Drive.

         Controller Drive Number indicates the drive number of the disk unit
         relative to the controller number.

         Controller Number contains the address on the physical disk channel of
         the disk controller.

         Controller Type contains the number identifying the type (make and
         model) of the disk controller.

         Drive Size indicates the size of the physical drive in blocks (1 block =
         4,096 bytes).  The drive size does not include the portion of the disk
         reserved for Hot Fix redirection in the event of media errors.

         Drive Cylinders indicates the number of physical cylinders on the drive.

         Drive Heads indicates the number of disk heads on the drive.

         Sectors Per Track indicates the number of sectors on each disk track (1
         sector = 512 bytes).

         Drive Definition String contains the make and model of the drive (null-
         terminated string).

         IO Error Count indicates the number of times I/O errors have occurred
         on the disk since the server was brought up.

         Hot Fix Table Start indicates the first block of the disk Hot Fix
         Redirection Table.  This field is only meaningful with SFT NetWare
         Level I or above.  The redirection table is used to replace bad disk blocks
         with usable blocks in the event that a media failure occurs on the disk.

         Hot Fix Table Size indicates the total number of redirection blocks set
         aside on the disk for Hot Fix redirection.  Some or all of these blocks
         may be in use.  This field is only meaningful with SFT NetWare Level I
         or above.  To determine the number of redirection blocks still available
         for future use, see the Hot Fix Blocks Available field.

         Hot Fix Blocks Available indicates the number of redirection blocks that
         are still available.  This field is only meaningful on SFT NetWare Level
         I or above.

         Hot Fix Disabled indicates whether Hot Fix is enabled or disabled.  This
         field is only meaningful with SFT NetWare Level I or above (0 =
         enabled).


ARGS: <> pAccess
      >  buPhysicalDiskNum
      <  pDiskStats

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 216  Read Physical Disk Statistics

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s216ReadPhyDiskStats
(
   pNWAccess          pAccess,
   nuint8            buPhysicalDiskNum,
   pNWNCPDiskStats   pDiskStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        216)
   #define NCP_STRUCT_LEN     ((nuint16)         2)
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REP_LEN        ((nuint)          93)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buPhysicalDiskNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(&pDiskStats->luSysIntervalMarker, &abuReply[0]);
      pDiskStats->buPhysicalDiskChannel = abuReply[4];
      pDiskStats->buDriveRemovableFlag = abuReply[5];
      pDiskStats->buPhysicalDriveType = abuReply[6];
      pDiskStats->buControllerDriveNum = abuReply[7];
      pDiskStats->buControllerNum = abuReply[8];
      pDiskStats->buControllerType = abuReply[9];
      NCopyHiLo32(&pDiskStats->luDriveSize, &abuReply[10]);
      NCopyHiLo16(&pDiskStats->suDriveCylinders, &abuReply[14]);
      pDiskStats->buDriveHeads = abuReply[16];
      pDiskStats->buSectorsPerTrack = abuReply[17];
      NWCMemMove(pDiskStats->abuDriveDefStrB64, &abuReply[18],
         (nuint) 64);
      NCopyHiLo16(&pDiskStats->suIOErrorCnt, &abuReply[82]);
      NCopyHiLo32(&pDiskStats->luHotFixTableStart, &abuReply[84]);
      NCopyHiLo16(&pDiskStats->suHotFixTableSize, &abuReply[88]);
      NCopyHiLo16(&pDiskStats->suHotFixBlocksAvailable, &abuReply[90]);
      pDiskStats->buHotFixDisabled = abuReply[92];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s216.c,v 1.7 1994/09/26 17:36:16 rebekah Exp $
*/
