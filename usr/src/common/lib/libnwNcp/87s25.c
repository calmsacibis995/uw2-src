/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s25.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s25NSSetInfo***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s25NSSetInfo
         (
            pNWAccess pAccess,
            nuint8   buSrcNamSpc,
            nuint8   buDstNamSpc,
            nuint8   buVolNum,
            nuint32  luDirBase,
            nuint32  luNSInfoBitMask,
            pnuint8  pbuNSInfoB512
         );

REMARKS: This is a NetWare 386 v3.11 call.

         This call sets specific name space information.  Note also that 1) this call
         is passed to the name space NLM and 2) this call is an expensive time user
         on the server.

         The NSInfoBitMask field is explained in more detail in the Introduction to
         Directory Services.

ARGS: <> pAccess
       > buSrcNamSpc
       > buDstNamSpc
       > buVolNum
       > luDirBase
       > luNSInfoBitMask
       > pbuNSInfoB512

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     87 19  Get NS Information
         87 23  Query NS Information Format

NCP:     87 25  Set NS Information

CHANGES: 14 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s25NSSetInfo
(
   pNWAccess pAccess,
   nuint8   buSrcNamSpc,
   nuint8   buDstNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luNSInfoBitMask,
   pnuint8  pbuNSInfoB512
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 25)
   #define REQ_LEN         ((nuint) 12)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];
   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buSrcNamSpc;
   abuReq[2] = buDstNamSpc;
   abuReq[3] = buVolNum;
   NCopyLoHi32(&abuReq[4], &luDirBase);
   NCopyLoHi32(&abuReq[8], &luNSInfoBitMask);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuNSInfoB512;
   reqFrag[1].uLen  = (nuint) 512;

   return NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
                  REPLY_FRAGS, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s25.c,v 1.7 1994/09/26 17:39:29 rebekah Exp $
*/
