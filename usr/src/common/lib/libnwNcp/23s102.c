/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s102.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s102ReadQCurrStatus****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s102ReadQCurrStatus
         (
            pNWAccess pAccess,
            nuint32  luInQueueID,
            pnuint32 pluOutQueueID,
            pnuint8  pbuQueueStatus,
            pnuint8  pbuCurrentEntries,
            pnuint8  pbuCurrentServers,
            pnuint32 pluServerIDList,
            pnuint8  pbuServerStationList
         );

REMARKS: This call returns the current status of a queue.  The Queue Status byte
         indicates the overall status of the queue.  Bits in the Queue Status byte
         are set as follows:

         - Bit 0x01 is set if the operator does not want to
         add jobs to the queue.

         - Bit 0x02 is set if the operator does not want
         additional servers attaching to the queue.

         - Bit 0x04 is set if the operator does not want
         servers to service jobs in the queue.

         Current Entries contains a count of the jobs currently in the queue
         (0 to 250).

         Current Servers contains the number of currently attached servers that can
         service this queue (0 to 25).

         The Server ID list and Server Station list identify the servers currently
         servicing the queue by Object ID Number and current station attachment.

         Any station security equivalent to any object listed in the queue's Q_USERS
         group property or Q_OPERATORS group property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luInQueueID
      <  pluOutQueueID (optional)
      <  pbuQueueStatus (optional)
      <  pbuCurrentEntries (optional)
      <  pbuCurrentServers (optional)
      <  pluServerIDList (optional)
      <  pbuServerStationList (optional)

INCLUDE: ncpqms.h

RETURN:  0x0000  Successful
         0x8999  Directory Full Error
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D2  No Queue Server
         0x89D3  No Queue Rights
         0x89D4  Queue Full
         0x89D5  No Queue Job
         0x89D6  No Job Right
         0x89D7  Queue Servicing
         0x89D8  Queue Not Active
         0x89D9  Station Not Server
         0x89DA  Queue Halted
         0x89DB  Maximum Queue Servers
         0x89FF  Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 120  Get Queue Job File Size
         23 107  Get Queue Job List
         23 108  Read Queue Job Entry
         23 106  Remove Job From Queue

NCP:     23 102  Read Queue Current Status

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s102ReadQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   pnuint32 pluOutQueueID,
   pnuint8  pbuQueueStatus,
   pnuint8  pbuCurrentEntries,
   pnuint8  pbuCurrentServers,
   pnuint32 pluServerIDList,
   pnuint8  pbuServerStationList
)
{
   #define NCP_FUNCTION     ((nuint)   23)
   #define NCP_SUBFUNCTION  ((nuint8) 102)
   #define NCP_STRUCT_LEN   ((nuint)    5)
   #define NCP_REQ_LEN      ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN      ((nuint)  132)

   nint32  lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luInQueueID);

   lCode = NWCRequestSingle(pAccess, (nuint) NCP_FUNCTION, abuReq,
                  NCP_REQ_LEN, abuReply, NCP_REP_LEN, NULL);

   if (lCode == 0)
   {
      nint   i, iOfs;
      nuint8 buTempCurrServers;

      if (pluOutQueueID)
         NCopyHiLo32(pluOutQueueID, &abuReply[0]);
      if (pbuQueueStatus)
         *pbuQueueStatus = abuReply[4];
      if (pbuCurrentEntries)
         *pbuCurrentEntries = abuReply[5];

      buTempCurrServers = abuReply[6];
      if (pbuCurrentServers)
         *pbuCurrentServers = buTempCurrServers;

      if (pluServerIDList)
      {
         for (i = 0, iOfs = 7; i < (nint) buTempCurrServers; i++, iOfs += 4)
            NCopyHiLo32(&pluServerIDList[i], &abuReply[iOfs]);
      }

      if (pbuServerStationList)
      {
         iOfs = 7 + 4*buTempCurrServers;
         for (i = 0; i < (nint) buTempCurrServers; i++, iOfs++)
            pbuServerStationList[i] = abuReply[iOfs];
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s102.c,v 1.7 1994/09/26 17:34:45 rebekah Exp $
*/
