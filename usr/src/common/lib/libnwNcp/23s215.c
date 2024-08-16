/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s215.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s215GetDriveMapTable**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s215GetDriveMapTable
         (
            pNWAccess pAccess,
            pnuint32 pluSysIntervalMarker,
            pnuint8  pbuSFTSupportLevel,
            pnuint8  pbuLogicalDriveCnt,
            pnuint8  pbuPhysicalDriveCnt,
            pnuint8  pbuDiskChannelTableB5,
            pnuint16 psuPendingIOCommands,
            pnuint8  pbuDriveMapTableB32,
            pnuint8  pbuDriveMirrorTableB32,
            pnuint8  pbuDeadMirrorTableB32,
            pnuint8  pbuReMirrorDriveNum,
            pnuint8  pbuFiller,
            pnuint32 pluReMirrorCurrentOffset,
            pnuint16 psuSFTErrorTableB60,
         );

REMARKS: This call returns the file server's Drive Mapping Table.  If the calling
         station does not have operator privileges, the Completion Code No
         Console Rights is returned.

         System Interval Marker indicates how long the file server has been up.
         This field is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         SFT Support Level indicates the SFT level offered by the file server.

            If the SFT level is 1, the file server offers hot disk error fix.

            If the SFT level is 2, the file server offers disk mirroring and
            transaction tracking.

            If the SFT level is 3, the file server offers physical file server
            mirroring.

         Logical Drive Count indicates the number of logical drives attached to
         the server.  If the file server supports SFT Level II or above and disks
         are mirrored, the Logical Drive Ccount will be lower than the actual
         number of physical disk subsystems attached to the file server.  The file
         server's operating system considers mirrored disks to be one logical
         drive.

         Physical Drive Count indicates the number of physical disk units
         attached to the server.

         Disk Channel Table field is a 5-byte table that indicates which disk
         channels exists on the server and what their drive types are.  (Each
         channel is 1 byte.)  A nonzero value in the Disk Channel Table indicates
         that the corresponding disk channel exists in the file server.  The drive
         types are

            1 = XT,
            2 = AT,
            3 = SCSI,
            4 = disk coprocessor, and
            50 to 255 = Value Added Disk Drive (VADD).

         Pending IO Commands indicates the number of outstanding disk
         controller commands.

         Drive Mapping Table is a 32-byte table containing the primary physical
         drive to which each logical drive is mapped (0xFF = no such logical
         drive).

         Drive Mirror Table is a 32-byte table containing the secondary physical
         drive to which each logical drive is mapped (0xFF = no such logical drive).

         Dead Mirror Table is a 32-byte table containing the secondary physical
         drive to which each logical drive was last mapped (0xFF = logical drive
         was never mirrored).  This table is used in conjunction with the Drive
         Mirror Table.  If the entry in the Drive Mirror Table shows that a drive
         is not currently mirrored, the table can be used to determine which drive
         previously mirrored the logical drive.  The Dead Mirror Table is used to
         remirror a logical drive after a mirror failure.

         ReMirror Drive Number indicates the physical drive number of the disk
         currently being remirrored (0xFF = no disk being remirrored).

         Filler contains no information.

         ReMirror Current Offset contains the block number that is currently
         being remirrored.

         SFT Error Table is a 60-byte table containing SFT internal error
         counters.


ARGS: <> pAccess
      <  pluSysIntervalMarker (optional)
      <  pbuSFTSupportLevel (optional)
      <  pbuLogicalDriveCnt (optional)
      <  pbuPhysicalDriveCnt (optional)
      <  pbuDiskChannelTableB5 (optional)
      <  psuPendingIOCommands (optional)
      <  pbuDriveMapTableB32 (optional)
      <  pbuDriveMirrorTableB32 (optional)
      <  pbuDeadMirrorTableB32 (optional)
      <  pbuReMirrorDriveNum (optional)
      <  pbuFiller (optional)
      <  pluReMirrorCurrentOffset (optional)
      <  psuSFTErrorTableB60 (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 215  Get Drive Mapping Table

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s215GetDriveMapTable
(
   pNWAccess pAccess,
   pnuint32 pluSysIntervalMarker,
   pnuint8  pbuSFTSupportLevel,
   pnuint8  pbuLogicalDriveCnt,
   pnuint8  pbuPhysicalDriveCnt,
   pnuint8  pbuDiskChannelTableB5,
   pnuint16 psuPendingIOCommands,
   pnuint8  pbuDriveMapTableB32,
   pnuint8  pbuDriveMirrorTableB32,
   pnuint8  pbuDeadMirrorTableB32,
   pnuint8  pbuReMirrorDriveNum,
   pnuint8  pbuReserved,
   pnuint32 pluReMirrorCurrentOffset,
   pnuint16 psuSFTErrorTableB60
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        215)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)         236)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      if (pluSysIntervalMarker)
         NCopyHiLo32(pluSysIntervalMarker, &abuReply[0]);
      if (pbuSFTSupportLevel)
         *pbuSFTSupportLevel = abuReply[4];
      if (pbuLogicalDriveCnt)
         *pbuLogicalDriveCnt = abuReply[5];
      if (pbuPhysicalDriveCnt)
         *pbuPhysicalDriveCnt = abuReply[6];
      if (pbuDiskChannelTableB5)
         NWCMemMove(pbuDiskChannelTableB5, &abuReply[7], (nuint) 5);
      if (psuPendingIOCommands)
         NCopyHiLo16(psuPendingIOCommands, &abuReply[12]);
      if (pbuDriveMapTableB32)
         NWCMemMove(pbuDriveMapTableB32, &abuReply[14], (nuint) 32);
      if (pbuDriveMirrorTableB32)
         NWCMemMove(pbuDriveMirrorTableB32, &abuReply[56], (nuint) 32);
      if (pbuDeadMirrorTableB32)
         NWCMemMove(pbuDeadMirrorTableB32, &abuReply[88], (nuint) 32);
      if (pbuReMirrorDriveNum)
         *pbuReMirrorDriveNum = abuReply[130];
      if (pbuReserved)
         *pbuReserved = abuReply[131];
      if (pluReMirrorCurrentOffset)
         NCopyHiLo32(pluReMirrorCurrentOffset, &abuReply[132]);
      if (psuSFTErrorTableB60)
      {
         nint i;

         for (i = 0; i < 60; i++)
            NCopyHiLo16(&psuSFTErrorTableB60[i], &abuReply[136+i*2]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s215.c,v 1.7 1994/09/26 17:36:14 rebekah Exp $
*/
