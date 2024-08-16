/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s216.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP104s216GetAuditFlags**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP104s216GetAuditFlags
         (
            pNWAccess pAccess,
            nuint32  luContID,
            pnuint8  pbuAuditFlags,
         )

REMARKS:

ARGS: <> pAccess
      >  luContID
      <  pbuAuditFlags

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     104 216  Query Audit Flags

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP104s216GetAuditFlags
(
   pNWAccess pAccess,
   nuint32  luContID,
   pnuint8  pbuAuditFlags
)
{
   #define NCP_FUNCTION    ((nuint) 104)
   #define NCP_SUBFUNCTION ((nuint8) 216)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luContID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      *pbuAuditFlags = abuReply[0];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s216.c,v 1.7 1994/09/26 17:31:46 rebekah Exp $
*/
