/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:28.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP28SyncRelPhyRec*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP28SyncRelPhyRec
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuNWHandleB6,
            nuint32  luLockOffset,
            nuint32  luLockLen,
         )

REMARKS: This call releases a byte range previously locked by the calling client.
         The released byte range will remain in the client's logged data block
         table and will be re-locked by subsequent calls to Lock Physical Record
         Set (function 110).

         Released byte ranges must match locked byte ranges.  Releasing only a portion
         of a previously locked byte range is not allowed.  If the byte range being
         released overlaps any other byte range lock(s) still in effect, all but the
         overlapped bytes will be released.

ARGS: <> pAccess
      >  buReserved
      >  pbuNWHandleB6
      >  luLockOffset
      >  luLockLen

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FF  Unlock Error

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     110 --  Lock Physical Record Set
         26 --  Log Physical Record
         29 --  Release Physical Record Set
         30 --  Clear Physical Record
         31 --  Clear Physical Record Set


NCP:     28 --  Release Physical Record

CHANGES: 2 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP28SyncRelPhyRec
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen
)
{
   #define NCP_FUNCTION    ((nuint) 28)
   #define REQ_LEN         ((nuint) 15)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buReserved;
   NWCMemMove(&abuReq[1], pbuNWHandleB6, (nuint) 6);
   NCopyHiLo32(&abuReq[7], &luLockOffset);
   NCopyHiLo32(&abuReq[11], &luLockLen);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/28.c,v 1.7 1994/09/26 17:37:44 rebekah Exp $
*/
