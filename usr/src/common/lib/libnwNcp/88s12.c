/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:88s12.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP88s12ReadAuditingFiles**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP88s12ReadAuditingFiles
         (
            pNWAccess pAccess,
            nuint32  luVolNum,
            nuint16  suAuditFileNum,
            nuint16  suAmtToRead,
            nuint32  luReadOffset,
            pnuint32 pluAmtRead,
            pnuint8  pbuBufferB512,
         )

REMARKS: There are currently three files that can be read, as shown below:
            0  Audit File
            1  History/Configuration File
            2  Old Audit File

ARGS: <> pAccess
      >  luVolNum
      >  suAuditFileNum
      >  suAmtToRead
      >  luReadOffset
      <  pluAmtRead
      <  pbuBufferB512

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     88 12  Read Auditing Files

CHANGES: 27 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP88s12ReadAuditingFiles
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditFileNum,
   nuint16  suAmtToRead,
   nuint32  luReadOffset,
   pnuint32 pluAmtRead,
   pnuint8  pbuBufferB512
)
{
   #define NCP_FUNCTION    ((nuint) 88)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define REQ_LEN         ((nuint) 13)
   #define REPLY_LEN       ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luVolNum);
   NCopyLoHi16(&abuReq[5], &suAuditFileNum);
   NCopyLoHi16(&abuReq[7], &suAmtToRead);
   NCopyLoHi32(&abuReq[9], &luReadOffset);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuBufferB512;
   replyFrag[1].uLen  = (nuint) 512;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if (lCode ==0)
   {
      NCopyLoHi32(pluAmtRead, &abuReply[0]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/88s12.c,v 1.7 1994/09/26 17:39:50 rebekah Exp $
*/
