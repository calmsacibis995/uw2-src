/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrfllok.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"
#include "nwclocal.h"

#ifdef N_PLAT_OS2
#include "nwundoc.h"
#endif

/*manpage*NWClearFileLock2**************************************************
SYNTAX:  NWCCODE N_API NWClearFileLock2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path
         );

REMARKS: Clears a file's lock and removes the file name from the log table.

         In order to avoid deadlock, a workstation is required to request
         those files it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those files.  The locking
         will only work if all files in the table are available.

ARGS:
      >  conn
      >  dirHandle
      >  path

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     07 --  Clear File

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWClearFileLock2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path
)
{
   NWCCODE ccode;
   nstr8   tempPath[260];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, path);
   NWConvertToSpecChar(tempPath);

   ccode = (NWCCODE) NWNCP7SyncClrFile(&access, (nuint8) dirHandle, (nuint8)NWCStrLen(path),
               path);

#ifdef N_PLAT_OS2
   if(ccode)
      return ccode;

   return(NWClearLockSemList(2));
#else
   return ccode;
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrfllok.c,v 1.7 1994/09/26 17:44:30 rebekah Exp $
*/

