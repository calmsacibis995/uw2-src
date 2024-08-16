/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:86s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpea.h"

/*manpage*NWNCP86s2EAWrite********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP86s2EAWrite
         (
            pNWAccess       pAccess,
            nuint16        suFlags,
            pNWNCPEAHandle pEAHandle,
            nuint32        luTtlWriteDataSize,
            nuint32        luWritePosition,
            nuint32        luAccessFlagSize,
            nuint16        suValueLen,
            nuint16        suKeyLen,
            pnuint8        pbuKey,
            pnuint8        pbuValue,
            pnuint32       pluErrorCode,
            pnuint32       pluBytesWritten,
            pnuint32       pluNewEAHandle
         );

REMARKS:

ARGS: <> pAccess
       > suFlags
       > pEAHandle
       > luTtlWriteDataSize
       > luWritePosition
       > luAccessFlagSize
       > suValueLen
       > suKeyLen
       > pbuKey
       > pbuValue
      <  pluErrorCode
      <  pluBytesWritten
      <  pluNewEAHandle

INCLUDE: ncpea.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 02  Write Extended Attribute

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP86s2EAWrite
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luTtlWriteDataSize,
   nuint32        luWritePosition,
   nuint32        luAccessFlagSize,
   nuint16        suValueLen,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint8        pbuValue,
   pnuint32       pluErrorCode,
   pnuint32       pluBytesWritten,
   pnuint32       pluNewEAHandle
)
{
   #define NCP_FUNCTION    ((nuint) 86)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define REQ_LEN         ((nuint) 27)
   #define REPLY_LEN       ((nuint) 12)
   #define REQ_FRAGS       ((nuint) 3)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   NCopyLoHi16( &abuReq[1],  &suFlags );
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
   NCopyLoHi32( &abuReq[11], &luTtlWriteDataSize );
   NCopyLoHi32( &abuReq[15], &luWritePosition );
   NCopyLoHi32( &abuReq[19], &luAccessFlagSize );
   NCopyLoHi16( &abuReq[23], &suValueLen );
   NCopyLoHi16( &abuReq[25], &suKeyLen );

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuKey;
   reqFrag[1].uLen  = suKeyLen;

   reqFrag[2].pAddr = pbuValue;
   reqFrag[2].uLen  = suValueLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi32( pluErrorCode, &abuReply[0] );
      NCopyLoHi32( pluBytesWritten, &abuReply[4] );
      NCopyLoHi32( pluNewEAHandle, &abuReply[8] );
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/86s2.c,v 1.7 1994/09/26 17:39:07 rebekah Exp $
*/
