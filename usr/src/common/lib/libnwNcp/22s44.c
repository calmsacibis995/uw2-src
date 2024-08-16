/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s44.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s44VolGetPurgeInfo**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s44VolGetPurgeInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint32 pluTotalBlocks,
            pnuint32 pluFreeBlocks,
            pnuint32 pluPurgeableBlocks,
            pnuint32 pluNotYetPurgeableBlocks,
            pnuint32 pluTotalDirEntries,
            pnuint32 pluAvailDirEntries,
            pnuint8  pbuReservedB4,
            pnuint8  pbuSectorsPerBlock,
            pnuint8  pbuVolNameLen,
            pnstr8   pbstrVolName,
         );

REMARKS: This function returns the real volume information for a 386 volume.  The old
         NCP calls cannot handle a volume that is bigger than 256 Megabytes.  It also
         returns information about deleted files.

         If the volume number specified in the request buffer is not mounted, a
         successful completion code is returned, but all data fields in the reply
         buffer are set to zero.


ARGS: <> pAccess
      >  buVolNum
      <  pluTotalBlocks           (optional)
      <  pluFreeBlocks            (optional)
      <  pluPurgeableBlocks       (optional)
      <  pluNotYetPurgeableBlocks (optional)
      <  pluTotalDirEntries       (optional)
      <  pluAvailDirEntries       (optional)
      <  pbuReservedB4            (optional)
      <  pbuSectorsPerBlock       (optional)
      <  pbuVolNameLen            (optional)
      <  pbstrVolName             (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 06  Get Volume Name
         22 05  Get Volume Number

NCP:     22 44  Get Volume and Purge Information

CHANGES: 13 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s44VolGetPurgeInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint32 pluTotalBlocks,
   pnuint32 pluFreeBlocks,
   pnuint32 pluPurgeableBlocks,
   pnuint32 pluNotYetPurgeableBlocks,
   pnuint32 pluTotalDirEntries,
   pnuint32 pluAvailDirEntries,
   pnuint8  pbuReservedB4,
   pnuint8  pbuSectorsPerBlock,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolName
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 44)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 46)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      if(pluTotalBlocks)
         NCopyLoHi32(pluTotalBlocks, &abuRep[0]);

      if(pluFreeBlocks)
         NCopyLoHi32(pluFreeBlocks, &abuRep[4]);

      if(pluPurgeableBlocks)
         NCopyLoHi32(pluPurgeableBlocks, &abuRep[8]);

      if(pluNotYetPurgeableBlocks)
         NCopyLoHi32(pluNotYetPurgeableBlocks, &abuRep[12]);

      if(pluTotalDirEntries)
         NCopyLoHi32(pluTotalDirEntries, &abuRep[16]);

      if(pluAvailDirEntries)
         NCopyLoHi32(pluAvailDirEntries, &abuRep[20]);

      if(pbuReservedB4)
         NWCMemMove(pbuReservedB4, &abuRep[24], 4);

      if(pbuSectorsPerBlock)
         *pbuSectorsPerBlock = abuRep[28];

      if(pbstrVolName && pbuVolNameLen)
      {
         *pbuVolNameLen = abuRep[29];
         NWCMemMove(pbstrVolName, &abuRep[30], (nuint) *pbuVolNameLen);
      }
   }
   return((NWRCODE) lCode);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s44.c,v 1.7 1994/09/26 17:34:31 rebekah Exp $
*/
