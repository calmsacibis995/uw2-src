/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s129.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s129DMFileInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s129DMFileInfo
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            nuint32  luDirBase,
            nuint32  luNamSpc,
            pnuint32 pluModuleID,
            pnuint32 pluRestoreTime,
            pnuint32 pluDataStreams
         );

REMARKS:

ARGS: <> pAccess
       > luVolNum
       > luDirBase
       > luNamSpc
      <  pluModuleID (optional)
      <  pluRestoreTime (optional)
      <  pluDataStreams (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     90 129  DM File Information

CHANGES: 5 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s129DMFileInfo
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint32  luDirBase,
   nuint32  luNamSpc,
   pnuint32 pluModuleID,
   pnuint32 pluRestoreTime,
   pnuint32 pluDataStreams
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 129)
   #define NCP_STRUCT_LEN  ((nuint16) 13)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 12)

   nint32  lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luVolNum);
   NCopyLoHi32(&abuReq[7], &luDirBase);
   NCopyLoHi32(&abuReq[11], &luNamSpc);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      if (pluModuleID)
         NCopyLoHi32(pluModuleID, &abuReply[0]);

      if (pluRestoreTime)
         NCopyLoHi32(pluRestoreTime, &abuReply[4]);

      if (pluDataStreams)
         NCopyLoHi32(pluDataStreams, &abuReply[8]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s129.c,v 1.7 1994/09/26 17:40:16 rebekah Exp $
*/
