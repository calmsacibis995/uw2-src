/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:destrque.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*NWDestroyQueue****************************************************
SYNTAX:  NWCCODE N_API NWDestroyQueue
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID
         )

REMARKS: Destroys the queue specified by 'queueID'. During this process the
         following things happen:

           All active jobs are aborted
           All servers are detached from the queue
           All jobs in the queue are destroyed
           All files associated with the queue are deleted
           The queue object and its properties are removed from the bindery
           The queue's subdirectory is deleted.

         You must have supervisor privileges to have this call complete
         successfully.

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 101  Destroy Queue

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWDestroyQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP23s101DestroyQ(&access, NSwap32(queueID)));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/destrque.c,v 1.7 1994/09/26 17:45:14 rebekah Exp $
*/
