/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s107.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s107GetQJobList********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s107GetQJobList
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            pnuint16 psuJobCount,
            pnuint16 psuJobNumbersB250
         );

REMARKS: This call returns the list of jobs contained in the queue specified by Queue
         ID.  The Job Count field contains the number of entries currently in the
         queue.  The Job Number array contains a list of entries in their current
         queue order.

         When used with Read Queue Job Entry (0x2222  108  --), this call returns the
         following information about all the jobs in a given queue:

         - Each job's position in the queue.

         - The number and type of jobs in the queue.  (This
         changes between a station's consecutive calls because the queue
         management environment is multi-threaded.)

         - The job number of each job in the queue.

         If a call to read information about a job in the queue fails with a No Queue
         Job error, the calling station can assume that the job has been completed
         or that it has been deleted from the queue.

         Any station security equivalent to an object listed in the queue's Q_USERS
         group property or Q_OPERATORS group property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
      <  psuJobCount
      <  psuJobNumbersB250

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

SEE:     23 120  Get Queue Job File Size
         23 102  Read Queue Current Status
         23 108  Read Queue Job Entry
         23 106  Remove Job From Queue

NCP:     23 107  Get Queue Job List

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s107GetQJobList
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pnuint16 psuJobCount,
   pnuint16 psuJobNumbersB250
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 107)
   #define NCP_STRUCT_LEN  ((nuint)    5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) (2 + (250 * 2)))

   nint32  lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);

   lCode = NWCRequestSingle(pAccess,
                            (nuint)NCP_FUNCTION,
                            abuReq,
                            (nuint)NCP_REQ_LEN,
                            abuReply,
                            (nuint)NCP_REPLY_LEN,
                            NULL);
   if(lCode == 0)
   {
      if(psuJobCount != NULL)
        NCopyHiLo16(psuJobCount, (pnuint16)&abuReply[0]);

      if(psuJobNumbersB250)
      {
         nuint i;

         for (i = 0; i < *psuJobCount; i++)
            NCopyHiLo16(&psuJobNumbersB250[i], (pnuint16)&abuReply[(i*2)+2]);
      }
   }

   return ((NWRCODE)lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s107.c,v 1.8 1994/09/26 17:34:54 rebekah Exp $
*/
