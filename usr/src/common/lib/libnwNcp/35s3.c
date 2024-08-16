/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s3AFPDelete***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s3AFPDelete
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            nuint8   buPathLen,
            pnstr8   pbstrPath
         )

REMARKS: This call deletes a file or a directory.
         The Path Mod String can be null.
         Base Directory ID alone can specify the file or the directory.
         Directories to be deleted must be empty.
         Files to be deleted must be closed by all users.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x898A  No Delete Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899E  Bad File Name
         0x899F  Directory Active
         0x89A0  Directory Not Empty
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     35 14  AFP 2.0 Create File
         35 02  AFP Create File
         35 13  AFP 2.0 Create Directory
         35 01  AFP Create Directory
         35 19  AFP Get MacIntosh Info On Deleted File

NCP:     35 03  AFP Delete

CHANGES: 19 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s3AFPDelete
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define NCP_STRUCT_LEN  ((nuint16) (7 + buPathLen))
   #define REQ_LEN         ((nuint) 9)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];
   nuint8  abuBuf[REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen    = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuBuf[0],&suNCPLen);
   abuBuf[2]   = NCP_SUBFUNCTION;
   abuBuf[3]   = buVolNum;
   NCopyHiLo32(&abuBuf[4],&luAFPEntryID);
   abuBuf[8]   = buPathLen;

   reqFrag[0].pAddr = abuBuf;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s3.c,v 1.7 1994/09/26 17:38:23 rebekah Exp $
*/
