/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP7SyncClrFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP7SyncClrFile
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
         );

REMARKS: This call clears one file from the calling client's logged file table.  If
         the specified file is open, it is closed, and its file handle is invalidated.
         The file can then be accessed by other clients.  If the calling client had
         the file open multiple times, then it is closed as many times and all
         associated file handles are invalidated.

ARGS: <> pAccess
      >  buDirHandle
      >  buFileNameLen
      >  pbstrFileName

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Unlock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     3 --  Log File
         4 --  Lock File Set
         5 --  Release File
         6 --  Release File Set
         8 --  Clear File Set

NCP:     07 --  Clear File

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP7SyncClrFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 7)
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
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/7.c,v 1.7 1994/09/26 17:38:51 rebekah Exp $
*/
