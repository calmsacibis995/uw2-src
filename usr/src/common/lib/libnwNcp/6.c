/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP6SyncReleaseFileSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
         );

REMARKS: This call releases all the client's currently logged or locked files for use
         by other clients.  File handles for any files that are open become invalid.
         The client will not be able to perform I/O to the files until they are again
         locked using the Lock File Set (function 4).  The files remain in the client's
         logged file table.

ARGS: <> pAccess
      >  buLockFlag

INCLUDE: ncpsync.h

RETURN:  0x0000   Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     03 --  Log File
         04 --  Lock File Set
         05 --  Release File
         07 --  Clear File
         08 --  Clear File Set

NCP:     06 --  Release File Set

CHANGES: 30 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP6SyncReleaseFileSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
)
{
   #define NCP_FUNCTION    ((nuint) 6)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buLockFlag;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/6.c,v 1.7 1994/09/26 17:38:40 rebekah Exp $
*/
