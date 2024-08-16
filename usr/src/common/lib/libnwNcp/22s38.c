/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s38.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s38TrusteesScanExt**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s38TrusteesScanExt
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buSetNum,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuNumEntries,
            pnuint32 pluObjIDB20,
            pnuint16 psuTrusteeRightsB20,
         );

REMARKS: This function returns the extended trustee information for a file or
         directory (up to 20 entries per call).  The NumberOfEntries indicates how
         many ObjectIDs and TrusteeRights actually were returned.

         Note that regardless of how many extended trustee rights entries are
         returned by this call, TrusteeRights values are returned starting at offset
         91 in the reply buffer.

ARGS: <> pAccess
      >  buDirHandle
      >  buSetNum
      >  buPathLen
      >  pbstrPath
      <  pbuNumEntries
      <  pluObjIDB20
      <  psuTrusteeRightsB20

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8998  Bad Volume Name
         0x899B  Bad Directory Handle
         0x899C  Invalid Path

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 12  Scan Directory For Trustees

NCP:     22 38  Scan File Or Directory For Extended Trustees

CHANGES: 14 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s38TrusteesScanExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSetNum,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNumEntries,
   pnuint32 pluObjIDB20,
   pnuint16 psuTrusteeRightsB20
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 38)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REPLY_LEN   ((nuint) 121)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buSetNum;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i, x, y;

      *pbuNumEntries = abuRep[0];

      for(i = 0, x = 1, y = 81; i < (nint) *pbuNumEntries; i++, x += 4, y += 2)
      {
         NCopyHiLo32(&pluObjIDB20[i], &abuRep[x]);
         NCopyLoHi16(&psuTrusteeRightsB20[i], &abuRep[y]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s38.c,v 1.7 1994/09/26 17:34:22 rebekah Exp $
*/
