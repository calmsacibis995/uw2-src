/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:signsem.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwmisc.h"

/*manpage*NWSignalSemaphore*************************************************
SYNTAX:  NWCCODE N_API NWSignalSemaphore
         (
           NWCONN_HANDLE conn,
           nuint32 semHandle
         )

REMARKS: Increments the semaphore value by one. If another client is waiting
         on the semaphore, a Successful Completion Code is returned to the
         waiting client.

ARGS: >  conn
      >  semHandle

INCLUDE: nwsync.h

RETURN:  0x0000 Success
         0x89FF Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     32 03  Signal Semaphpore

CHANGES: 30 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSignalSemaphore
(
  NWCONN_HANDLE   conn,
  nuint32         semHandle
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP32s3SyncSemSignal(&access, semHandle));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/signsem.c,v 1.7 1994/09/26 17:50:04 rebekah Exp $
*/
