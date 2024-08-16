/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s224.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s224GetLogicalRecInfo******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s224GetLogicalRecInfo
         (
            pNWAccess pAccess,
            pnuint16 psuIterHnd,
            nuint8   buLogicalRecNameLen,
            pnuint8  pbstrLogicalRecName,
            pnuint16 psuUseCnt,
            pnuint16 psuShareableLockCnt,
            pnuint8  pbuLocked,
            pnuint8  pbuNumRecs,
            pNWNCPLogicalRecInfo2x pLogRecInfoB128
         );

REMARKS: Scans for all record locks on a specified logical name

ARGS: <> pAccess
      <> psuIterHnd
      >  buLogicalRecNameLen
      >  pbstrLogicalRecName
      <  psuUseCnt (optional)
      <  psuShareableLockCnt (optional)
      <  pbuLocked (optional)
      <  pbuNumRecs (optional)
      <  pLogRecInfoB128 (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89c6  No Console Rights

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 224  Get Logical Record Information

CHANGES: 3 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s224GetLogicalRecInfo
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buLogicalRecNameLen,
   pnuint8  pbstrLogicalRecName,
   pnuint16 psuUseCnt,
   pnuint16 psuShareableLockCnt,
   pnuint8  pbuLocked,
   pnuint8  pbuNumRecs,
   pNWNCPLogicalRecInfo2x pLogRecInfoB128
)
{
   #define NCP_FUNCTION       ((nuint)     23)
   #define NCP_SUBFUNCTION    ((nuint8)   224)
   #define NCP_STRUCT_LEN     ((nuint16) (4 + buLogicalRecNameLen))
   #define NCP_REQ_LEN        ((nuint)      6)
   #define NCP_REP_LEN        ((nuint)    512)
   #define NCP_REQ_FRAGS      ((nuint)      2)
   #define NCP_REP_FRAGS      ((nuint)      1)

   nint32  lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], psuIterHnd);
   abuReq[5] = buLogicalRecNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrLogicalRecName;
   reqFrag[1].uLen  = buLogicalRecNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      if (psuUseCnt)
         NCopyHiLo16(psuUseCnt, &abuReply[0]);

      if (psuShareableLockCnt)
         NCopyHiLo16(psuShareableLockCnt, &abuReply[2]);

      NCopyHiLo16(psuIterHnd, &abuReply[4]);

      if (pbuLocked)
         *pbuLocked = abuReply[6];

      if(pbuNumRecs)
         *pbuNumRecs = abuReply[7];

      if(pLogRecInfoB128)
      {
         nint i, j;

         for (i = 0, j = 8; i < (nint) abuReply[7]; i++, j += 4)
         {
            NCopyHiLo16(&pLogRecInfoB128[i].suConnNum, &abuReply[j]);
            pLogRecInfoB128[i].buTaskNum    = abuReply[j+2];
            pLogRecInfoB128[i].buLockStatus = abuReply[j+3];
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s224.c,v 1.7 1994/09/26 17:36:30 rebekah Exp $
*/
