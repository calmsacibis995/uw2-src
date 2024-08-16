/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s30.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s30GetMediaObjectInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s30GetMediaObjectInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luObjNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEMediaInfo       pInfo,
         )

REMARKS:

ARGS: <> pAccess,
       > luObjNum,
       < pVConsoleInfo,
       < psuReserved,
       < pInfo,

INCLUDE: ncpfse.h

RETURN:

SERVER:

CLIENT:

SEE:

NCP:     123 30 Get Media Manager Object Information

CHANGES: 22 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s30GetMediaObjectInfo
(
   pNWAccess                 pAccess,
   nuint32                  luObjNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEMediaInfo       pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 30)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 208)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luObjNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi32(&pVConsoleInfo->luServerTime, &abuReply[0]);
      pVConsoleInfo->buConsoleVer= abuReply[4];
      pVConsoleInfo->buConsoleRev= abuReply[5];

      NCopyLoHi16(psuReserved, &abuReply[6]);

      NWCMemMove(pInfo->pbstrLabel, &abuReply[8], (nuint) 64);
      NCopyLoHi32(&pInfo->luObjType, &abuReply[72]);
      NCopyLoHi32(&pInfo->luObjTimeStamp, &abuReply[76]);
      NCopyLoHi32(&pInfo->luMediaType, &abuReply[80]);
      NCopyLoHi32(&pInfo->luCartridgeType, &abuReply[84]);
      NCopyLoHi32(&pInfo->luUnitSize, &abuReply[88]);
      NCopyLoHi32(&pInfo->luBlockSize, &abuReply[92]);
      NCopyLoHi32(&pInfo->luCapacity, &abuReply[96]);
      NCopyLoHi32(&pInfo->luPreferredUnitSize, &abuReply[100]);
      NWCMemMove(pInfo->pbstrName, &abuReply[104], (nuint) 64);
      NCopyLoHi32(&pInfo->luType, &abuReply[168]);
      NCopyLoHi32(&pInfo->luStatus, &abuReply[172]);
      NCopyLoHi32(&pInfo->luFunctionMask, &abuReply[176]);
      NCopyLoHi32(&pInfo->luControlMask, &abuReply[180]);
      NCopyLoHi32(&pInfo->luParentCnt, &abuReply[184]);
      NCopyLoHi32(&pInfo->luSiblingCnt, &abuReply[188]);
      NCopyLoHi32(&pInfo->luChildCnt, &abuReply[192]);
      NCopyLoHi32(&pInfo->luSpecificInfoSize, &abuReply[196]);
      NCopyLoHi32(&pInfo->luObjUniqueID, &abuReply[200]);
      NCopyLoHi32(&pInfo->luMediaSlot, &abuReply[204]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s30.c,v 1.7 1994/09/26 17:32:23 rebekah Exp $
*/
