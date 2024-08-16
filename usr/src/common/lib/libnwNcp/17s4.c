/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s4ScanSpoolFileQueue**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s4ScanSpoolFileQueue
         (
            pNWAccess pAccess,
            nuint8   buTargetPrinter,
            nuint8   buIterHnd,
            pNWNCPQueueInfo pQueueInfo,
         );

REMARKS: This call can be used iteratively by a client to scan a printer's queue.
         To get the first entry in the printer's job queue, the client should set
         Last Job Number to zero.  On subsequent requests, the client should set
         Last Job Number to Job Number returned by the server.

         File Name contains the name of the file to be printed.  Volume Number
         indicates which of the server's volumes the file resides on.  Print Flags,
         Tab Size, Target Printer, Copies, and Form Type contain the print parameters
         that were in effect when the job was placed in the print queue.  Originating
         Client contains the connection number of the client that entered the job in
         the queue.  Time Spooled contains the first 6 bytes of the system time clock
         at the time the job was entered in the queue.  Banner Name contains the file
         name the client wants printed on the banner page.  Object ID contains the ID
         of the object that the client logged in as when the job was placed in the
         print queue.

ARGS: <> pAccess
      >  buTargetPrinter
      >  buIterHnd
      <  pQueueInfo

INCLUDE: ncpprint.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8987  Create Filename Error
         0x8988  Invalid File Handle
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8993  No Read Privileges
         0x8994  No Write Privileges
         0x8995  File Detached
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D2  No Queue Server
         0x89D3  No Queue Rights
         0x89D4  Queue Full
         0x89D5  No Queue Job
         0x89D6  No Job Rights
         0x89DA  Queue Halted
         0x89E8  Write To Group
         0x89EA  No Such Member
         0x89EB  Property Not Set Property
         0x89EC  No Such Set
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure, Failure, No Files Found, Lock Error, Bad Printer

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     17 04  Scan Spool File Queue

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s4ScanSpoolFileQueue
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   nuint8   buIterHnd,
   pNWNCPQueueInfo pQueueInfo
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 4)
   #define NCP_STRUCT_LEN  ((nuint16) 3)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 81)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buTargetPrinter;
   abuReq[4] = buIterHnd;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
         abuReply, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      pQueueInfo->buJobNumber = abuReply[0];
      NCopyLoHi16(&pQueueInfo->suReserved1, &abuReply[1]);
      NWCMemMove(pQueueInfo->abuFileName, &abuReply[3], (nuint) 14);
      pQueueInfo->buVolumeNumber = abuReply[17];
      pQueueInfo->buPrintFlags = abuReply[18];
      pQueueInfo->buTabSize = abuReply[19];
      pQueueInfo->buTargetPrinter = abuReply[20];
      pQueueInfo->buCopies = abuReply[21];
      pQueueInfo->buFormType = abuReply[22];
      pQueueInfo->buOriginatingClient = abuReply[23];
      NWCMemMove(pQueueInfo->abuTimeSpooled, &abuReply[24], (nuint) 6);
      NWCMemMove(pQueueInfo->abuReserved2, &abuReply[30], (nuint) 15);
      NWCMemMove(pQueueInfo->abuBannerName, &abuReply[45], (nuint) 14);
      NWCMemMove(pQueueInfo->abuReserved3, &abuReply[59], (nuint) 18);
      NCopyHiLo32(&pQueueInfo->luObjectID, &abuReply[77]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s4.c,v 1.7 1994/09/26 17:33:17 rebekah Exp $
*/
