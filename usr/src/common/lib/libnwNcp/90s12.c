/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s12.c	1.4"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s12SetCompFileSize**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s12SetCompFileSize
         (
            pNWAccess pAccess,
            pnuint8  pbuHandleB6,
            nuint32  luFileSize,
            pnuint32 pluOldFileSize,
            pnuint32 pluNewFileSize
         );

REMARKS:

ARGS: <> pAccess
       > pbuHandleB6
       > luHandle
       > luFileSize
      <  pluOldFileSize (optional)
      <  pluNewFileSize

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.10

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     90 12  Set Compressed File Size

CHANGES: 10 Dec 1993 - written - alim
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s12SetCompFileSize
(
   pNWAccess pAccess,
   pnuint8  pbuHandleB6,
   nuint32  luFileSize,
   pnuint32 pluOldFileSize,
   pnuint32 pluNewFileSize
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define NCP_STRUCT_LEN  ((nuint16) 11)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 8)

   nint32  lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NWCMemMove(&abuReq[3], pbuHandleB6, 6);
   NCopyLoHi32(&abuReq[9], &luFileSize);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);

   if(lCode == 0)
   {
      if(pluOldFileSize)
         NCopyLoHi32(pluOldFileSize, &abuReply[0]);

      NCopyLoHi32(pluNewFileSize, &abuReply[4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s12.c,v 1.6 1994/09/26 17:40:14 rebekah Exp $
*/
