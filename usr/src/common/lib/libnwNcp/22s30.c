/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s30.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s30Scan**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s30Scan
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            nuint8            buSrchAttrs,
            nuint8            buSrchPatternLen,
            pnuint8           pbuSrchPattern,
            pnuint32          pluIterHnd,
            pNWNCPEntryUnion  pEntryInfo,
         );

REMARKS: This function scans a directory using an 8.3 wild card (server wild card
         format).  It will return information about the file or directory.  To
         initialize a scan, set the sequence number to FFFFFFFFh.  To continue
         scanning, pass the previously returned sequence number.


ARGS: <> pAccess
      >  buDirHandle
      >  buSrchAttrs
      >  buSrchPatternLen
      >  pbuSrchPattern
      <> pluIterHnd
      <  pEntryInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x89FB  386 File Structure Not Supported On
         0x89FF  No More Matches In Directory

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 40  Scan Directory Disk Space

NCP:     22 30  Scan a Directory

CHANGES: 10 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s30Scan
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buSrchPatternLen,
   pnuint8           pbuSrchPattern,
   pnuint32          pluIterHnd,
   pNWNCPEntryUnion  pEntryInfo
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 30)
   #define NCP_STRUCT_LEN  ((nuint16) 8 + buSrchPatternLen)
   #define NCP_REQ_LEN     ((nuint) 10)
   #define NCP_REPLY_LEN   ((nuint) 337)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buSrchAttrs;
   NCopyLoHi32(&abuReq[5],pluIterHnd);
   abuReq[9] = buSrchPatternLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbuSrchPattern;
   reqFrag[1].uLen  = (nuint) buSrchPatternLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi32(pluIterHnd,&abuRep[0]);
      NWNCPUnpackEntryUnion(pEntryInfo, &abuRep[4], NCP_SUBFUNCTION);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s30.c,v 1.7 1994/09/26 17:34:12 rebekah Exp $
*/
