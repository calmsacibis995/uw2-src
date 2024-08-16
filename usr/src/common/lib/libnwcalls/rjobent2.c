/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rjobent2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwintern.h"
#include "nwserver.h"

/*manpage*NWReadQueueJobEntry2**********************************************
SYNTAX:  NWCCODE N_API NWReadQueueJobEntry2
         (
            NWCONN_HANDLE           conn,
            nuint32                 queueID,
            nuint32                 jobNumber,
            NWQueueJobStruct NWPTR  userJobStruct
         )

REMARKS:

ARGS:  > conn
       > queueID
       > jobNumber
      <  userJobStruct

INCLUDE: nwqms.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 122  Read Queue Job Entry
         23 108  Read Queue Job Entry

CHANGES: 15 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWReadQueueJobEntry2
(
   NWCONN_HANDLE           conn,
   nuint32                 queueID,
   nuint32                 jobNumber,
   NWQueueJobStruct NWPTR  userJobStruct
)
{
   NWCCODE ccode;
   nuint16 serverVer;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      NWNCPQMSJobStruct2 localJobStruct;

      if ((ccode = (NWCCODE) NWNCP23s122ReadQJobEntry(&access, queueID,
         jobNumber, &localJobStruct)) == 0)
      {
         /* the first three fields of the NCP structure are not returned
            by NWCalls; other than that, the structures are identical */

         NWCMemMove(userJobStruct, &localJobStruct.luClientStation,
            sizeof(NWQueueJobStruct));

         __NWSwapNWJobStructIDs(userJobStruct);
      }
   }
   else
   {
      QueueJobStruct tempJob;

      if ((ccode = (NWCCODE) NWNCP23s108ReadQJobEntry(&access, queueID,
         (nuint16) jobNumber, (pNWNCPQMSJobStruct) &tempJob)) == 0)
      {
         __NWSwapJobStructIDs(&tempJob);

         ConvertQueueToNWQueue(userJobStruct, &tempJob);
      }
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rjobent2.c,v 1.7 1994/09/26 17:49:12 rebekah Exp $
*/
