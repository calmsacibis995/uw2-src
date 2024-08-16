/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s113.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s113ServiceQJob********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s113ServiceQJob
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suTargetServiceType,
            pNWNCPQMSJobStruct  pJobStructOut
         );

REMARKS: This call allows a queue server client to select a new job for servicing.
         All jobs in the queue are searched to find the one that meets the following
         criteria:

         - The Target Server ID Number must be -1L or match
         the ID number of the calling queue server.

         - The Target Execution Time must be 0xFF or a time
         earlier than the time indicated on the current system clock.

         - The Job Type must match the server's specified Target Service
         Type, or the server's specified Target Service Type must be -1.

         - The Operator Hold, User Hold, and Entry Open flags in the job's
            Job Control Flags must all be reset to zero.

         - The Server ID Number must be zero, indicating that the job is not
            currently being serviced by some other server.

         If a job meets all of the above criteria, the job is marked for servicing
         by the calling station as follows:

         - The serving station ID is entered into the job's Server Station,
         Server Task, and Server ID Number fields.

         - The job file is opened for read and write access by the server.

         - The updated job entry record is delivered to the calling server
         for service.

         Only a station that has previously attached itself to the specified queue as
         a server can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > suTargetServiceType
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

SEE:     23 111  Attach Queue Server To Queue

NCP:     23 113  Service Queue Job

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s113ServiceQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suTargetServiceType,
   pNWNCPQMSJobStruct  pJobStructOut
)
{
   #define SIZEOF_NCPQMSJOBSTRUCT 256
   #define NCP_FUNCTION     ((nuint)   23)
   #define NCP_SUBFUNCTION  ((nuint8) 113)
   #define NCP_STRUCT_LEN   ((nuint)    7)
   #define NCP_REQ_LEN      ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN    ((nuint) (SIZEOF_NCPQMSJOBSTRUCT - 202))

   nint32  lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suTargetServiceType);

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
         NWCMemSet(pJobStructOut->abuJobDescription, 0x00, 50);
         NWCMemSet(pJobStructOut->abuClientRecordArea, 0x00, 152);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s113.c,v 1.7 1994/09/26 17:35:03 rebekah Exp $
*/
