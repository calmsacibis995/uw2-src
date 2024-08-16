/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:108.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP108SyncLockLogRecSet********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP108SyncLockLogRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
            nuint16  suLockTimeout,
         )

REMARKS: This call locks all the client's logged synchronization strings.  If all
         files cannot be locked immediately, the file server will retry the
         call for the amount of time specified in the timeout value (Lock Timeout).
         If all records cannot be locked, the call will fail and Lock Error will be
         returned as the Completion Code.  In this case, none of the records in the
         set are locked by the calling station.

ARGS: <> pAccess
      >  buLockFlag,
      >  suiLockTimeOut

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     9 --  Log Logical Record
         11 --  Clear Logical Record
         14 --  Clear Logical Record Set
         12 --  Release Logical Record
         13 --  Release Logical Record Set
         10 --  Lock Logical Record Set (old)

NCP:     108 --  Lock Logical Record Set

CHANGES: 3 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP108SyncLockLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
)
{
   #define NCP_FUNCTION    ((nuint) 108)
   #define NCP_REQ_LEN     ((nuint) 3)
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];

   abuReq[0] = buLockFlag;
   NCopyLoHi16(&abuReq[1], &suLockTimeout);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/108.c,v 1.7 1994/09/26 17:31:56 rebekah Exp $
*/
