/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s132.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s132AbortServicingQJob*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s132AbortServicingQJob
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint32  luJobNumber
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Abort
         Servicing Queue Job (0x2222 23  115).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to
         Queue NCPs for information on both the old and new job structures.

ARGS: <> pAccess
       > luQueueID
       > luJobNumber

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 115  Abort Servicing Queue Job

NCP:     23 132  Abort Servicing Queue Job

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s132AbortServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 132)
   #define NCP_STRUCT_LEN  ((nuint)    9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo32(&abuReq[7], &luJobNumber);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s132.c,v 1.7 1994/09/26 17:35:32 rebekah Exp $
*/
