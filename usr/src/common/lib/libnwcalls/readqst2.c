/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:readqst2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwserver.h"

/*manpage*NWReadQueueCurrentStatus2*****************************************
SYNTAX:  NWCCODE N_API NWReadQueueCurrentStatus2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            pnuint32       queueStatus,
            pnuint32       numberOfJobs,
            pnuint32       numberOfServers,
            pnuint32       serverIDList,
            pnuint32       serverConnList
         )

REMARKS: This call read the current status of the queue.  The queue status
         byte indicates the current status of the queue as per the following
         bits:

         Bit 1 - set if the operator dosen't want to add jobs to hte queue.

         Bit 2 - set if the operator dosen't want additional servers
                 attaching to the queue.

         Bit 4 - set if the operator dosen't want servers to service jobs in
                 the queue.

         The 'numJobs' argument contains the number of jobs currently on the
         queue.  The 'numServers' argument contains the number of currently
         active servers that can service this queue.  The 'serverIDList' and
         the 'serverConnList' identify the servers currently servicing the
         queue of ObjectID and current station attachment.

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

NCP:     23 125  Read Queue Current Status
         23 102  Read Queue Current Status

CHANGES: 25 Jun 1993 - bug fix - jwoodbur
            ServerConnList was being written out with wrong starting
             offset resulting in possible stack overrun.
            PNW version was not being checked correctly on serverConnList
             write
         15 Sep 1993 - redone - djharris
         02 Dec 1993 - fixed swapping of server ID's - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReadQueueCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   pnuint32       queueStatus,
   pnuint32       numberOfJobs,
   pnuint32       numberOfServers,
   pnuint32       serverIDList,
   pnuint32       serverConnList
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint32 numServers;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      if ((ccode = (NWCCODE) NWNCP23s125ReadQCurrStatus(&access, queueID,
         NULL, queueStatus, numberOfJobs, &numServers,
         serverIDList, serverConnList)) == 0)
      {
         if (numberOfServers)
            *numberOfServers = numServers;
      }
   }
   else
   {
      nuint8  abuTempServerConnList[25];
      nuint8  buTempQueueStatus, buTempNumberOfJobs, buTempNumberOfServers;

      if ((ccode = (NWCCODE) NWNCP23s102ReadQCurrStatus(&access, queueID,
         NULL, &buTempQueueStatus, &buTempNumberOfJobs, &buTempNumberOfServers,
         serverIDList, abuTempServerConnList)) == 0)
      {
         nint i;

         numServers = (nuint32) buTempNumberOfServers;

         if (queueStatus)
            *queueStatus = (nuint32) buTempQueueStatus;
         if (numberOfJobs)
            *numberOfJobs = (nuint32) buTempNumberOfJobs;
         if (numberOfServers)
            *numberOfServers = (nuint32) buTempNumberOfServers;

         if(serverConnList)
         {
            for(i = 0; i < (nint) buTempNumberOfServers; i++)
               serverConnList[i] = (nuint32) abuTempServerConnList[i];
         }
      }
   }

   if (serverIDList && ccode == 0)
   {
      nint i;

      /* NWCalls is broken and expects the server ID's to be anti-native */
      for (i = 0; i < (nint) numServers; i++)
         serverIDList[i] = NSwap32(serverIDList[i]);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/readqst2.c,v 1.7 1994/09/26 17:48:50 rebekah Exp $
*/
