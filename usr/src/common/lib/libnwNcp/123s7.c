/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s7GetGarbageCollectionInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s7GetGarbCollectInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluFailedAllocReqCnt,
            pnuint32                 pluNumAllocs,
            pnuint32                 pluNoMoreMemAvlCnt,
            pnuint32                 pluNumGarbageCol,
            pnuint32                 pluFoundSomeMem,
            pnuint32                 pluNumChecks,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluFailedAllocReqCnt (optional)
      <  pluNumAllocs (optional)
      <  pluNoMoreMemAvlCnt (optional)
      <  pluNumGarbageCol (optional)
      <  pluFoundSomeMem (optional)
      <  pluNumChecks (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 07  Get Garbage Collection Info

CHANGES: 24 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s7GetGarbCollectInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluFailedAllocReqCnt,
   pnuint32                 pluNumAllocs,
   pnuint32                 pluNoMoreMemAvlCnt,
   pnuint32                 pluNumGarbageCol,
   pnuint32                 pluFoundSomeMem,
   pnuint32                 pluNumChecks
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 7)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 32)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      if (pluFailedAllocReqCnt)
         NCopyLoHi32(pluFailedAllocReqCnt, &abuReply[8]);
      if (pluNumAllocs)
         NCopyLoHi32(pluNumAllocs, &abuReply[12]);
      if (pluNoMoreMemAvlCnt)
         NCopyLoHi32(pluNoMoreMemAvlCnt, &abuReply[16]);
      if (pluNumGarbageCol)
         NCopyLoHi32(pluNumGarbageCol, &abuReply[20]);
      if (pluFoundSomeMem)
         NCopyLoHi32(pluFoundSomeMem, &abuReply[24]);
      if (pluNumChecks)
         NCopyLoHi32(pluNumChecks, &abuReply[28]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s7.c,v 1.7 1994/09/26 17:33:01 rebekah Exp $
*/
