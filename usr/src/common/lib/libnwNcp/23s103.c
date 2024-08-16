/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s103.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s103SetQCurrStatus*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s103SetQCurrStatus
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint8   buQueueStatus
         );

REMARKS: This call allows an operator to control the adding of jobs and servers to a
         queue by setting bits in the Queue Status byte as follows:

         Bit   Bit Description

         0x01  Prevent jobs from being added to the queue.

         0x02  Prevent servers from attaching to the queue in
               order to service it.

         0x04  Prevent servers from servicing jobs in the queue.

         Only a station with operator privileges can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > buQueueStatus

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

NCP:     23 103  Set Queue Current Status

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s103SetQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint8   buQueueStatus
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_STRUCT_LEN  ((nuint)    6)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_SUBFUNCTION ((nuint8) 103)

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);
   abuReq[7] = buQueueStatus;

   return NWCRequestSingle(pAccess,
                           (nuint)NCP_FUNCTION,
                           abuReq,
                           (nuint)NCP_REQ_LEN,
                           NULL,
                           (nuint)0,
                           NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s103.c,v 1.7 1994/09/26 17:34:47 rebekah Exp $
*/
