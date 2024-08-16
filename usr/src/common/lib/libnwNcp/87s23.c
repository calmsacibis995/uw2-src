/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s23.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s23NSQueryInfoFormat**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s23NSQueryInfoFormat
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buVolNum,
            pnuint32 pluFixedBitMask,
            pnuint32 pluVariableBitMask,
            pnuint32 pluHugeBitMask,
            pnuint16 psuFixedBitsDefined,
            pnuint16 psuVariableBitsDefined,
            pnuint16 psuHugeBitsDefined,
            pnuint32 pluFieldsLenTableB32
         );

REMARKS: This is a NetWare 386 v3.11 call.  It is used to query for the
         format of NS information.

         The FixedBitMask field, the VariableBitMask field, and the
         HugeBitMask field are explained in more detail in the Introduction
         to Directory Services.

ARGS: <> pAccess
       > buNamSpc
       > buVolNum
      <  pluFixedBitMask (optional)
      <  pluVariableBitMask (optional)
      <  pluHugeBitMask (optional)
      <  psuFixedBitsDefined (optional)
      <  psuVariableBitsDefined (optional)
      <  psuHugeBitsDefined (optional)
      <  pluFieldsLenTableB32 (optional)

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     87 19  Get NS Info
         87 25  Set NS Info

NCP:     87 23  Query NS Information Format

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s23NSQueryInfoFormat
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   pnuint32 pluFixedBitMask,
   pnuint32 pluVariableBitMask,
   pnuint32 pluHugeBitMask,
   pnuint16 psuFixedBitsDefined,
   pnuint16 psuVariableBitsDefined,
   pnuint16 psuHugeBitsDefined,
   pnuint32 pluFieldsLenTableB32
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 23)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) (18 + 128))

   nint32 lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buVolNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN, abuReply,
               REPLY_LEN, NULL);

   if (lCode == 0)
   {
      nint i, j;

      if (pluFixedBitMask)
         NCopyLoHi32(pluFixedBitMask, &abuReply[0]);

      if (pluVariableBitMask)
         NCopyLoHi32(pluVariableBitMask, &abuReply[4]);

      if (pluHugeBitMask)
         NCopyLoHi32(pluHugeBitMask, &abuReply[8]);

      if (psuFixedBitsDefined)
         NCopyLoHi16(psuFixedBitsDefined, &abuReply[12]);

      if (psuVariableBitsDefined)
         NCopyLoHi16(psuVariableBitsDefined, &abuReply[14]);

      if (psuHugeBitsDefined)
         NCopyLoHi16(psuHugeBitsDefined, &abuReply[16]);

      if (pluFieldsLenTableB32)
      {
         for (i = 0, j = 18; i < 32; i++, j += 4)
            NCopyLoHi32(&pluFieldsLenTableB32[i], &abuReply[j]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s23.c,v 1.7 1994/09/26 17:39:26 rebekah Exp $
*/
