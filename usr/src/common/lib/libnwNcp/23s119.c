/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s119.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s119SetQServerCurrStatus***********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s119SetQServerCurrStatus
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            pnuint8  pbuServerStatusRecordB64
         );

REMARKS: This call allows a queue server to update the queue manager's copy of the
         queue server's Server Status Record.  The Server Status Record is a record
         (one per queue server) that contains any information shared by the queue
         server and queue clients.  The first 4 bytes should contain the estimated
         cost for the queue server to complete an average job of the type it services.
         The queue manager does not interpret the contents of this status record.

         Only a station that has previously attached to the specified queue as a
         queue server can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > pbuServerStatusRecordB64

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

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 118  Read Queue Server Current Status

NCP:     23 119  Set Queue Server Current Status

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s119SetQServerCurrStatus
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pnuint8  pbuServerStatusRecordB64
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 119)
   #define NCP_STRUCT_LEN  ((nuint)   69)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);
   NWCMemMove(&abuReq[7], pbuServerStatusRecordB64, 64);

   return NWCRequestSingle(pAccess,
                           (nuint)NCP_FUNCTION,
                           abuReq,
                           (nuint)NCP_REQ_LEN,
                           NULL,
                           (nuint)0,
                           NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s119.c,v 1.7 1994/09/26 17:35:11 rebekah Exp $
*/
