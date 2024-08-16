/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s17.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s17WriteAuditFilCfgHdr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s17WriteAuditFilCfgHdr
         (
            pNWAccess    pAccess,
            nuint32     luVolNum,
            nuint16     suAuditVerDate,
            pnuint8     pbuKeyB8,
            pNWNCPAuditConfigFileHdr pConfigHdr
         );

REMARKS:

ARGS: <> pAccess
       > luVolNum
       > suAuditVerDate
       > pbuKeyB8
       > pConfigHdr

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 17   Write Audit File Configuration Header

CHANGES: 27 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s17WriteAuditFilCfgHdr
(
   pNWAccess    pAccess,
   nuint32     luVolNum,
   nuint16     suAuditVerDate,
   pnuint8     pbuKeyB8,
   pNWNCPAuditConfigFileHdr pConfigHdr
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 17)
   #define REQ_LEN         ((nuint) 143)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nint i, j;

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);
   NCopyLoHi16(&abuReq[5], &suAuditVerDate);
   NWCMemMove(&abuReq[7], pbuKeyB8, 8);
   NCopyLoHi16(&abuReq[15], &pConfigHdr->suAuditFileVerDate);
   abuReq[17] = pConfigHdr->buAuditFlag;
   abuReq[18] = pConfigHdr->buErrMsgDelayInMinsFactor;
   NWCMemMove(&abuReq[19], pConfigHdr->abuEncryptedPass, 16);
   NCopyLoHi32(&abuReq[35], &pConfigHdr->luVolAuditFileMaxSize);
   NCopyLoHi32(&abuReq[39], &pConfigHdr->luVolAuditFileSizeThresh);
   NCopyLoHi32(&abuReq[43], &pConfigHdr->luAuditRecCount);
   NCopyLoHi32(&abuReq[47], &pConfigHdr->luHistRecCount);

   for(i = 0, j = 51; i < 7; i++, j += 4)
   {
      NCopyLoHi32(&abuReq[j], &pConfigHdr->aluReservedB7[i]);
   }

   NWCMemMove(&abuReq[79], pConfigHdr->abuMapB64, 64);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                   NULL, REPLY_LEN, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s17.c,v 1.7 1994/09/26 17:39:56 rebekah Exp $
*/
