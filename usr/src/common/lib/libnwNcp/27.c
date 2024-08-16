/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:27.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP27SyncLockPhyRecSet*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP27SyncLockPhyRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
            nuint16  suLockTimeout,
         )

REMARKS: This call locks all byte ranges currently logged by the calling client.  If
         all byte ranges are not immediately available, the server will retry the
         call until the amount of time specified in the Lock Timeout value expires.

         If bit 1 of the Lock flag is set, the lock types are shareable (read-only);
         otherwise, the lock types are exclusive (read-write).

ARGS: <> pAccess,
      >  buLockFlag,
      >  suLockTimeout,

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     26 --  Log Physical Record
         28 --  Release Physical Record
         29 --  Release Physical Record Set
         30 --  Clear Physical Record
         31 --  Clear Physical Record Set

NCP:     27 --  Lock Physical Record Set

CHANGES: 2 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP27SyncLockPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
)
{
   #define NCP_FUNCTION    ((nuint) 27)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0]  = buLockFlag;
   NCopyLoHi16(&abuReq[1], &suLockTimeout);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/27.c,v 1.7 1994/09/26 17:37:42 rebekah Exp $
*/
