/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:abrtjob2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwqms.h"
#include "nwfile.h"
#include "nwserver.h"

/*manpage*NWAbortServicingQueueJob2*****************************************
SYNTAX:  NWCCODE N_API NWAbortServicingQueueJob2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            NWFILE_HANDLE  fileHandle
         )

REMARKS: This function processes a request to inform the Queue Management
         System (QMS) that it cannot complete servicing a job previously
         started.

ARGS:  > conn
       > queueID
       > jobNumber
       > fileHandle

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 132  Abort Servicing Queue Job
         23 115  Abort Servicing Queue Job

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved.
****************************************************************************/
NWCCODE N_API NWAbortServicingQueueJob2
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

   if ((ccode = NWCloseFile(fileHandle)) != 0)
	return(ccode);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
    return ccode;

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE) NWNCP23s132AbortServicingQJob(&access, queueID,
         jobNumber));
   }
   else
   {
      return ((NWCCODE) NWNCP23s115AbortServicingQJob(&access, queueID,
         (nuint16) jobNumber));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/abrtjob2.c,v 1.7 1994/09/26 17:43:42 rebekah Exp $
*/
