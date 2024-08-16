/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:atservqu.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*NWAttachQueueServerToQueue****************************************
SYNTAX:  NWCCODE N_API NWAttachQueueServerToQueue
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID
         )

REMARKS:

ARGS:  > conn
       > queueID

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 111  Attach Queue Server to Queue

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAttachQueueServerToQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP23s111AttachQServerToQ(&access, NSwap32(queueID)));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/atservqu.c,v 1.7 1994/09/26 17:44:13 rebekah Exp $
*/
