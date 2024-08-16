/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s29.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s29GetDirEffRights**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP87s29GetDirEffRights
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buDstNamSpc,
            nuint16           suSrchAttrs,
            nuint32           luReturnInfoMask,
            pNWNCPCompPath    pCompPath,
            pnuint16          psuEffRights,
            pNWNCPEntryStruct pEntry,
         )

REMARKS:

ARGS: <> pAccess
      >  buNamSpc
      >  buDstNamSpc
      >  suSrchAttrs
      >  luReturnInfoMask
      >  pCompPath
      <  psuEffRights
      <  pEntry

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 03  Get Effective Directory Rights

NCP:     87 29  Get Effective Directory Rights

CHANGES: 9 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s29GetDirEffRights
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDstNamSpc,
   nuint16           suSrchAttrs,
   nuint32           luReturnInfoMask,
   pNWNCPCompPath    pCompPath,
   pnuint16          psuEffRights,
   pNWNCPEntryStruct pEntry
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 29)
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 2)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 3)
   #define INFO_SIZE       ((nuint) 77)
   #define NAME_LEN        ((nuint) 256)

   nint32   lCode;
   nuint8 abuEntryInfoPacked[INFO_SIZE];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDstNamSpc;

   NCopyLoHi16(&abuReq[3], &suSrchAttrs);
   NCopyLoHi32(&abuReq[5], &luReturnInfoMask);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = abuEntryInfoPacked;
   replyFrag[1].uLen  = INFO_SIZE;

   replyFrag[2].pAddr = pEntry->abuName;
   replyFrag[2].uLen  = NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi16(psuEffRights, &abuReply[0]);

      NWNCPUnpackEntryStruct(pEntry, abuEntryInfoPacked, luReturnInfoMask);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s29.c,v 1.7 1994/09/26 17:39:34 rebekah Exp $
*/
