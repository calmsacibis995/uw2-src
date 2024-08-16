/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chjobpo2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwserver.h"

/*manpage*NWChangeQueueJobPosition2*****************************************
SYNTAX:  NWCCODE N_API NWChangeQueueJobPosition2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            nuint32        newJobPosition
         )

REMARKS:

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 110  Change Queue Job Position
         23 130  Change Queue Job Priority (3.11 or later)

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWChangeQueueJobPosition2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   nuint32        newJobPosition
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {

      /*jobNumber = NSwap32(jobNumber); */

      return ((NWCCODE)NWNCP23s130ChangeJobPriority(&access, queueID,
         jobNumber, newJobPosition));
   }
   else
   {
      return ((NWCCODE)NWNCP23s110ChangeQJobPosition(&access, queueID,
         (nuint16)jobNumber, (nuint8)newJobPosition));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chjobpo2.c,v 1.7 1994/09/26 17:44:18 rebekah Exp $
*/
