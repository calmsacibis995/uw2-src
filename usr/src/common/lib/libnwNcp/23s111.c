/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s111.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s111AttachQServerToQ***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s111AttachQServerToQ
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
         );

REMARKS: This call allows a station to attach itself to a specified queue as a queue
         server so that it can service jobs from the queue.  A queue can have as many
         as 25 servers attached to it at a time.  If 25 servers are already attached
         to the target queue, a Maximum Queue Servers error is returned.

         The station making this call must be security equivalent to one of the
         objects listed in the queue's Q_SERVERS group property.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID

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

SEE:     23 112  Detach Queue Server From Queue
         23 113  Service Queue Job

NCP:     23 111  Attach Queue Server To Queue

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s111AttachQServerToQ
(
   pNWAccess pAccess,
   nuint32  luQueueID
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 111)
   #define NCP_STRUCT_LEN  ((nuint)    5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s111.c,v 1.7 1994/09/26 17:35:00 rebekah Exp $
*/
