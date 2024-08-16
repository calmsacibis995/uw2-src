/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s15.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPGetFileInformation*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s15AFPGetDirEntryInfo
         (
            pNWAccess       pAccess,
            nuint8         buVolNum,
            nuint32        luAFPEntryID,
            nuint16        suReqBitMap,
            pnstr8         pbstrPath,
            pNWNCPAFPFileInfo pMacFileInfo,
         )

REMARKS:

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  suReqBitMap
      >  pbstrPath
      <  pMacFileInfo

INCLUDE: ncpafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     35 16  AFP 2.0 Set File Information
         35 05  AFP Get File Information
         35 09  AFP Set File Information

NCP:     35 15  AFP Get File Information

CHANGES: 19 Aug 1993 - Written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s15AFPGetDirEntryInfo
(
   pNWAccess       pAccess,
   nuint8         buVolNum,
   nuint32        luAFPEntryID,
   nuint16        suReqBitMap,
   nuint8         buPathLen,
   pnstr8         pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 15)
   #define NCP_STRUCT_LEN  ((nuint16) (9 + buPathLen))
   #define REQ_LEN         ((nuint) 11)
   #define REPLY_LEN       ((nuint) AFP_FILE_INFO_LEN)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   NWCFrag     reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8      receivePacket[REPLY_LEN];
   nuint16     suNCPLen;
   nuint8      abuReq[REQ_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;

   NCopyHiLo32(&abuReq[4], &luAFPEntryID);
   NCopyHiLo16(&abuReq[8], &suReqBitMap);

   abuReq[10] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = receivePacket;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWCUnpackAFPPacket(pMacFileInfo, receivePacket);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s15.c,v 1.7 1994/09/26 17:38:15 rebekah Exp $
*/
