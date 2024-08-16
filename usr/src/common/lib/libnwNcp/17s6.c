/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s6GetPrinterStatus**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s6GetPrinterStatus
         (
            pNWAccess pAccess,
            nuint8   buTargetPrinter,
            pnuint8  pbuHalted,
            pnuint8  pbuOffLine,
            pnuint8  pbuFormType,
            pnuint8  pbuRedirectedPrinter,
         );

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

ARGS: <> pAccess
      >  buTargetPrinter
      <  pbuHalted (optional)
      <  pbuOffLine (optional)
      <  pbuFormType (optional)
      <  pbuRedirectedPrinter (optional)

INCLUDE: ncpprint.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89FF   Bad Printer
         0x89FB   Bad Dir Handle
         0x89FD   Bad Station Number

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     17 06  Get Printer Status

CHANGES: 20 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s6GetPrinterStatus
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   pnuint8  pbuHalted,
   pnuint8  pbuOffLine,
   pnuint8  pbuFormType,
   pnuint8  pbuRedirectedPrinter
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 4)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buTargetPrinter;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuRep,
               NCP_REPLY_LEN, NULL);
   if(lCode == 0)
   {
      if(pbuHalted)
         *pbuHalted = abuRep[0];

      if(pbuOffLine)
         *pbuOffLine = abuRep[1];

      if(pbuFormType)
         *pbuFormType = abuRep[2];

      if(pbuRedirectedPrinter)
         *pbuRedirectedPrinter = abuRep[3];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s6.c,v 1.7 1994/09/26 17:33:20 rebekah Exp $
*/
