/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s14.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s14AFPCreateFile*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s14AFPCreateFile
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            nuint8   buDelFileFlag,
            pnuint8  pbuFinderInfoB32,
            pnuint8  pbuProDOSInfoB6,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint32 pluNewAFPEntryID,
         )

REMARKS:

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buDelFileFlag
      >  pbuFinderInfoB32
      >  pbuProDOSInfoB6
      >  buPathLen
      >  pbstrPath
      <  pluNewAFPEntryID

INCLUDE: ncpafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     35 03 AFP Delete
         35 02 AFP Create File

NCP:     35 14  AFP Create File

CHANGES: 19 Aug 1993 - Written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s14AFPCreateFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buDelFileFlag,
   pnuint8  pbuFinderInfoB32,
   pnuint8  pbuProDOSInfoB6,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluNewAFPEntryID
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 14)
   #define NCP_STRUCT_LEN  ((nuint16) (46 + buPathLen))
   #define REQ_LEN         ((nuint) 48)
   #define REPLY_LEN       ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   nuint16  suPathLen;
   NWCFrag  reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8   abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suPathLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suPathLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;

   NCopyHiLo32(&abuReq[4], &luAFPEntryID);

   abuReq[8] = buDelFileFlag;

   NWCMemMove(&abuReq[9],  pbuFinderInfoB32, FINDER_INFO_LEN);
   NWCMemMove(&abuReq[41], pbuProDOSInfoB6, PRODOS_INFO_LEN);

   abuReq[47] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag, REPLY_FRAGS,
                 replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyHiLo32(pluNewAFPEntryID, &abuReply[0]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s14.c,v 1.7 1994/09/26 17:38:14 rebekah Exp $
*/
