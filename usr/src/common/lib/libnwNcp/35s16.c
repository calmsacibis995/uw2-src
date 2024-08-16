/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s16.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPSetFileInformation*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s16AFPSetFileInfo
         (
            pNWAccess       pAccess,
            nuint8         buVolNum,
            nuint32        luAFPEntryID,
            nuint16        suReqBitMap,
            pNWNCPAFPFileInfo pMacFileInfo,
         )

REMARKS:

ARGS: <> pAccess,
      >  buVolNum,
      >  luAFPEntryID,
      >  suReqBitMap,
      >  pMacFileInfo,

INCLUDE: ncpafp.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     35 05   AFP Get File Information
         35 17   AFP 2.0 Scan File Information

NCP:     35 16  AFP Set File Information

CHANGES: 25 Aug 1993 - Written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s16AFPSetFileInfo
(
   pNWAccess       pAccess,
   nuint8         buVolNum,
   nuint16        suReqBitMap,
   nuint8         buPathLen,
   pnstr8         pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 16)
   #define NCP_STRUCT_LEN  ((nuint16) (61 + buPathLen))
   #define REQ1_LEN        ((nuint) 24)
   #define REQ2_LEN        ((nuint) 7)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8      abuReq1[REQ1_LEN], abuReq2[REQ2_LEN];
   NWCFrag     reqFrag[REQ_FRAGS];
   nuint16     suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq1[0], &suNCPLen);

   abuReq1[2] = NCP_SUBFUNCTION;
   abuReq1[3] = buVolNum;

   NCopyHiLo32(&abuReq1[4],  &pMacFileInfo->luEntryID);
   NCopyHiLo16(&abuReq1[8],  &suReqBitMap);
   NCopyHiLo16(&abuReq1[10], &pMacFileInfo->suAttr);
   NCopyHiLo16(&abuReq1[12], &pMacFileInfo->suCreationDate);
   NCopyHiLo16(&abuReq1[14], &pMacFileInfo->suAccessDate);
   NCopyHiLo16(&abuReq1[16], &pMacFileInfo->suModifyDate);
   NCopyHiLo16(&abuReq1[18], &pMacFileInfo->suModifyTime);
   NCopyHiLo16(&abuReq1[20], &pMacFileInfo->suBackupDate);
   NCopyHiLo16(&abuReq1[22], &pMacFileInfo->suBackupTime);

   NWCMemMove(&abuReq2[0], pMacFileInfo->abuProDOSInfo, PRODOS_INFO_LEN);

   abuReq2[6] = buPathLen;

   reqFrag[0].pAddr = abuReq1;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pMacFileInfo->abuFinderInfo;
   reqFrag[1].uLen  = FINDER_INFO_LEN;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = REQ2_LEN;

   reqFrag[3].pAddr = pbstrPath;
   reqFrag[3].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s16.c,v 1.7 1994/09/26 17:38:17 rebekah Exp $
*/
