/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:86s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpea.h"

/*manpage*NWNCP86s3EARead********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP86s3EARead
         (
            pNWAccess       pAccess,
            nuint16        suFlags,
            pNWNCPEAHandle pEAHandle,
            nuint32        luReadPosition,
            nuint32        luInspectSize,
            nuint16        suKeyLen,
            pnuint8        pbuKey,
            pnuint32       pluErrorCode,
            pnuint32       pluTtlValuesLen,
            pnuint32       pluNewEAHandle,
            pnuint32       pluAccessFlag,
            pnuint16       psuValueLen,
            pnuint8        pbuValue
         );

REMARKS:

ARGS: <> pAccess
       > suFlags
       > pEAHandle
       > luReadPosition
       > luInspectSize
       > suKeyLen
       > pbuKey
      <  pluErrorCode
      <  pluTtlValuesLen
      <  pluNewEAHandle
      <  pluAccessFlag
      <  psuValueLen
      <  pbuValue

INCLUDE: ncpea.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 03  Read Extended Attribute

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP86s3EARead
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luReadPosition,
   nuint32        luInspectSize,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint32       pluErrorCode,
   pnuint32       pluTtlValuesLen,
   pnuint32       pluNewEAHandle,
   pnuint32       pluAccessFlag,
   pnuint16       psuValueLen,
   pnuint8        pbuValue
)
{
   #define NCP_FUNCTION    ((nuint) 86)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define REQ_LEN         ((nuint) 21)
   #define REPLY_LEN       ((nuint) 18)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;

   NCopyLoHi16( &abuReq[1], &suFlags);
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
   NCopyLoHi32( &abuReq[11], &luReadPosition );
   NCopyLoHi32( &abuReq[15], &luInspectSize );
   NCopyLoHi16( &abuReq[19], &suKeyLen );

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuKey;
   reqFrag[1].uLen  = suKeyLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuValue;
   replyFrag[1].uLen  = 512 - REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi32( pluErrorCode, &abuReply[0] );
      NCopyLoHi32( pluTtlValuesLen, &abuReply[4] );
      NCopyLoHi32( pluNewEAHandle, &abuReply[8] );
      NCopyLoHi32( pluAccessFlag, &abuReply[12] );
      NCopyLoHi16( psuValueLen, &abuReply[16] );
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/86s3.c,v 1.7 1994/09/26 17:39:09 rebekah Exp $
*/
