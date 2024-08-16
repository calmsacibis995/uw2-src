/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s16.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s16DelScan************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s16DelScan
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buDataStream,
            nuint32           luRetMask,
            pNWNCPCompPath    pCompPath,
            pnuint32          pluIterHnd,
            pnuint16          psuDeleteTime,
            pnuint16          psuDeleteDate,
            pnuint32          pluDeletorID,
            pnuint32          pluVolNum,
            pnuint32          pluDirBase,
            pNWNCPEntryStruct pEntry
         );

REMARKS: Scans a subdirectory for any files that have been deleted but not
         yet purged.  The NWHandlePathStruct must point to a SubDirectory
         path.  No file names or wild cards are allowed when using this call
         to search for salvageable files or subdirectories.

         The ScanSequence field is set to -1 on the first call.  On each
         subsequent call it is replaced by the NextScanSequence value.

ARGS: <> pAccess
       > buNamSpc
       > buDataStream
       > luRetMask
       > pCompPath
      <> pluIterHnd
      <  psuDeleteTime (optional)
      <  psuDeleteDate (optional)
      <  pluDeletorID (optional)
      <  pluVolNum (optional)
      <  pluDirBase (optional)
      <  pEntry

INCLUDE: ncpfile.h

RETURN:  0x0000   SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 27  Scan Salvageable Files

NCP:     87 16  Scan Salvageable Files

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s16DelScan
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDataStream,
   nuint32           luRetMask,
   pNWNCPCompPath    pCompPath,
   pnuint32          pluIterHnd,
   pnuint16          psuDeleteTime,
   pnuint16          psuDeleteDate,
   pnuint32          pluDeletorID,
   pnuint32          pluVolNum,
   pnuint32          pluDirBase,
   pNWNCPEntryStruct pEntry
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 16)
   #define REQ_LEN         ((nuint) 11)
   #define ENTRY_LEN       ((nuint) 77)
   #define REPLY_LEN       ((nuint) 20)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 3)
   #define MAX_NAME_LEN    ((nuint) 256)

   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN], abuEntry[ENTRY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   NWRCODE ccode;

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDataStream;
   NCopyLoHi32(&abuReq[3], &luRetMask);
   NCopyLoHi32(&abuReq[7], pluIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = abuEntry;
   replyFrag[1].uLen  = ENTRY_LEN;

   replyFrag[2].pAddr = pEntry->abuName;
   replyFrag[2].uLen  = MAX_NAME_LEN;

   if((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      NCopyLoHi32(pluIterHnd, &abuReply[0]);
      if (psuDeleteTime)
         NCopyLoHi16(psuDeleteTime, &abuReply[4]);
      if (psuDeleteDate)
         NCopyLoHi16(psuDeleteDate, &abuReply[6]);
      if (pluDeletorID)
         NCopyHiLo32(pluDeletorID, &abuReply[8]);
      if (pluVolNum)
         NCopyLoHi32(pluVolNum, &abuReply[12]);
      if (pluDirBase)
         NCopyLoHi32(pluDirBase, &abuReply[16]);

      NWNCPUnpackEntryStruct(pEntry, &abuEntry[0], luRetMask);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s16.c,v 1.7 1994/09/26 17:39:17 rebekah Exp $
*/
