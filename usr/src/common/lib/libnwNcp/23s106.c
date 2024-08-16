/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s106.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s106RemoveJobFromQ*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s106RemoveJobFromQ
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suJobNumber
         );

REMARKS: This call removes a job from a job queue.  The Job Number field contains the
         number that the queue manager assigned to the job when it was first entered
         into the queue.  Jobs are removed as follows:

         - The specified job is removed from the queue.

         - The associated file is closed and deleted.

         - The service is aborted if the job is being
         serviced.

         - Any further I/O requests made by the server to
         the job's queue file will fail, and an Illegal
         File Handle error will be returned.

         Only an operator or the client that created the job can make this call.

         See Introduction to Queue NCPs for information on both the old and new
         job structures.

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

CLIENT:  DOS OS2 WIN NT

SEE:     23 102  Read Queue Current Status
         23 120  Get Queue Job File Size
         23 107  Get Queue Job List

NCP:     23 106  Remove Job From Queue

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s106RemoveJobFromQ
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 106)
   #define NCP_STRUCT_LEN  ((nuint)    7)
   #define NCP_REQ_LEN     ((nuint) (NCP_STRUCT_LEN + 2))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suJobNumber);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s106.c,v 1.7 1994/09/26 17:34:53 rebekah Exp $
*/
