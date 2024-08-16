/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setsvrst.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*NWSetQueueServerCurrentStatus*************************************
SYNTAX:  NWCCODE N_API NWSetQueueServerCurrentStatus
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            void NWPTR     statusRecord
         )

REMARKS: Allows a queue server to update the queue manager's copy
         of the queue server's Server Status Record.

ARGS:  > conn
       > queueID
      <  statusRecord

INCLUDE: nwqms.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 119  Set Queue Server Current Status

CHANGES: 21 Sep 1993 - NWNCP Enabled- djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetQueueServerCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   void NWPTR     statusRecord
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP23s119SetQServerCurrStatus(&access,
      NSwap32(queueID), statusRecord));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setsvrst.c,v 1.7 1994/09/26 17:50:02 rebekah Exp $
*/
