/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s122.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s122ReadQJobEntry******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s122ReadQJobEntry
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint32  luJobNumber,
            pNWNCPQMSJobStruct2 pJobStructOut
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Read
         Queue Job Entry (0x2222  23  108).  This new NCP allows the use of the high
         connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to Queue
         NCPs for information on both the old and new job structures.

ARGS: <> pAccess
       > luQueueID
       > luJobNumber
      <  pJobStructOut (optional)

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 108  Read Queue Job Entry

NCP:     23 122  Read Queue Job Entry

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s122ReadQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber,
   pNWNCPQMSJobStruct2 pJobStructOut
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 122)
   #define NCP_STRUCT_LEN  ((nuint)    9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)  280)

   nint32   lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);
   NCopyHiLo32((pnuint32)&abuReq[7], &luJobNumber);

   lCode = NWCRequestSingle(pAccess,
                            (nuint)NCP_FUNCTION,
                            abuReq,
                            (nuint)NCP_REQ_LEN,
                            abuReply,
                            (nuint)NCP_REPLY_LEN,
                            NULL);

   if (lCode == 0)
   {
      if (pJobStructOut != NULL)
      {
         NCopyHiLo16(&pJobStructOut->suRecordInUseFlag, (pnuint16)&abuReply[0]);
         NCopyHiLo32(&pJobStructOut->luPreviousRecord, (pnuint32)&abuReply[2]);
         NCopyHiLo32(&pJobStructOut->luNextRecord, (pnuint32)&abuReply[6]);
         NCopyLoHi32(&pJobStructOut->luClientStation, (pnuint32)&abuReply[10]);
         NCopyLoHi32(&pJobStructOut->luClientTask, (pnuint32)&abuReply[14]);
         NCopyHiLo32(&pJobStructOut->luClientID, (pnuint32)&abuReply[18]);
         NCopyHiLo32(&pJobStructOut->luTargetServerID, (pnuint32)&abuReply[22]);
         NWCMemMove(pJobStructOut->abuTargetExecutionTime, &abuReply[26], 6);
         NWCMemMove(pJobStructOut->abuJobEntryTime, &abuReply[32], 6);
         NCopyHiLo32(&pJobStructOut->luJobNumber, (pnuint32)&abuReply[38]);
         NCopyHiLo16(&pJobStructOut->suJobType, (pnuint16)&abuReply[42]);
         NCopyLoHi16(&pJobStructOut->suJobPosition, (pnuint16)&abuReply[44]);
         NCopyLoHi16(&pJobStructOut->suJobControlFlags, (pnuint16)&abuReply[46]);
         NWCMemMove(pJobStructOut->abuJobFileName, &abuReply[48], 14);
         NCopyHiLo32(&pJobStructOut->luJobFileHandle, (pnuint32)&abuReply[62]);
         NCopyHiLo32(&pJobStructOut->luServicingServerStation,
                     (pnuint32)&abuReply[66]);
         NCopyHiLo32(&pJobStructOut->luServicingServerTask,
                     (pnuint32)&abuReply[70]);
         NCopyHiLo32(&pJobStructOut->luServicingServerID,
                     (pnuint32)&abuReply[74]);
         NWCMemMove(pJobStructOut->abuJobDescription, &abuReply[78], 50);
         NWCMemMove(pJobStructOut->abuClientRecordArea, &abuReply[128], 152);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s122.c,v 1.8 1994/09/26 17:35:17 rebekah Exp $
*/