/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s21.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s21GetLANConfigInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s21GetLANConfigInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luBoardNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSELANInfo         pInfo,
         )

REMARKS:

ARGS: <> pAccess
      >  luBoardNum
      <  pVConsoleInfo        (optional)
      <  psuReserved          (optional)
      <  pInfo                (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 21  LAN Configuration Information

CHANGES: 23 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s21GetLANConfigInfo
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSELANInfo         pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 21)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 152)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luBoardNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pInfo)
      {
         pInfo->buConfigMajorVer = abuReply[8];
         pInfo->buConfigMinorVer = abuReply[9];
         NWCMemMove(pInfo->abuNodeAddress, &abuReply[10], (nuint) 6);
         NCopyLoHi16(&pInfo->suModeFlags, &abuReply[16]);
         NCopyLoHi16(&pInfo->suBoardNum, &abuReply[18]);
         NCopyLoHi16(&pInfo->suBoardInstance, &abuReply[20]);
         NCopyLoHi32(&pInfo->luMaximumSize, &abuReply[22]);
         NCopyLoHi32(&pInfo->luMaxRecvSize, &abuReply[26]);
         NCopyLoHi32(&pInfo->luRecvSize, &abuReply[30]);
         NCopyLoHi32(&pInfo->aluReserved1[0], &abuReply[34]);
         NCopyLoHi32(&pInfo->aluReserved1[1], &abuReply[38]);
         NCopyLoHi32(&pInfo->aluReserved1[2], &abuReply[42]);
         NCopyLoHi16(&pInfo->suCardID, &abuReply[46]);
         NCopyLoHi16(&pInfo->suMediaID, &abuReply[48]);
         NCopyLoHi16(&pInfo->suTransportTime, &abuReply[50]);
         NWCMemMove(pInfo->abuReserved, &abuReply[52], (nuint) 16);
         pInfo->buMajorVer= abuReply[68];
         pInfo->buMinorVer= abuReply[69];
         NCopyLoHi16(&pInfo->suDriverFlags, &abuReply[70]);
         NCopyLoHi16(&pInfo->suSendRetries, &abuReply[72]);
         NCopyLoHi32(&pInfo->luDriverLink, &abuReply[74]);
         NCopyLoHi16(&pInfo->suSharingFlags, &abuReply[78]);
         NCopyLoHi16(&pInfo->suDriverSlot, &abuReply[80]);
         NCopyLoHi16(&pInfo->asuIOPortsAndLengths[0], &abuReply[82]);
         NCopyLoHi16(&pInfo->asuIOPortsAndLengths[1], &abuReply[84]);
         NCopyLoHi16(&pInfo->asuIOPortsAndLengths[2], &abuReply[86]);
         NCopyLoHi16(&pInfo->asuIOPortsAndLengths[3], &abuReply[88]);
         NCopyLoHi32(&pInfo->luMemoryDecode0, &abuReply[90]);
         NCopyLoHi16(&pInfo->suMemoryLength0, &abuReply[94]);
         NCopyLoHi32(&pInfo->luMemoryDecode1, &abuReply[96]);
         NCopyLoHi16(&pInfo->suMemoryLength2, &abuReply[100]);
         pInfo->abuInterrupt[0] = abuReply[102];
         pInfo->abuInterrupt[1] = abuReply[103];
         pInfo->abuDMAUsage[0]= abuReply[104];
         pInfo->abuDMAUsage[1]= abuReply[105];
         NCopyLoHi32(&pInfo->aluReserved2[0], &abuReply[106]);
         NCopyLoHi32(&pInfo->aluReserved2[1], &abuReply[110]);
         NCopyLoHi32(&pInfo->aluReserved2[2], &abuReply[114]);
         NWCMemMove(pInfo->pbstrLogicalName, &abuReply[118], (nuint) 18);
         NCopyLoHi32(&pInfo->aluLinearMemory[0], &abuReply[136]);
         NCopyLoHi32(&pInfo->aluLinearMemory[1], &abuReply[140]);
         NCopyLoHi16(&pInfo->suChannelNum, &abuReply[144]);
         NWCMemMove(pInfo->abuIOReserved, &abuReply[146], (nuint) 6);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s21.c,v 1.7 1994/09/26 17:32:16 rebekah Exp $
*/
