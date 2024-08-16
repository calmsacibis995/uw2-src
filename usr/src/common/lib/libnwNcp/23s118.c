/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s118.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s118ReadQServerCurrStat************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s118ReadQServerCurrStat
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint32  luServerID,
            nuint8   buServerStation,
            pnuint8  pbuServerStatusRecordB64
         );

REMARKS: This call returns the current status of a queue server.  The queue management
         process (in the file server) maintains a 64-nuint8 status record for each queue
         server attached to a queue.  The information in the Server Status Record can
         be anything of use to queue servers and queue clients; this record is not
         used or interpreted by the queue manager.

         The first 4 bytes of Server Status Record is the estimated price for the
         given queue server to complete an average job.

         Any station security equivalent to one of the objects listed in the queue's
         Q_USERS group property or Q_OPERATORS group property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > luServerID
       > buServerStation
      <  pbuServerStatusRecordB64

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

SEE:     23 119  Set Queue Server Current Status

NCP:     23 118  Read Queue Server Current Status

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s118ReadQServerCurrStat
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luServerID,
   nuint8   buServerStation,
   pnuint8  pbuServerStatusRecordB64
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 118)
   #define NCP_STRUCT_LEN  ((nuint)   10)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)   64)

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo32(&abuReq[7], &luServerID);
   abuReq[11] = buServerStation;

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      pbuServerStatusRecordB64, NCP_REPLY_LEN, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s118.c,v 1.7 1994/09/26 17:35:10 rebekah Exp $
*/
