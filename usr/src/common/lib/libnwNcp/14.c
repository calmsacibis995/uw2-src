/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:14.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP14SyncClrLogRecSet*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP14SyncClrLogRecSet
         (
            pNWAccess pAccess,
            nuint8   buLockFlag,
         );

REMARKS: This call releases all of the client's synchronization strings
         (logical record locks) and clears the client's synchronization string table.

ARGS:
      <> pAccess
      >  buLockFlag

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     09 --  Log Logical Record
         10 --  Lock Logical Record Set
         11 --  Clear Logical Record
         12 --  Release Logical Record
         13 --  Release Logical Record Set

NCP:     14 --  Clear Logical Record Set

CHANGES: 30 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP14SyncClrLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
)
{
   #define NCP_FUNCTION    ((nuint) 14)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buLockFlag;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                            NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/14.c,v 1.7 1994/09/26 17:33:07 rebekah Exp $
*/
