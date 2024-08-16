/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP3SyncLogFile*************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP3SyncLogFile
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buLockFlag,
            nuint16  suLockTimeout,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
         );

REMARKS:

ARGS: <> pAccess
      >  buDirHandle
      >  buLockFlag
      >  suLockTimeout
      >  buFileNameLen
      >  pbstrFileName

INCLUDE: ncpsync.h

RETURN:  This call logs the specified file for exclusive use by the calling
         client.  If bit 0 of the Lock flag is set, the server will
         immediately attempt to lock the file.

         A file may be locked by a client even if the file does not yet
         exist.  This reserves the file name for use by the client that
         locked it.

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     04 --  Lock File Set
         05 --  Release File
         06 --  Release File Set
         07 --  Clear File
         08 --  Clear File Set
         105 --  Log File

NCP:     03 --  Log File

CHANGES: 3 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP3SyncLogFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 3)
   #define REQ_LEN         ((nuint) 5)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = buDirHandle;
   abuReq[1] = buLockFlag;
   NCopyHiLo16(&abuReq[2], &suLockTimeout);
   abuReq[4] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag, REPLY_FRAGS,
               NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/3.c,v 1.7 1994/09/26 17:37:46 rebekah Exp $
*/
