/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s2TTSEndTrans*******************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s2TTSEndTrans
         (
            pNWAccess pAccess,
            pnuint32 pluTransNum,
         );

REMARKS: Ends an explicit transaction and returns a Transaction Number.  A
         transaction is not necessarily completed to disk when this
         function returns.  The returned Transaction Number can be used by
         TTS Transaction Status (function 34, subfunction 4) to verify a
         successful transaction completion to disk.

ARGS: <> pAccess
      <  pluTransNum

INCLUDE: ncptts.h

RETURN:  0x8900  Successful
         0x89FF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 03  TTS Abort Transaction
         34 01  TTS Begin Transaction
         34 04  TTS Transaction Status

NCP:     34 02  TTS End Transaction

CHANGES: 12 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s2TTSEndTrans
(
   pNWAccess pAccess,
   pnuint32 pluTransNum
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 5)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess,
                            NCP_FUNCTION,
                            abuReq,
                            REQ_LEN,
                            pluTransNum,
                            REPLY_LEN,
                            NULL);

   if(lCode == 0)
   {
     *pluTransNum = NSwapLoHi32(*pluTransNum);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s2.c,v 1.7 1994/09/26 17:38:01 rebekah Exp $
*/
