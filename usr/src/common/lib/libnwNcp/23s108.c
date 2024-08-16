/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s108.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s108ReadQJobEntry******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s108ReadQJobEntry
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suJobNumber,
            pNWNCPQMSJobStruct pJobStructOut
         );

REMARKS: This call returns the full 256-byte queue job record, given the Queue ID and
         the Job Number.  All fields are filled in.

         Any station security equivalent to an object listed in the queue's Q_USERS
         group property or Q_OPERATORS group property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > suJobNumber
      <  pJobStructOut

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

SEE:     23 102  Read Queue Current Status
         23 120  Get Queue Job File Size
         23 107  Get Queue Job List
         23 106  Remove Job From Queue

NCP:     23 108  Read Queue Job Entry

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s108ReadQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber,
   pNWNCPQMSJobStruct pJobStructOut
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 108)
   #define NCP_STRUCT_LEN  ((nuint)    7)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)  256)

   nint32  lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suJobNumber);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      abuReply, NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      if (pJobStructOut)
      {
         pJobStructOut->buClientStation = abuReply[0];
         pJobStructOut->buClientTask = abuReply[1];
         NCopyHiLo32(&pJobStructOut->luClientID, &abuReply[2]);
         NCopyHiLo32(&pJobStructOut->luTargetServerID, &abuReply[6]);
         NWCMemMove(pJobStructOut->abuTargetExecutionTime, &abuReply[10], 6);
         NWCMemMove(pJobStructOut->abuJobEntryTime, &abuReply[16], 6);
         NCopyHiLo16(&pJobStructOut->suJobNumber, &abuReply[22]);
         NCopyHiLo16(&pJobStructOut->suJobType, &abuReply[24]);
         pJobStructOut->buJobPosition = abuReply[26];
         pJobStructOut->buJobControlFlags = abuReply[27];
         NWCMemMove(pJobStructOut->abuJobFileName, &abuReply[28], 14);
         NWCMemMove(pJobStructOut->abuJobFileHandle, &abuReply[42], 6);
         pJobStructOut->buServicingServerStation = abuReply[48];
         pJobStructOut->buServicingServerTask = abuReply[49];
         NCopyHiLo32(&pJobStructOut->luServicingServerID, &abuReply[50]);
         NWCMemMove(pJobStructOut->abuJobDescription, &abuReply[54], 50);
         NWCMemMove(pJobStructOut->abuClientRecordArea, &abuReply[104], 152);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s108.c,v 1.7 1994/09/26 17:34:56 rebekah Exp $
*/
