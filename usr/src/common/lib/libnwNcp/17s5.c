/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s5DeleteSpoolFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s5DeleteSpoolFile
         (
            pNWAccess pAccess,
            nuint8   buTargetPrinter,
            nuint8   buNumberOfJobs,
            pnuint8  pbuJobListB199,
         );

REMARKS: This call allows a client to remove one or more jobs from a print queue.
         Job Number must contain a valid job number returned by Scan Spool File Queue
         (0x2222 17  4).

         Clients can remove jobs from a spool queue only if they are supervisor
         equivalent or if they are currently logged in with the same Object ID that
         appears with the job.

         A client can remove a list of jobs by appending multiple Job Number bytes
         to the request and adjusting Sub Function Structure Len accordingly.

ARGS: <> pAccess
      >  buTargetPrinter
      >  buNumberOfJobs
      >  pbuJobListB199

INCLUDE: ncpprint.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8987  Create Filename Error
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8994  No Write Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D3  No Queue Rights
         0x89D6  No Job Rights
         0x89E8  Write To Group
         0x89EA  No Such Member
         0x89EB  Property Not Set Property
         0x89EC  No Such Set
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FE  Spool Delete Error
         0x89FF  No Files Found, Hard Failure, Lock Error

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     17 05  Delete Spool File

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s5DeleteSpoolFile
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   nuint8   buNumberOfJobs,
   pnuint8  pbuJobListB199
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 5)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buNumberOfJobs))
   #define FIXED_NCP_LEN   ((nuint) 4)
   #define VAR_NCP_LEN     ((nuint) 199)
   #define REQ_LEN         (FIXED_NCP_LEN + VAR_NCP_LEN)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[2];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buTargetPrinter;
   abuReq[4] = buNumberOfJobs;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = FIXED_NCP_LEN;

   reqFrag[1].pAddr = pbuJobListB199;
   reqFrag[1].uLen  = buNumberOfJobs;

   return(NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s5.c,v 1.7 1994/09/26 17:33:19 rebekah Exp $
*/
