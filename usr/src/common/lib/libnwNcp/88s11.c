/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s11ReadAuditFileCfgHdr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s11ReadAuditFileCfgHdr
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            pNWNCPAuditConfigFileHdr pHeader
         );

REMARKS:

ARGS: <> pAccess
       > luVolNum
      <  pHeader

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 11  Read Audit File Configuration Header

CHANGES: 27 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s11ReadAuditFileCfgHdr
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   pNWNCPAuditConfigFileHdr pHeader
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 11)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 128)

   nint32 lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      nint i, j;

      NCopyLoHi16(&pHeader->suAuditFileVerDate, &abuReply[0]);
      pHeader->buAuditFlag = abuReply[2];
      pHeader->buErrMsgDelayInMinsFactor = abuReply[3];
      NWCMemMove(pHeader->abuEncryptedPass, &abuReply[4], 16);
      NCopyLoHi32(&pHeader->luVolAuditFileMaxSize, &abuReply[20]);
      NCopyLoHi32(&pHeader->luVolAuditFileSizeThresh, &abuReply[24]);
      NCopyLoHi32(&pHeader->luAuditRecCount, &abuReply[28]);
      NCopyLoHi32(&pHeader->luHistRecCount, &abuReply[32]);

      for (i = 0, j = 36; i < 7; i++, j += 4)
         NCopyLoHi32(&pHeader->aluReservedB7[i], &abuReply[j]);

      NWCMemMove(pHeader->abuMapB64, &abuReply[64], 64);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s11.c,v 1.7 1994/09/26 17:39:48 rebekah Exp $
*/
