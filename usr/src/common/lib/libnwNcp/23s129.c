/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s129.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s129GetQJobList********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s129GetQJobList
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint32  luQueueStartPosition,
            pnuint32 pluTotalQueueJobs,
            pnuint32 pluReplyQueueJobNumbers,
            pnuint32 pluJobNumberListB125
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Get
         Queue Job List (0x2222  23  107).  This new NCP allows the use of the high
         connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to
         Queue NCPs for information on the new job structure.

         The TotalQueueJobs field contains the number of jobs in the queue.

         The ReplyQueueJobNumbers field contains the number of jobs in the
         JobNumberList, with the maximum amount being 125 (in a reply packet).

         See Introduction to Queue NCPs for information on both the old and new job
         structures.

ARGS: <> pAccess
       > luQueueID
       > luQueueStartPosition
      <  pluTotalQueueJobs (optional)
      <  pluReplyQueueJobNumbers (optional)
      <  pluJobNumberListB125 (optional)

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 107  Get Queue Job List

NCP:     23 129  Get Queue Job List

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s129GetQJobList
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luQueueStartPosition,
   pnuint32 pluTotalQueueJobs,
   pnuint32 pluReplyQueueJobNumbers,
   pnuint32 pluJobNumberListB125
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 129)
   #define NCP_STRUCT_LEN  ((nuint)    9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)    8 + (125 * 4))

   nint32   lCode;
   nuint8  abuReq[NCP_STRUCT_LEN+2];
   nuint8  abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);
   NCopyLoHi32((pnuint32)&abuReq[7], &luQueueStartPosition);

   lCode = NWCRequestSingle(pAccess,
                            (nuint)NCP_FUNCTION,
                            abuReq,
                            (nuint)NCP_REQ_LEN,
                            abuReply,
                            (nuint)NCP_REPLY_LEN,
                            NULL);

   if(lCode == 0)
   {
      nuint32 luNumJobs;

      if(pluTotalQueueJobs != NULL)
        NCopyLoHi32(pluTotalQueueJobs, (pnuint32)&abuReply[0]);

      NCopyLoHi32(&luNumJobs, (pnuint32)&abuReply[4]);

      if(pluReplyQueueJobNumbers != NULL)
         *pluReplyQueueJobNumbers = luNumJobs;

      if(pluJobNumberListB125 != NULL)
      {
         nint i, j;

         for (i = 0, j = 8; i < (nint)luNumJobs; i++, j += 4)
            NCopyHiLo32(&pluJobNumberListB125[i], &abuReply[j]);
      }
   }
   return((NWRCODE)lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s129.c,v 1.8 1994/09/26 17:35:28 rebekah Exp $
*/
