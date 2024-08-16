/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:86s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpea.h"

/*manpage*NWNCP86s4EAEnum********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP86s4EAEnum
         (
            pNWAccess       pAccess,
            nuint16        suFlags,
            pNWNCPEAHandle pEAHandle,
            nuint32        luInspectSize,
            nuint16        suSequence,
            nuint16        suKeyLen,
            pnuint8        pbuKey,
            pnuint32       pluErrorCode,
            pnuint32       pluTtlEAs,
            pnuint32       pluTtlEAsDataSize,
            pnuint32       pluTtlEAsKeySize,
            pnuint32       pluNewEAHandle,
            pNWNCPEAEnum   pEAEnum,
            pnuint8        pbuEnumDataB512
         );

REMARKS:

ARGS: <> pAccess
       > suFlags
       > pEAHandle
       > luInspectSize
       > suSequence
       > suKeyLen
       > pbuKey
      <  pluErrorCode
      <  pluTtlEAs
      <  pluTtlEAsDataSiz
      <  pluTtlEAsKeySize
      <  pluNewEAHandle
      <  pEAEnum
      <  pbuEnumDataB512

INCLUDE: ncpea.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 04  Enumerate Extended Attribute

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
    Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP86s4EAEnum
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luInspectSize,
   nuint16        suSequence,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint32       pluErrorCode,
   pnuint32       pluTtlEAs,
   pnuint32       pluTtlEAsDataSize,
   pnuint32       pluTtlEAsKeySize,
   pnuint32       pluNewEAHandle,
   pNWNCPEAEnum   pEAEnum,
   pnuint8        pbuEnumDataB512
)
{
   #define NCP_FUNCTION    ((nuint) 86)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define REQ_LEN         ((nuint) 19)
   #define DATA_SIZE       ((nuint) 512)
   #define REPLY_LEN       ((nuint) 24)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[1], &suFlags);
   if (suFlags & NWEA_USE_EAHANDLE)
   {
      NCopyLoHi32( &abuReq[3], &pEAHandle->TYPE_10.luEAHandle );
      NCopyLoHi32( &abuReq[7], &pEAHandle->TYPE_10.luReserved );
   }
   else if (suFlags & NWEA_USE_NWHANDLE)
   {
      NWCMemMove( &abuReq[3], pEAHandle->TYPE_01.abuNWHandleB4, 4 );
      NWCMemMove( &abuReq[7], pEAHandle->TYPE_01.abuReservedB4, 4 );
   }
   else
   {
      NCopyLoHi32( &abuReq[3], &pEAHandle->TYPE_00.luVolNum );
      NCopyLoHi32( &abuReq[7], &pEAHandle->TYPE_00.luDirBase );
   }
   NCopyLoHi32(&abuReq[11], &luInspectSize);
   NCopyLoHi16(&abuReq[15], &suSequence);
   NCopyLoHi16(&abuReq[17], &suKeyLen);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuKey;
   reqFrag[1].uLen  = suKeyLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuEnumDataB512;
   replyFrag[1].uLen  = DATA_SIZE;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
          REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi32(pluErrorCode,                   &abuReply[0]);
      NCopyLoHi32(pluTtlEAs,                      &abuReply[4]);
      NCopyLoHi32(pluTtlEAsDataSize,              &abuReply[8]);
      NCopyLoHi32(pluTtlEAsKeySize,               &abuReply[12]);
      NCopyLoHi32(pluNewEAHandle,                 &abuReply[16]);
      NCopyLoHi16(&pEAEnum->lvl_0_6.suReserved1,  &abuReply[20]);
      NCopyLoHi16(&pEAEnum->lvl_0_6.suReserved2,  &abuReply[22]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/86s4.c,v 1.7 1994/09/26 17:39:10 rebekah Exp $
*/
