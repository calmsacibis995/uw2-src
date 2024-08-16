/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s2SetSpoolFileFlags**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s2SetSpoolFileFlags
         (
            pNWAccess pAccess,
            nuint8   buPrintFlags,
            nuint8   buTabSize,
            nuint8   buTargetPrinter,
            nuint8   buCopies,
            nuint8   buFormType,
            nuint8   buReserved,
            pnstr8   pbstrBannerNameB14,
         );

REMARKS: This call allows a client to set the print environment for a print job.
         When the print server adds a job to the print queue, it records the
         client's current print parameters and later uses them to control the
         printing of the job.

         Print Flags is a bit field.  The bits have the following meaning when set:

               Bit   Bit Definition

         3     Form feeds should be suppressed (not sent)
               at the end of the job

         5     The spool file should be deleted after
               printing

         6     The print server should expand tabs in the
               file

         7     A banner page should be printed

         Tab Size has meaning only if bit 6 is set in Print Flags.  If bit 6 is set,
         the print server assumes that the file being printed is a standard ASCII
         file.  The print server will assume tab stops exist in every column that is
         a multiple of Tab Size; any tabs that are encountered while printing the file
         will be replaced by enough spaces to move the printer over to the next tab
         stop column.  If tabsa are being expanded by the print server, the print
         server will interpret any Control Z character (0x1A) encountered in the
         print file as an end-of-file mark and will stop printing the file.

         If the tab expansion bit (bit 6) is cleared in Print Flags, the printer makes
         no assumptions about the nature of the file it is printing or the nature of
         the device the file is being sent to.  If bit 6 is cleared, the print server
         can drive many different types of devices (printers, plotters, and so on),
         but the burden of appropriatel controlling these devices is on the client
         using the device.

         Target Printer indicates which of the print server's printers the data file
         should be queued to.  Printer numbers start at printer 0.

         Copies indicates the number of times the spool file should be sent to the
         printer.

         Form Type indicates the form number the client wants the job to be printed
         on.

         Banner Name indicates the filename the client wants to appear on the print
         banner page.  This field should be null-padded if the desired name is less
         than 14 characters long.

         After each job is added to the queue, the print server resets the print
         parameters to default values.  The default values are as follows:

         Print Flags           0 (no banner, no tabs, no
                                 file delete)
         Tab Size              8
         Target Printer        0
         Copies                1
         Form Type             0
         Banner Name           0 (no banner name)

ARGS: <> pAccess
      >  buPrintFlags
      >  buTabSize
      >  buTargetPrinter,
      >  buCopies,
      >  buFormType,
      >  buReserved,
      >  pbstrBannerNameB14

INCLUDE: ncpprint.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89D2  No Queue Server
         0x89D3  No Queue Rights
         0x89E8  Write To Group
         0x89EA  No Such Member
         0x89EB  Property Not Set Property
         0x89EC  No Such Set
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Failure, Bad Printer

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     17 01  Close Spool File
         17 00  Write To Spool File
         17 10  Get Printer's Queue

NCP:     17 02  Set Spool File Flags

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s2SetSpoolFileFlags
(
   pNWAccess pAccess,
   nuint8   buPrintFlags,
   nuint8   buTabSize,
   nuint8   buTargetPrinter,
   nuint8   buCopies,
   nuint8   buFormType,
   nuint8   buReserved,
   pnstr8   pbstrBannerNameB14
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 2)
   #define NCP_STRUCT_LEN  ((nuint16) 21)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)
   #define BANNER_LEN      ((nuint) 14)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buPrintFlags;
   abuReq[4] = buTabSize;
   abuReq[5] = buTargetPrinter;
   abuReq[6] = buCopies;
   abuReq[7] = buFormType;
   abuReq[8] = buReserved;
   NWCMemMove(&abuReq[9], pbstrBannerNameB14, BANNER_LEN);

   return(NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s2.c,v 1.7 1994/09/26 17:33:14 rebekah Exp $
*/
