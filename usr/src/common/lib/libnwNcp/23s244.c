/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s244.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP23s244ConvertPathToEntry**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s244ConvertPathToEntry
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuVolNum,
            pnuint32 pluDirBase,
         );

REMARKS: Remarks

ARGS: <> pAccess
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <  pbuVolNum
      <  pluDirBase

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 243  Map Directory Number To Path

NCP:     23 244  Convert Path To Dir Entry

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s244ConvertPathToEntry
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuVolNum,
   pnuint32 pluDirBase
)
{
   #define NCP_FUNCTION       ((nuint)         23)
   #define NCP_SUBFUNCTION    ((nuint8)        244)
   #define NCP_STRUCT_LEN     ((nuint16) (3 + buPathLen))
   #define NCP_REQ_LEN        ((nuint)           5)
   #define NCP_REP_LEN        ((nuint)           5)
   #define NCP_REQ_FRAGS      ((nuint)           2)
   #define NCP_REP_FRAGS      ((nuint)           1)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
            replyFrag, NULL);
   if (lCode == 0)
   {
      *pbuVolNum = abuReply[0];
      NCopyLoHi32(pluDirBase, &abuReply[1]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s244.c,v 1.7 1994/09/26 17:37:00 rebekah Exp $
*/
