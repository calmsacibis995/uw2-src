/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s31.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s31GetMediaObjectList*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s31GetMediaObjectList
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            nuint32                  luObjType,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNextStartNum,
            pnuint32                 pluObjCnt,
            pnuint32                 pluObjB128,
         )
REMARKS:

ARGS: <> pAccess
      >  luStartNum
      >  luObjType
      <  pVConsoleInfo      (optional)
      <  puReserved         (optional)
      <  pluNextStartNum
      <  pluObjCount
      <  pluObjB128

INCLUDE: ncpfse.h

RETURN:

SERVER:

CLIENT:

SEE:

NCP:     123 31 Get Media Manager Objects List

CHANGES: 22 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s31GetMediaObjectList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luObjType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluObjCnt,
   pnuint32                 pluObjB128
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 31)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define MAX_OBJBUF_LEN  ((nuint) 128)
   #define NCP_REQ_LEN     ((nuint) 11)
   #define NCP_REPLY_LEN   ((nuint) 16)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint32 luTemp;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nint i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);
   NCopyLoHi32(&abuReq[7], &luObjType);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pluObjB128;
   replyFrag[1].uLen  = MAX_OBJBUF_LEN;


   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if(psuReserved !=NULL)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      NCopyLoHi32(pluNextStartNum, &abuReply[8]);
      NCopyLoHi32(pluObjCnt, &abuReply[12]);

      for(i = 0; i < (nint)*pluObjCnt; ++i)
      {
         luTemp=pluObjB128[i];
         NCopyLoHi32(&pluObjB128[i], &luTemp);
      }

   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s31.c,v 1.7 1994/09/26 17:32:25 rebekah Exp $
*/
