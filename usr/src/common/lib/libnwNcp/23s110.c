/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s110.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s110ChangeQJobPosition*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s110ChangeQJobPosition
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suJobNumber,
            nuint8   buNewPosition
         );

REMARKS: This function changes a job's position in a queue.  Position 1 is the first
         position and position 250 is the last position in a full queue.  If an
         operator specifies a position number higher than the position number of
         the job currently at the end of the queue, the job is placed at the end of
         the queue.  When a job is moved, the job positions of all job entries in the
         queue are updated to reflect the new positions.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > suJobNumber
       > buNewPosition

INCLUDE: ncpqms.h

RETURN:  0x0000  Successful
         0x8996  Server Out of Memory
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D5  No Queue Job
         0x89D6  No Job Rights
         0x89FE  Bindery Locked
         0x89FF  Bindery Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 109  Change Queue Job Entry
         23 107  Get Queue Job List
         23 120  Get Queue Job File Size

NCP:     23 110  Change Queue Job Position

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s110ChangeQJobPosition
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber,
   nuint8   buNewPosition
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 110)
   #define NCP_STRUCT_LEN  ((nuint)    8)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suJobNumber);
   abuReq[9] = buNewPosition;

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s110.c,v 1.7 1994/09/26 17:34:58 rebekah Exp $
*/
