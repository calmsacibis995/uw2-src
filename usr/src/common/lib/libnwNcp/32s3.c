/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:32s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP32s3SyncSemSignal**************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP32s3SyncSemSignal
         (
            pNWAccess pAccess,
            nuint32  luSemHandle,
         )

REMARKS: This call increments the semaphore value by one.  If another client
         is waiting on the semaphore, a Successful Completion Code is returned
         to the waiting client.

ARGS: <> pAccess,
      >  luSemHandle,

INCLUDE: ncpsync.h

RETURN:  0x0000 Successful
         0x89FF LockError

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     32 02  Wait On Semaphore

NCP:     32 03  Signal Semaphore (old)

CHANGES: 30 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP32s3SyncSemSignal
(
   pNWAccess pAccess,
   nuint32  luSemHandle
)
{
   #define NCP_FUNCTION    ((nuint) 32)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1],&luSemHandle);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/32s3.c,v 1.7 1994/09/26 17:37:53 rebekah Exp $
*/
