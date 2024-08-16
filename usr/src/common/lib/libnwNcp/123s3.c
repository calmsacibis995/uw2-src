/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s3GetFileSystemsInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s3GetFileSystemsInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEFileSystemInfo  pFileSystemInfo,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pFileSystemInfo

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 03  NetWare File Systems Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s3GetFileSystemsInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEFileSystemInfo  pFileSystemInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 60)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuRep );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuRep[6]);

      NCopyLoHi32(&pFileSystemInfo->luFATMovedCnt, &abuRep[8]);
      NCopyLoHi32(&pFileSystemInfo->luFATWriteErrorCnt, &abuRep[12]);
      NCopyLoHi32(&pFileSystemInfo->luSomeoneElseDidItCnt0, &abuRep[16]);
      NCopyLoHi32(&pFileSystemInfo->luSomeoneElseDidItCnt1, &abuRep[20]);
      NCopyLoHi32(&pFileSystemInfo->luSomeoneElseDidItCnt2, &abuRep[24]);
      NCopyLoHi32(&pFileSystemInfo->luIRanOutSomeoneElseDidItCnt0, &abuRep[28]);
      NCopyLoHi32(&pFileSystemInfo->luIRanOutSomeoneElseDidItCnt1, &abuRep[32]);
      NCopyLoHi32(&pFileSystemInfo->luIRanOutSomeoneElseDidItCnt2, &abuRep[36]);
      NCopyLoHi32(&pFileSystemInfo->luTurboFATBuildScrewedUpCnt, &abuRep[40]);
      NCopyLoHi32(&pFileSystemInfo->luExtraUseCountNodeCnt, &abuRep[44]);
      NCopyLoHi32(&pFileSystemInfo->luExtraExtraUseCountNodeCnt, &abuRep[48]);
      NCopyLoHi32(&pFileSystemInfo->luErrReadingLastFATCnt, &abuRep[52]);
      NCopyLoHi32(&pFileSystemInfo->luSomeoneElseUsingThisFileCnt, &abuRep[56]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s3.c,v 1.7 1994/09/26 17:32:22 rebekah Exp $
*/
