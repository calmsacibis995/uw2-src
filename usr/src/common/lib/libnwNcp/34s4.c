/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s4TTSTransStatus********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s4TTSTransStatus
         (
            pNWAccess pAccess,
            nuint32  luTransNum,
         );

REMARKS: Verifies whether a transaction has been written to disk.

         Because of the file server caching algorithms, 3 to 5 seconds (or
         longer) may elapse before transactions are actually written.
         Transactions are written to disk in the order in which they are
         ended.

ARGS: <> pAccess

      >  luTransNum
         luTransNum is obtained through NCP 34 02 (TTS End Transaction).


INCLUDE: ncptts.h

RETURN:  0x00  Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 02  TTS End Transaction

NCP:     34 04  TTS Transaction Status

CHANGES: 12 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s4TTSTransStatus
(
   pNWAccess pAccess,
   nuint32  luTransNum
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luTransNum);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s4.c,v 1.7 1994/09/26 17:38:03 rebekah Exp $
*/
