/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s41.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s41GetProtocolConfig**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s41GetProtocolConfig
         (
            pNWAccess                 pAccess,
            nuint32                  luStackNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint8                  pbuConfigMajorVer,
            pnuint8                  pbuConfigMinorVer,
            pnuint8                  pbuStackMajorVer,
            pnuint8                  pbuStackMinorVer,
            pnstr8                   pbstrShortStackNameB16,
            pnuint8                  pbuStackFullNameLen,
            pnstr8                   pbstrStackFullName,
         )

REMARKS:

ARGS: <> pAccess
      >  luStackNum
      <  pVConsoleInfo           (optional)
      <  psuReserved             (optional)
      <  pbuConfigMajorVer       (optional)
      <  pbuConfigMinorVer       (optional)
      <  pbuStackMajorVer        (optional)
      <  pbuStackMinorVer        (optional)
      <  pbstrShortStackName
      <  pbuStackFullNameLen
      <  pbstrStackFullName

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 41  Get Protocol Stack Configuration Information

CHANGES: 23 Sep 1993 - written -  rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s41GetProtocolConfig
(
   pNWAccess                 pAccess,
   nuint32                  luStackNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuConfigMajorVer,
   pnuint8                  pbuConfigMinorVer,
   pnuint8                  pbuStackMajorVer,
   pnuint8                  pbuStackMinorVer,
   pnstr8                   pbstrShortStackNameB16,
   pnuint8                  pbuStackFullNameLen,
   pnstr8                   pbstrStackFullName
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 41)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN0   ((nuint) 12)
   #define NCP_REPLY_LEN1   ((nuint) 16)
   #define NCP_REPLY_LEN2   ((nuint) 1)
   #define NCP_REPLY_LEN3   ((nuint) 256)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 4)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN0];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStackNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN0;

   replyFrag[1].pAddr = pbstrShortStackNameB16;
   replyFrag[1].uLen  = NCP_REPLY_LEN1;

   replyFrag[2].pAddr = pbuStackFullNameLen;
   replyFrag[2].uLen  = NCP_REPLY_LEN2;

   replyFrag[3].pAddr = pbstrStackFullName;
   replyFrag[3].uLen  = NCP_REPLY_LEN3;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if(psuReserved!=NULL)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if(pbuConfigMajorVer!=NULL)
         *pbuConfigMajorVer= abuReply[8];

      if(pbuConfigMinorVer!=NULL)
         *pbuConfigMinorVer= abuReply[9];

      if(pbuStackMajorVer!=NULL)
         *pbuStackMajorVer= abuReply[10];

      if(pbuStackMinorVer!=NULL)
         *pbuStackMinorVer= abuReply[11];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s41.c,v 1.7 1994/09/26 17:32:33 rebekah Exp $
*/
