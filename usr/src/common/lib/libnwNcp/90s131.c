/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s131.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s131MigratStatusInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s131MigratStatusInfo
         (
            pNWAccess pAccess
            pnuint32 pluDMPresent,
            pnuint32 pluDMMajorVer,
            pnuint32 pluDMMinorVer,
            pnuint32 pluDMSMRegistered,
         )

REMARKS:

ARGS: <> pAccess
      <  pluDMPresent (optional)
      <  pluDMMajorVer (optional)
      <  pluDMMinorVer (optional)
      <  pluDMSMRegistered (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     90 131  Migrator Status Info

CHANGES: 5 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s131MigratStatusInfo
(
   pNWAccess pAccess,
   pnuint32 pluDMPresent,
   pnuint32 pluDMMajorVer,
   pnuint32 pluDMMinorVer,
   pnuint32 pluDMSMRegistered
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 131)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 16)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if (pluDMPresent)
         NCopyLoHi32(pluDMPresent, &abuReply[0]);
      if (pluDMMajorVer)
         NCopyLoHi32(pluDMMajorVer, &abuReply[4]);
      if (pluDMMinorVer)
         NCopyLoHi32(pluDMMinorVer, &abuReply[8]);
      if (pluDMSMRegistered)
         NCopyLoHi32(pluDMSMRegistered, &abuReply[12]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s131.c,v 1.7 1994/09/26 17:40:19 rebekah Exp $
*/
