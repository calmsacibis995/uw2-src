/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s1CloseSpoolFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s1CloseSpoolFile
         (
            pNWAccess pAccess,
            nuint8   buAbortQueueFlag,
         );

REMARKS: This call closes the client's current spool file.  If the Abort Queue
         flag is zero, the spool file is placed at the end of the print queue.
         If the Abort Queue flag is set, the spool file is not placed in
         the print queue; instead, the spool file's status flags are examined.

         If the spool file is marked for deletion after printing, the file is
         immediately deleted; if not, the print server closes the spool file and
         disregards it.

ARGS: <> pAccess
      >  buAbortQueueFlag

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
         0x89DA  Queue Halted
         0x89E8  Write To Group
         0x89EA  No Such Member
         0x89EB  Property Not Set Property
         0x89EC  No Such Set
         0x89FC  No Such Object
         0x89FD  Bad Station Number
         0x89FE  Directory Locked
         0x89FF  Bad Printer, Lock Error, Hard Failure,
         Fi89les Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     17 02  Set Spool File Flags

NCP:     17 01  Close Spool File

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s1CloseSpoolFile
(
   pNWAccess pAccess,
   nuint8   buAbortQueueFlag
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 1)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buAbortQueueFlag;

   return(NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s1.c,v 1.7 1994/09/26 17:33:12 rebekah Exp $
*/
