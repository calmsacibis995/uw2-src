/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s2ScanFirst***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s2ScanFirst
         (
            pNWAccess        pAccess,
            nuint8          buNamSpc,
            nuint8          buReserved,
            pNWNCPCompPath  pCompPath,
            pNWNCPSearchSeq pIterHnd
         );

REMARKS: This is a NetWare 386 v3.11 call that replaces the earlier call File
         Search Initialize (0x2222  62  --).  This call will initialize the
         search for a file or subdirectory, this search function is a
         stateless search.

         The SearchSequence Definition field is a 9 byte field, consisting of
         a 1 byte Volume number, a 4 byte Directory Number, and a 4 byte
         Current Directory Number.  The Search Sequence is generated only by
         the Server on each completed Search call.

ARGS: <> pAccess
       > buNamSpc
       > buReserved
       > pCompPath
      <  pIterHnd

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 02  Initialize Search

CHANGES: 1 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s2ScanFirst
(
   pNWAccess        pAccess,
   nuint8          buNamSpc,
   nuint8          buReserved,
   pNWNCPCompPath  pCompPath,
   pNWNCPSearchSeq pIterHnd
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 9)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWRCODE rcode;

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   rcode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if (rcode == 0)
   {
      pIterHnd->buVolNum = abuReply[0];
      NCopyLoHi32(&pIterHnd->luDirNum, &abuReply[1]);
      NCopyLoHi32(&pIterHnd->luEntryNum, &abuReply[5]);
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s2.c,v 1.7 1994/09/26 17:39:22 rebekah Exp $
*/
