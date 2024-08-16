/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s12.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s12GetDirCacheInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s12GetDirCacheInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEDirCacheInfo    pInfo,
         )

REMARKS:

ARGS: <> pAccess
       > pVConsoleInfo  (optional)
       > psuReserved    (optional)
       > pInfo

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 11  Get NLM Information

CHANGES: 23 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s12GetDirCacheInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEDirCacheInfo    pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 64)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if(psuReserved!=NULL) NCopyLoHi16(psuReserved, &abuReply[6]);
      if(pInfo!=NULL)
      {
         NCopyLoHi32(&pInfo->luMinTimeSinceFileDelete, &abuReply[8]);
         NCopyLoHi32(&pInfo->luAbsMinTimeSinceFileDelete, &abuReply[12]);
         NCopyLoHi32(&pInfo->luMinNumDirCacheBuffs, &abuReply[16]);
         NCopyLoHi32(&pInfo->luMaxNumDirCacheBuffs, &abuReply[20]);
         NCopyLoHi32(&pInfo->luNumDirCacheBuffs, &abuReply[24]);
         NCopyLoHi32(&pInfo->luMinNonReferencedTime, &abuReply[28]);
         NCopyLoHi32(&pInfo->luWaitTimeBeforeNewBuff, &abuReply[32]);
         NCopyLoHi32(&pInfo->luMaxConcurrentWrites, &abuReply[36]);
         NCopyLoHi32(&pInfo->luDirtyWaitTime, &abuReply[40]);
         NCopyLoHi32(&pInfo->luDoubleReadFlag, &abuReply[44]);
         NCopyLoHi32(&pInfo->luMapHashNodeCnt, &abuReply[48]);
         NCopyLoHi32(&pInfo->luSpaceRestrictionNodeCnt, &abuReply[52]);
         NCopyLoHi32(&pInfo->luTrusteeListNodeCnt, &abuReply[56]);
         NCopyLoHi32(&pInfo->luPercentOfVolUsedByDirs, &abuReply[60]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s12.c,v 1.7 1994/09/26 17:32:07 rebekah Exp $
*/
