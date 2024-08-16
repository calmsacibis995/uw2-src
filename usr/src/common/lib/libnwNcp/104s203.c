/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s203.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP104s203ChgAuditorContPass**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP104s203ChgAuditorContPass
         (
            pNWAccess pAccess,
            nuint32  luContID,
            nuint16  suAuditVerDate,
            pnuint8  pbuOldKeyB8,
            pnuint8  pbuNewPassB16,
         )

REMARKS:

ARGS: <> pAccess
      >  luContID
      >  suAuditVerDate
      >  pbuOldKeyB8
      >  pbuNewPassB16

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     104 203  Change Auditor Container Password

CHANGES: 28 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP104s203ChgAuditorContPass
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuOldKeyB8,
   pnuint8  pbuNewPassB16
)
{
   #define NCP_FUNCTION    ((nuint) 104)
   #define NCP_SUBFUNCTION ((nuint8) 203)
   #define REQ_LEN         ((nuint) 31)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luContID);
   NCopyLoHi16(&abuReq[5], &suAuditVerDate);
   NWCMemMove(&abuReq[7], pbuOldKeyB8, (nuint) 8);
   NWCMemMove(&abuReq[15], pbuNewPassB16, (nuint) 16);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s203.c,v 1.7 1994/09/26 17:31:34 rebekah Exp $
*/
