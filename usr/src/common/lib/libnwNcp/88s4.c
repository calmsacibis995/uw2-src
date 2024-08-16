/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s4ChgAuditorVolPass**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s4ChgAuditorVolPass
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            nuint16  suAuditVerDate,
            pnuint8  pbuOldKeyB8,
            pnuint8  pbuNewPassB16,
         )

REMARKS:

ARGS: <> pAccess
      >  luVolNum
      >  suAuditVerDate
      >  pbuOldKeyB8
      >  pbuNewPassB16

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 04  Change Auditor Volume Password

CHANGES: 27 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s4ChgAuditorVolPass
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuOldKeyB8,
   pnuint8  pbuNewPassB16
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define REQ_LEN         ((nuint) 31)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);
   NCopyLoHi16(&abuReq[5], &suAuditVerDate);
   NWCMemMove(&abuReq[7], pbuOldKeyB8, (nuint) 8);
   NWCMemMove(&abuReq[15], pbuNewPassB16, (nuint) 16);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s4.c,v 1.7 1994/09/26 17:40:06 rebekah Exp $
*/
