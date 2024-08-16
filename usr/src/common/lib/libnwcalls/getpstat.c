/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getpstat.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpprint.h"
#include "nwcaldef.h"
#include "nwprint.h"

/*manpage*NWGetPrinterStatus************************************************
SYNTAX:  NWCCODE N_API NWGetPrinterStatus
         (
            NWCONN_HANDLE conn,
            nuint16  printerNumber,
            PRINTER_STATUS N_FAR * status
         )


REMARKS: This call allows a client to check the status of a shared printer.

         Printer Halted will be 0xFF if the printer has been halted from the system
         console; otherwise it will be 0.

         Printer Off Line will be 0xFF if the printer is off-line; otherwise it will
         be 0.

         Current Form Type will contain the number of the form currently mounted
         on the printer.

         Redirected Printer will contain the number of the printer that jobs destined
         for Target Printer are being sent to.  Redirected Printer will usually be
         the same as the Target Printer specified by the client.  If Redirected
         Printer and Target Printer are not the same, the target printer has been
         redirected from the system console, and jobs sent to it are automatically
         rerouted and placed in a print queue of the printer indicated by Redirected
         Printer.

         This NCP works only on a 286 server (v2.15 andprevious) when a printer
         is configured into the system with the NETGEN utility.  The "P" command at
         the console must respond with a list of printers for this NCP to work.

ARGS: >  conn
      >  printerNumber
      <  status

               typedef struct PRINTER_STATUS
               {
                  nuint8  printerHalted;
                  nuint8  printerOffline;
                  nuint8  currentFormType;
                  nuint8  redirectedPrinter;
               } PRINTER_STATUS;

INCLUDE: nwprint.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89FF  Bad Printer
         0x89FB  Bad Dir Handle
         0x89FD  Bad Station Number

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     17 06  Get Printer Status

CHANGES: 20 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetPrinterStatus
(
   NWCONN_HANDLE  conn,
   nuint16        printerNumber,
   PRINTER_STATUS N_FAR * status
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP17s6GetPrinterStatus(&access,
                        (nuint8) printerNumber, &status->printerHalted,
                        &status->printerOffline, &status->currentFormType,
                        &status->redirectedPrinter));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getpstat.c,v 1.7 1994/09/26 17:46:18 rebekah Exp $
*/
