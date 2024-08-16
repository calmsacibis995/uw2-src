/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:waitsem.c	1.5"
#include "nwcaldef.h"
#ifdef N_PLAT_OS2
/*#include "nwintern.h"*/
#include "nwcint.h"
#include "nwundoc.h"
#include "nwerror.h"
#endif
#include "ntypes.h"
#include "nwsync.h"
#include "nwmisc.h"

/*manpage*NWWaitOnSemaphore*************************************************
SYNTAX:  NWCCODE N_API NWWaitOnSemaphore
         (
            NWCONN_HANDLE conn,
            nuint32 semHandle,
            nuint16 timeOutValue
         )

REMARKS: Waits on a semaphore.  The semaphore value is decremented by one,
         if the value is greater than or equal to 0 a Successful Completion
         Code is returned.  if the value is less than 0 the call will not
         return until the value is 0 or the timeout has expired, which will
         return a Timeout Completion Code.

ARGS: >  conn
      >  semHandle
      >  timeOutValue

INCLUDE: nwsync.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     32 03  Signal Semaphore

NCP:     32 02  Wait On Semaphore

CHANGES: 2 Sep 1993 - purposely NOT NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWWaitOnSemaphore
(
  NWCONN_HANDLE   conn,
  nuint32         semHandle,
  nuint16         timeOutValue
)
{
  nuint8       reqBuf[7];
  NW_FRAGMENT  reqFrag[1];

  reqBuf[0] = 2;
  NCopyLoHi32(&reqBuf[1], &semHandle);
  NCopyHiLo16(&reqBuf[5], &timeOutValue);

  reqFrag[0].fragAddress = reqBuf;
  reqFrag[0].fragSize = 7;

#ifdef N_PLAT_OS2
  if (NWCCallGate(_NWC_WAIT_LOCK_AVAIL, &conn))
  {
    return (NWLockRequest(conn, 111, 1, reqFrag, 0, NULL));
  } else
#endif
    return (NWRequest(conn, 32, 1, reqFrag, 0, NULL));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/waitsem.c,v 1.7 1994/09/26 17:50:31 rebekah Exp $
*/
