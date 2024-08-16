/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s6DelUserAuditProp**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s6DelUserAuditProp
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            nuint16  suAuditVerDate,
            pnuint8  pbuKeyB8,
            nuint32  luUserID,
         )

REMARKS:

ARGS: <> pAccess
      >  luVolNum
      >  suAuditVerDate
      >  pbuKeyB8
      >  luUserID

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 06  Delete User Audit Property

CHANGES: 27 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s6DelUserAuditProp
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   nuint32  luUserID
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define REQ_LEN         ((nuint) 19)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);
   NCopyLoHi16(&abuReq[5], &suAuditVerDate);
   NWCMemMove(&abuReq[7], pbuKeyB8, (nuint) 8);
   NCopyLoHi32(&abuReq[15], &luUserID);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                  NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s6.c,v 1.7 1994/09/26 17:40:08 rebekah Exp $
*/
