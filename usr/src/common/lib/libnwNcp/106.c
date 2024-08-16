/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:106.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
/*#include "ncpsync.h"*/

/*manpage*NWNCP106SyncLockFileSet**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP106SyncLockFileSet
         (
            pNWAccess pAccess,
            nuint16  suLockTimeOut,
         );

REMARKS: This call locks all files logged by the calling client's current task.  If
         all files cannot be locked immediately, the file server will retry the
         call for the amount of time specified in the timeout value (Lock Timeout).
         If all files cannot be locked, no files are locked.


ARGS: <> pAccess
      >  suLockTimeOut

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     3 --  Log File
         4 --  Lock File Set
         5 --  Release File
         6 --  Release File Set
         7 --  Clear File
         8 --  Clear File Set

NCP:     106 --  Lock File Set

CHANGES: 9 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP106SyncLockFileSet
(
   pNWAccess pAccess,
   nuint16  suLockTimeOut
)
{
   #define NCP_FUNCTION    ((nuint) 106)
   #define NCP_REQ_LEN     ((nuint) 2)
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];

   NCopyLoHi16(&abuReq[0], &suLockTimeOut);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/106.c,v 1.7 1994/09/26 17:31:55 rebekah Exp $
*/
