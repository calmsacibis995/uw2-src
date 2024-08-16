/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s9UserAudited**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s9IsUserAudited
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            nuint32  luUserID,
            pnuint32 pluUserAuditFlag
         );

REMARKS:

ARGS: <> pAccess
       > luVolNum
       > luUserID
      <  pluUserAuditFlag

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 09  Query User Being Audited

CHANGES: 27 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s9IsUserAudited
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint32  luUserID,
   pnuint32 pluUserAuditFlag
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 9)
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 4)

   nint32 lCode;
   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);
   NCopyLoHi32(&abuReq[5], &luUserID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               pluUserAuditFlag, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      *pluUserAuditFlag = NSwapLoHi32(*pluUserAuditFlag);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s9.c,v 1.7 1994/09/26 17:40:11 rebekah Exp $
*/
