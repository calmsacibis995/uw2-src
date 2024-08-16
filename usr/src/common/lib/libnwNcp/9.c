/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP9SyncLogLogRec***********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP9SyncLogLogRec
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
            nuint16  suLockTimeout,
            nuint8   buRecNameLen,
            pnstr8   pbstrRecName
         );

REMARKS: This call logs a synchronization string for use by the calling client.
         Synchronization strings may be up to 128 characters long.  If bit 0 of
         the Lock flag is set, the server will immediately attempt to lock the string.
         The Lock Timeout value determines how long the server will try to lock the
         string before reporting failure.

ARGS:
      <> pAccess
       > buLockFlag
       > suLockTimeout
       > buRecNameLen
       > pbstrRecName

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     10 --  Lock Logical Record Set
         11 --  Clear Logical Record
         14 --  Clear Logical Record Set
         12 --  Release Logical Record
         13 --  Release Logical Record Set

NCP:     09 --  Log Logical Record

CHANGES: 1 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP9SyncLogLogRec
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buRecNameLen,
   pnstr8   pbstrRecName
)
{
   #define NCP_FUNCTION    ((nuint) 9)
   #define REQ_LEN         ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = buLockFlag;
   NCopyHiLo16(&abuReq[1], &suLockTimeout);
   abuReq[3] = buRecNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrRecName;
   reqFrag[1].uLen  = buRecNameLen;

   return NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/9.c,v 1.7 1994/09/26 17:40:12 rebekah Exp $
*/
