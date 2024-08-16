/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s60.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s60GetServerSetInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s60GetServerSetInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluTotalNumSetCommands,
            pnuint32                 pluNextStartNum,
            pnuint32                 pluSetCommandType,
            pnuint32                 pluSetCommandCategory,
            pnuint32                 pluSetCommandFlags,
            pnuint8                  pbuCmdNameLen,
            pnstr8                   pbstrSetCmdNameB483,
            pnuint8                  pbuCmdValueLen,
            pnstr8                   pbstrSetCmdValueB483,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluTotalNumSetCommands (optional)
      <  pluNextStartNum (optional)
      <  pluSetCommandType (optional)
      <  pluSetCommandCategory (optional)
      <  pluSetCommandFlags (optional)
      <  pbuCmdNameLen (optional)
      <  pbstrSetCmdNameB483 (optional)
      <  pbuCmdValueLen (optional)
      <  pbstrSetCmdValueB483 (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 60  Get Server Set Commands Information

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s60GetServerSetInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalNumSetCommands,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluSetCommandType,
   pnuint32                 pluSetCommandCategory,
   pnuint32                 pluSetCommandFlags,
   pnuint8                  pbuCmdNameLen,
   pnstr8                   pbstrSetCmdNameB483,
   pnuint8                  pbuCmdValueLen,
   pnstr8                   pbstrSetCmdValueB483
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 60)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_STRING_LEN  ((nuint) 484)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 28)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint8 abuStrings[MAX_STRING_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = abuStrings;
   replyFrag[1].uLen  = MAX_STRING_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      pnstr8  pbstrIndex;
      nuint16 suLen;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(&psuReserved, &abuReply[6]);

      if (pluTotalNumSetCommands)
         NCopyLoHi32(&pluTotalNumSetCommands, &abuReply[8]);
      if (pluNextStartNum)
         NCopyLoHi32(&pluNextStartNum, &abuReply[12]);
      if (pluSetCommandType)
         NCopyLoHi32(&pluSetCommandType, &abuReply[16]);
      if (pluSetCommandCategory)
         NCopyLoHi32(&pluSetCommandCategory, &abuReply[20]);
      if (pluSetCommandFlags)
         NCopyLoHi32(&pluSetCommandFlags, &abuReply[24]);

      pbstrIndex = abuStrings;

      suLen = *pbstrIndex++;
      if (pbuCmdNameLen)
         *pbuCmdNameLen = (nuint8) suLen;
      if (pbstrSetCmdNameB483)
         NWCMemMove((nptr)pbstrSetCmdNameB483, (nptr)pbstrIndex, suLen);

      suLen = *pbstrIndex++;
      if (pbuCmdValueLen)
         *pbuCmdValueLen = (nuint8) suLen;
      if (pbstrSetCmdValueB483)
         NWCMemMove((nptr)pbstrSetCmdValueB483, (nptr)pbstrIndex, suLen);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s60.c,v 1.7 1994/09/26 17:32:59 rebekah Exp $
*/
