/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:readqsta.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*NWReadQueueCurrentStatus******************************************
SYNTAX:  NWCCODE N_API NWReadQueueCurrentStatus
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            pnuint8        queueStatus,
            pnuint16       numberOfJobs,
            pnuint16       numberOfServers,
            pnuint32       serverIDList,
            pnuint16       serverConnList
         )

REMARKS: Reads the current status of the queue.  The queue status
         byte indicates the current status of the queue as per the
         following bits:

         Bit 0x01 - set if the operator dosen't want to add jobs to the
                  queue.

         Bit 0x02 - set if the operator dosen't want additional servers
                  attaching to the queue.

         Bit 0x04 - set if the operator dosen't want servers to service jobs
                  in the queue.

         The 'numberOfJobs' variable contains the number of jobs currently
         on the queue.  The 'numberOfServers' variable contains the number
         of currently active servers that can service this queue.  The
         'serverIDList' and the 'serverConnList' identify the servers
         currently servicing the queue of ObjectID and currentstation
         attachment.

ARGS:  > conn
       > queueID
      <  queueStatus (optional)
      <  numberOfJobs (optional)
      <  numberOfServers (optional)
      <  serverIDList (optional)
      <  serverConnList (optional)

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 102  Read Queue Current Status

CHANGES: 15 Sep 1993 - NWNCP Enabled - djharris
         02 Dec 1993 - fixed swapping of server ID's - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReadQueueCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   pnuint8        queueStatus,
   pnuint16       numberOfJobs,
   pnuint16       numberOfServers,
   pnuint32       serverIDList,
   pnuint16       serverConnList
)
{
   NWCCODE ccode;
   nuint8  tempServerConnList[25];
   nuint8  tmpNumJobs, tmpNumServs;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if ((ccode = (NWCCODE) NWNCP23s102ReadQCurrStatus(&access, queueID,
      NULL, queueStatus, &tmpNumJobs, &tmpNumServs, serverIDList,
      tempServerConnList)) == 0)
   {
      nuint i;

      if (numberOfJobs)
         *numberOfJobs    = tmpNumJobs;
      if (numberOfServers)
         *numberOfServers = tmpNumServs;

      if(serverConnList)
      {
         for(i = 0; i < (nuint) tmpNumServs; i++)
            serverConnList[i] = (nuint16)tempServerConnList[i];
      }

      /* NWCalls is broken and expects the server ID's to be anti-native */
      if (serverIDList)
      {
         for (i = 0; i < (nuint) tmpNumServs; i++)
            serverIDList[i] = NSwap32(serverIDList[i]);
      }
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/readqsta.c,v 1.7 1994/09/26 17:48:52 rebekah Exp $
*/
