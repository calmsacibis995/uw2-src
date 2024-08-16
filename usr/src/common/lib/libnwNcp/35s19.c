/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s19.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s19AFPGMacInfoDelFile**************************************************
SYNTAX:   N_EXTERN_LIBRARY( NWRCODE )
          NWNCP35s19AFPGMacInfoDelFile
          (
             pNWAccess pAccess,
             nuint8   buVolNum,
             nuint32  luAFPEntryID,
             pnuint8  pbuFinderInfoB32,
             pnuint8  pbuProDOSInfoB6,
             pnuint32 pluResFork,
             pnuint8  pbuFileNameLen,
             pnstr8   pbstrFileName,
          )
REMARKS: Return the Finder and ProDOS Info structures for a deleted
         Macintosh directory entry.


ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      <  pbuFinderInfoB32
      <  pbuProDOSInfoB6
      >  pluResFork
      <  pbuFileNameLen
      >  pbstrFileName

INCLUDE: "ncpafp.h"

RETURN:  0x0000   Successful
         0x89C6   No Console Rights
         0x89FF   Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     Disable File Server Login (0x2222  23  203)
         Get File Server Login Status (0x2222  23  205)
         Enable File Server Login (0x2222  23  204)

NCP:     35  19  Get Macintosh Info On Deleted File

CHANGES: 20 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s19AFPGMacInfoDelFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint8  pbuFinderInfoB32,
   pnuint8  pbuProDOSInfoB6,
   pnuint32 pluResFork,
   pnuint8  pbuFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 19)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 4)
   #define FILE_NAME_LEN   ((nuint) 255)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 5)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suTemp;

   suTemp = NCP_STRUCT_LEN;
   NCopyHiLo16 (abuReq, &suTemp);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = pbuFinderInfoB32;
   replyFrag[0].uLen  = FINDER_INFO_LEN;

   replyFrag[1].pAddr = pbuProDOSInfoB6;
   replyFrag[1].uLen  = PRODOS_INFO_LEN;

   replyFrag[2].pAddr = abuReply;
   replyFrag[2].uLen  = REPLY_LEN;

   replyFrag[3].pAddr = pbuFileNameLen;
   replyFrag[3].uLen  = (nuint) 1;

   replyFrag[4].pAddr = pbstrFileName;
   replyFrag[4].uLen  = FILE_NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
            REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(pluResFork, &abuReply[0]);
   }

   return ((NWRCODE)lCode);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s19.c,v 1.7 1994/09/26 17:38:21 rebekah Exp $
*/
