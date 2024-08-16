/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s3ScanNext***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s3ScanNext
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buDataStream,
            nuint16           suSrchAttrs,
            nuint32           luRetMask,
            pNWNCPSearchSeq   pIterHnd,
            nuint8            buSrchPatternLen,
            pnuint8           pbuSrchPattern,
            pnuint8           pbuReserved,
            pNWNCPEntryStruct pEntryStruct
         );

REMARKS:
         This is a NetWare 386 v3.11 call that replaces the earlier call, Search For A
         File (0x2222  64  --).

         The SearchAttributes field, ReturnInfoMask field, NetWareInfoStruct field,
         and NetWareFileNameStruct field are explained in more detail in the
         Introduction to Directory Services.

         This function will search for a file or subdirectory starting with the
         Search Sequence number returned by the Initialize Search request.

ARGS: <> pAccess
       > buNamSpc
       > buDataStream
       > suSrchAttrs
       > luRetMask
      <> pIterHnd
       > buSrchPatternLen
       > pbuSrchPattern
      <  pbuReserved (optional)
      <  pEntryStruct

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 03  Search For File or Subdirectory

CHANGES: 1 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s3ScanNext
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDataStream,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   pNWNCPSearchSeq   pIterHnd,
   nuint8            buSrchPatternLen,
   pnuint8           pbuSrchPattern,
   pnuint8           pbuReserved,
   pNWNCPEntryStruct pEntryStruct
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define REQ_LEN         ((nuint) 19)
   #define REPLY_LEN       ((nuint) 87)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)
   #define NAME_LEN        ((nuint) 256)

   nint32  lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDataStream;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);
   NCopyLoHi32(&abuReq[5], &luRetMask);
   abuReq[9] = pIterHnd->buVolNum;
   NCopyLoHi32(&abuReq[10], &pIterHnd->luDirNum);
   NCopyLoHi32(&abuReq[14], &pIterHnd->luEntryNum);
   abuReq[18] = buSrchPatternLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuSrchPattern;
   reqFrag[1].uLen  = buSrchPatternLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pEntryStruct->abuName;
   replyFrag[1].uLen  = NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag, REPLY_FRAGS,
               replyFrag, NULL);

   if(lCode == 0)
   {
      pIterHnd->buVolNum = abuReply[0];
      NCopyLoHi32(&pIterHnd->luDirNum, &abuReply[1]);
      NCopyLoHi32(&pIterHnd->luEntryNum, &abuReply[5]);

      if (pbuReserved)
         *pbuReserved = abuReply[9];

      /* force unpacking of the name length by ORing the return
         mask with 1 (to turn bit 0 on) */

      NWNCPUnpackEntryStruct(pEntryStruct, &abuReply[10], luRetMask | 1);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s3.c,v 1.7 1994/09/26 17:39:35 rebekah Exp $
*/
