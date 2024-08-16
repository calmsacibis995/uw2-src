/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s123.c	1.6"
#include "ntypes.h"
#include "unicode.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s123ChangeQJobEntry**************
**************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s123ChangeQJobEntry
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            pNWNCPQMSJobStruct2 pJobStructIn
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Change
         Queue Job Entry (0x2222  23  109).  This new NCP allows the use of the high
         connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to
         Queue NCPs for information on both the old and new job structures.

ARGS: <> pAccess
       > luQueueID
       > pJobStructIn
       > pNewJobStruct

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 109  Change Queue Job Entry

NCP:     23 123  Change Queue Job Entry

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s123ChangeQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct2 pJobStructIn
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 123)
   #define NCP_STRUCT_LEN  ((nuint) (280+5))
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3],  &luQueueID);
   NCopyHiLo16((pnuint16)&abuReq[7],  &pJobStructIn->suRecordInUseFlag);
   NCopyHiLo32((pnuint32)&abuReq[9],  &pJobStructIn->luPreviousRecord);
   NCopyHiLo32((pnuint32)&abuReq[13], &pJobStructIn->luNextRecord);
   NCopyLoHi32((pnuint32)&abuReq[17], &pJobStructIn->luClientStation);
   NCopyLoHi32((pnuint32)&abuReq[21], &pJobStructIn->luClientTask);
   NCopyHiLo32((pnuint32)&abuReq[25], &pJobStructIn->luClientID);
   NCopyHiLo32((pnuint32)&abuReq[29], &pJobStructIn->luTargetServerID);
   NWCMemMove(&abuReq[33], pJobStructIn->abuTargetExecutionTime, 6);
   NWCMemMove(&abuReq[39], pJobStructIn->abuJobEntryTime, 6);
   NCopyHiLo32((pnuint32)&abuReq[45], &pJobStructIn->luJobNumber);
   NCopyHiLo16((pnuint16)&abuReq[49], &pJobStructIn->suJobType);
   NCopyHiLo16((pnuint16)&abuReq[51], &pJobStructIn->suJobPosition);
   NCopyLoHi16((pnuint16)&abuReq[53], &pJobStructIn->suJobControlFlags);
   NWCMemMove(&abuReq[55], pJobStructIn->abuJobFileName, 14);
   NCopyHiLo32((pnuint32)&abuReq[69], &pJobStructIn->luJobFileHandle);
   NCopyHiLo32((pnuint32)&abuReq[73], &pJobStructIn->luServicingServerStation);
   NCopyHiLo32((pnuint32)&abuReq[77], &pJobStructIn->luServicingServerTask);
   NCopyHiLo32((pnuint32)&abuReq[81], &pJobStructIn->luServicingServerID);
   NWCMemMove(&abuReq[85], pJobStructIn->abuJobDescription, 50);
   NWCMemMove(&abuReq[135], pJobStructIn->abuClientRecordArea, 152);

   return NWCRequestSingle(pAccess, 
                           (nuint)NCP_FUNCTION, 
                           abuReq, 
                           (nuint)NCP_REQ_LEN,
                           NULL,
                           (nuint)0,
                           NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s123.c,v 1.9 1994/09/26 17:35:19 rebekah Exp $
*/
