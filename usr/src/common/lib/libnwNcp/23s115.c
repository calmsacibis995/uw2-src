/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s115.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s115AbortServicingQJob*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s115AbortServicingQJob
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suJobNumber
         );

REMARKS: This call allows a server to inform the queue manager that it cannot complete
         servicing a job previously accepted for service.  This function closes the
         job file and resets the calling server's access rights to the file server to
         their original (login) values.

         This call also checks a job's Service Restart flag to ensure that the job can
         be restarted automatically after a server failure.  If the Service Restart
         flag is set, the job's Server Station, Server Task, and Server ID Number
         fields are cleared, and the job remains in its current position in the queue.
         If the Service Restart flag is zero, the job entry is destroyed, and the job
         file is deleted.

         An aborted job returns to its former position in the job queue if its
         Service Restart flag is set.  For example, if a job was at the beginning of
         the queue before being called, it would return to the beginning of the queue
         after being aborted.  An aborted job could, therefore, be next in line for
         service.  For this reason, a server should not abort a job because of an
         error in the job's format or requests; the server should use the Finish
         Servicing a Queue Job call to remove the job from the queue.

         A server should abort a job only if some temporary internal problem prevents
         it from completing that job.  For example, a printer server might abort a
         job if the paper in the printer jammed; when the print server calls the same
         job after the paper jam is fixed, it should be able to service the job
         successfully.  A server should not abort a job that is attempting to access
         data without proper security clearance; if aborted, the job would remain in
         the queue and be serviced and aborted again and again.

         Only a queue server that has previously accepted a job for service can make
         this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > suJobNumber

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

CLIENT:  DOS WIN OS2 NT

SEE:     23 105  Close File And Start Queue Job
         23 114  Finish Servicing Queue Job
         23 104  Create Queue Job And File

NCP:     23 115  Abort Servicing Queue Job

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s115AbortServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 115)
   #define NCP_STRUCT_LEN  ((nuint)    7)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suJobNumber);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s115.c,v 1.7 1994/09/26 17:35:06 rebekah Exp $
*/
