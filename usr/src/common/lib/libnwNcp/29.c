/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:29.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP29SyncRelPhyRecSet********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP29SyncRelPhyRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlags,
         )

REMARKS: This NCP releases all byte ranges locked by the calling client.  The
         byte ranges are left in the client's data byte range table for future
         locking with Lock Physical Record Set (function 110).

ARGS: <> pAccess,
      >  buLockFlags,

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     109 --  Log Physical Record
         110 --  Lock Physical Record Set
         28 --  Release Physical Record
         30 --  Clear Physical Record
         31 --  Clear Physical Record Set

NCP:     29 --  Release Physical Record Set

CHANGES: 2 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP29SyncRelPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlags
)
{
   #define NCP_FUNCTION    ((nuint) 29)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buLockFlags;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/29.c,v 1.7 1994/09/26 17:37:45 rebekah Exp $
*/
