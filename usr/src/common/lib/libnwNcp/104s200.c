/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s200.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP104s200GetContAuditStatus**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP104s200GetContAuditStatus
         (
            pNWAccess pAccess,
            nuint32  luContID,
            pNWNCPContAuditStatus pStatus,
         )

REMARKS:

ARGS: <> pAccess
      >  luContID
      <  pStatus

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     104 200  Query Container Audit Status

CHANGES: 27 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP104s200GetContAuditStatus
(
   pNWAccess pAccess,
   nuint32  luContID,
   pNWNCPContAuditStatus pStatus
)
{
   #define NCP_FUNCTION    ((nuint) 104)
   #define NCP_SUBFUNCTION ((nuint8) 200)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 32)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luContID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(&pStatus->suAuditVerDate, &abuReply[0]);
      NCopyLoHi16(&pStatus->suAuditFileVerDate, &abuReply[2]);
      NCopyLoHi32(&pStatus->luAuditingEnabled, &abuReply[4]);
      NCopyLoHi32(&pStatus->luContAuditSize, &abuReply[8]);
      NCopyLoHi32(&pStatus->luContAuditFileCfgSize, &abuReply[12]);
      NCopyLoHi32(&pStatus->luContAuditFileMaxSize, &abuReply[16]);
      NCopyLoHi32(&pStatus->luContAuditFileSizeThresh, &abuReply[20]);
      NCopyLoHi32(&pStatus->luAuditRecCount, &abuReply[24]);
      NCopyLoHi32(&pStatus->luHistoryRecCount, &abuReply[28]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s200.c,v 1.7 1994/09/26 17:31:32 rebekah Exp $
*/
