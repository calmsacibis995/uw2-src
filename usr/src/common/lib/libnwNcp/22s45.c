/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s45.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s45GetDirInfo**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s45GetDirInfo
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            pnuint32 pluTotalBlocks,
            pnuint32 pluAvailableBlocks,
            pnuint32 pluTotalDirEntries,
            pnuint32 pluAvailDirEntries,
            pnuint8  pbuReservedB4,
            pnuint8  pbuSectorsPerBlock,
            pnuint8  pbuVolNameLen,
            pnstr8   pbstrVolNameB16,
         );

REMARKS: This function returns the real size information for a 386 directory.  The old
         NCP calls cannot handle volumes bigger than 256 Megabytes.  This function
         also  includes space limitations on the user and volume when calculating the
         space available.


ARGS: <> pAccess
      >  buDirHandle
      <  pluTotalBlocks
      <  pluAvailableBlocks
      <  pluTotalDirEntries
      <  pluAvailDirEntries
      <  pbuReservedB4
      <  pbuSectorsPerBlock
      <  pbuVolNameLen
      <  pbstrVolNameB16

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x899B  Bad Directory Handle

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 25  Set Directory Information

NCP:     22 45  Get Directory Information

CHANGES: 13 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s45GetDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint32 pluTotalBlocks,
   pnuint32 pluAvailableBlocks,
   pnuint32 pluTotalDirEntries,
   pnuint32 pluAvailDirEntries,
   pnuint8  pbuReservedB4,
   pnuint8  pbuSectorsPerBlock,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolNameB16
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 45)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 38)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, NULL);

   if(lCode == 0)
   {
      if(pluTotalBlocks)
         NCopyLoHi32(pluTotalBlocks, &abuRep[0]);

      if(pluAvailableBlocks)
         NCopyLoHi32(pluAvailableBlocks, &abuRep[4]);

      if(pluTotalDirEntries)
         NCopyLoHi32(pluTotalDirEntries, &abuRep[8]);

      if(pluAvailDirEntries)
         NCopyLoHi32(pluAvailDirEntries, &abuRep[12]);

      if(pbuReservedB4)
         NWCMemMove(pbuReservedB4, &abuRep[16], 4);

      if(pbuSectorsPerBlock)
         *pbuSectorsPerBlock = abuRep[20];

      if(pbuVolNameLen && pbstrVolNameB16)
      {
         *pbuVolNameLen = abuRep[21];
         NWCMemMove(pbstrVolNameB16, &abuRep[22], (nuint) *pbuVolNameLen);
      }
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s45.c,v 1.7 1994/09/26 17:34:32 rebekah Exp $
*/
