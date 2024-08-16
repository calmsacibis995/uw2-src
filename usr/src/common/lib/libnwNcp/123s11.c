/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s11GetNLMInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s11GetNLMInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luNLMNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSENLMInfo         pNLMInfo,
            pnuint8                  pbuFileNameLen,
            pnstr8                   pbstrFileName,
            pnuint8                  pbuNLMNameLen,
            pnstr8                   pbstrNLMName,
            pnuint8                  pbuCopyrightLen,
            pnstr8                   pbstrCopyright,
         )

REMARKS:

ARGS: <> pAccess
      >  luNLMNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pNLMInfo (optional)
      <  pbuFileNameLen (optional)
      <  pbstrFileName (optional)
      <  pbuNLMNameLen (optional)
      <  pbstrNLMName (optional)
      <  pbuCopyrightLen (optional)
      <  pbstrCopyright (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 11 Get NLM Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s11GetNLMInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNLMNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSENLMInfo         pNLMInfo,
   pnuint8                  pbuFileNameLen,
   pnstr8                   pbstrFileName,
   pnuint8                  pbuNLMNameLen,
   pnstr8                   pbstrNLMName,
   pnuint8                  pbuCopyrightLen,
   pnstr8                   pbstrCopyright
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 11)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_STRS_LEN    ((nuint) (NWNCP_FSE_MAX_FILE_NAME +\
                                     NWNCP_FSE_MAX_NLM_NAME +\
                                     NWNCP_FSE_MAX_CRIGHT_NAME + 2))
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 69)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN], abuReply1[REPLY_LEN];
   nuint8  abuReply2[MAX_STRS_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luNLMNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply1;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = abuReply2;
   replyFrag[1].uLen  = MAX_STRS_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      pnstr   pbstrTemp;
      nuint16 suStrLen;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply1 );

      NCopyLoHi16(psuReserved, &abuReply1[6]);

      NCopyLoHi32(&pNLMInfo->luIdNum, &abuReply1[8]);
      NCopyLoHi32(&pNLMInfo->luFlags, &abuReply1[12]);
      NCopyLoHi32(&pNLMInfo->luType, &abuReply1[16]);
      NCopyLoHi32(&pNLMInfo->luParentID, &abuReply1[20]);
      NCopyLoHi32(&pNLMInfo->luMajorVer, &abuReply1[24]);
      NCopyLoHi32(&pNLMInfo->luMinorVer, &abuReply1[28]);
      NCopyLoHi32(&pNLMInfo->luRevision, &abuReply1[32]);
      NCopyLoHi32(&pNLMInfo->luYear, &abuReply1[36]);
      NCopyLoHi32(&pNLMInfo->luMonth, &abuReply1[40]);
      NCopyLoHi32(&pNLMInfo->luDay, &abuReply1[44]);
      NCopyLoHi32(&pNLMInfo->luAllocAvailBytes, &abuReply1[48]);
      NCopyLoHi32(&pNLMInfo->luAllocFreeCnt, &abuReply1[52]);
      NCopyLoHi32(&pNLMInfo->luLastGarbCollect, &abuReply1[56]);
      NCopyLoHi32(&pNLMInfo->luMsgLanguage, &abuReply1[60]);
      NCopyLoHi32(&pNLMInfo->luNumPublics, &abuReply1[64]);

      suStrLen  = (nuint16) abuReply1[68];
      pbstrTemp = &abuReply2[0];

      if (pbuFileNameLen)
         *pbuFileNameLen = (nuint8) suStrLen;
      if (pbstrFileName)
         NWCMemMove(pbstrFileName, pbstrTemp, suStrLen);
      pbstrTemp += suStrLen;

      suStrLen = *pbstrTemp++;
      if (pbuNLMNameLen)
         *pbuFileNameLen = (nuint8) suStrLen;
      if (pbstrNLMName)
         NWCMemMove(pbstrNLMName, pbstrTemp, suStrLen );
      pbstrTemp += suStrLen;

      suStrLen = *pbstrTemp++;
      if (pbuCopyrightLen)
         *pbuCopyrightLen = (nuint8) suStrLen;
      if (pbstrCopyright)
         NWCMemMove(pbstrCopyright, pbstrTemp, suStrLen );
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s11.c,v 1.7 1994/09/26 17:32:06 rebekah Exp $
*/
