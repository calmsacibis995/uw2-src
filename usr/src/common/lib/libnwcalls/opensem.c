/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:opensem.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwsync.h"

/*manpage*NWOpenSemaphore***************************************************
SYNTAX:  NWCCODE N_API NWOpenSemaphore
         (
            NWCONN_HANDLE conn,
            pnstr8        semName,
            nint16        initVal,
            pnuint32      semHandle,
            pnuint16      openCount
         );

REMARKS: Opens a named semaphore.  If the semaphore exists the initial value
         is ignored.  The semaphore can only be accessed using the returned
         handle.

ARGS:  > conn
       > semName
       > initVal
      <  semHandle
      <  openCount (optional)

INCLUDE: nwsync.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     32 --  Open Semaphore

CHANGES: 7 Mar 1994 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWOpenSemaphore
(
  NWCONN_HANDLE conn,
  pnstr8        semName,
  nint16        initVal,
  pnuint32      semHandle,
  pnuint16      openCount
)
{
   NWCCODE ccode;
   nuint8 tempOpenCount;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE)NWNCP32s0SyncSemOpen(&access, (nuint8) initVal,
                            (nuint8) NWCStrLen(semName),
                            semName, semHandle, &tempOpenCount)) == 0)
   {
      if (openCount)
         *openCount = (nuint16) tempOpenCount;
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/opensem.c,v 1.7 1994/09/26 17:48:31 rebekah Exp $
*/
