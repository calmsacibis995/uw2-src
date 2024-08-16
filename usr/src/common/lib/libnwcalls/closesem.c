/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:closesem.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwsync.h"
#include "nwmisc.h"

/*manpage*NWCloseSemaphore**************************************************
SYNTAX:  NWCCODE N_API NWCloseSemaphore
         (
            NWCONN_HANDLE conn,
            nuint32 semHandle
         )

REMARKS: This call allows a client to close a semaphore that is no longer needed.  The
         corresponding Semaphore Open Count (number of clients accessing the
         semaphore) is decremented by one.  If the Semaphore Open Count is zero after
         this call, the server deletes the semaphore.

ARGS: >  conn
      >  semHandle

INCLUDE: nwsync.h

RETURN:  0x00  Successful
         0xFF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     32 01  Examine Semaphore

NCP:     32 04  Close Semaphore

CHANGES: 30 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCloseSemaphore
(
   NWCONN_HANDLE  conn,
   nuint32        semHandle
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP32s4SyncSemClose(&access, semHandle));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/closesem.c,v 1.7 1994/09/26 17:44:28 rebekah Exp $
*/
