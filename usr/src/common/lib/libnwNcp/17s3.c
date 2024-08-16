/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s3SpoolADiskFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s3SpoolADiskFile
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buNameLen,
            pnstr8   pbstrNameB256,
         );

REMARKS: This call allows a client to send an existing file to a print queue so the
         client does not need to recopy the file into a separate spool file.  The
         server uses the client's current print parameters and adds the file to the
         job queue of the appropriate printer.

         Clients must have open and read privileges for any files they spool.

ARGS: <> pAccess
      >  buDirHandle
      >  buNameLen
      >  pbstrNameB256

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
         0x89FE  Directory Locked
         0x89FF  No Files Found, Lock Error, Hard Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     17 03  Spool A Disk File

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s3SpoolADiskFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buNameLen,
   pnstr8   pbstrNameB256
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 3)
   #define NCP_STRUCT_LEN  ((nuint16) (3 + buNameLen))
   #define REQ_FRAG_LEN    ((nuint) 5)
   #define MAX_NAME_LEN    ((nuint) 256)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_FRAG_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_FRAG_LEN;

   reqFrag[1].pAddr = pbstrNameB256;
   reqFrag[1].uLen  = MAX_NAME_LEN;

   return(NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s3.c,v 1.7 1994/09/26 17:33:16 rebekah Exp $
*/
