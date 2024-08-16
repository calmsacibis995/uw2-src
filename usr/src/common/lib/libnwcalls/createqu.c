/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:createqu.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwclocal.h"

/*manpage*NWCreateQueue*****************************************************
SYNTAX:  NWCCODE N_API NWCreateQueue
         (
            NWCONN_HANDLE  conn,
            pnstr8         queueName,
            nuint16        queueType,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint32       queueID
         )

REMARKS: Creates a Queue of type 'queueType' of name 'queueName'.

         NOTE: The queue type needs to be passed in swapped, i.e. if you
               want queue type 3 (printer), you need to pass in a 0x0300
               rather than 0x0003.

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 100  Create Queue

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCreateQueue
(
   NWCONN_HANDLE  conn,
   pnstr8         queueName,
   nuint16        queueType,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint32       queueID
)
{
   NWCCODE ccode;
   char    tempPath[260];
   nuint32 nativeQueueID;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemMove(tempPath, path, NWCStrLen(path));
   NWConvertToSpecChar(tempPath);

   if ((ccode = (NWCCODE)NWNCP23s100CreateQ(&access, NSwap16(queueType),
      (nuint8) NWCStrLen(queueName), queueName, (nuint8) dirHandle,
      (nuint8) NWCStrLen(path), path, &nativeQueueID)) == 0)
   {
      /* NWCalls is broken and expects the queue ID to be anti-native */
      *queueID = NSwap32(nativeQueueID);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/createqu.c,v 1.7 1994/09/26 17:44:55 rebekah Exp $
*/
