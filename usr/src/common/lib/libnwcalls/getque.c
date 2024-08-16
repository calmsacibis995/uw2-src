/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getque.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*NWGetPrinterQueueID***********************************************
SYNTAX:  NWCCODE N_API NWGetPrinterQueueID
         (
            NWCONN_HANDLE  conn,
            nuint16        printerNumber,
            pnuint32       queueID
         )

REMARKS: Returns the object ID of the queue servicing the specified printer.

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     17 10  Get Printer's Queue

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetPrinterQueueID
(
   NWCONN_HANDLE  conn,
   nuint16        printerNumber,
   pnuint32       queueID
)
{
   nuint32 nativeQueueID;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE)NWNCP17s10GetPrintersQueue(&access,
      (nuint8)printerNumber, &nativeQueueID)) == 0)
   {
      /* NWCalls is broken and expects the queue ID to be anti-native */
      *queueID = NSwap32(nativeQueueID);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getque.c,v 1.7 1994/09/26 17:46:20 rebekah Exp $
*/
