/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s109.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s109ChangeQJobEntry****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s109ChangeQJobEntry
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            pNWNCPQMSJobStruct pJobStructIn
         );

REMARKS: This call allows a client to change information for a queue job entry.

         The client selects the queue entry to be changed by designating the job
         number in the Job Number field of the Job Structure in the request message.
         The client can replace the information in the following fields:

         - Target Server ID Number

         - Target Execution Time

         - Job Type

         - User Hold Flag

         - Service Restart Flag

         - Service Auto-Start Flag

         - Text Job Description

         - Client Record Area

         - Operator Hold Flag
            (If the calling client is an operator, this field can also be reset.)

         This call can be used with Read a Job Queue's Entry to change a portion of
         the job's record.  However, if the target entry is already being serviced,
         a Queue Servicing error is returned and no changes are made to the job's
         record.

         See Introduction to Queue NCPs for information on the old Job Structure.

         The client that created the job or an operator can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > pJobStructIn

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

SEE:     23 110  Change Queue Job Position
         23 107  Get Queue Job List
         23 120  Get Queue Job File Size

NCP:     23 109  Change Queue Job Entry

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s109ChangeQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct pJobStructIn
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 109)
   #define NCP_STRUCT_LEN  ((nuint)  256+5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);

   abuReq[7] = pJobStructIn->buClientStation;
   abuReq[8] = pJobStructIn->buClientTask;
   NCopyHiLo32(&abuReq[9], &pJobStructIn->luClientID);
   NCopyHiLo32(&abuReq[13], &pJobStructIn->luTargetServerID);
   NWCMemMove(&abuReq[17], pJobStructIn->abuTargetExecutionTime, 6);
   NWCMemMove(&abuReq[23], pJobStructIn->abuJobEntryTime, 6);
   NCopyHiLo16(&abuReq[29], &pJobStructIn->suJobNumber);
   NCopyHiLo16(&abuReq[31], &pJobStructIn->suJobType);
   abuReq[33] = pJobStructIn->buJobPosition;
   abuReq[34] = pJobStructIn->buJobControlFlags;
   NWCMemMove(&abuReq[35], pJobStructIn->abuJobFileName, 14);
   NWCMemMove(&abuReq[49], pJobStructIn->abuJobFileHandle, 6);
   abuReq[55] = pJobStructIn->buServicingServerStation;
   abuReq[56] = pJobStructIn->buServicingServerTask;
   NCopyHiLo32(&abuReq[57], &pJobStructIn->luServicingServerID);
   NWCMemMove(&abuReq[61], pJobStructIn->abuJobDescription, 50);
   NWCMemMove(&abuReq[111], pJobStructIn->abuClientRecordArea, 152);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s109.c,v 1.7 1994/09/26 17:34:57 rebekah Exp $
*/
