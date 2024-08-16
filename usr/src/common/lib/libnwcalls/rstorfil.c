/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rstorfil.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwclocal.h"
#include "nwdel.h"

/*manpage*NWRestoreErasedFile***********************************************
SYNTAX:  NWCCODE N_API NWRestoreErasedFile
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath,
            pnstr8         oldName,
            pnstr8         newName
         );

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  dirPath
      <  oldName (optional)
      <  newName (optional)

INCLUDE: nwdel.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 17  Recover Erased File

CHANGES: 17 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRestoreErasedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   pnstr8         oldName,
   pnstr8         newName
)
{
   nstr8  tempPath[260];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, dirPath);
   NWConvertToSpecChar(tempPath);

   return ((NWCCODE) NWNCP22s17DelRecover(&access, (nuint8) dirHandle,
               (nuint8) NWCStrLen(tempPath), tempPath,
               oldName, newName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rstorfil.c,v 1.7 1994/09/26 17:49:14 rebekah Exp $
*/
