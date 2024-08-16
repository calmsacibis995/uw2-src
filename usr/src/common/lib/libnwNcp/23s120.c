/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s120.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s120GetQJobFileSize****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s120GetQJobFileSize
         (
            pNWAccess pAccess,
            nuint32  luInQueueID,
            nuint16  suInJobNumber,
            pnuint32 pluOutQueueID,
            pnuint16 psuOutJobNumber,
            pnuint32 pluFileSize
         );

REMARKS: This call returns the current length of a file associated with a queue entry.
         If the queue entry is still open (not committed by its maker) or is in
         service, the entry size returned by this call does not necessarily reflect
         the final file size.

         Any station security equivalent to an object listed in the queue's Q_USERS
         group property or Q_OPERATORS group property can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.


ARGS: <> pAccess
       > luInQueueID
       > suInJobNumber
      <  pluOutQueueID (optional)
      <  psuOutJobNumber (optional)
      <  pluFileSize

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

SEE:     23 107  Get Queue Job List
         23 102  Read Queue Current Status
         23 108  Read Queue Job Entry
         23 106  Remove Job From Queue

NCP:     23 120  Get Queue Job File Size

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s120GetQJobFileSize
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   nuint16  suInJobNumber,
   pnuint32 pluOutQueueID,
   pnuint16 psuOutJobNumber,
   pnuint32 pluFileSize
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 120)
   #define NCP_STRUCT_LEN  ((nuint)    7)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)   10)

   nint32  lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luInQueueID);
   NCopyHiLo16(&abuReq[7], &suInJobNumber);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      abuReply, NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      if (pluOutQueueID)
         NCopyHiLo32(pluOutQueueID, &abuReply[0]);

      if (psuOutJobNumber)
         NCopyHiLo16(psuOutJobNumber, &abuReply[4]);

      NCopyHiLo32(pluFileSize, &abuReply[6]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s120.c,v 1.7 1994/09/26 17:35:13 rebekah Exp $
*/
