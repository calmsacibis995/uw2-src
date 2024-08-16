/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s131.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s131FinishServicingQJob************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s131FinishServicingQJob
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint32  luJobNumber,
            nuint32  luChargeInfo
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Finish
         Servicing Queue Job (0x2222  23  114).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to Queue
         NCPs for information on both the old and new job structures.

ARGS: <> pAccess
       > luQueueID
       > luJobNumber
       > luChargeInfo

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 114  Finish Servicing Queue Job

NCP:     23 131  Finish Servicing Queue Job

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s131FinishServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber,
   nuint32  luChargeInfo
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 131)
   #define NCP_STRUCT_LEN  ((nuint)   13)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luQueueID);
   NCopyHiLo32((pnuint32)&abuReq[7], &luJobNumber);
   NCopyHiLo32((pnuint32)&abuReq[11], &luChargeInfo);

   return NWCRequestSingle(pAccess,
                           (nuint)NCP_FUNCTION,
                           abuReq,
                           (nuint)NCP_REQ_LEN,
                           NULL,
                           (nuint)0,
                           NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s131.c,v 1.7 1994/09/26 17:35:31 rebekah Exp $
*/
