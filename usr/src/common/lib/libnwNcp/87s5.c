/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s5TrusteeScan********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s5TrusteesScan
         (
            pNWAccess       pAccess,
            nuint8         buNamSpc,
            nuint8         buReserved,
            nuint16        suSrchAttrs,
            pNWNCPCompPath pCompPath,
            pnuint32       pluIterHnd,
            pnuint16       psuObjIDCnt,
            pNWNCPTrustees pTrusteesB20
         );

REMARKS: The Scan Sequence field is set to 0 on the first call.  On each
         subsequent call it is replaced by the NextScanSequence value.

         When Next Scan Sequence number is equal to -1, all the trustees have
         been recived by the client.

         Note also that the server will send the maximum trustee structures
         (maximum of 20) until the last call.  The last call contains a
         partial packet of trustee structures.

ARGS: <> pAccess
       > buNamSpc
       > buReserved
       > suSrchAttrs
       > pCompPath
      <> pluIterHnd
      <  psuObjIDCnt
      <  pTrusteesB20

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 12  Scan Directory For Trustees
         22 38  Scan File or Directory For Extended Trustees

NCP:     87 05  Scan File or SubDirectory for Trustees

CHANGES: 14 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s5TrusteesScan
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   pNWNCPCompPath pCompPath,
   pnuint32       pluIterHnd,
   pnuint16       psuObjIDCnt,
   pNWNCPTrustees pTrusteesB20
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 5)
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 126)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32  lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuRep[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);
   NCopyLoHi32(&abuReq[5], pluIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode ==0)
   {
      nint i, j;

      NCopyLoHi32(pluIterHnd, &abuRep[0]);
      NCopyLoHi16(psuObjIDCnt, &abuRep[4]);

      for(i = 0, j = 6; i < (nint) *psuObjIDCnt; i++, j += 6)
      {
         NCopyHiLo32(&pTrusteesB20[i].luObjID, &abuRep[j]);
         NCopyLoHi16(&pTrusteesB20[i].suRights, &abuRep[j + 4]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s5.c,v 1.7 1994/09/26 17:39:40 rebekah Exp $
*/
