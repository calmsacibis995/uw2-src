/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:12.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP12SyncRelLogRec**************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP12SyncRelLogRec
         (
            pNWAccess pAccess,
            nuint8   buRecNameLen,
            pnuint8  pbuRecName,
         )

REMARKS: Releases a lock on a logical record at the
         specified file server, but will not remove it from the log table.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

ARGS:

INCLUDE: ncpsync.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     12 --  Release Logical Record

CHANGES:  30 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP12SyncRelLogRec
(
   pNWAccess pAccess,
   nuint8   buRecNameLen,
   pnstr8  pbuRecName
)
{
   #define NCP_FUNCTION    ((nuint) 12)
   #define REQ_LEN         ((nuint) 1)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];

   reqFrag[0].pAddr = &buRecNameLen;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuRecName;
   reqFrag[1].uLen  = (nuint) buRecNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/12.c,v 1.7 1994/09/26 17:32:02 rebekah Exp $
*/
