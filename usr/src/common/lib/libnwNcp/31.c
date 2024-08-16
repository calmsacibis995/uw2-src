/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:31.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP31SyncClrPhyRecSet******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP31SyncClrPhyRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlags,
         )

REMARKS: This call clears the client's data byte range table of all byte
         ranges held by the file server for the calling task.  All locked
         ranges are released.

         Unlocks all physical records identified in a log table and removes
         them from the log table.

         The log table resides on the file server and is associated
         exclusively with the requesting task on the workstation.

         NetWare ignores this call if the workstation attempts to unlock and
         clear physical records that are not locked or logged.

ARGS: <> pAccess
      >  buLockFlags

INCLUDE: ncpsync.h

RETURN:  0x0000 Success

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     26 --  Log Physical Record
         27 --  Lock Physical Record Set
         28 --  Release Physical Record
         29 --  Release Physical Record Set
         30 --  Clear Physical Record

NCP:     31 --  Clear Physical Record Set

CHANGES: 27 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP31SyncClrPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlags
)
{
   #define NCP_FUNCTION    ((nuint) 31)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[1];

   abuReq[0] = buLockFlags;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/31.c,v 1.7 1994/09/26 17:37:48 rebekah Exp $
*/
