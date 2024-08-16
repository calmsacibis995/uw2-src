/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s4GetUserInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s4GetUserInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luConnNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEUserInfo        pUserInfo,
            pnuint8                  pbuUserNameLen
            pnstr8                   pbstrUserNameB434,
         )

REMARKS:

ARGS: <> pAccess
      >  luConnNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pUserInfo
      <  pbstrUserNameB434

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 04  User Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s4GetUserInfo
(
   pNWAccess                 pAccess,
   nuint32                  luConnNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEUserInfo        pUserInfo,
   pnuint8                  pbuUserNameLen,
   pnstr8                   pbstrUserNameB434
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_USERNAME_LEN ((nuint) 434)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 78)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 3)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32( &abuReq[3], &luConnNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuUserNameLen;
   replyFrag[1].uLen  = (nuint) 1;

   replyFrag[2].pAddr = pbstrUserNameB434;
   replyFrag[2].uLen  = MAX_USERNAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pUserInfo)
      {
         NCopyLoHi32(&pUserInfo->luConnNum, &abuReply[8]);
         NCopyLoHi32(&pUserInfo->luUseCnt, &abuReply[12]);
         pUserInfo->buConnServiceType = abuReply[16];
         NWCMemMove(pUserInfo->abuLoginTime, &abuReply[17], (nuint) 7);
         NCopyLoHi32(&pUserInfo->luStatus, &abuReply[24]);
         NCopyLoHi32(&pUserInfo->luExpirationTime, &abuReply[26]);
         NCopyLoHi32(&pUserInfo->luObjType, &abuReply[32]);
         pUserInfo->buTransationFlag = abuReply[36];
         pUserInfo->buLogLockThreshold = abuReply[37];
         pUserInfo->buRecLockThreshold = abuReply[38];
         pUserInfo->buFileWriteFlags = abuReply[39];
         pUserInfo->buFileWriteState = abuReply[40];
         pUserInfo->buFiller = abuReply[41];
         NCopyLoHi16(&pUserInfo->suFileLockCnt, &abuReply[42]);
         NCopyLoHi16(&pUserInfo->suRecordLockCnt, &abuReply[44]);
         NWCMemMove(pUserInfo->abuTotalBytesRead, &abuReply[46], (nuint) 6);
         NWCMemMove(pUserInfo->abuTotalBytesWritten, &abuReply[52], (nuint) 6);
         NCopyLoHi32(&pUserInfo->luTotalRequests, &abuReply[58]);
         NCopyLoHi32(&pUserInfo->luHeldRequests, &abuReply[62]);
         NWCMemMove(pUserInfo->abuHeldBytesRead, &abuReply[66], (nuint) 6);
         NWCMemMove(pUserInfo->abuHeldBytesWritten, &abuReply[72], (nuint) 6);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s4.c,v 1.7 1994/09/26 17:32:30 rebekah Exp $
*/
