/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:relfllok.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#define  FILE_LOCKS_ONLY         /* only use the locks part of nwfile.h*/
#include "nwfile.h"
#include "nwclocal.h"

#ifdef N_PLAT_OS2
#include "nwundoc.h"
#endif

/*manpage*NWReleaseFileLock2************************************************
SYNTAX:  NWCCODE N_API NWReleaseFileLock2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path
         )

REMARKS: Unlocks the specified file in the log table of the requesting
         workstation, but does not remove the file from the table.

         This call allows the application to release files for use by other
         workstations without forcing the workstation to log the particular
         the next time the file is needed.

ARGS: >  conn
      >  dirHandle
      >  path

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     05 --  Release File

CHANGES: 21 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReleaseFileLock2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path
)
{
   nstr8   tempPath[260];
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, path);
   NWConvertToSpecChar(tempPath);

   ccode = (NWCCODE) NWNCP5SyncRelFile(&access, (nuint8) dirHandle,
               (nuint8) NWCStrLen(path), path);

#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);
   return (NWClearLockSemList(2));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/relfllok.c,v 1.7 1994/09/26 17:48:55 rebekah Exp $
*/

