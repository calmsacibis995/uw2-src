/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:18.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP18VolGetInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP18VolGetInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint16 psuSectorsPerCluster,
            pnuint16 psuTotalVolClusters,
            pnuint16 psuAvailClusters,
            pnuint16 psuTotalDirSlots,
            pnuint16 psuAvailDirSlots,
            pnstr8   pbstrVolNameB16,
            pnuint16 psuRemovableFlag,
         );

REMARKS: This call returns the same information as Get Volume Info With Handle
         (0x2222  22  21).  This call allows a client to check the physical space
         available on a volume without having to determine which mounted volume
         number the client's directory handle points to.

ARGS: <> pAccess
      >  buVolNum
      <  psuSectorsPerCluster (optional)
      <  psuTotalVolClusters (optional)
      <  psuAvailClusters (optional)
      <  psuTotalDirSlots (optional)
      <  psuAvailDirSlots (optional)
      <  pbstrVolNameB16 (optional)
      <  psuRemovableFlag (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8998  Disk Map Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 21  Get Volume Info With Handle

NCP:     18 --  Get Volume Info With Number

CHANGES: 15 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP18VolGetInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint16 psuSectorsPerCluster,
   pnuint16 psuTotalVolClusters,
   pnuint16 psuAvailClusters,
   pnuint16 psuTotalDirSlots,
   pnuint16 psuAvailDirSlots,
   pnstr8   pbstrVolNameB16,
   pnuint16 psuRemovableFlag
)
{
   #define NCP_FUNCTION    ((nuint) 18)
   #define NCP_REQ_LEN     ((nuint) 1)
   #define NCP_REPLY_LEN   ((nuint) 28)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   abuReq[0] = buVolNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if (psuSectorsPerCluster)
         NCopyHiLo16(psuSectorsPerCluster, &abuReply[0]);
      if (psuTotalVolClusters)
         NCopyHiLo16(psuTotalVolClusters, &abuReply[2]);
      if (psuAvailClusters)
         NCopyHiLo16(psuAvailClusters, &abuReply[4]);
      if (psuTotalDirSlots)
         NCopyHiLo16(psuTotalDirSlots, &abuReply[6]);
      if (psuAvailDirSlots)
         NCopyHiLo16(psuAvailDirSlots, &abuReply[8]);
      if (pbstrVolNameB16)
         NWCMemMove(pbstrVolNameB16, &abuReply[10], (nuint) 16);
      if (psuRemovableFlag)
         NCopyHiLo16(psuRemovableFlag, &abuReply[26]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/18.c,v 1.7 1994/09/26 17:33:21 rebekah Exp $
*/
