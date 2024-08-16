/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:13.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP13SyncRelLogRecSet***********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP13SyncRelLogRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
         )

REMARKS: This call releases all the client's locked synchronization strings for use
         by other clients.  The strings remain in the calling client's synchronization
         string table and will be re-locked when the client calls Lock Logical Record
         Set (function 10).

ARGS:<>  pAccess
      >  buLockFlag

INCLUDE: ncpsync.h

RETURN:

SERVER:

CLIENT:

SEE:

NCP:     13 Release Logical Record Set

CHANGES: 30 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP13SyncRelLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
)
{
   #define NCP_FUNCTION    ((nuint) 13)
   #define REQ_LEN         ((nuint) 1)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];

   reqFrag[0].pAddr = &buLockFlag;
   reqFrag[0].uLen  = REQ_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/13.c,v 1.7 1994/09/26 17:33:06 rebekah Exp $
*/
