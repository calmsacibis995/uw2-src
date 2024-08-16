/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP5SyncRelFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP5SyncRelFile
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
         );

REMARKS: This call releases the specified file, previously locked by the calling
         client.  Releasing a file allows it to be used by others clients.  If the
         file is open, the file handle becomes invalid.  The file remains in the
         calling client's logged file table and can be locked by future calls to
         Lock File Set (function 4).  The client cannot perform I/O to the file until
         the file is again locked by the client.

ARGS: <> pAccess
      >  buDirHandle
      >  buFileNameLen
      >  pbstrFileName

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x899C  Invalid Path
         0x89FF  Unlock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     3 --  Log File
         4 --  Lock File Set
         6 --  Release File Set
         7 --  Clear File
         8 --  Clear File Set

NCP:     05 --  Release File

CHANGES: 21 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP5SyncRelFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 5)
   #define REQ_LEN         ((nuint) 2)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = buDirHandle;
   abuReq[1] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/5.c,v 1.7 1994/09/26 17:38:39 rebekah Exp $
*/
