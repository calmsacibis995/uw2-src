/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:32s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP32s2SyncSemWaitOn*************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP32s2SyncSemWaitOn
         (
            pNWAccess pAccess,
            nuint32  luSemHandle,
            nuint16  suTimeOut,
         )

REMARKS: This call allows a client to wait on a semaphore.  The semaphore value is
         decremented by one.  If the resulting value is greater than or equal to
         zero, a Successful Completion Code is returned to the calling client.  If
         the resulting value is less than zero, the server will wait for a Signal
         Semaphore (0x2222  32  3) for the amount of time specified by Semaphore
         Time Out.  If no signal arrives, the server automatically re-increments the
         semaphore value and returns a Timeout Completion Code to the calling client,
         indicating that the semaphore is not available.

         Waits on a semaphore.  The semaphore value is decremented by one,
         if the value is greater than or equal to 0 a Successful Completion
         Code is returned.  if the value is less than 0 the call will not
         return until the value is 0 or the timeout has expired, which will
         return a Timeout Completion Code.

ARGS: <> pAccess,
      >  luSemHandle,
      >  suTimeOut,

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     32 03  Signal Semaphore

NCP:     32 02  Wait On Semaphore

CHANGES: 2 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP32s2SyncSemWaitOn
(
   pNWAccess pAccess,
   nuint32  luSemHandle,
   nuint16  suTimeOut
)
{
   #define NCP_FUNCTION    ((nuint) 32)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1],&luSemHandle);
   NCopyLoHi16(&abuReq[5],&suTimeOut);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/32s2.c,v 1.7 1994/09/26 17:37:52 rebekah Exp $
*/
