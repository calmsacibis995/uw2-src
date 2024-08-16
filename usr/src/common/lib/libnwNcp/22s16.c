/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s16.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s16DelPurge**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s16DelPurge
         (
            pNWAccess pAccess,
         );

REMARKS: This is an old NetWare 286 call that is supported in NetWare v2.1 or
         previous.  It is replaced by the NetWare 386 v3.0 call, Purge Salvageable
         File (0x2222  22  29).

         When files are deleted by a client, they are moved to a holding area on the
         volume until they are either purged (using this call), restored (using
         Restore Erased File (0x2222  22  17), or replaced by other files deleted by
         the client.

         This call purges, or permanantly deletes all files that the file server is
         holding for the client on all of the server's volumes.  The space
         relinquished by purged files can now be used by the file server.  Files
         deleted when this call is made cannot be recovered using Restore Erased File.
         Files that are deleted by the client after this call is made will again be
         placed in the holding area.

ARGS: <> pAccess

INCLUDE: ncpfile.h

RETURN:  0x0000            Successful
         0x8981            Out Of Handles
         0x8996            Server Out Of Memory
         0x8998            Disk Map Error
         0x89A1            Directory I/O Error
         0x89FF            Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     22 17  Recover Erased File (old)
         22 29  Purge Salvageable File
         22 28  Recover Salvageable File
         23 206  Purge All Erased Files

NCP:     22 16  Purge Erased Files (old)

CHANGES: 15 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s16DelPurge
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 16)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s16.c,v 1.7 1994/09/26 17:33:52 rebekah Exp $
*/
