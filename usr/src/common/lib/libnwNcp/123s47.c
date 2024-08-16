/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s47.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s47GetLoadedMediaList**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s47GetLoadedMediaList
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluMaxNumMedia,
            pnuint32                 pluNumMedias,
            pnuint32                 pluMediasB32,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluMaxNumMedia (optional)
      <  pluNumMedias
      <  pluMediasB32

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 47  Get Loaded Media Num List

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s47GetLoadedMediaList
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumMedia,
   pnuint32                 pluNumMedias,
   pnuint32                 pluMediasB32
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 47)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define MAX_MEDIA_LEN    ((nuint) 128)
   #define NCP_REQ_LEN     ((nuint) 3)
   #define NCP_REPLY_LEN   ((nuint) 16)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pluMediasB32;
   replyFrag[1].uLen  = MAX_MEDIA_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pluMaxNumMedia)
         NCopyLoHi32(pluMaxNumMedia, &abuReply[8]);
      NCopyLoHi32(pluNumMedias, &abuReply[12]);

      for (i = 0; i < (nint) *pluNumMedias; i++)
         pluMediasB32[i] = NSwapLoHi32(pluMediasB32[i]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s47.c,v 1.7 1994/09/26 17:32:41 rebekah Exp $
*/
