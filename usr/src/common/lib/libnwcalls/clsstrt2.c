/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clsstrt2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwqms.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwserver.h"
#include "nwconnec.h"

/*manpage*NWCloseFileAndStartQueueJob2**************************************
SYNTAX:  NWCCODE N_API NWCloseFileAndStartQueueJob2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            NWFILE_HANDLE  fileHandle
         )

REMARKS:

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 105  Close File And Start Queue Job
         23 127  Close File And Start Queue Job (3.11 server or later)

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCloseFileAndStartQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWFILE_HANDLE  fileHandle
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWCloseFile(fileHandle)) != 0)
      return ccode;

#ifndef N_PLAT_UNIX
   NWLockConnection(conn);
#endif

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE) NWNCP23s127CloseFileStartQJob(&access, queueID,
         jobNumber));
   }
   else
   {
      return ((NWCCODE) NWNCP23s105CloseFileStartQJob(&access, queueID,
         (nuint16)jobNumber));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clsstrt2.c,v 1.7 1994/09/26 17:44:38 rebekah Exp $
*/
