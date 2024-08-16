/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s53.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s53GetKnownNetworks**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s53GetKnownNetworks
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumEntries,
            pNWNCPFSENetList         pList,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      <  pVConsoleInfo
      <  psuReserved
      <  pluNumEntries
      <  pList

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 53  Get Known Networks Information

CHANGES: 23 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s53GetKnownNetworks
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSENetList         pList
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 53)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 522)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   nint i;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluNumEntries, &abuReply[8]);

      for (i=0; i < (nint) *pluNumEntries; i++)
      {
         NCopyLoHi32(&pList->aNetworks[i].luNetIDNum, &abuReply[12+i*10]);
         NCopyLoHi16(&pList->aNetworks[i].suHopsToNet, &abuReply[16+i*10]);
         NCopyLoHi16(&pList->aNetworks[i].suNetStatus, &abuReply[18+i*10]);
         NCopyLoHi16(&pList->aNetworks[i].suTimeToNet, &abuReply[20+i*10]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s53.c,v 1.7 1994/09/26 17:32:49 rebekah Exp $
*/
