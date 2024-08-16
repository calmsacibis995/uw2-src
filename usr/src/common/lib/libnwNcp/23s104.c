/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s104.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s104CreateQJobAndFile**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s104CreateQJobAndFile
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            pNWNCPQMSJobStruct  pJobStructIn,
            pNWNCPQMSJobStruct  pJobStructOut
         );

REMARKS: This call enters a new job into a queue.  The client must provide both the
         queue ID number to which the job should be appended and the entire 256-byte
         job record.  The job record fields that the client must supply are

         - Target Server ID Number

         - Target Execution Time

         - Job Type

         - Job Control Flags (Operator Hold, User Hold,
         Service Restart, AutoStart)

         - Text Job Description

         - Client Record Area

         The queue management process fills in the job record and returns it (minus
         the Text Job Description and Client Record Area fields) to the calling
         station.  All fields are initialized as outlined in the job field
         descriptions above.  In particular, the queue management process creates a
         file whose name and handle are contained in the Job File Name and Job
         File Handle fields.  The calling station can place information (commands,
         text, etc.) destined for the queue server in this file.

         A client using a DOS workstation can open the NETQ device to attach the
         returned handle to the DOS file created by Create Queue Job And File.

         Any station that is security equivalent to one of the objects listed in the
         target queue's Q_USER property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > pJobStructIn
      <  pJobStructOut (optional)

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

SEE:     23 105  Close File And Start Queue Job

NCP:     23 104  Create Queue Job And File

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s104CreateQJobAndFile
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct  pJobStructIn,
   pNWNCPQMSJobStruct  pJobStructOut
)
{
   #define SIZEOF_NCPQMSJOBSTRUCT 256
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 104)
   #define NCP_STRUCT_LEN  ((nuint) (SIZEOF_NCPQMSJOBSTRUCT + 5))
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) (SIZEOF_NCPQMSJOBSTRUCT - 202))

   nint32  lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
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
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s104.c,v 1.7 1994/09/26 17:34:49 rebekah Exp $
*/
