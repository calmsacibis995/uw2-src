/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:svoldskr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWScanVolDiskRestrictions2****************************************
SYNTAX:  NWCCODE N_API NWScanVolDiskRestrictions2
         (
            NWCONN_HANDLE  conn,
            nuint8         volNum,
            pnuint32       iterHnd,
            NWVOL_RESTRICTIONS NWPTR volInfo
         )

REMARKS: Returns a list of the disk restrictions for a volume. The
         information is returned in volInfo, and contains the object
         restrictions that have been made for the volume (see
         NWVolumeRestrictions below).  Note that all restrictions are in 4K
         blocks.

         if 'restriction' is greater than 0x4000 0000 the object has no
         restrictions.

ARGS:
       > volNum
         Volume number for which to return the restrictions

       > iterHnd
         Pointer containing the sequence number to use in the search,
         initially needs to be set to 0.

       < volInfo
         Pointer to a structure of type NWVolumeRestrictions.

         struct
         {
           nuint8  numberOfEntries;
           struct
           {
             nuint32 objectID;
             nuint32 restriction;
           } resInfo[12];
         } NWVolumeRestrictions;

INCLUDE: nwvol.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 32  Scan Volume's User Disk Restrictions

CHANGES: 15 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWScanVolDiskRestrictions2
(
   NWCONN_HANDLE   conn,
   nuint8          volNum,
   pnuint32        iterHnd,
   NWVOL_RESTRICTIONS NWPTR volInfo
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE)NWNCP22s32VolScanRestrict(&access, volNum, *iterHnd,
               &volInfo->numberOfEntries,
               (pNWNCPRestrictions) volInfo->resInfo)) == 0)
   {
      int i;

      for(i = 0; i < (int) volInfo->numberOfEntries; i++)
      {
         volInfo->resInfo[i].objectID = NSwapHiLo32(volInfo->resInfo[i].objectID);
      }
   }

   return (ccode);
}

NWCCODE N_API NWScanVolDiskRestrictions
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  pnuint32        iterHnd,
  NWVolumeRestrictions NWPTR volInfo
)
{
   NWVOL_RESTRICTIONS volInfo2;
   NWCCODE ccode;

   if((ccode = NWScanVolDiskRestrictions2(conn, volNum, iterHnd, &volInfo2)) != 0)
      return (ccode);

   NWCMemMove(volInfo, &volInfo2, sizeof(NWVolumeRestrictions));

   return (0);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/svoldskr.c,v 1.7 1994/09/26 17:50:14 rebekah Exp $
*/
